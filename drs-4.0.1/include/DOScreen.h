/*
 * DOScreen.h
 * DRS oscilloscope screen header file
 * $Id: DOScreen.h 20380 2012-11-13 13:38:01Z ritt $
 */

class Osci;
class DOFrame;

enum PaintModes {
   kPMWaveform,
   kPMTimeCalibration,
};

class DOScreen : public wxWindow
{
public:
   DOScreen(wxWindow* parent, Osci *osci, DOFrame *frame);
   ~DOScreen();
   void SetChnOn(int i, int value ) { m_chnon[i] = value; }
   int  GetChnOn(int i)             { return m_chnon[i];  }
   int  GetCurChn()                 { return m_chn; }
   void SetPaintMode(int pm)        { m_paintMode = pm; }
   void SetPos(int i, double value) { m_offset[i] = value; }
   void SetScale(int i, int sclae);
   void SetHScale(int hscale);
   void SetHScaleInc(int increment);
   int  GetPaintMode()              { return m_paintMode; }
   void SetScreenOffset(int offset) { m_screenOffset = offset; }
   int  GetScreenSize()             { return m_screenSize; }
   int  GetScreenOffset()           { return m_screenOffset; }
   int  GetScaleIndex(int i)        { return m_scale[i];  }
   double GetScale(int i)           { return m_scaleTable[m_scale[i]]; }
   double GetOffset(int i)          { return m_offset[i]; }
   int  GetHScale()                 { return m_hscale;    }
   void SetDisplayDateTime(bool flag) { m_displayDateTime = flag; }
   void SetDisplayShowGrid(bool flag) { m_displayShowGrid = flag; }
   void SetDisplayLines(bool flag)    { m_displayLines = flag; }
   void SetDisplayMode(int mode, int n) { m_displayMode = mode; m_displayN = n; }
   void SetMathDisplay(int id, bool flag);
   wxDC *GetDC()                    { return m_dc; }
   int  GetX1() { return m_x1; }
   int  GetX2() { return m_x2; }
   int  timeToX(float t);
   int  voltToY(float v);
   int  voltToY(int chn, float v);
   double XToTime(int x);
   double YToVolt(int y);
   double YToVolt(int chn, int y);
   double GetT1();
   double GetT2();
   
   static const int m_scaleTable[10];
   static const int m_hscaleTable[13];
   
   // event handlers
   void OnPaint(wxPaintEvent& event);
   void OnSize(wxSizeEvent& event);
   
   // drawing routines
   void DrawWaveform(wxDC& dc, wxCoord w, wxCoord h, bool printing);
   void DrawHisto(wxDC& dc, wxCoord w, wxCoord h, bool printing);
   void DrawTcalib(wxDC& dc, wxCoord w, wxCoord h, bool printing);
   void DrawMath(wxDC& dc, wxCoord width, wxCoord height, bool printing);
   void DrawPeriodJitter(wxDC& dc, int chn, bool printing);
   void DrawHAxis(wxDC &dc, int x1, int y1, int width,
                  int minor, int major, int text, int label, int grid, double xmin, double xmax);
   
   void OnMouse(wxMouseEvent& event);
   
private:
   // any class wishing to process wxWidgets events must use this macro
   DECLARE_EVENT_TABLE()
   
   // pointer for main Osci object
   Osci *m_osci;
   
   // pointer to DOFrame object
   DOFrame *m_frame;
   
   // coordinates of display area
   int m_x1, m_x2, m_y1, m_y2;
   
   // current device context
   wxDC *m_dc;
   
   // stop watch for screen updates
   wxStopWatch m_sw;
   
   // paing mode
   int m_paintMode;
   
   // curent channel index
   int m_chn;
   
   // offset and size of display area in ns
   int m_screenSize, m_screenOffset;
   
   // cursor variables
   int m_clientHeight, m_clientWidth;
   double m_mouseX;
   double m_mouseY;
   int m_MeasX1, m_MeasX2, m_MeasY1, m_MeasY2;
   
   double m_xCursorA, m_xCursorB, m_yCursorA, m_yCursorB;
   int m_idxA, m_idxB;
   double m_uCursorA, m_uCursorB, m_tCursorA, m_tCursorB;
   
   // waveform propoerties
   int     m_chnon[4];
   double  m_offset[4];
   int     m_scale[4];
   int     m_hscale;
   
   // math display
   bool    m_mathFlag[2][4];
   
   // display properties
   bool m_displayDateTime, m_displayShowGrid, m_displayLines;
   int  m_displayMode, m_displayN;
   
   // grid drawing (screen vs. printer)
   void DrawDot(wxDC& dc, wxCoord w, wxCoord h, bool printing);
   
   // find waveform point close to mouse cursor
   bool FindClosestWafeformPoint(int& idx_min, int& x_min, int& y_min);
   
   // optional debug message
   char m_debugMsg[80];
};
