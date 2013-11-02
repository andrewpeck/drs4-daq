/*
 * Osci.cpp
 * DRS oscilloscope main class
 * $Id: DOFrame.cpp 20427 2012-11-28 09:33:02Z ritt $
 */

#define MULTI_THREAD_READOUT

#include "DRSOscInc.h"

#include "pos.xpm"
#include "neg.xpm"

#ifndef O_TEXT
#define O_TEXT 0
#define O_BINARY 0
#endif

char svn_revision[] = "$Id: DOFrame.cpp 20427 2012-11-28 09:33:02Z ritt $";

char drsosc_version[] = "4.0.0";

// critical section between main and event processing thread
wxCriticalSection *g_epcs;

/*------------------------------------------------------------------*/

class MyPrintout: public wxPrintout
{
public:
   MyPrintout(DOScreen *screen, const wxChar *title):wxPrintout(title) { m_screen = screen; }
   bool OnPrintPage(int page);
private:
   DOScreen *m_screen;
};

/*------------------------------------------------------------------*/

BEGIN_EVENT_TABLE(DOFrame, wxFrame)
   EVT_TIMER        (wxID_ANY, DOFrame::OnTimer )
   EVT_MENU         (wxID_EXIT, DOFrame::OnExit )
END_EVENT_TABLE()

DOFrame::DOFrame( wxWindow* parent )
:
DOFrame_fb( parent )
{
   // colors for four channels
   m_color[0] = wxColor(255, 255,   0); // yellow
   m_color[1] = wxColor(170, 170, 255); // light blue
   m_color[2] = wxColor(255, 150, 150); // light red
   m_color[3] = wxColor(150, 255, 150); // light green
   m_color[4] = wxColor(170, 170, 170); // grey for ext trigger

   // colors for printing
   m_pcolor[0] = wxColor(128, 128,   0); // dark yellow
   m_pcolor[1] = wxColor(  0,   0, 255); // blue
   m_pcolor[2] = wxColor(255,   0,   0); // red
   m_pcolor[3] = wxColor(  0, 255,   0); // green
   m_pcolor[4] = wxColor(128, 128, 128); // grey for ext trigger

   // initialize variables
   m_acqPerSecond = 0;
   m_lastTriggerUpdate = time(NULL);
   m_reqSamplingSpeed = 2;
   m_actSamplingSpeed = 0;
   m_WFFile = NULL;
   m_WFfd = 0;
   m_actCursor = 0;
   m_cursorA = m_cursorB = false;
   m_snap = true;
   m_hideControls = false;
   m_stat = true;
   m_hist = false;
   m_indicator = true;
   m_first = true;
   m_freqLocked = false;
   m_nStat = 1000;
   m_progress = 0;
   m_oldIdle = false;

   m_configDialog = NULL;
   m_measureDialog = NULL;
   m_triggerDialog = NULL;
   m_displayDialog = NULL;
   m_epthread = NULL;
   g_epcs = NULL;

   // initialize source
   m_board = 0;
   m_firstChannel = 0;
   m_chnSection = 0;

   // initialize settings
   m_trgMode = TM_AUTO;
   m_trgSource = 0; // CH1
   m_trgLevel = 0.25;
   m_trgDelay = 0;
   m_trgDelayNs = 0;
   m_trgNegative = false;
   m_trgCorr = true;
   m_trgConfig = 0;
   m_refClk = false; // internal Refclk
   
   m_HScale = 4;  // 20 ns / div.
   m_HOffset = 0;

   for (int i=0 ; i<4 ; i++) {
      m_chnOn[i] = false;
      m_chnOffset[i] = 0;
      m_chnScale[i] = 6; // 100 mV / div
   }

   m_chnOn[0] = true;
   m_clkOn = false;
   m_range = 0;
   m_spikeRemoval = true;

   // overwrite settings from configuration file
   LoadConfig(m_xmlError, sizeof(m_xmlError));

   // set controls
   m_slTrgLevel->SetValue((int)(-m_trgLevel*1000));
   if (m_trgMode == TM_AUTO)
      m_rbAuto->SetValue(1);
   else
      m_rbNormal->SetValue(1);
   if (m_trgNegative)
      m_bpPolarity->SetBitmapLabel(wxBitmap(neg_xpm));
   else
      m_bpPolarity->SetBitmapLabel(wxBitmap(pos_xpm));
   switch (m_trgSource) {
     case 0: m_rbS0->SetValue(1); break;
     case 1: m_rbS1->SetValue(1); break;
     case 2: m_rbS2->SetValue(1); break;
     case 3: m_rbS3->SetValue(1); break;
     case 4: m_rbS4->SetValue(1); break;
   }
   m_slTrgDelay->SetValue(100-m_trgDelay);
   if (m_trgConfig) {
      m_rbS0->Enable(false);
      m_rbS1->Enable(false);
      m_rbS2->Enable(false);
      m_rbS3->Enable(false);
      m_rbS4->Enable(false);
   }

   m_btCh1->SetValue(m_chnOn[0]);
   m_btCh2->SetValue(m_chnOn[1]);
   m_btCh3->SetValue(m_chnOn[2]);
   m_btCh4->SetValue(m_chnOn[3]);

   m_slPos1->SetValue(m_chnOffset[0]/2);
   m_slPos2->SetValue(m_chnOffset[1]/2);
   m_slPos3->SetValue(m_chnOffset[2]/2);
   m_slPos4->SetValue(m_chnOffset[3]/2);

   // create Osci object
#ifdef MULTI_THREAD_READOUT
   m_osci = new Osci(m_reqSamplingSpeed, true);
#else
   m_osci = new Osci(m_reqSamplingSpeed, false);
#endif

   // update sampling speed from calibration of first board
   m_actSamplingSpeed = m_osci->GetSamplingSpeed();
   m_reqSamplingSpeed = ((int)(m_actSamplingSpeed*100+0.5))/100.0;
   m_osci->SetSamplingSpeed(m_actSamplingSpeed);

   // set previous trigger delay
   m_osci->SetTriggerDelay(m_trgDelay);
   m_trgDelayNs = m_osci->GetTriggerDelayNs();

   // enable trigger config button for evaluation boards V4.0
   if (m_osci->GetNumberOfBoards() > 0)
      EnableTriggerConfig(m_osci->GetCurrentBoard()->GetBoardType() == 8);

   // create Measurement objects
   for (int i=0 ; i<Measurement::N_MEASUREMENTS ; i++)
      for (int chn=0 ; chn<4 ; chn++) {
         m_measurement[i][chn] = new Measurement(i);
         m_measFlag[i][chn] = false;
      }

   // create screen object
   m_screen = new DOScreen(m_pnScreen, m_osci, this);
   wxBoxSizer *vbox1 = new wxBoxSizer(wxVERTICAL);
   vbox1->Add(m_screen, 1, wxEXPAND);
   m_pnScreen->SetSizer(vbox1);

   // set initial horizontal scale and offset
   m_screen->SetHScale(m_HScale);

   // calculate trigger position from screen offset
   double trgFrac = (GetTrgPosition() - m_HOffset) / m_screen->GetScreenSize();
   RecalculateHOffset(trgFrac);

   wxString wxstr;
   if (DOScreen::m_hscaleTable[m_screen->GetHScale()] >= 1000) {
      wxstr.Printf(_T("%d us/div"), DOScreen::m_hscaleTable[m_HScale]/1000);
   } else {
      wxstr.Printf(_T("%d ns/div"), DOScreen::m_hscaleTable[m_HScale]);
   }
   m_stHScale->SetLabel(wxstr);

   // set initial channel settings
   
#ifdef _MSC_VER
   /* adjust font size under Windows */
   m_stScale1->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT,
                              wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
   m_stScale2->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT,
                              wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
   m_stScale3->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT,
                              wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
   m_stScale4->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT,
                              wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));   
#endif
   
   for (int i=0 ; i<4 ; i++) {
      m_screen->SetChnOn(i, m_chnOn[i]);
      m_osci->SetChnOn(i, m_chnOn[i]);
      m_screen->SetPos(i, m_chnOffset[i]/1000.0);
      m_screen->SetScale(i, m_chnScale[i]);

      wxString wxst;
      if (DOScreen::m_scaleTable[m_chnScale[i]] >= 1000)
        wxst.Printf(_T("%d V"), DOScreen::m_scaleTable[m_chnScale[i]]/1000);
      else
        wxst.Printf(_T("%dmV"), DOScreen::m_scaleTable[m_chnScale[i]]);

      if (i == 0) m_stScale1->SetLabel(wxst);
      if (i == 1) m_stScale2->SetLabel(wxst);
      if (i == 2) m_stScale3->SetLabel(wxst);
      if (i == 3) m_stScale4->SetLabel(wxst);
   }

   m_osci->SetClkOn(m_clkOn);
   m_osci->SelectSource(m_board, m_firstChannel, m_chnSection);
   m_osci->SetInputRange(m_range);
   m_osci->SetSpikeRemoval(m_spikeRemoval);

   // update sampling speed according to refclk
   SetRefclk(m_refClk);

   // create timer
   m_timer = new wxTimer(this);

   // create modeless dialog boxes
   m_configDialog = new ConfigDialog(this);
   m_measureDialog = new MeasureDialog(this);
   m_triggerDialog = new TriggerDialog(this);
   m_displayDialog = new DisplayDialog(this);

   // initialize status bar
   CreateStatusBar();
   UpdateStatusBar();

   // issue warning if timing calibration not valid
   m_osci->CheckTimingCalibration();

   // start event processing thread
#ifdef MULTI_THREAD_READOUT
   g_epcs = new wxCriticalSection();
   m_epthread = new EPThread(this);
#endif

   // Start acquisition
   m_osci->SetTriggerMode(m_trgMode);
   m_osci->SetTriggerSource(m_trgSource);
   m_osci->SetTriggerLevel(m_trgLevel, m_trgNegative);
   m_osci->SetTriggerDelay(m_trgDelay);
   m_trgDelayNs = m_osci->GetTriggerDelayNs();
   m_osci->SetChnOn(0, true);

   m_single = false;
   m_running = true;
   m_rearm = false;
   m_osci->Enable(true);
   m_osci->SetRunning(m_running);
   m_timer->Start(100, true);
   m_acquisitions = 0;
   m_stopWatch.Start();
   m_stopWatch1.Start();
}

/*------------------------------------------------------------------*/

DOFrame::~DOFrame()
{
   m_running = false;
   if (m_timer->IsRunning())
      m_timer->Stop();
   delete m_timer;

   SaveConfig();

   if (m_epthread) {
      m_epthread->Delete();
      while (!m_epthread->IsFinished())
         wxThread::Sleep(10);
   }

   if (g_epcs)
      delete g_epcs;

   for (int i=0 ; i<Measurement::N_MEASUREMENTS ; i++)
      for (int chn=0 ; chn<4 ; chn++)
         delete m_measurement[i][chn];

   delete m_osci;
   delete m_screen;
}

/*------------------------------------------------------------------*/

void DOFrame::UpdateWaveforms()
{
   if (m_epthread) {
      g_epcs->Enter();
      memcpy(m_time, m_epthread->GetTime(), m_osci->GetWaveformDepth(0)*sizeof(float));
      for (int i=0 ; i<4 ; i++)
         memcpy(m_waveform[i], m_epthread->GetWaveform(i), m_osci->GetWaveformDepth(i)*sizeof(float));
      g_epcs->Leave();
   }
}

/*------------------------------------------------------------------*/

float *DOFrame::GetTime(int c)
{
   if (m_epthread)
      return m_time;
   else
      return m_osci->GetTime(c);
}

/*------------------------------------------------------------------*/

float *DOFrame::GetWaveform(int c)
{
   if (m_epthread)
      return m_waveform[c];
   else
      return m_osci->GetWaveform(c);
}

/*------------------------------------------------------------------*/

void DOFrame::UpdateStatusBar()
{
   if (m_osci->GetNumberOfBoards() == 0) {
      SetStatusText(_T("No DRS boards found!"));
   } else {
      DRSBoard *b = m_osci->GetCurrentBoard();
      wxString wxstr;

#ifdef HAVE_VME
      if (b->GetTransport() == 1)
         wxstr.Printf(_T("Connected to VME board slot %2d %s, serial #%d, firmware revision %d, T=%1.1lf C"), 
            (b->GetSlotNumber() >> 1)+2, ((b->GetSlotNumber() & 1) == 0) ? "upper" : "lower", 
            b->GetBoardSerialNumber(), b->GetFirmwareVersion(), b->GetTemperature());
      else
#endif
         wxstr.Printf(_T("Connected to USB board #%d, serial #%d, firmware revision %d, T=%1.1lf C"), 
            b->GetSlotNumber(), b->GetBoardSerialNumber(), b->GetFirmwareVersion(), b->GetTemperature());

      SetStatusText(wxstr);
   }
}

/*------------------------------------------------------------------*/

void DOFrame::SaveConfig()
{
   char str[256], name[256];

#ifdef OS_DARWIN
   MXML_WRITER *xml = mxml_open_file("DRSOsc.app/drsosc.cfg");
#else
   MXML_WRITER *xml = mxml_open_file("drsosc.cfg");
#endif
   if (xml == NULL) 
      return;

   mxml_start_element(xml, "DRSOsc");

   sprintf(str, "%1.3lf", m_trgLevel);
   mxml_write_element(xml, "TrgLevel", str);
   sprintf(str, "%d", m_trgMode);
   mxml_write_element(xml, "TrgMode", str);
   sprintf(str, "%d", m_trgNegative);
   mxml_write_element(xml, "TrgNegative", str);
   sprintf(str, "%d", m_trgSource);
   mxml_write_element(xml, "TrgSource", str);
   sprintf(str, "%d", m_trgDelay);
   mxml_write_element(xml, "TrgDelay", str);
   sprintf(str, "%d", m_trgConfig);
   mxml_write_element(xml, "TrgConfig", str);

   sprintf(str, "%d", m_HScale);
   mxml_write_element(xml, "HScale", str);
   sprintf(str, "%1.3lg", m_reqSamplingSpeed);
   mxml_write_element(xml, "SamplingSpeed", str);
   sprintf(str, "%d", m_freqLocked);
   mxml_write_element(xml, "FreqLocked", str);
   sprintf(str, "%d", m_HOffset);
   mxml_write_element(xml, "HOffset", str);

   for (int i=0 ; i<4 ; i++) {
      sprintf(name, "ChnOn%d", i);
      sprintf(str, "%d", m_chnOn[i]);
      mxml_write_element(xml, name, str);
      sprintf(name, "ChnOffset%d", i);
      sprintf(str, "%d", m_chnOffset[i]);
      mxml_write_element(xml, name, str);
      sprintf(name, "ChnScale%d", i);
      sprintf(str, "%d", m_chnScale[i]);
      mxml_write_element(xml, name, str);
   }

   sprintf(str, "%d", m_chnSection);
   mxml_write_element(xml, "ChnSection", str);
   
   sprintf(str, "%d", m_clkOn);
   mxml_write_element(xml, "ClkOn", str);

   sprintf(str, "%1.2lf", m_range);
   mxml_write_element(xml, "Range", str);

   sprintf(str, "%d", m_spikeRemoval);
   mxml_write_element(xml, "SpikeRemoval", str);

   sprintf(str, "%d", (int)m_refClk);
   mxml_write_element(xml, "ExtRefclk", str);

   mxml_end_element(xml);
   mxml_close_file(xml);
}

/*------------------------------------------------------------------*/

void DOFrame::LoadConfig(char *error, int size)
{
#ifdef OS_DARWIN
   PMXML_NODE root = mxml_parse_file("DRSOsc.app/drsosc.cfg", error, size, NULL);
#else
   PMXML_NODE root = mxml_parse_file("drsosc.cfg", error, size, NULL);
#endif
   if (root == NULL)
      return;

   PMXML_NODE node = mxml_find_node(root, "DRSOsc/TrgLevel");
   if (node) m_trgLevel = atof(mxml_get_value(node));
   node = mxml_find_node(root, "DRSOsc/TrgMode");
   if (node) m_trgMode = atoi(mxml_get_value(node));
   node = mxml_find_node(root, "DRSOsc/TrgNegative");
   if (node) m_trgNegative = atoi(mxml_get_value(node)) == 1;
   node = mxml_find_node(root, "DRSOsc/TrgSource");
   if (node) m_trgSource = atoi(mxml_get_value(node));
   node = mxml_find_node(root, "DRSOsc/TrgDelay");
   if (node) m_trgDelay = atoi(mxml_get_value(node));
   node = mxml_find_node(root, "DRSOsc/TrgConfig");
   if (node) m_trgConfig = atoi(mxml_get_value(node));
   node = mxml_find_node(root, "DRSOsc/HScale");
   if (node) m_HScale = atoi(mxml_get_value(node));
   node = mxml_find_node(root, "DRSOsc/SamplingSpeed");
   if (node) m_reqSamplingSpeed = atof(mxml_get_value(node));
   node = mxml_find_node(root, "DRSOsc/FreqLocked");
   if (node) m_freqLocked = atoi(mxml_get_value(node)) == 1;
   node = mxml_find_node(root, "DRSOsc/HOffset");
   if (node) m_HOffset = atoi(mxml_get_value(node));

   for (int i=0 ; i<4 ; i++) {
      char str[256];
      sprintf(str, "DRSOsc/ChnOn%d", i);
      node = mxml_find_node(root, str);
      if (node) m_chnOn[i] = atoi(mxml_get_value(node)) == 1;
      sprintf(str, "DRSOsc/ChnOffset%d", i);
      node = mxml_find_node(root, str);
      if (node) m_chnOffset[i] = atoi(mxml_get_value(node));
      sprintf(str, "DRSOsc/ChnScale%d", i);
      node = mxml_find_node(root, str);
      if (node) m_chnScale[i] = atoi(mxml_get_value(node));
   }

   node = mxml_find_node(root, "DRSOsc/ChnSection");
   if (node) m_chnSection = atoi(mxml_get_value(node));

   node = mxml_find_node(root, "DRSOsc/ClkOn");
   if (node) m_clkOn = atoi(mxml_get_value(node)) == 1;

   node = mxml_find_node(root, "DRSOsc/Range");
   if (node) m_range = atof(mxml_get_value(node));

   node = mxml_find_node(root, "DRSOsc/SpikeRemoval");
   if (node) m_spikeRemoval = atoi(mxml_get_value(node)) == 1;

   node = mxml_find_node(root, "DRSOsc/ExtRefclk");
   if (node) m_refClk = atoi(mxml_get_value(node)) == 1;

   mxml_free_tree(root);
}

/*------------------------------------------------------------------*/

void DOFrame::OnSave(wxCommandEvent& WXUNUSED(event))
{
   wxString filename;

   if (!m_WFFile && !m_WFfd) {
      filename = wxFileSelector(_T("Choose a file to write to"), wxEmptyString, wxEmptyString, _T(".xml"), _T("XML file (*.xml)|*.xml|Binary file (*.dat)|*.dat"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

      if (!filename.empty()) {
         m_btSave->SetLabel(_T("Close"));
         m_btSave->SetToolTip(_T("Stop saving waveforms"));

//	 if (filename.Find(_T(".")) != wxNOT_FOUND) {
//		 m_WFfd = 0;
//		 m_WFFile = mxml_open_file(filename.char_str());
//		 if (m_WFFile)
//			 mxml_start_element(m_WFFile, "DRSOSC");
//	 } 
//	 else {
		 m_WFFile = NULL;
		 m_WFfd = open(filename.char_str(), O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0644);
		 assert(m_WFfd > 0);
//	 }
	 m_osci->SetEventSerial(0);
      }
   } else {
      CloseWFFile(false);
   }
}

/*------------------------------------------------------------------*/

void DOFrame::OnPrint(wxCommandEvent& WXUNUSED(event))
{
   wxPrinter p;
   MyPrintout printout(m_screen, _T("My printout"));
   p.Print(this, &printout, true);
}

/*------------------------------------------------------------------*/

bool MyPrintout::OnPrintPage(int page)
{
   wxCoord w, h;

   wxDC *dc = wxPrintout::GetDC();
   if (dc == NULL)
      return false;

   dc->GetSize(&w, &h);
   FitThisSizeToPage(wxSize(800, 600));
   m_screen->DrawWaveform(*dc, 800, 600, true);

   return true;
}

/*------------------------------------------------------------------*/

void DOFrame::OnConfig( wxCommandEvent& event )
{
   m_configDialog->Show(true);
}

/*------------------------------------------------------------------*/

void DOFrame::OnMeasure( wxCommandEvent& event )
{
   m_measureDialog->Show(true);
}

/*------------------------------------------------------------------*/

void DOFrame::OnDisplay( wxCommandEvent& event )
{
   m_displayDialog->Show(true);
}

/*------------------------------------------------------------------*/

void DOFrame::OnExit( wxCommandEvent& event )
{
   Close(true);
}

/*------------------------------------------------------------------*/

void DOFrame::OnAbout( wxCommandEvent& event )
{
   AboutDialog ad(this);
   ad.ShowModal();
}

/*------------------------------------------------------------------*/

void DOFrame::OnTrgLevelChange(wxScrollEvent& event)
{
   double f = (-m_slTrgLevel->GetValue()) / 1000.0; // -0.5 ... 0.5
   
   /* convert to voltage according to screen settings */
   int chn = m_osci->GetTriggerSource();
   m_trgLevel = m_screen->GetScale(chn)*10*f/1000 + m_screen->GetOffset(chn);
   m_lastTriggerUpdate = time(NULL);
   m_osci->SetTriggerLevel(m_trgLevel, m_trgNegative);
   m_screen->Refresh();
}

/*------------------------------------------------------------------*/

void DOFrame::OnTrgDelayChange(wxScrollEvent& event)
{
   m_trgDelay = 100-m_slTrgDelay->GetValue();
   m_osci->SetTriggerDelay(m_trgDelay);
   m_trgDelayNs = m_osci->GetTriggerDelayNs();
   m_screen->Refresh();
}

/*------------------------------------------------------------------*/

double DOFrame::GetTrgPosition()
{
   // return current trigger position in ns relative to left edge of waveform
   return m_osci->GetWaveformLength() - m_trgDelayNs;
}

/*------------------------------------------------------------------*/

void DOFrame::OnTrigger(wxCommandEvent& event)
{
   if (event.GetId() == ID_RUN) {
      if (m_running) {
         m_running = false;
         m_single = false;
         m_osci->SetRunning(false);
         m_osci->SetSingle(false);
         m_btRun->SetLabel(_T("Run"));
         m_acqPerSecond = 0;
         m_acquisitions = 0;
         m_oldIdle = false;
      } else {
         m_running = true;
         m_single = false;
         m_osci->SetSingle(false);
         m_osci->SetRunning(true);
         m_btRun->SetLabel(_T("Stop"));
         m_acquisitions = 0;
         m_oldIdle = false;
         m_stopWatch.Start();
         m_stopWatch1.Start();
      }
   } else if (event.GetId() == ID_SINGLE) {
      if (m_running)
         m_osci->SetRunning(false);
      m_running = false;
      m_osci->SetSingle(true);
      m_single = true;
      m_btRun->SetLabel(_T("Run"));

      if (m_osci->IsArmed()) {
         /* no trigger happened, so issue a software trigger */
         m_osci->SingleTrigger();
         m_rearm = true;
      } else
         m_osci->Start();

      m_acqPerSecond = 0;
      m_acquisitions = 0;
   } else if (event.GetId() == ID_TRGCFG) {
      m_triggerDialog->Show();
   }

   m_screen->Refresh();
}

/*------------------------------------------------------------------*/

void DOFrame::OnTrgButton(wxCommandEvent& event)
{
   if (event.GetId() == ID_TR_NORMAL) {
      m_trgMode = TM_NORMAL;
   } else if (event.GetId() == ID_TR_AUTO) {
      m_trgMode = TM_AUTO;
   } else if (event.GetId() == ID_TR_POLARITY) {
      m_trgNegative = !m_trgNegative;
      if (m_trgNegative)
         m_bpPolarity->SetBitmapLabel(wxBitmap(neg_xpm));
      else
         m_bpPolarity->SetBitmapLabel(wxBitmap(pos_xpm));
   } else if (event.GetId() == ID_TRG1) {
      m_trgSource = 0;
   } else if (event.GetId() == ID_TRG2) {
      m_trgSource = 1;
   } else if (event.GetId() == ID_TRG3) {
      m_trgSource = 2;
   } else if (event.GetId() == ID_TRG4) {
      m_trgSource = 3;
   } else if (event.GetId() == ID_TRG5) {
      m_trgSource = 4;
   }

   m_osci->SetTriggerMode(m_trgMode);
   if (m_trgConfig)
      m_osci->SetTriggerConfig(m_trgConfig);
   else
      m_osci->SetTriggerSource(m_trgSource);
   m_osci->SetTriggerLevel(m_trgLevel, m_trgNegative);
   m_osci->SetTriggerDelay(m_trgDelay);
   m_lastTriggerUpdate = time(NULL);
   m_screen->Refresh();
}

/*------------------------------------------------------------------*/

void DOFrame::OnChnOn(wxCommandEvent& event)
{
   int i, id;

   id = event.GetId();

   if (id == ID_CHON1) i = 0;
   if (id == ID_CHON2) i = 1;
   if (id == ID_CHON3) i = 2;
   if (id == ID_CHON4) i = 3;

   m_chnOn[i] = event.IsChecked();
   m_screen->SetChnOn(i, event.IsChecked());
   m_osci->SetChnOn(i, event.IsChecked());
   m_screen->Refresh();
}

/*------------------------------------------------------------------*/

void DOFrame::OnPosChange(wxScrollEvent& event)
{
   // 2mV per tick -0.5 ... +0.5 V
   m_chnOffset[0] = m_slPos1->GetValue()*2;
   m_chnOffset[1] = m_slPos2->GetValue()*2;
   m_chnOffset[2] = m_slPos3->GetValue()*2;
   m_chnOffset[3] = m_slPos4->GetValue()*2;
   for (int i=0 ; i<4 ; i++)
      m_screen->SetPos(i, m_chnOffset[i]/1000.0);

   m_screen->Refresh();
}

/*------------------------------------------------------------------*/

void DOFrame::OnScaleChange(wxCommandEvent& event)
{
   int id, i, inc;

   id = event.GetId();

   if (id == ID_SCALEUP1) { i=0; inc = -1; }
   if (id == ID_SCALEUP2) { i=1; inc = -1; }
   if (id == ID_SCALEUP3) { i=2; inc = -1; }
   if (id == ID_SCALEUP4) { i=3; inc = -1; }

   if (id == ID_SCALEDN1) { i=0; inc = +1; }
   if (id == ID_SCALEDN2) { i=1; inc = +1; }
   if (id == ID_SCALEDN3) { i=2; inc = +1; }
   if (id == ID_SCALEDN4) { i=3; inc = +1; }

   m_chnScale[i] += inc;
   if (m_chnScale[i] > 9)
      m_chnScale[i] = 9;
   if (m_chnScale[i] < 0)
      m_chnScale[i] = 0;

   m_screen->SetScale(i, m_chnScale[i]);

   wxString wxst;
   if (DOScreen::m_scaleTable[m_chnScale[i]] >= 1000)
     wxst.Printf(_T("%d V"), DOScreen::m_scaleTable[m_chnScale[i]]/1000);
   else
     wxst.Printf(_T("%dmV"), DOScreen::m_scaleTable[m_chnScale[i]]);

   if (i == 0) m_stScale1->SetLabel(wxst);
   if (i == 1) m_stScale2->SetLabel(wxst);
   if (i == 2) m_stScale3->SetLabel(wxst);
   if (i == 3) m_stScale4->SetLabel(wxst);

   m_screen->Refresh();
   StatReset();
}

/*------------------------------------------------------------------*/

void DOFrame::ChangeHScale(int delta)
{
   // remember current trigger position inside screen
   double trgFrac = (GetTrgPosition()-m_screen->GetScreenOffset()) / m_screen->GetScreenSize();

   // change screen scale
   if (delta)
      m_screen->SetHScaleInc(delta < 0 ? -1 : +1);
   m_HScale = m_screen->GetHScale();

   // adjust sampling speed if necessary
   if (!m_freqLocked) {
      m_reqSamplingSpeed = 100.0/DOScreen::m_hscaleTable[m_HScale];
      if (m_reqSamplingSpeed > m_osci->GetMaxSamplingSpeed())
         m_reqSamplingSpeed = m_osci->GetMaxSamplingSpeed();
      if (m_reqSamplingSpeed < m_osci->GetMinSamplingSpeed())
         m_reqSamplingSpeed = m_osci->GetMinSamplingSpeed();
      m_osci->SetSamplingSpeed(m_reqSamplingSpeed);
      m_actSamplingSpeed = m_osci->GetSamplingSpeed(); // get real speed (depends on divider ratio)
   }

   // update config dialog
   if (m_configDialog != NULL)
      m_configDialog->FreqChange();

   RecalculateHOffset(trgFrac);
   StatReset();
}

void DOFrame::RecalculateHOffset(double trgFrac)
{
   wxString wxstr;

   // force recalculation of trigger delay
   m_osci->SetTriggerDelay(m_trgDelay);
   m_trgDelayNs = m_osci->GetTriggerDelayNs();

   // calculate new screen offset to keep trigger point at same position
   m_HOffset = (int)(GetTrgPosition() - trgFrac * (double)m_screen->GetScreenSize() + 0.5);

   // adjust horizontal slider accordingly
   double v = (m_HOffset + m_screen->GetScreenSize()/2 - m_osci->GetWaveformLength()/2) / m_osci->GetWaveformLength();
   if (v < -0.5) // limit range
      v = -0.5;
   if (v > 0.5)
      v = 0.5;
   m_slHOffset->SetValue((int)(-v*2000 + 0.5));

   // convert back slider setting to m_HOffset with valid range
   m_HOffset = (int)(m_osci->GetWaveformLength()*v - m_screen->GetScreenSize()/2 + m_osci->GetWaveformLength()/2 + 0.5);

   m_screen->SetScreenOffset(m_HOffset);

   // update label
   if (DOScreen::m_hscaleTable[m_HScale] >= 1000) {
      wxstr.Printf(_T("%d us/div"), DOScreen::m_hscaleTable[m_HScale]/1000);
   } else {
      wxstr.Printf(_T("%d ns/div"), DOScreen::m_hscaleTable[m_HScale]);
   }
   m_stHScale->SetLabel(wxstr);

   m_screen->Refresh();
}

/*------------------------------------------------------------------*/

void DOFrame::OnHScaleChange(wxCommandEvent& event)
{
   if (event.GetId() == ID_HSCALEUP)
      ChangeHScale(-1);
   else
      ChangeHScale(+1);
   StatReset();
   m_screen->Refresh();
}

/*------------------------------------------------------------------*/

void DOFrame::OnHOffsetChange(wxScrollEvent& event)
{
   double v = -m_slHOffset->GetValue()/2000.0; // -0.5 ... +0.5

   // scale from (size/2-length) to (size/2)
   m_HOffset = (int)(m_osci->GetWaveformLength()*v - m_screen->GetScreenSize()/2 + m_osci->GetWaveformLength()/2 + 0.5);
   m_screen->SetScreenOffset(m_HOffset);
   m_screen->Refresh();
}

/*------------------------------------------------------------------*/

void DOFrame::SetDisplayDateTime(bool flag)
{
   m_screen->SetDisplayDateTime(flag);
}

/*------------------------------------------------------------------*/

void DOFrame::SetDisplayShowGrid(bool flag)
{
   m_screen->SetDisplayShowGrid(flag);
}

/*------------------------------------------------------------------*/

void DOFrame::SetDisplayLines(bool flag)
{
   m_screen->SetDisplayLines(flag);
}

/*------------------------------------------------------------------*/

void DOFrame::SetDisplayMode(int mode, int n)
{
   m_screen->SetDisplayMode(mode, n);
}

/*------------------------------------------------------------------*/

void DOFrame::SetDisplayCalibrated(bool flag)
{
   m_osci->SetCalibrated(flag);
   StatReset();
}

/*------------------------------------------------------------------*/

void DOFrame::SetDisplayCalibrated2(bool flag)
{
   m_osci->SetCalibrated2(flag);
   StatReset();
}

/*------------------------------------------------------------------*/

void DOFrame::SetDisplayTCalOn(bool flag)
{
   m_osci->SetTCalOn(flag);
   StatReset();
}

/*------------------------------------------------------------------*/

void DOFrame::SetDisplayTrgCorr(bool flag)
{
   m_trgCorr = flag;
}

/*------------------------------------------------------------------*/

void DOFrame::SetRefclk(bool flag)
{
   m_osci->SetRefclk(flag);
   m_refClk = flag;
   SetSamplingSpeed(m_reqSamplingSpeed);
}

/*------------------------------------------------------------------*/

void DOFrame::SetSource(int board, int firstChannel, int chnSection)
{
   m_board = board;
   m_firstChannel = firstChannel;
   m_chnSection = chnSection;
}

/*------------------------------------------------------------------*/

void DOFrame::SetDisplayRotated(bool flag)
{
   m_osci->SetRotated(flag);
   StatReset();
}

/*------------------------------------------------------------------*/

void DOFrame::ToggleControls()
{
   if (m_hideControls) {
      m_pnControls->Show();
      m_hideControls = false;
   } else {
      m_pnControls->Hide();
      m_hideControls = true;
   }
   this->Layout();
}

/*------------------------------------------------------------------*/

void DOFrame::OnTimer(wxTimerEvent& event)
{
   char str[256];

   if (m_first) {
      if (m_osci->GetError(str, sizeof(str)))
         wxMessageBox((wxChar*)str,
                         _T("DRS Oscilloscope Error"), wxOK | wxICON_STOP, this);

      if (m_osci->GetNumberOfBoards() == 0)
            wxMessageBox(_T("No DRS board found\r\nRunning in demo mode"),
                            _T("DRS Oscilloscope Error"), wxOK | wxICON_STOP, this);

      if (m_osci->GetNumberOfBoards() && 
          fabs(GetRange() - GetOsci()->GetCalibratedInputRange()) > 0.001) {
         wxString str;

         str.Printf(_T("This board was calibrated at %1.2lg V ... %1.2lg V\nYou must recalibrate the board if you use a different input range"), 
            GetOsci()->GetCalibratedInputRange()-0.5, GetOsci()->GetCalibratedInputRange()+0.5);
         wxMessageBox(str, _T("DRS Oscilloscope Warning"), wxOK | wxICON_EXCLAMATION, this);
      }
   }

   m_first = false;

   /* calculate number of acquistions once per second */
   if (m_osci->GetNumberOfBoards() > 0 && m_stopWatch.Time() > 1000) {
      if (g_epcs)
         g_epcs->Enter();
      m_acqPerSecond = (int) (1000.0 * m_acquisitions / m_stopWatch.Time() + 0.5);
      m_acquisitions = 0;
      if (g_epcs)
         g_epcs->Leave();

      m_stopWatch.Start();

      // update temperature and number of acquisitions
      UpdateStatusBar();
   }

   if (m_epthread == NULL) {
      // handle event processing in the main thread
      if (m_running) {
         if (m_osci->HasNewEvent()) {
            ProcessEvents();
            m_oldIdle = false;
            m_stopWatch1.Start();
         } else {
            if (m_osci->GetTriggerMode() == TM_AUTO && m_stopWatch1.Time() > 1000) {
               m_osci->SingleTrigger();
               while (!m_osci->HasNewEvent());
               m_osci->ReadWaveforms();
               ProcessEvents();
               m_acquisitions++;
            }
         }
      }

      if (m_single) {
         if (m_osci->HasNewEvent()) {
            ProcessEvents();
            m_oldIdle = false;
         }
      }

      if (m_osci->IsIdle() && !m_oldIdle) {
         m_oldIdle = true;
         m_screen->Refresh(); // need one refresh to display "TRIG?"
      }

      m_timer->Start(10, true);
   } else {
      m_screen->Refresh();
      m_timer->Start(40, true);
   }
}

/*------------------------------------------------------------------*/

void DOFrame::ProcessEvents(void)
{
   int status;
   wxStopWatch sw;

   while (m_osci->HasNewEvent() && sw.Time() < 100) {
      m_osci->ReadWaveforms();
      if (m_rearm) {
         m_osci->Start();
         m_rearm = false;
      }

      if (m_trgCorr)
         m_osci->CorrectTriggerPoint(GetTrgPosition());
      status = m_osci->SaveWaveforms(m_WFFile, m_WFfd);
      if (status < 0)
         CloseWFFile(true);

      EvaluateMeasurements();
      IncrementAcquisitions();

      if (m_osci->GetNumberOfBoards() == 0)
         break;
   }

   m_screen->Refresh();
}

void DOFrame::IncrementAcquisitions()
{
   if (g_epcs)
      g_epcs->Enter();
   m_acquisitions++;
   if (g_epcs)
      g_epcs->Leave();
}

/*------------------------------------------------------------------*/

void DOFrame::CloseWFFile(bool errorFlag)
{
   if (errorFlag)
      wxMessageBox(_T("Error writing waveforms to file\nWriting will be stopped"),
                     _T("DRS Oscilloscope Error"), wxOK | wxICON_STOP, this);

   if (g_epcs)
      g_epcs->Enter();
   if (m_WFFile)
      mxml_close_file(m_WFFile);
   if (m_WFfd)
      close(m_WFfd);
   m_WFFile = NULL;
   m_WFfd   = 0;
   if (g_epcs)
      g_epcs->Leave();

   m_btSave->SetLabel(_T("Save"));
   m_btSave->SetToolTip(_T("Save waveforms"));
}

/*------------------------------------------------------------------*/

void DOFrame::EvaluateMeasurements()
{
   double x1[2048], y1[2048], x2[2048], y2[2048];

   double t1 = m_screen->GetT1();
   double t2 = m_screen->GetT2();

   for (int idx = 0 ; idx<Measurement::N_MEASUREMENTS ; idx++) {
      for (int chn=0 ; chn<4 ; chn++) {
         Measurement *m;
         m = GetMeasurement(idx, chn);

         if (m && m_osci->GetWaveformDepth(chn)) {
            float *wf1 = m_osci->GetWaveform(chn);
            float *wf2 = m_osci->GetWaveform((chn+1)%4);
            float *time = m_osci->GetTime(chn);
            int   n_inside = 0;

            for (int i=0 ; i<m_osci->GetWaveformDepth(chn) ; i++) {
               if (time[i] >= t1) {
                  x1[n_inside] = time[i];
                  y1[n_inside] = wf1[i];
                  x2[n_inside] = time[i];
                  y2[n_inside] = wf2[i];
                  n_inside++;
               }
               if (time[i] > t2)
                  break;
            }

            m->Measure(x1, y1, x2, y2, n_inside);
         }
      }
   }
}

/*------------------------------------------------------------------*/

void DOFrame::OnCursor(wxCommandEvent& event)
{
   if (event.GetId() == ID_CURSORA) {
      if (m_toggleCursorA->GetValue() == 1) {
         m_cursorA = true;
         m_actCursor = 1;
      } else {
         m_cursorA = false;
         if (m_actCursor == 1)
            m_actCursor = 0;
      }
   } else {
      if (m_toggleCursorB->GetValue() == 1) {
         m_cursorB = true;
         m_actCursor = 2;
      } else {
         m_cursorB = false;
         if (m_actCursor == 2)
            m_actCursor = 0;
      }
   }

   m_screen->Refresh();
}

/*------------------------------------------------------------------*/

void DOFrame::OnSnap(wxCommandEvent& event)
{
   m_snap = event.IsChecked();
}

/*------------------------------------------------------------------*/

void DOFrame::SetMeasurement(int id, bool flag)
{
   int m, chn;

   m = (id - ID_LEVEL1) / 4;
   chn = (id - ID_LEVEL1) % 4;

   if (m < Measurement::N_MEASUREMENTS) {
      m_measFlag[m][chn] = flag;
      m_measurement[m][chn]->ResetStat();
   }
}

/*------------------------------------------------------------------*/

void DOFrame::SetMathDisplay(int id, bool flag)
{
   m_screen->SetMathDisplay(id, flag);
}

/*------------------------------------------------------------------*/

void DOFrame::SetStat(bool flag) 
{ 
   m_stat = flag;
   StatReset();
}

/*------------------------------------------------------------------*/

void DOFrame::SetHist(bool flag) 
{ 
   m_hist = flag;
}

/*------------------------------------------------------------------*/

void DOFrame::SetStatNStat(int n) 
{ 
   for (int i=0 ; i<Measurement::N_MEASUREMENTS ; i++)
      for (int j=0 ; j<4 ; j++)
         m_measurement[i][j]->SetNStat(n);
}


/*------------------------------------------------------------------*/

void DOFrame::SetIndicator(bool flag) 
{ 
   m_indicator = flag;
}

/*------------------------------------------------------------------*/

Measurement* DOFrame::GetMeasurement(int m, int chn)
{
   if (m>=0 && m<Measurement::N_MEASUREMENTS && chn>=0 && chn<4)
      if (m_measFlag[m][chn])
         return m_measurement[m][chn];

   return NULL;
}

/*------------------------------------------------------------------*/

void DOFrame::StatReset()
{
   for (int i=0 ; i<Measurement::N_MEASUREMENTS ; i++)
      for (int j=0 ; j<4 ; j++)
         m_measurement[i][j]->ResetStat();
}

/*------------------------------------------------------------------*/

void DOFrame::SetSamplingSpeed(double speed)
{
   if (speed > m_osci->GetMaxSamplingSpeed())
      speed = m_osci->GetMaxSamplingSpeed();
   if (speed < m_osci->GetMinSamplingSpeed())
      speed = m_osci->GetMinSamplingSpeed();

    // remember current trigger position inside screen
   double trgFrac = (GetTrgPosition()-m_screen->GetScreenOffset()) / m_screen->GetScreenSize();

   m_reqSamplingSpeed = speed;
   m_osci->SetSamplingSpeed(speed);
   m_actSamplingSpeed = m_osci->GetSamplingSpeed(); // get real speed (depends on divider ratio)

   RecalculateHOffset(trgFrac);
   StatReset();
}

/*------------------------------------------------------------------*/

void DOFrame::EnableTriggerConfig(bool flag)
{
   m_btTrgCfg->Enable(flag);
   m_btTrgCfg->Show(flag);
}

/*------------------------------------------------------------------*/

void DOFrame::SetTriggerConfig(int id, bool flag)
{
   int chn;

   if (id >= ID_OR1 && id <= ID_OREXT) {
      chn = (id - ID_OR1);
      if (flag)
         m_trgConfig |= (1 << chn);
      else
         m_trgConfig &= ~(1 << chn);
   }

   if (id >= ID_AND1 && id <= ID_ANDEXT) {
      chn = (id - ID_AND1);
      if (flag)
         m_trgConfig |= (1 << (chn+8));
      else
         m_trgConfig &= ~(1 << (chn+8));
   }

   if (m_trgConfig > 0) {
      m_rbS0->Enable(false);
      m_rbS1->Enable(false);
      m_rbS2->Enable(false);
      m_rbS3->Enable(false);
      m_rbS4->Enable(false);
      m_osci->SetTriggerConfig(m_trgConfig);
   } else {
      m_rbS0->Enable(true);
      m_rbS1->Enable(true);
      m_rbS2->Enable(true);
      m_rbS3->Enable(true);
      m_rbS4->Enable(true);
      m_osci->SetTriggerSource(m_trgSource);
   }

}

