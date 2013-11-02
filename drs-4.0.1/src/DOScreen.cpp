/*
 * doscreen.cpp
 * DRS Oscilloscope screen class
 * $Id: DOScreen.cpp 20380 2012-11-13 13:38:01Z ritt $
 */

#include "DRSOscInc.h"

/*------------------------------------------------------------------*/

BEGIN_EVENT_TABLE(DOScreen, wxWindow)
   EVT_PAINT        (DOScreen::OnPaint)
   EVT_SIZE         (DOScreen::OnSize)
   EVT_MOUSE_EVENTS (DOScreen::OnMouse)
   EVT_MOUSEWHEEL   (DOScreen::OnMouse)
END_EVENT_TABLE()

/*------------------------------------------------------------------*/

const int DOScreen::m_scaleTable[10]  = { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000 };
const int DOScreen::m_hscaleTable[13] = { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000 };

/*------------------------------------------------------------------*/

DOScreen::DOScreen(wxWindow *parent, Osci *osci, DOFrame *frame)
        : wxWindow(parent, wxID_ANY)
{
   m_osci = osci;
   m_frame  = frame;
   m_hscale = 0;
   m_screenSize = m_screenOffset = 0;
   m_paintMode = kPMWaveform;
   m_displayDateTime = false;
   m_displayShowGrid = true;
   m_displayLines = true;
   m_displayMode = ID_DISPSAMPLE;
   m_displayN = 16;
   m_clientWidth = m_clientHeight = 0;
   m_mouseX = m_mouseY = 0;
   m_MeasX1 = m_MeasX2 = m_MeasY1 = m_MeasY2 = 0;
   for (int i=0 ; i<4 ; i++) {
      m_chnon[i]  = false;
      m_offset[i] = 0;
      m_scale[i]  = 6;
   }
   memset(m_mathFlag, 0, sizeof(m_mathFlag));
   m_debugMsg[0] = 0;

   SetBackgroundColour(*wxBLACK);
   SetBackgroundStyle(wxBG_STYLE_CUSTOM);
}

/*------------------------------------------------------------------*/

DOScreen::~DOScreen()
{
}

/*------------------------------------------------------------------*/

void DOScreen::OnPaint(wxPaintEvent& event)
{
   wxCoord w, h;

   // get size form paint DC (buffered paint DC won't work)
   wxPaintDC pdc(this);
   pdc.GetSize(&w, &h);
   m_clientWidth = w;
   m_clientHeight = h;

   // obtain buffered paint DC to avoid flickering
   wxBufferedPaintDC dc(this);

   if (m_paintMode == kPMWaveform) {
      DrawWaveform(dc, w, h, false);
      DrawMath(dc, w, h, false);
      DrawHisto(dc, w, h, false);
   } else
      DrawTcalib(dc, w, h, false);
}

/*------------------------------------------------------------------*/

void DOScreen::DrawDot(wxDC& dc, wxCoord x, wxCoord y, bool printing)
{
   if (printing)
      dc.DrawRectangle(x, y, 1, 1);
   else
      dc.DrawPoint(x, y);
}

/*------------------------------------------------------------------*/

bool DOScreen::FindClosestWafeformPoint(int& idx_min, int& x_min, int& y_min)
{
   int dist, dist_min, x, y;

   // find point on waveform closest to mouse position
   idx_min = -1;
   dist_min = 1000000;

   wxPoint mpos((int)(m_mouseX*(m_clientWidth-21)+20), 
                (int)(m_mouseY*(m_clientHeight-21)+1));

   for (int idx=0 ; idx<4 ; idx++) {
      if (m_chnon[idx]) {
         if (m_osci->GetWaveformDepth(idx)) {
            float *wf = m_osci->GetWaveform(idx);
            float *time = m_osci->GetTime(idx);

            for (int i=0 ; i<m_osci->GetWaveformDepth(idx) ; i++) {

               x = timeToX(time[i]);
               y = voltToY(idx, wf[i]);

               dist = (x-mpos.x)*(x-mpos.x) +
                      (y-mpos.y)*(y-mpos.y);
               if (dist < dist_min) {
                  dist_min = dist;
                  idx_min = idx;
                  x_min = x;
                  y_min = y;
               }
            }
         }
      }
   }

   return dist_min < 2000;
}

/*------------------------------------------------------------------*/

int DOScreen::timeToX(float t)
{
   return (int) ((double)(t-m_screenOffset)/m_screenSize*(m_x2-m_x1)+m_x1 + 0.5);
}

double DOScreen::XToTime(int x)
{
   return (double) m_screenSize*(x - m_x1)/(m_x2-m_x1)+m_screenOffset;
}

double DOScreen::GetT1()
{
   return (double) m_screenOffset;
}

double DOScreen::GetT2()
{
   return (double) m_screenSize+m_screenOffset;
}

int DOScreen::voltToY(float v)
{
   return voltToY(m_chn, v);
}

int DOScreen::voltToY(int chn, float v)
{
   v = v - m_offset[chn]*1000;
   return (int) ((m_y1+m_y2)/2-v/10.0/m_scaleTable[m_scale[chn]]*(m_y2-m_y1) + 0.5);
}

double DOScreen::YToVolt(int y)
{
   return YToVolt(m_chn, y);;
}

double DOScreen::YToVolt(int chn, int y)
{
   double v;

   v = (((double) m_y1+m_y2)/2 - y)/(m_y2-m_y1)*m_scaleTable[m_scale[chn]]*10;
   v = v + m_offset[chn]*1000;

   return v;
}

/*------------------------------------------------------------------*/

void DOScreen::SetMathDisplay(int id, bool flag)
{
   int m, chn;

   m = (id - ID_FFT1) / 4;
   chn = (id - ID_FFT1) % 4;

   m_mathFlag[m][chn] = flag;
}

/*------------------------------------------------------------------*/

void DOScreen::DrawWaveform(wxDC& dc, wxCoord width, wxCoord height, bool printing)
{
   int x, y, x_old, y_old, w, h, y_top, n;
   int idx_min, x_min, y_min;
   wxString wxst, wxst2;

   if (m_frame->GetOsci()->SkipDisplay())
      return;

   // tell DOFrame to pull new waveforms from EPThread
   m_frame->UpdateWaveforms();

   m_dc = &dc;
   m_x1 = 20;
   m_y1 = 1;
   m_x2 = width-1;
   m_y2 = height-20;
   x_old = y_old = 0;
   y_top = 1;
   
   /* if histo display is on, only show waveform in upper half */
   if (m_frame->IsHist())
      m_y2 = height/2-20;

#ifdef _MSC_VER
   int fontSize = 8;
#else
   int fontSize = 10;
#endif   

   // set font for all text output
   dc.SetFont(wxFont(fontSize, wxFONTFAMILY_DEFAULT,
                     wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

   // draw overall frame
   if (printing) {
      dc.SetBackground(*wxWHITE_BRUSH);
      dc.SetPen(*wxWHITE_PEN);
      dc.SetBrush(*wxWHITE_BRUSH);
   } else {
      dc.SetBackground(*wxBLACK_BRUSH);
      dc.SetPen(*wxBLACK_PEN);
      dc.SetBrush(*wxBLACK_BRUSH);
   }

   //static int n_event = 0;
   //if (++n_event % 30 == 0) {
   if (m_frame->IsHist())
      dc.DrawRectangle(0, 0, width, height/2);
   else
      dc.DrawRectangle(0, 0, width, height);
   dc.SetPen(*wxGREY_PEN);
   dc.DrawRectangle(m_x1, m_y1, m_x2-m_x1, m_y2-m_y1);
   //}

   // display optional debug messages
   if (*m_frame->GetOsci()->GetDebugMsg()) {
      wxst = wxString((const wxChar *)m_frame->GetOsci()->GetDebugMsg());
      dc.SetPen(wxPen(*wxLIGHT_GREY, 1, wxSOLID));
      dc.SetBrush(*wxGREEN);
      dc.SetTextForeground(*wxBLACK);
      dc.GetTextExtent(wxst, &w, &h);
      dc.DrawRoundedRectangle(m_x1+2, m_y1+2, w+5, 15, 2);
      dc.DrawText(wxst, m_x1+4, m_y1+2);
   }
   if (m_debugMsg[0]) {
      wxst = wxString((const wxChar *)m_debugMsg);
      dc.SetPen(wxPen(*wxLIGHT_GREY, 1, wxSOLID));
      dc.SetBrush(*wxGREEN);
      dc.SetTextForeground(*wxBLACK);
      dc.GetTextExtent(wxst, &w, &h);
      dc.DrawRoundedRectangle(m_x1+2, m_y1+2, w+5, 15, 2);
      dc.DrawText(wxst, m_x1+4, m_y1+2);
   }

   // draw grid
   if (m_displayShowGrid) {
      dc.SetPen(*wxGREY_PEN);
      for (int i=1 ; i<10 ; i++) {
         if (i == 5) {
            for (int j=m_x1+1 ; j<m_x2 ; j+=2)
               DrawDot(dc, j, i*(m_y2-m_y1)/10+m_y1, printing);
         } else {
            for (int j=1 ; j<50 ; j++)
               DrawDot(dc, m_x1+j*(m_x2-m_x1)/50, i*(m_y2-m_y1)/10+m_y1, printing);
         }
      }

      for (int i=1 ; i<50 ; i++) {
         for (int j=-4 ; j<5 ; j+=2)
            DrawDot(dc, m_x1+i*(m_x2-m_x1)/50, (m_y2-m_y1)/2+m_y1+j, printing);
      }

      for (int i=1 ; i<10 ; i++) {
         if (i == 5) {
            for (int j=m_y1+1 ; j<m_y2 ; j+=2)
               DrawDot(dc, i*(m_x2-m_x1)/10+m_x1, j, printing);
         } else {
            for (int j=1 ; j<50 ; j++)
               DrawDot(dc, i*(m_x2-m_x1)/10+m_x1, m_y1+j*(m_y2-m_y1)/50, printing);
         }
      }

      for (int i=1 ; i<50 ; i++) {
         for (int j=-4 ; j<5 ; j+=2)
            DrawDot(dc, (m_x2-m_x1)/2+m_x1+j, m_y1+i*(m_y2-m_y1)/50, printing);
      }
   }

   // show horizontal resolution
   if (m_hscaleTable[m_hscale] >= 1000)
     wxst.Printf(wxT("%d us/div  "), m_hscaleTable[m_hscale]/1000);
   else
     wxst.Printf(wxT("%d ns/div  "), m_hscaleTable[m_hscale]);
   if (m_frame->GetActSamplingSpeed() < 1)
     wxst2.Printf(wxT("%1.0lf MS/s Calib"), m_frame->GetActSamplingSpeed()*1000);
   else
     wxst2.Printf(wxT("%1.4lg GS/s Calib"), m_frame->GetActSamplingSpeed());

   wxst = wxst+wxst2;
   dc.SetPen(wxPen(*wxLIGHT_GREY, 1, wxSOLID));
   dc.SetBrush(*wxGREEN);
   dc.SetTextForeground(*wxBLACK);
   dc.GetTextExtent(wxst, &w, &h);
   dc.DrawRoundedRectangle(m_x2-20-w, m_y2+3, w+10, 15, 2);
   dc.DrawText(wxst, m_x2-15-w, m_y2+3);

   // cross out calibration if not valid
   if (m_frame->GetOsci()->GetNumberOfBoards() > 0) {
      if (!m_frame->GetOsci()->IsTCalibrated() || !m_frame->GetOsci()->IsTCalOn()) {
         dc.SetPen(wxPen(*wxRED, 1, wxSOLID));
         dc.DrawLine(m_x2-44, m_y2+10, m_x2-14, m_y2+10);
      }
   }

   // show trigger settings
   int x_start = m_x2 - 20 - w;
   int tc = m_frame->GetTriggerConfig();
   if (tc > 0) {
      wxString wxst1, wxst2;
      wxst1.Append((char)wxT('('));
      for (int i=0 ; i<5 ; i++)
         if (tc & (1<<i)) {
            if (i < 4)
               wxst1.Append((char) (wxT('1'+i)));
            else
               wxst1.Append((char) wxT('E'));
            wxst1.Append((char) wxT('|'));
         }
      for (int i=0 ; i<5 ; i++)
         if (tc & (1<<(i+8))) {
            if (i < 4)
               wxst1.Append((char) (wxT('1'+i)));
            else
               wxst1.Append((char) wxT('E'));
            wxst1.Append((char) wxT('&'));
         }
      if (wxst1.Length() > 0)
         wxst1 = wxst1.Left(wxst1.Length()-1);
      wxst1.Append((char) wxT(')'));

      wxst2.Printf(wxT(" %1.3lf V %1.0lf ns"), m_frame->GetTrgLevel(), m_frame->GetTrgDelay());
      wxst = wxst1+wxst2;
      dc.GetTextExtent(wxst, &w, &h);
      dc.SetPen(wxPen(*wxLIGHT_GREY, 1, wxSOLID));
      dc.SetBrush(m_frame->GetColor(4, printing));
      dc.DrawRoundedRectangle(x_start-20-w-20, m_y2+3, w+10+20, 15, 2);
      dc.DrawText(wxst, x_start-15-w, m_y2+3);
   } else {
      if (m_frame->GetTrgSource() == 4)
        wxst.Printf(wxT("EXT %1.0lf ns"), m_frame->GetTrgDelay());
      else
        wxst.Printf(wxT("%1.3lf V %1.0lf ns"), m_frame->GetTrgLevel(), m_frame->GetTrgDelay());
      dc.GetTextExtent(wxst, &w, &h);
      dc.SetPen(wxPen(*wxLIGHT_GREY, 1, wxSOLID));
      dc.SetBrush(m_frame->GetColor(m_frame->GetTrgSource(), printing));
      dc.DrawRoundedRectangle(x_start-20-w-20, m_y2+3, w+10+20, 15, 2);
      dc.DrawText(wxst, x_start-15-w, m_y2+3);
   }

   wxPoint p[6];
   if (m_frame->GetTrgPolarity() == 0) {
      // positive edge
      p[0] = wxPoint(0, 11);
      p[1] = wxPoint(5, 11);
      p[2] = wxPoint(10, 3);
      p[3] = wxPoint(15, 3);
   } else {
      // negative edge
      p[0] = wxPoint(0,  3);
      p[1] = wxPoint(5,  3);
      p[2] = wxPoint(10,11);
      p[3] = wxPoint(15,11);
   }
   dc.SetPen(wxPen(*wxBLACK, 1, wxSOLID));
   dc.DrawLines(4, p, x_start-20-w-15, m_y2+3);

   dc.SetBrush(*wxGREEN);
   dc.SetPen(wxPen(*wxLIGHT_GREY, 1, wxSOLID));
   x_start = x_start - 20 - w - 15;
   if (m_osci->IsRunning()) {
      wxst.Printf(wxT("%d Acq/s"), m_frame->GetAcqPerSecond());
      dc.GetTextExtent(wxst, &w, &h);
      dc.DrawRoundedRectangle(x_start-20-w, m_y2+3, w+10, 15, 2);
      dc.DrawText(wxst, x_start-15-w, m_y2+3);
   }

   for (int idx=3 ; idx>=0 ; idx--) {
      if (m_chnon[idx] && !m_frame->IsFirst()) {
         // draw marker
         dc.DestroyClippingRegion();
         dc.SetPen(wxPen(*wxLIGHT_GREY, 1, wxSOLID));
         dc.SetBrush(m_frame->GetColor(idx, printing));
         y = (int) ((m_y1+m_y2)/2+m_offset[idx]*1000/10.0/m_scaleTable[m_scale[idx]]*(m_y2-m_y1) + 0.5);

         if (y < m_y1) {
            wxPoint p[3];
            p[0] = wxPoint(10, 0);
            p[1] = wxPoint(17, 7);
            p[2] = wxPoint( 3, 7);
            dc.DrawPolygon(3, p, 0, m_y1);
         } else if (y > m_y2) {
            wxPoint p[3];
            p[0] = wxPoint(10, 7);
            p[1] = wxPoint(17, 0);
            p[2] = wxPoint( 3, 0);
            dc.DrawPolygon(3, p, 0, m_y2-8);
         } else {
            wxPoint p[7];
            p[0] = wxPoint(0,  6);
            p[1] = wxPoint(1,  7);
            p[2] = wxPoint(12, 7);
            p[3] = wxPoint(19, 0);
            p[4] = wxPoint(12,-7);
            p[5] = wxPoint(1, -7);
            p[6] = wxPoint(0, -6);
            dc.DrawPolygon(7, p, 0, y);

            wxst.Printf(wxT("%d"), idx+1);
            dc.GetTextExtent(wxst, &w, &h);
            dc.SetTextForeground(*wxBLACK);
            dc.SetTextBackground(m_frame->GetColor(idx, printing));
            dc.DrawText(wxst, 3, y-h/2);
         }

         if (m_scaleTable[m_scale[idx]] >= 1000)
            wxst.Printf(wxT("%d V/div"), m_scaleTable[m_scale[idx]]/1000);
         else
            wxst.Printf(wxT("%d mV/div"), m_scaleTable[m_scale[idx]]);
         if (m_chnon[idx] == 2)
            wxst+wxT(" CLK");
         dc.GetTextExtent(wxst, &w, &h);
         dc.DrawRoundedRectangle(m_x1 + 10 + idx*100, m_y2+3, w+10, 15, 2);
         dc.DrawText(wxst, m_x1 + 15 + idx*100, m_y2+3);

         // draw waveform
         dc.SetClippingRegion(m_x1+1, m_y1+1, m_x2-m_x1-2, m_y2-m_y1-2);
         dc.SetPen(wxPen(m_frame->GetColor(idx, printing), 1, wxSOLID));
         
         if (m_osci->GetWaveformDepth(idx)) {
            float *wf = m_frame->GetWaveform(idx);
            float *time = m_frame->GetTime(idx);
            m_chn = idx;
            int spacing = timeToX(1/m_osci->GetSamplingSpeed()) - timeToX(0);

            for (int i=0 ; i<m_osci->GetWaveformDepth(idx) ; i++) {
               x = timeToX(time[i]);
               y = voltToY(wf[i]);
               if (x >= m_x1 && x <= m_x2) {
                  if (m_displayLines) {
                     if (i > 0)
                        dc.DrawLine(x_old, y_old, x, y);
                     if (i > 0 && spacing > 5) {
                        // draw points if separate by more than 5 pixels
                        dc.DrawRectangle(x-1, y-1, 3, 3);
                        dc.DrawPoint(x-2, y);
                        dc.DrawPoint(x+2, y);
                        dc.DrawPoint(x, y-2);
                        dc.DrawPoint(x, y+2);
                     }
                  } else
                     DrawDot(dc, x, y, printing);
               }
               x_old = x;
               y_old = y;
            }
         }
      }
   }

   // draw trigger level
   if (m_frame->GetTrgSource() < 4) {
      double v = (m_frame->GetTrgLevel() - m_offset[m_frame->GetTrgSource()])*1000;
      y = (int) ((m_y1+m_y2)/2-(v/10.0/m_scaleTable[m_scale[m_frame->GetTrgSource()]]*(m_y2-m_y1) + 0.5));

      p[0] = wxPoint(-8,  0);
      p[1] = wxPoint( 0, -5);
      p[2] = wxPoint( 0,  5);
      if (m_frame->GetTriggerConfig()) {
         dc.SetBrush(m_frame->GetColor(4, printing));
         dc.SetPen(m_frame->GetColor(4, printing));
      } else {
         dc.SetBrush(m_frame->GetColor(m_frame->GetTrgSource(), printing));
         dc.SetPen(m_frame->GetColor(m_frame->GetTrgSource(), printing));
      }
      dc.DrawPolygon(3, p, m_x2-2, y);
      if (time(NULL) - m_frame->GetLastTriggerUpdate() < 2 && !m_frame->IsFirst()) {
         dc.DrawLine(m_x1, y, m_x2-2, y);
      }
   }

   // draw trigger horizontal position marker
   p[0] = wxPoint(-5, 0);
   p[1] = wxPoint( 4, 0);
   p[2] = wxPoint( 4, 7);
   p[3] = wxPoint( 0, 11);
   p[4] = wxPoint( -1, 11);
   p[5] = wxPoint(-5, 7);
   dc.SetBrush(m_frame->GetColor(m_frame->GetTrgSource(), printing));
   dc.SetPen(m_frame->GetColor(m_frame->GetTrgSource(), printing));
   dc.DrawPolygon(6, p, (wxCoord)((m_frame->GetTrgPosition()-GetScreenOffset()) / GetScreenSize() * (m_x2-m_x1) + m_x1), 
                        (wxCoord)m_y1);

   wxst = _T("T");
   dc.SetPen(wxPen(*wxLIGHT_GREY, 1, wxSOLID));
   dc.SetBrush(m_frame->GetColor(m_frame->GetTrgSource(), printing));
   dc.SetTextForeground(*wxBLACK);
   dc.GetTextExtent(wxst, &w, &h);
   dc.DrawText(wxst, (wxCoord)((m_frame->GetTrgPosition()-GetScreenOffset()) / GetScreenSize() * (m_x2-m_x1) + m_x1 - w/2), 
                     (wxCoord)(m_y1-1));

   // print "TRIG?" or "AUTO" if no recent trigger
   if (m_osci->GetNumberOfBoards() && m_osci->IsIdle()) {
      dc.SetTextForeground(*wxGREEN);
      if (m_osci->GetTriggerMode() == TM_AUTO)
         wxst = _T("AUTO");
      else
         wxst = _T("TRIG?");
      dc.GetTextExtent(wxst, &w, &h);
      dc.DrawText(wxst, m_x2 - w - 2, m_y1 + 1);
   }

   // display date/time
   if (m_displayDateTime) {
      dc.SetTextForeground(*wxGREEN);
      wxst.Printf(wxT("%s"),wxDateTime::Now().Format(_T("%c")).c_str()); 
      dc.GetTextExtent(wxst, &w, &h);
      dc.DrawText(wxst, m_x2 - w - 2, m_y2 - h - 1);
   }

   // display cursor
   if (width > 0 && height > 0) {
      wxPoint mpos((int)(m_mouseX*(width-21)+20), (int)(m_mouseY*(height-21)+1));
      if (m_frame->IsCursorA() || m_frame->IsCursorB()) {

         // find point on waveform closest to mouse position
         FindClosestWafeformPoint(idx_min, x_min, y_min);
         if (m_frame->IsSnap()) {
            x = x_min;
            y = y_min;
         } else {
            x = mpos.x;
            y = mpos.y;
         }

         if (m_frame->ActiveCursor() == 1) {
            m_idxA = idx_min;
            m_xCursorA = (double)(x - 20)/ (width - 21);
            m_yCursorA = (double)(y - 1)/ (height - 21);
            m_uCursorA = YToVolt(y);
            m_tCursorA = XToTime(x);
         } else if (m_frame->ActiveCursor() == 2) {
            m_idxB = idx_min;
            m_xCursorB = (double)(x - 20)/ (width - 21);
            m_yCursorB = (double)(y - 1)/ (height - 21);
            m_uCursorB = YToVolt(y);
            m_tCursorB = XToTime(x);
         }

         // draw cursor A
         if (m_frame->IsCursorA()) {
            dc.SetPen(wxPen(m_frame->GetColor(m_idxA, printing), 1, wxSHORT_DASH));
            int x = (int)(m_xCursorA*(width-21)+20);
            int y = (int)(m_yCursorA*(height-21)+1);
            dc.DrawLine(x, m_y1, x, m_y2);
            dc.DrawLine(m_x1, y, m_x2, y);
            dc.DrawLine(x-3, y, x, y+3);
            dc.DrawLine(x, y+3, x+3, y);
            dc.DrawLine(x+3, y, x, y-3);
            dc.DrawLine(x, y-3, x-3, y);

            dc.SetTextForeground(*wxGREEN);
            dc.GetTextExtent(_T("Cursor A:"), &w, &h);
            dc.DrawText(_T("Cursor A:"), m_x1 + 3, y_top);
            wxString str;
            str.Printf(_T("%1.1lf ns / %1.1lf mV"), m_tCursorA, m_uCursorA); 
            dc.SetTextForeground(m_frame->GetColor(m_idxA, printing));
            dc.DrawText(str, m_x1 + w + 10, y_top);
            y_top += h;
         }

         // draw cursor B
         if (m_frame->IsCursorB()) {
            dc.SetPen(wxPen(m_frame->GetColor(m_idxB, printing), 1, wxDOT));
            int x = (int)(m_xCursorB*(width-21)+20);
            int y = (int)(m_yCursorB*(height-21)+1);
            dc.DrawLine(x, m_y1, x, m_y2);
            dc.DrawLine(m_x1, y, m_x2, y);
            dc.DrawLine(x-3, y, x, y+3);
            dc.DrawLine(x, y+3, x+3, y);
            dc.DrawLine(x+3, y, x, y-3);
            dc.DrawLine(x, y-3, x-3, y);
            
            dc.SetTextForeground(*wxGREEN);
            dc.GetTextExtent(_T("Cursor B:"), &w, &h);
            dc.DrawText(_T("Cursor B:"), m_x1 + 3, y_top);
            wxString str;
            str.Printf(_T("%1.1lf ns / %1.1lf mV"), m_tCursorB, m_uCursorB); 
            dc.SetTextForeground(m_frame->GetColor(m_idxB, printing));
            dc.DrawText(str, m_x1 + w + 10, y_top);
            y_top += h;
         }

         // cursor difference
         if (m_frame->IsCursorA() && m_frame->IsCursorB()) {
            dc.SetTextForeground(*wxGREEN);
            dc.GetTextExtent(_T("Diff:"), &w, &h);
            dc.DrawText(_T("Diff:"), m_x1 + 3, y_top);
            wxString str;
            str.Printf(_T("%1.1lf ns / %1.1lf mV"), m_tCursorB - m_tCursorA, m_uCursorB - m_uCursorA); 
            dc.DrawText(str, m_x1 + w + 10, y_top);
            y_top += h;
         }

         y_top += h; // leave some space
      }
   }

   // count measurements
   for (int idx = n = 0 ; idx<Measurement::N_MEASUREMENTS ; idx++) {
      for (int chn=0 ; chn<4 ; chn++) {
         if (m_frame->GetMeasurement(idx, chn) != NULL)
            n++;
      }
   }

   // display measurements
   dc.SetFont(wxFont(fontSize, wxMODERN, wxNORMAL, wxNORMAL));
   dc.GetTextExtent(_T("A"), &w, &h);
   if (m_frame->IsStat() && n > 0) {
      dc.SetTextForeground(*wxGREEN);
      dc.DrawText(_T("     Min      Max     Mean    Sigma      N"), m_x1+100+80, y_top);
      dc.GetTextExtent(_T("     Min      Max     Mean    Sigma      N"), &w, &h);
      m_MeasX1 = m_x1 + 100 + 80;
      m_MeasX2 = m_MeasX1 + w;
      y_top += h;
   }

   m_MeasY1 = y_top;
   for (int idx = 0 ; idx<Measurement::N_MEASUREMENTS ; idx++) {
      for (int chn=0 ; chn<4 ; chn++) {
         Measurement *m;
         m = m_frame->GetMeasurement(idx, chn);
         m_chn = chn;
         
         if (m != NULL) {

            dc.SetFont(wxFont(fontSize, wxFONTFAMILY_DEFAULT,
                              wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
            dc.SetTextForeground(m_frame->GetColor(chn, printing));
            wxString str;
            str.Printf(_T("%s [CH%d]:"), m_frame->GetMeasurementName(idx).c_str(), chn+1);
            dc.DrawText(str, m_x1 + 3, y_top);

            double x1[2048], y1[2048], x2[2048], y2[2048];

            if (m_osci->GetWaveformDepth(chn)) {
               float *wf1 = m_frame->GetWaveform(chn);
               float *wf2 = m_frame->GetWaveform((chn+1)%4);
               float *time = m_frame->GetTime(chn);
               int   n_inside = 0;

               for (int i=0 ; i<m_osci->GetWaveformDepth(chn) ; i++) {
                  if (timeToX(time[i]) >= m_x1) {
                     x1[n_inside] = time[i];
                     y1[n_inside] = wf1[i];
                     x2[n_inside] = time[i];
                     y2[n_inside] = wf2[i];
                     n_inside++;
                  }
                  if (timeToX(time[i]) > m_x2)
                     break;
               }
               dc.SetFont(wxFont(fontSize, wxMODERN, wxNORMAL, wxNORMAL));
               dc.SetPen(wxPen(m_frame->GetColor(chn, printing), 1, wxSOLID));

               m->Measure(x1, y1, x2, y2, n_inside, false, m_frame->IsIndicator() && m_chnon[chn] ? this : NULL);
               dc.DrawText(m->GetString(), m_x1+100, y_top);

               if (m_frame->IsStat()) {
                  dc.DrawText(m->GetStat(), m_x1+100+80, y_top);
               }
            }

            y_top += h;
         }
      }
   }

   m_MeasY2 = y_top;
   
   dc.DestroyClippingRegion();
}

/*------------------------------------------------------------------*/

void DOScreen::DrawHisto(wxDC& dc, wxCoord width, wxCoord height, bool printing)
{
   int i, x, y, x_old, y_old, w, h, y_top;
   int idx_min;
   wxString wxst, wxst2;
   int histo[1000];
   
   if (m_frame->GetOsci()->SkipDisplay())
      return;
   
   if (!m_frame->IsHist())
      return;

   m_dc = &dc;
   m_x1 = 20;
   m_y1 = height/2+1;
   m_x2 = width-1;
   m_y2 = height-20;
   x_old = y_old = 0;
   y_top = 1;
   
   /* if all waveform channels are off, use full screen */
   if (!m_chnon[0] && !m_chnon[1] && !m_chnon[2] && !m_chnon[3])
      m_y1 = 1;
   
#ifdef _MSC_VER
   int fontSize = 8;
#else
   int fontSize = 10;
#endif   
   
   // set font for all text output
   dc.SetFont(wxFont(fontSize, wxFONTFAMILY_DEFAULT,
                     wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
   
   // draw overall frame
   if (printing) {
      dc.SetBackground(*wxWHITE_BRUSH);
      dc.SetPen(*wxWHITE_PEN);
      dc.SetBrush(*wxWHITE_BRUSH);
   } else {
      dc.SetBackground(*wxBLACK_BRUSH);
      dc.SetPen(*wxBLACK_PEN);
      dc.SetBrush(*wxBLACK_BRUSH);
   }
   
   dc.DrawRectangle(0, height/2, width, height/2);
   dc.SetPen(*wxGREY_PEN);
   dc.DrawRectangle(m_x1, m_y1, m_x2-m_x1, m_y2-m_y1);
   
   for (int idx = 0 ; idx<Measurement::N_MEASUREMENTS ; idx++) {
      for (int chn=0 ; chn<4 ; chn++) {
         Measurement *m = m_frame->GetMeasurement(idx, chn);
         if (m == NULL)
            continue;
         
         /* find min and max of array */
         double *a = m->GetArray();
         int n = m->GetNMeasured();
         double min = a[0];
         double max = a[0];
         for (i=1 ; i<n ; i++) {
            if (a[i] < min)
               min = a[i];
            if (a[i] > max)
               max = a[i];
         }
         
         /* build histogram */
         int nBins = (int) sqrt((double)n);
         if (nBins > 1000)
            nBins = 1000;
         
         for (i=0 ; i<nBins ; i++)
            histo[i] = 0;

         /* fill data into histo */
         for (i=0 ; i<n ; i++) {
            int bin = (int)((a[i]-min)/(max-min)*nBins);
            if (bin >= nBins)
               bin = nBins-1;
            if (bin < 0)
               bin = 0;
            histo[bin]++;
         }

         int hmax = histo[0];
         for (i=1 ; i<nBins ; i++) {
            if (histo[i] > hmax)
               hmax = histo[i];
         }
         
         /* draw histo */
         dc.SetPen(wxPen(m_frame->GetColor(chn, printing), 1, wxSOLID));
         for (i=0 ; i<nBins ; i++) {
            x = m_x1+(int)((double)i/nBins*(m_x2-m_x1));
            y = m_y2-(int)((double)histo[i]/hmax*(m_y2-m_y1-1));
            if (i>0) {
               dc.DrawLine(x_old, y_old, x, y_old);
               dc.DrawLine(x, y_old, x, y);
            }
            x_old = x;
            y_old = y;
         }
         dc.DrawLine(x_old, y_old, m_x2, y_old);
         
         DrawHAxis(dc, m_x1, m_y2, m_x2-m_x1, 3, 4, 5, 7, 0, min, max);
      }
   }
   
   return;
   
   // display cursor
   if (width > 0 && height > 0) {
      wxPoint mpos((int)(m_mouseX*(width-21)+20), (int)(m_mouseY*(height-21)+1));
      if (m_frame->IsCursorA() || m_frame->IsCursorB()) {
         
         // find point on waveform closest to mouse position
         //FindClosestWafeformPoint(idx_min, x_min, y_min);
         //if (m_frame->IsSnap()) {
         //   x = x_min;
         //   y = y_min;
         //} else {
            x = mpos.x;
            y = mpos.y;
         //}
         
         if (m_frame->ActiveCursor() == 1) {
            m_idxA = idx_min;
            m_xCursorA = (double)(x - 20)/ (width - 21);
            m_yCursorA = (double)(y - 1)/ (height - 21);
            m_uCursorA = YToVolt(y);
            m_tCursorA = XToTime(x);
         } else if (m_frame->ActiveCursor() == 2) {
            m_idxB = idx_min;
            m_xCursorB = (double)(x - 20)/ (width - 21);
            m_yCursorB = (double)(y - 1)/ (height - 21);
            m_uCursorB = YToVolt(y);
            m_tCursorB = XToTime(x);
         }
         
         // draw cursor A
         if (m_frame->IsCursorA()) {
            dc.SetPen(wxPen(m_frame->GetColor(m_idxA, printing), 1, wxSHORT_DASH));
            int x = (int)(m_xCursorA*(width-21)+20);
            int y = (int)(m_yCursorA*(height-21)+1);
            dc.DrawLine(x, m_y1, x, m_y2);
            dc.DrawLine(m_x1, y, m_x2, y);
            dc.DrawLine(x-3, y, x, y+3);
            dc.DrawLine(x, y+3, x+3, y);
            dc.DrawLine(x+3, y, x, y-3);
            dc.DrawLine(x, y-3, x-3, y);
            
            dc.SetTextForeground(*wxGREEN);
            dc.GetTextExtent(_T("Cursor A:"), &w, &h);
            dc.DrawText(_T("Cursor A:"), m_x1 + 3, y_top);
            wxString str;
            str.Printf(_T("%1.1lf ns / %1.1lf mV"), m_tCursorA, m_uCursorA); 
            dc.SetTextForeground(m_frame->GetColor(m_idxA, printing));
            dc.DrawText(str, m_x1 + w + 10, y_top);
            y_top += h;
         }
         
         // draw cursor B
         if (m_frame->IsCursorB()) {
            dc.SetPen(wxPen(m_frame->GetColor(m_idxB, printing), 1, wxDOT));
            int x = (int)(m_xCursorB*(width-21)+20);
            int y = (int)(m_yCursorB*(height-21)+1);
            dc.DrawLine(x, m_y1, x, m_y2);
            dc.DrawLine(m_x1, y, m_x2, y);
            dc.DrawLine(x-3, y, x, y+3);
            dc.DrawLine(x, y+3, x+3, y);
            dc.DrawLine(x+3, y, x, y-3);
            dc.DrawLine(x, y-3, x-3, y);
            
            dc.SetTextForeground(*wxGREEN);
            dc.GetTextExtent(_T("Cursor B:"), &w, &h);
            dc.DrawText(_T("Cursor B:"), m_x1 + 3, y_top);
            wxString str;
            str.Printf(_T("%1.1lf ns / %1.1lf mV"), m_tCursorB, m_uCursorB); 
            dc.SetTextForeground(m_frame->GetColor(m_idxB, printing));
            dc.DrawText(str, m_x1 + w + 10, y_top);
            y_top += h;
         }
         
         // cursor difference
         if (m_frame->IsCursorA() && m_frame->IsCursorB()) {
            dc.SetTextForeground(*wxGREEN);
            dc.GetTextExtent(_T("Diff:"), &w, &h);
            dc.DrawText(_T("Diff:"), m_x1 + 3, y_top);
            wxString str;
            str.Printf(_T("%1.1lf ns / %1.1lf mV"), m_tCursorB - m_tCursorA, m_uCursorB - m_uCursorA); 
            dc.DrawText(str, m_x1 + w + 10, y_top);
            y_top += h;
         }
         
         y_top += h; // leave some space
      }
   }
  
}

/*------------------------------------------------------------------*/

#define LN10 2.302585094
#define LOG2 0.301029996
#define LOG5 0.698970005

/* min/max/abs macros */
#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

void DOScreen::DrawHAxis(wxDC &dc, int x1, int y1, int width,
           int minor, int major, int text, int label, int grid, double xmin, double xmax)
{
   double dx, int_dx, frac_dx, x_act, label_dx, major_dx, x_screen, maxwidth;
   int tick_base, major_base, label_base, n_sig1, n_sig2, xs, w, h;
   wxString str;
   double base[] = { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000 };
   
   if (xmax <= xmin || width <= 0)
      return;
   
   /* use 5 as min tick distance */
   dx = (xmax - xmin) / (double) (width / 5);
   
   frac_dx = modf(log(dx) / LN10, &int_dx);
   if (frac_dx < 0) {
      frac_dx += 1;
      int_dx -= 1;
   }
   
   tick_base = frac_dx < LOG2 ? 1 : frac_dx < LOG5 ? 2 : 3;
   major_base = label_base = tick_base + 1;
   
   /* rounding up of dx, label_dx */
   dx = pow(10, int_dx) * base[tick_base];
   major_dx = pow(10, int_dx) * base[major_base];
   label_dx = major_dx;
   
   /* number of significant digits */
   if (xmin == 0)
      n_sig1 = 0;
   else
      n_sig1 = (int) floor(log(fabs(xmin)) / LN10) - (int) floor(log(fabs(label_dx)) / LN10) + 1;
   
   if (xmax == 0)
      n_sig2 = 0;
   else
      n_sig2 =
      (int) floor(log(fabs(xmax)) / LN10) - (int) floor(log(fabs(label_dx)) / LN10) + 1;
   
   n_sig1 = MAX(n_sig1, n_sig2);
   n_sig1 = MAX(n_sig1, 4);
   
   /* determination of maximal width of labels */
   str.Printf(_T("%1.*lG"), n_sig1, floor(xmin / dx) * dx);
   dc.GetTextExtent(str, &w, &h);
   maxwidth = h / 2 * str.Len();
   str.Printf(_T("%1.*lG"), n_sig1, floor(xmax / dx) * dx);
   maxwidth = MAX(maxwidth, h / 2 * str.Len());
   str.Printf(_T("%1.*lG"), n_sig1, floor(xmax / dx) * dx + label_dx);
   maxwidth = MAX(maxwidth, h / 2 * str.Len());
   
   /* increasing label_dx, if labels would overlap */
   while (maxwidth > 0.7 * label_dx / (xmax - xmin) * width) {
      label_base++;
      label_dx = pow(10, int_dx) * base[label_base];
      if (label_base % 3 == 2 && major_base % 3 == 1) {
         major_base++;
         major_dx = pow(10, int_dx) * base[major_base];
      }
   }
   
   x_act = floor(xmin / dx) * dx;
   
   dc.DrawLine(x1, y1, x1 + width, y1);
   
   do {
      x_screen = (x_act - xmin) / (xmax - xmin) * width + x1;
      xs = (int) (x_screen + 0.5);
      
      if (x_screen > x1 + width + 0.001)
         break;
      
      if (x_screen >= x1) {
         if (fabs(floor(x_act / major_dx + 0.5) - x_act / major_dx) <
             dx / major_dx / 10.0) {
            
            if (fabs(floor(x_act / label_dx + 0.5) - x_act / label_dx) <
                dx / label_dx / 10.0) {
               /* label tick mark */
               dc.DrawLine(xs, y1, xs, y1 + text);
               
               /* grid line */
               if (grid != 0 && xs > x1 && xs < x1 + width)
                  dc.DrawLine(xs, y1, xs, y1 + grid);
               
               /* label */
               if (label != 0) {
                  str.Printf(_T("%1.*lG"), n_sig1, x_act);
                  dc.GetTextExtent(str, &w, &h);
                  dc.DrawText(str, (int) xs - w / 2, y1 + label);
               }
            } else {
               /* major tick mark */
               dc.DrawLine(xs, y1, xs, y1 + major);
               
               /* grid line */
               if (grid != 0 && xs > x1 && xs < x1 + width)
                  dc.DrawLine(xs, y1 - 1, xs, y1 + grid);
            }
            
         } else
            /* minor tick mark */
            dc.DrawLine(xs, y1, xs, y1 + minor);
         
      }
      
      x_act += dx;
      
      /* supress 1.23E-17 ... */
      if (fabs(x_act) < dx / 100)
         x_act = 0;
      
   } while (1);
}

/*------------------------------------------------------------------*/

void DOScreen::DrawTcalib(wxDC& dc, wxCoord width, wxCoord height, bool printing)
{
   int x, y, x_old, y_old;
   wxString str;

   m_dc = &dc;
   m_x1 = 20;
   m_y1 = 1;
   m_x2 = width-1;
   m_y2 = height-20;
   x_old = y_old = 0;

#ifdef _MSC_VER
   int fontSize = 8;
#else
   int fontSize = 10;
#endif   

   // draw overall frame
   if (printing) {
      dc.SetBackground(*wxWHITE_BRUSH);
      dc.SetPen(*wxWHITE_PEN);
      dc.SetBrush(*wxWHITE_BRUSH);
   } else {
      dc.SetBackground(*wxBLACK_BRUSH);
      dc.SetPen(*wxBLACK_PEN);
      dc.SetBrush(*wxBLACK_BRUSH);
   }

   dc.DrawRectangle(0, 0, width, height);
   dc.SetPen(*wxGREY_PEN);
   dc.DrawRectangle(m_x1, m_y1, m_x2-m_x1, m_y2-m_y1);

   // draw grid
   if (m_displayShowGrid) {
      dc.SetPen(*wxGREY_PEN);
      for (int i=1 ; i<10 ; i++) {
         if (i == 5) {
            for (int j=m_x1+1 ; j<m_x2 ; j+=2)
               DrawDot(dc, j, i*(m_y2-m_y1)/10+m_y1, printing);
         } else {
            for (int j=1 ; j<50 ; j++)
               DrawDot(dc, m_x1+j*(m_x2-m_x1)/50, i*(m_y2-m_y1)/10+m_y1, printing);
         }
      }

      for (int i=1 ; i<50 ; i++) {
         for (int j=-4 ; j<5 ; j+=2)
            DrawDot(dc, m_x1+i*(m_x2-m_x1)/50, (m_y2-m_y1)/2+m_y1+j, printing);
      }

      for (int i=1 ; i<10 ; i++) {
         if (i == 5) {
            for (int j=m_y1+1 ; j<m_y2 ; j+=2)
               DrawDot(dc, i*(m_x2-m_x1)/10+m_x1, j, printing);
         } else {
            for (int j=1 ; j<50 ; j++)
               DrawDot(dc, i*(m_x2-m_x1)/10+m_x1, m_y1+j*(m_y2-m_y1)/50, printing);
         }
      }

      for (int i=1 ; i<50 ; i++) {
         for (int j=-4 ; j<5 ; j+=2)
            DrawDot(dc, (m_x2-m_x1)/2+m_x1+j, m_y1+i*(m_y2-m_y1)/50, printing);
      }

      dc.SetFont(wxFont(fontSize, wxFONTFAMILY_DEFAULT,
                        wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
      dc.SetTextForeground(*wxWHITE);
      dc.DrawText(wxT("ns"), m_x1 - 15, m_y1 + 10);
      for (int i=-4 ; i<=4 ; i++) {
         int w, h;
         str.Printf(_T("%1d"), -i);
         dc.GetTextExtent(str, &w, &h);
         dc.DrawText(str, m_x1 - 2 - w, m_y1+(5+i)*(m_y2-m_y1)/10 - h/2);
      }

   }

   // draw progress bar
   dc.SetBrush(*wxGREEN);
   dc.SetPen(*wxWHITE_PEN);
   dc.DrawRoundedRectangle(m_x1, m_y2+3, (int)((m_x2-m_x1)*m_frame->GetProgress()/100.0), 6, 2);

   // draw waveform in background
   dc.SetClippingRegion(m_x1+1, m_y1+1, m_x2-m_x1-2, m_y2-m_y1-2);
   dc.SetPen(*wxMEDIUM_GREY_PEN);
   dc.SetTextForeground(*wxLIGHT_GREY);

   if (m_frame->GetProgress() < 100) {
      if (m_osci->GetCurrentBoard()->GetBoardType() == 6)
         str.Printf(_T("Calibration waveform chip %d"), m_osci->GetChip()+1);
      else
         str.Printf(_T("Calibration waveform"));
      dc.DrawText(str, m_x1 + 10, m_y1 + 10);
      
      if (m_osci->GetWaveformDepth(3)) {
         float wf[2048];
         int tc = m_osci->GetCurrentBoard()->GetStopCell(m_osci->GetChip());
         m_osci->GetCurrentBoard()->GetWave(m_osci->GetChip(), 8, wf, true, tc, 0, false);

         float *time = m_osci->GetTime(3);

         for (int i=0 ; i<m_osci->GetWaveformDepth(3) ; i++) {
            x = timeToX(time[i]);
            y = voltToY(wf[i]);
            if (i > 0)
               dc.DrawLine(x_old, y_old, x, y);
            x_old = x;
            y_old = y;
         }
      }
   }

   // draw timing calibration array
   dc.SetClippingRegion(m_x1+1, m_y1+1, m_x2-m_x1-2, m_y2-m_y1-2);
   
   if (m_osci->GetWaveformDepth(3)) {
      float time[2048];

      if (m_osci->GetTimeCalibration(0, time, true)) {

         /* differential nonlinearity */
         dc.SetPen(*wxGREEN_PEN);
         dc.SetTextForeground(*wxGREEN);
         dc.DrawText(wxT("Effective bin width"), m_x1 + 10, m_y1 + 30);
         for (int i=1 ; i<m_osci->GetWaveformDepth(3) ; i++) {
            x = (int)((double)i/m_osci->GetWaveformDepth(3)*(m_x2-m_x1)+m_x1 + 0.5);

            y = (int) ((m_y1+m_y2)/2-time[i]/10.0*(m_y2-m_y1) + 0.5);

            if (i > 1)
               dc.DrawLine(x_old, y_old, x, y);
            x_old = x;

            y_old = y;
         }

         /* integral nonlinearity */
         dc.SetPen(*wxRED_PEN);
         dc.SetTextForeground(*wxRED);
         dc.DrawText(wxT("Integral nonlinearity"), m_x1 + 10, m_y1 + 50);
         m_osci->GetTimeCalibration(1, time, true);
         for (int i=0 ; i<m_osci->GetWaveformDepth(3) ; i++) {
            x = (int)((double)i/m_osci->GetWaveformDepth(3)*(m_x2-m_x1)+m_x1 + 0.5);

            y = (int) ((m_y1+m_y2)/2-time[i]/10.0*(m_y2-m_y1) + 0.5);

            if (i > 0)
               dc.DrawLine(x_old, y_old, x, y);
            x_old = x;
            y_old = y;
         }
      }
   }
   
   dc.DestroyClippingRegion();
}

/*------------------------------------------------------------------*/

void DOScreen::DrawMath(wxDC& dc, wxCoord width, wxCoord height, bool printing)
{
   for (int math=0 ; math < 2 ; math++)
      for (int idx=3 ; idx>=0 ; idx--)
         if (m_mathFlag[math][idx]) {
            if (math == 0);
            else if (math == 1)
               DrawPeriodJitter(dc, idx, printing);
         }
}

/*------------------------------------------------------------------*/

void DOScreen::DrawPeriodJitter(wxDC& dc, int idx, bool printing)
{
   int i, j, w, h, n_pos, n_neg, xs, ys, x_old, y_old;
   double miny, maxy, mean;
   double t_pos[1000], t_neg[1000], t_average[1000], t_delta[1000];
   wxString str;

#ifdef _MSC_VER
   int fontSize = 8;
#else
   int fontSize = 10;
#endif   

   dc.SetFont(wxFont(fontSize, wxMODERN, wxNORMAL, wxNORMAL));
   dc.SetClippingRegion(m_x1+1, m_y1+1, m_x2-m_x1-2, m_y2-m_y1-2);
   dc.SetPen(wxPen(m_frame->GetColor(idx, printing), 1, wxSOLID));

   float *y = m_osci->GetWaveform(idx);
   float *x = m_osci->GetTime(idx);
   int n = m_osci->GetWaveformDepth(idx);

   miny = maxy = y[0];
   for (i=0 ; i<n ; i++) {
      if (y[i] > maxy)
         maxy = y[i];
      if (y[i] < miny)
         miny = y[i];
   }
   mean = (miny + maxy)/2;

   if (maxy - miny < 50)
      return;

   /* search zero crossings */
   n_pos = n_neg = 0;
   for (i=1 ; i<n ; i++) {
      if (y[i] > mean && y[i-1] <= mean) {
         t_pos[n_pos++] = (mean*(x[i]-x[i-1])+x[i-1]*y[i]-x[i]*y[i-1])/(y[i]-y[i-1]); 
      }
      if (y[i] < mean && y[i-1] >= mean && n_pos > 0) {
         t_neg[n_neg++] = (mean*(x[i]-x[i-1])+x[i-1]*y[i]-x[i]*y[i-1])/(y[i]-y[i-1]); 
      }
   }

   if (n_pos == 0 || n_neg == 0)
      return;

   for (n=i=j=0 ;  ; i++,j++) {
      if (i+1 >= n_pos)
         break;
      t_average[n] = (t_pos[i+1] + t_pos[i]) / 2;
      t_delta[n++] = t_pos[i+1] - t_pos[i];
      if (j+1 >= n_neg)
         break;
      t_average[n] = (t_neg[j+1] + t_neg[j]) / 2;
      t_delta[n++] = t_neg[j+1] - t_neg[j];
   }

   for (i=0,mean=0 ; i<n ; i++)
      mean += t_delta[i];
   mean /= n;

   x_old = y_old = 0;
   for (i=0 ; i<n ; i++) {
      xs = timeToX(t_average[i]);
      ys = (int) ((m_y1+m_y2)/2-5*(t_delta[i] - mean)/10.0*(m_y2-m_y1) + 0.5);

      if (i > 0)
         dc.DrawLine(x_old, y_old, xs, ys);
      x_old = xs;
      y_old = ys;
   }

   str.Printf(_T("%1.3lf ns"), mean);
   dc.SetPen(wxPen(*wxLIGHT_GREY, 1, wxSOLID));
   dc.SetBrush(*wxBLACK);
   dc.GetTextExtent(str, &w, &h);
   dc.DrawRoundedRectangle(m_x1+1, m_y1+(m_y2-m_y1)/2 - h/2, w+5, h, 2);
   dc.DrawText(str, m_x1 + 3, m_y1+(m_y2-m_y1)/2 - h/2);

   str.Printf(_T("+0.2 ns"));
   dc.GetTextExtent(str, &w, &h);
   dc.DrawRoundedRectangle(m_x1+1, m_y1+4*(m_y2-m_y1)/10 - h/2, w+5, h, 2);
   dc.DrawText(str, m_x1+3, m_y1+4*(m_y2-m_y1)/10 - h/2);

   str.Printf(_T("-0.2 ns"));
   dc.GetTextExtent(str, &w, &h);
   dc.DrawRoundedRectangle(m_x1+1, m_y1+6*(m_y2-m_y1)/10 - h/2, w+5, h, 2);
   dc.DrawText(str, m_x1+3, m_y1+6*(m_y2-m_y1)/10 - h/2);
}

/*------------------------------------------------------------------*/

void DOScreen::OnSize(wxSizeEvent& event)
{
   wxWindow::Refresh();
}

/*------------------------------------------------------------------*/

void DOScreen::SetScale(int idx, int scale)
{
   m_scale[idx] = scale;
}

/*------------------------------------------------------------------*/

void DOScreen::SetHScale(int hscale)       
{ 
   m_hscale = hscale; 
   m_screenSize = m_hscaleTable[hscale] * 10;
}

/*------------------------------------------------------------------*/

void DOScreen::SetHScaleInc(int increment)
{
   int s = m_hscale += increment;
   if (s < 0)
      s = 0;

   /* limit to min 500 ns/div */
   if (s > 8)
      s = 8;

   SetHScale(s);
}

/*------------------------------------------------------------------*/

void DOScreen::OnMouse(wxMouseEvent& event)
{ 
   wxSize s;
   int x, y;

   if (m_clientHeight > 21 && m_clientWidth > 21) {
      m_mouseX = (double) (event.GetPosition().x - 20) / (m_clientWidth - 21);
      m_mouseY = (double) (event.GetPosition().y - 1) / (m_clientHeight - 21);
   }

   x = event.GetPosition().x - 20;
   y = event.GetPosition().y - 1;
   if (x > m_MeasX1 && x < m_MeasX2 &&
       y > m_MeasY1 && y < m_MeasY2) {
      SetCursor(wxCURSOR_HAND);
      if (event.LeftDown())
         m_frame->StatReset();
   } else
      SetCursor(wxNullCursor);

   if (event.LeftDown()) {
      this->SetFocus();
      if (m_frame->ActiveCursor() == 1)
         m_frame->SetActiveCursor(0);
      else if (m_frame->ActiveCursor() == 2)
         m_frame->SetActiveCursor(0);
      else {
         if (m_frame->IsCursorA() && !m_frame->IsCursorB())
            m_frame->SetActiveCursor(1);
         else if (!m_frame->IsCursorA() && m_frame->IsCursorB())
            m_frame->SetActiveCursor(2);
         else if (m_frame->IsCursorA() && m_frame->IsCursorB()){
            // activate cursor closes to mouse pointer
            double d1 = (m_xCursorA-m_mouseX)*(m_xCursorA-m_mouseX) +
                     (m_yCursorA-m_mouseY)*(m_yCursorA-m_mouseY);
            double d2 = (m_xCursorB-m_mouseX)*(m_xCursorB-m_mouseX) +
                     (m_yCursorB-m_mouseY)*(m_yCursorB-m_mouseY);
            m_frame->SetActiveCursor(d1 < d2 ? 1 : 2);
         }
      }
   }

   if (event.RightDown()) {
      m_frame->ToggleControls();
   }

   if (event.GetWheelRotation()) {
      m_frame->ChangeHScale(event.GetWheelRotation());
   }

   wxWindow::Refresh();
}

