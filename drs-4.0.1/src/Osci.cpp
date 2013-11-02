/*
 * Osci.cpp
 * DRS oscilloscope main class
 * $Id: Osci.cpp 18623 2011-11-24 06:54:19Z ritt $
 */

#include "DRSOscInc.h"
#include "rb.h"

#ifndef _MSC_VER
#include <sys/time.h>
#endif

int g_rbh;

#define RB_SIZE (1024*1024) // 1 MB fits 60 waveforms

#define MULTI_THREAD_READOUT

void GetTimeStamp(TIMESTAMP &ts);

/*------------------------------------------------------------------*/

Osci::Osci(double samplingSpeed, bool mthread)
{
   m_drs = NULL;
   m_thread = NULL;
   m_running = false;
   m_single = false;
   m_armed = false;
   m_samplingSpeed = samplingSpeed;
   m_triggerCell = 0;
   m_writeSR = 0;
   m_waveDepth = 1024;
   m_trgMode = TM_AUTO;
   m_trgSource = 0;
   m_trgNegative = false;
   m_trgDelay = 0;
   m_trgLevel = 0;
   for (int i=0 ; i<4 ; i++)
      m_chnOn[i] = false;
   m_clkOn = false;
   m_refClk = false;
   m_calibOn = false;
   m_evSerial = 0;
   m_calibrated = true;
   m_calibrated2 = true;
   m_tcalon = true;
   m_rotated = true;
   m_nDRS = 0;
   m_board = 0;
   m_chip = 0;
   m_chnOffset = 0;
   m_chnSection = 0;
   m_spikeRemoval = false;
   m_inputRange = 0;
   m_skipDisplay = false;
   m_debugMsg[0] = 0;

   ScanBoards();
   SelectSource(0, 0, 0);
   
   if (mthread) {
      rb_create(RB_SIZE, 32*1024*2, &g_rbh);
      m_thread = new OsciThread(this);
   }
} 

/*------------------------------------------------------------------*/

Osci::~Osci()
{
   if (m_thread) {
      m_thread->Delete();
      while (!m_thread->IsFinished())
         wxThread::Sleep(10);
   }
   if (g_rbh)
      rb_delete(g_rbh);
   delete m_drs;
} 

/*------------------------------------------------------------------*/

void GetTimeStamp(TIMESTAMP &ts)
{
#ifdef _MSC_VER
   SYSTEMTIME t;
   static unsigned int ofs = 0;

   GetLocalTime(&t);
   if (ofs == 0)
      ofs = timeGetTime() - t.wMilliseconds;
   ts.Year         = t.wYear;
   ts.Month        = t.wMonth;
   ts.Day          = t.wDay;
   ts.Hour         = t.wHour;
   ts.Minute       = t.wMinute;
   ts.Second       = t.wSecond;
   ts.Milliseconds = (timeGetTime() - ofs) % 1000;
#else
   struct timeval t;
   struct tm *lt;
   time_t now;

   gettimeofday(&t, NULL);
   time(&now);
   lt = localtime(&now);

   ts.Year         = lt->tm_year+1900;
   ts.Month        = lt->tm_mon+1;
   ts.Day          = lt->tm_mday;
   ts.Hour         = lt->tm_hour;
   ts.Minute       = lt->tm_min;
   ts.Second       = lt->tm_sec;
   ts.Milliseconds = t.tv_usec/1000;
#endif /* OS_UNIX */
}


/*------------------------------------------------------------------*/

int Osci::ScanBoards()
{
   /* pause readout thread if prsent */
   if (m_thread)
      m_thread->Enable(false);

   if (m_drs)
      delete m_drs;

   m_drs = new DRS();
   m_nDRS = m_drs->GetNumberOfBoards();
   m_board = 0;

   for (int i=0 ; i< m_nDRS ; i++) {
      DRSBoard *b = m_drs->GetBoard(i);
      b->Init();
      if (b->GetBoardType() == 3) {  // DRS2 board
         b->SetDominoMode(1);
         b->EnableTrigger(1, 0);     // fast trigger
         if (m_samplingSpeed > 2)
            m_samplingSpeed = 2;
         b->SetFrequency(m_samplingSpeed, true);
         m_samplingSpeed = b->GetFrequency();
         b->SetNumberOfChannels(10);
#ifdef _MSC_VER
         b->SetCalibrationDirectory("C:/experiment/calibrations");
#else
         b->SetCalibrationDirectory("/home/meg/meg/online/calibrations");
#endif

         if (i == 0)
            printf("Reading calibration for sampling speed %lg GSPS\n", m_samplingSpeed);

         if (!b->GetResponseCalibration()->ReadCalibration(0) ||
             !b->GetResponseCalibration()->ReadCalibration(1)) {
            wxString str;

            str.Printf(_T("Cannot read calibration for %1.1lf GSPS VME board slot %2d %s, serial #%d"), 
                m_samplingSpeed,
               (b->GetSlotNumber() >> 1)+2, 
               ((b->GetSlotNumber() & 1) == 0) ? "upper" : "lower",
                b->GetBoardSerialNumber());

            wxMessageBox(str, _T("DRS Oscilloscope Error"), wxOK | wxICON_STOP, NULL);
         } else {
            printf("Calibration read for VME board slot %2d %s, serial #%04d\n", 
               (b->GetSlotNumber() >> 1)+2, 
               ((b->GetSlotNumber() & 1) == 0) ? "upper" : "lower",
                b->GetBoardSerialNumber());
         }

      } else {                       // DRS4 board

         /* obtain default sampling speed from calibration of first board */
         if (i == 0)
            m_samplingSpeed = b->GetCalibratedFrequency();

         b->SetFrequency(m_samplingSpeed, true);
         m_samplingSpeed = b->GetFrequency();
         if (b->GetTransport() == TR_USB2)
            b->SetChannelConfig(0, 8, 8);
         else
            b->SetChannelConfig(7, 8, 8);
         b->SetDecimation(0);
         b->SetDominoMode(1);
         b->SetReadoutMode(1);
         b->SetDominoActive(1);
         if (b->GetBoardType() == 5 || b->GetBoardType() == 7 || b->GetBoardType() == 8) {
            b->SetTranspMode(1);     // Evaluation board with build-in trigger
            b->EnableTrigger(0, 1);  // Enable analog trigger
            b->SetTriggerSource(0);  // on CH0
         } else if (b->GetBoardType() == 6) {
            b->SetTranspMode(0);     // VPC Mezzanine board
            b->EnableTrigger(0, 0);  // Disable analog trigger
         }
         b->SetRefclk(0);
         b->SetFrequency(m_samplingSpeed, true);
         b->EnableAcal(0, 0);
         b->EnableTcal(0, 0);
         b->StartDomino();
      }
   }

   /* resume readout thread if prsent */
   if (m_thread)
      m_thread->Enable(true);

   return m_nDRS;
}

/*------------------------------------------------------------------*/

void Osci::CheckTimingCalibration()
{
   for (int i=0 ; i< m_nDRS ; i++) {
      DRSBoard *b = m_drs->GetBoard(i);
      if (b->GetDRSType() == 4) {

         if (!b->IsVoltageCalibrationValid()) {
            wxString str;

            if (b->GetTransport() != TR_VME) {
               str.Printf(_T("Board on USB%d has invalid voltage calibration\nOnly raw data will be displayed"),
                  b->GetSlotNumber());
            } else {
               str.Printf(_T("VME board in slot %2d %s has invalid voltage calibration\nOnly raw data will be displayed"),
                  (b->GetSlotNumber() >> 1)+2, 
                  ((b->GetSlotNumber() & 1) == 0) ? "upper" : "lower");
            }
            wxMessageBox(str, _T("DRS Oscilloscope Warning"), wxOK | wxICON_EXCLAMATION);
         } else {
            if (!b->IsTimingCalibrationValid()) {
               wxString str;

               if (b->GetTransport() != TR_VME) {
                  str.Printf(_T("Board on USB%d has been timing calibrated at %1.4lg GSPS\nYou must redo the timing calibration at %1.4lg GSPS to obtain precise timing results"),
                     b->GetSlotNumber(), b->GetCalibratedFrequency(), m_samplingSpeed);
               } else {
                  str.Printf(_T("VME board in slot %2d %s has been timing calibrated at %1.4lg GSPS\nYou must redo the timing calibration at %1.4lg GSPS to obtain precise timing results"),
                     (b->GetSlotNumber() >> 1)+2, 
                     ((b->GetSlotNumber() & 1) == 0) ? "upper" : "lower",
                      b->GetCalibratedFrequency(), b->GetFrequency());
               }
               wxMessageBox(str, _T("DRS Oscilloscope Warning"), wxOK | wxICON_EXCLAMATION);
            }
         }
      }
   }
}

/*------------------------------------------------------------------*/

void Osci::SelectSource(int board, int firstChannel, int chnSection)
{
   if (board >= m_nDRS)
      return;

   DRSBoard *b = m_drs->GetBoard(board);

   /* stop drs_readout state machine to be ready for configuration change */
   if (b->IsBusy()) {
      b->SoftTrigger();
      for (int i=0 ; i<10 && b->IsBusy() ; i++)
         wxMilliSleep(10);
   }

   if (b->GetBoardType() == 6 && b->GetTransport() == TR_USB2) {
      if (firstChannel == 0 || firstChannel == 2) {
         if (chnSection == 0)
            b->SetChannelConfig(1, 8, 8);
         else
            b->SetChannelConfig(0, 8, 8);
      } else {
         if (chnSection == 0)
            b->SetChannelConfig(3, 8, 8);
         else
            b->SetChannelConfig(2, 8, 8);
      }
      m_board      = board;
      m_chip       = firstChannel;
      m_chnOffset  = 0;
      m_chnSection = chnSection;
   } else {
      m_board      = board;
      m_chip       = firstChannel;
      m_chnOffset  = chnSection;
      m_chnSection = chnSection;

      if (b->GetBoardType() == 3) {
         m_chip      = firstChannel / 2;
         m_chnOffset = (firstChannel % 2)* 4;
      }

      if (b->GetBoardType() == 5 || b->GetBoardType() == 7 || b->GetBoardType() == 8) {
         if (chnSection == 2)
            b->SetChannelConfig(0, 8, 4);
         else
            b->SetChannelConfig(0, 8, 8);
      } else {
         if (chnSection == 2)
            b->SetChannelConfig(7, 8, 4);
         else
            b->SetChannelConfig(7, 8, 8);
      }
   }
}

/*------------------------------------------------------------------*/

OsciThread::OsciThread(Osci *o) : wxThread()
{
   m_osci = o;
   m_enabled = false;
   m_finished = false;
   m_active = false;
   Create();
   Run();
}

/*------------------------------------------------------------------*/

void OsciThread::Enable(bool flag) 
{ 
   m_enabled = flag;
   if (!flag)
      while (m_active)
          wxThread::Sleep(10);
}

/*------------------------------------------------------------------*/

bool OsciThread::IsIdle()
{
   if (m_osci->IsRunning() && m_sw1.Time() > 1000)
      return true;

   if (m_osci->IsSingle() && m_osci->IsArmed() && m_sw1.Time() > 1000)
      return true;

   return false;
}

/*------------------------------------------------------------------*/

void OsciThread::ResetSW()
{
   m_sw1.Start();
}

/*------------------------------------------------------------------*/

void *OsciThread::Entry()
{
   int size, status, n, m;
   unsigned char *pdata;
   unsigned short *ptc;
   bool autoTriggered;
   TIMESTAMP ts;

   n = m = 0;
   autoTriggered = false;
   do {
      if (m_osci->GetDRS()->GetNumberOfBoards() > 0 && m_enabled) {
         m_active = true;
         if (m_osci->HasTriggered()) {

            n = 0;
            if (!autoTriggered)
               m_sw1.Start(); // we got a real trigger
            autoTriggered = false;

            // wait for space in ring buffer
            do {
               status = rb_get_wp(g_rbh, (void **)&pdata, 100);
               if (status == RB_SUCCESS) {

                  // transfer waveforms
                  m_osci->GetCurrentBoard()->TransferWaves(pdata);
                  size = m_osci->GetCurrentBoard()->GetWaveformBufferSize();

                  ptc = (unsigned short *)(pdata + size);
                  ptc[0] = m_osci->GetCurrentBoard()->GetStopCell(0);
                  ptc[1] = m_osci->GetCurrentBoard()->GetStopWSR(0);
                  size += 4;

                  // add timestamp
                  GetTimeStamp(ts);
                  memcpy(pdata + size, &ts, sizeof(ts));
                  size += sizeof(ts);

                  // commit data to ring buffer
                  rb_increment_wp(g_rbh, size);

                  // restart domino in running mode
                  if (m_osci->IsRunning()) {
                     // only if not in multi buffer mode
                     if (!m_osci->GetCurrentBoard()->IsMultiBuffer())
                        m_osci->GetCurrentBoard()->StartDomino();
                  }
               
                  // in single mode, just clear armed flag
                  if (m_osci->IsSingle())
                     m_osci->SetArmed(false);          
               } else
                  wxThread::Sleep(10);

            } while (status != RB_SUCCESS && !TestDestroy());

#ifndef _MSC_VER // Needed for Linux only, otherwise GUI freezes
            if (m++ % 10 == 0)
               wxThread::Yield();
#endif

         } else {
            if (m_osci->GetTriggerMode() == TM_AUTO && m_osci->IsRunning() &&
                m_sw1.Time() > 1000) {
               if (m_sw2.Time() > 300) {
                  wxThread::Sleep(30);  // sleep 3 times a second a bit to test for real trigger
                  m_sw2.Start();
               }
               if (!m_osci->HasTriggered()) {
                  m_osci->SingleTrigger();
                  autoTriggered = true;
               }
            } else {
               n++;
               // sleep once in a while to save CPU
               if (n == 100) {
                  n = 0;
                  wxThread::Sleep(100);
               } else
                  wxThread::Yield();
            }
         }
      } else {
         wxThread::Sleep(10);
         m_active = false;
      }
   } while (!TestDestroy());

   m_finished = true;
   return NULL;
}

/*------------------------------------------------------------------*/

void Osci::SetRunning(bool flag)
{
   m_running = flag;
   if (m_running)
      Start();
   else {
      Stop();
      if (m_drs->GetNumberOfBoards() > 0)
         DrainEvents();
   }
} 

/*------------------------------------------------------------------*/

void Osci::Enable(bool flag)
{
   if (m_thread)
      m_thread->Enable(flag);
} 

/*------------------------------------------------------------------*/

void Osci::Start()
{
   for (int i=0 ; i< m_drs->GetNumberOfBoards() ; i++) {
      DRSBoard *b = m_drs->GetBoard(i);

      /* start domino wave */
      b->StartDomino();
   }
   m_armed = true;
} 

/*------------------------------------------------------------------*/

void Osci::Stop()
{
   for (int i=0 ; i< m_drs->GetNumberOfBoards() ; i++) {
      DRSBoard *b = m_drs->GetBoard(i);

      /* stop domino wave */
      b->SoftTrigger();
   }
   m_armed = false;
} 

/*------------------------------------------------------------------*/

void Osci::DrainEvents()
{
   while (HasNewEvent())
      ReadWaveforms();
} 

/*------------------------------------------------------------------*/

void Osci::SetSingle(bool flag)
{
  m_single = flag;
  if (m_thread)
     m_thread->ResetSW();
} 
  
/*------------------------------------------------------------------*/

void Osci::SingleTrigger()
{
   for (int i=0 ; i< m_drs->GetNumberOfBoards() ; i++)
      m_drs->GetBoard(i)->SoftTrigger();
} 

/*------------------------------------------------------------------*/

bool Osci::HasNewEvent()
{
   int n;

   if (m_drs->GetNumberOfBoards() > 0) {
      if (m_thread) {
         rb_get_buffer_level(g_rbh, &n);
         return n > 0;
      } else {
         if (m_armed && m_drs->GetBoard(m_board)->IsBusy() == 0)
            return true;
         return false;
      }
   } else
      return m_running;
} 

/*------------------------------------------------------------------*/

bool Osci::HasTriggered()
{
   if (m_drs->GetNumberOfBoards() > 0) {
      if (m_running)
         return m_drs->GetBoard(m_board)->IsEventAvailable() > 0;
      if (m_single && m_armed)
         return m_drs->GetBoard(m_board)->IsEventAvailable() > 0;
      return false;
   }
   return true;
} 

/*------------------------------------------------------------------*/

bool Osci::IsIdle()
{
   if (m_thread)
      return m_thread->IsIdle();
   return false;
}

/*------------------------------------------------------------------*/

void Osci::ReadWaveforms()
{
unsigned char *pdata;
unsigned short *ptc;
int size = 0;

   m_skipDisplay = false;
   m_armed = false;
   m_evSerial++;
   if (m_drs->GetNumberOfBoards() == 0) {
      for (int w=0 ; w<4 ; w++)
         for (int i=0 ; i<GetWaveformDepth(w) ; i++) {
            m_waveform[w][i] = sin(i/m_samplingSpeed/10*M_PI+w*M_PI/4)*100;
            m_waveform[w][i] += ((double)rand()/RAND_MAX-0.5)*5;
            m_time[i] = 1/m_samplingSpeed*i;
         }
      m_waveDepth = kNumberOfBins;
      GetTimeStamp(m_evTimestamp);
   } else {
      int ofs = m_chnOffset;
      int chip = m_chip;

      if (m_drs->GetBoard(m_board)->GetBoardType() == 3) {
         // DRS2 Mezzanine Board 1
         m_drs->GetBoard(m_board)->TransferWaves();
         int tc = m_drs->GetBoard(m_board)->GetTriggerCell(chip);

         m_drs->GetBoard(m_board)->GetTime(chip, m_samplingSpeed, tc, m_time, m_tcalon, m_rotated);
         m_drs->GetBoard(m_board)->GetWave(chip, 0+ofs, m_waveform[0], m_calibrated, tc, !m_rotated);
         m_drs->GetBoard(m_board)->GetWave(chip, 1+ofs, m_waveform[1], m_calibrated, tc, !m_rotated);
         m_drs->GetBoard(m_board)->GetWave(chip, 2+ofs, m_waveform[2], m_calibrated, tc, !m_rotated);
         if (m_clkOn)
            m_drs->GetBoard(m_board)->GetWave(chip, 9, m_waveform[3], m_calibrated, tc, !m_rotated);
         else
            m_drs->GetBoard(m_board)->GetWave(chip, 3+ofs, m_waveform[3], m_calibrated, tc, !m_rotated);
      } else if (m_drs->GetBoard(m_board)->GetBoardType() == 5 || m_drs->GetBoard(m_board)->GetBoardType() == 7 ||
                 m_drs->GetBoard(m_board)->GetBoardType() == 8) {
         
         // DRS4 Evaluation Board 1.1 + 2.0 + 3.0
         if (m_thread) {
            // get waveforms from ring buffer
            if (rb_get_rp(g_rbh, (void **)&pdata, 0) != RB_SUCCESS)
               return;

            int n;
            rb_get_buffer_level(g_rbh, &n);
            // sprintf(m_debugMsg, "%1.1lf %%", 100.0*n/RB_SIZE);
            
            // transfer waveforms to buffer
            size = m_drs->GetBoard(m_board)->GetWaveformBufferSize();
            memcpy(m_wavebuffer, pdata, size);
            ptc = (unsigned short *)(pdata + size);
            m_triggerCell = ptc[0];
            m_writeSR = ptc[1];
            size += 4;

            // transfer timestamp
            memcpy(&m_evTimestamp, pdata + size, sizeof(m_evTimestamp));
            size += sizeof(m_evTimestamp);

            // free space in ring buffer
            rb_increment_rp(g_rbh, size);
         } else {
            // get waveforms directly from device
            m_drs->GetBoard(m_board)->TransferWaves(m_wavebuffer, 0, 8);
            m_triggerCell = m_drs->GetBoard(m_board)->GetStopCell(chip);
            m_writeSR = m_drs->GetBoard(m_board)->GetStopWSR(chip);
            GetTimeStamp(m_evTimestamp);
         }

         m_waveDepth = m_drs->GetBoard(m_board)->GetChannelDepth();
         m_drs->GetBoard(m_board)->GetTime(0, m_triggerCell, m_time, m_tcalon, m_rotated);
         if (m_clkOn && GetWaveformDepth(0) > kNumberOfBins) {
            for (int i=0 ; i<kNumberOfBins ; i++)
               m_timeClk[i] = m_time[i] + GetWaveformLength()/2;
         } else {
            for (int i=0 ; i<kNumberOfBins ; i++)
               m_timeClk[i] = m_time[i];
         }
         
         if (m_drs->GetBoard(m_board)->GetChannelCascading() == 2) {
            m_drs->GetBoard(m_board)->GetWave(m_wavebuffer, 0, 0, m_waveform[0], m_calibrated, m_triggerCell, m_writeSR, !m_rotated, 0, m_calibrated2);
            m_drs->GetBoard(m_board)->GetWave(m_wavebuffer, 0, 1, m_waveform[1], m_calibrated, m_triggerCell, m_writeSR, !m_rotated, 0, m_calibrated2);
            m_drs->GetBoard(m_board)->GetWave(m_wavebuffer, 0, 2, m_waveform[2], m_calibrated, m_triggerCell, m_writeSR, !m_rotated, 0, m_calibrated2);
            if (m_clkOn)
               m_drs->GetBoard(m_board)->GetWave(m_wavebuffer, 0, 8, m_waveform[3], m_calibrated, m_triggerCell, 0, !m_rotated);
            else
               m_drs->GetBoard(m_board)->GetWave(m_wavebuffer, 0, 3, m_waveform[3], m_calibrated, m_triggerCell, m_writeSR, !m_rotated, 0, m_calibrated2);
            if (m_spikeRemoval)
               RemoveSpikes(true);
         } else {
            m_drs->GetBoard(m_board)->GetWave(m_wavebuffer, 0, 0+ofs, m_waveform[0], m_calibrated, m_triggerCell, 0, !m_rotated, 0, m_calibrated2);
            m_drs->GetBoard(m_board)->GetWave(m_wavebuffer, 0, 2+ofs, m_waveform[1], m_calibrated, m_triggerCell, 0, !m_rotated, 0, m_calibrated2);
            m_drs->GetBoard(m_board)->GetWave(m_wavebuffer, 0, 4+ofs, m_waveform[2], m_calibrated, m_triggerCell, 0, !m_rotated, 0, m_calibrated2);
            if (m_clkOn)
               m_drs->GetBoard(m_board)->GetWave(m_wavebuffer, 0, 8, m_waveform[3], m_calibrated, m_triggerCell, 0, !m_rotated);
            else
               m_drs->GetBoard(m_board)->GetWave(m_wavebuffer, 0, 6+ofs, m_waveform[3], m_calibrated, m_triggerCell, 0, !m_rotated, 0, m_calibrated2);
            if (m_spikeRemoval)
               RemoveSpikes(false);
         }
      } else if (m_drs->GetBoard(m_board)->GetBoardType() == 6) {
         // DRS4 Mezzanine Board 1
         m_drs->GetBoard(m_board)->TransferWaves(0, 8);
         m_triggerCell = m_drs->GetBoard(m_board)->GetStopCell(chip);
         m_writeSR = m_drs->GetBoard(m_board)->GetStopWSR(chip);
         m_waveDepth = m_drs->GetBoard(m_board)->GetChannelDepth();

         m_drs->GetBoard(m_board)->GetTime(chip, m_samplingSpeed, m_triggerCell, m_time, m_tcalon, m_rotated);
         if (m_clkOn && GetWaveformDepth(0) > kNumberOfBins) {
            for (int i=0 ; i<kNumberOfBins ; i++)
               m_timeClk[i] = m_time[i] + GetWaveformLength()/2;
         } else {
            for (int i=0 ; i<kNumberOfBins ; i++)
               m_timeClk[i] = m_time[i];
         }

         if (m_drs->GetBoard(m_board)->GetChannelCascading() == 2) {
            m_drs->GetBoard(m_board)->GetWave(chip, 0, m_waveform[0], m_calibrated, m_triggerCell, m_writeSR, !m_rotated, 0, m_calibrated2);
            m_drs->GetBoard(m_board)->GetWave(chip, 1, m_waveform[1], m_calibrated, m_triggerCell, m_writeSR, !m_rotated, 0, m_calibrated2);
            m_drs->GetBoard(m_board)->GetWave(chip, 2, m_waveform[2], m_calibrated, m_triggerCell, m_writeSR, !m_rotated, 0, m_calibrated2);
            if (m_clkOn)
               m_drs->GetBoard(m_board)->GetWave(chip, 8, m_waveform[3], m_calibrated, m_triggerCell, m_writeSR, !m_rotated);
            else {
               m_drs->GetBoard(m_board)->GetWave(chip, 3, m_waveform[3], m_calibrated, m_triggerCell, m_writeSR, !m_rotated, 0, m_calibrated2);
               if (m_spikeRemoval)
                  RemoveSpikes(true);
            }
         } else {
            m_drs->GetBoard(m_board)->GetWave(chip, 0+ofs, m_waveform[0], m_calibrated, m_triggerCell, 0, !m_rotated, 0, m_calibrated2);
            m_drs->GetBoard(m_board)->GetWave(chip, 2+ofs, m_waveform[1], m_calibrated, m_triggerCell, 0, !m_rotated, 0, m_calibrated2);
            m_drs->GetBoard(m_board)->GetWave(chip, 4+ofs, m_waveform[2], m_calibrated, m_triggerCell, 0, !m_rotated, 0, m_calibrated2);
            if (m_clkOn)
               m_drs->GetBoard(m_board)->GetWave(chip, 8, m_waveform[3], m_calibrated, m_triggerCell, 0, !m_rotated);
            else {
               m_drs->GetBoard(m_board)->GetWave(chip, 6+ofs, m_waveform[3], m_calibrated, m_triggerCell, 0, !m_rotated, 0, m_calibrated2);
               if (m_spikeRemoval)
                  RemoveSpikes(false);
            }
         }
      }

      /* extrapolate the first two samples (are noisy) */
      for (int i=0 ; i<4 ; i++) {
         m_waveform[i][1] = 2*m_waveform[i][2] - m_waveform[i][3];
         m_waveform[i][0] = 2*m_waveform[i][1] - m_waveform[i][2];
      }
   }

   /* auto-restart in running mode */
   if (m_thread == NULL && m_running)
      Start();
} 

/*------------------------------------------------------------------*/

unsigned char buffer[100000];

int Osci::SaveWaveforms(MXML_WRITER *xml, int fd)
{
   char str[80];
   unsigned char *p;
   unsigned short d;
   float t;

   if (xml == NULL && fd == 0)
      return 0;

   if (xml) {
      mxml_start_element(xml, "Event");
      sprintf(str, "%d", m_evSerial);
      mxml_write_element(xml, "Serial", str);
      sprintf(str, "%4d/%02d/%02d %02d:%02d:%02d.%03d", m_evTimestamp.Year, m_evTimestamp.Month,
         m_evTimestamp.Day, m_evTimestamp.Hour, m_evTimestamp.Minute, m_evTimestamp.Second,
         m_evTimestamp.Milliseconds);
      mxml_write_element(xml, "Time", str);
      mxml_write_element(xml, "HUnit", "ns");
      mxml_write_element(xml, "VUnit", "mV");

      for (int i=0 ; i<4 ; i++) {
         if (m_chnOn[i]) {
            sprintf(str, "CHN%d", i+1);
            mxml_start_element(xml, str);
            strcpy(str, "\n");
            for (int j=0 ; j<m_waveDepth ; j++) {
               sprintf(str, "%1.3f,%1.1f", m_time[j], m_waveform[i][j]);
               mxml_write_element(xml, "Data", str);
            }
            mxml_end_element(xml); // CHNx
         }
      }

      mxml_end_element(xml); // Event
   }
   if (fd) {
      p = buffer;
      memcpy(p, "EHDR", 4);
      p += 4;
      *(int *)p = m_evSerial;
      p += sizeof(int);
      *(unsigned short *)p = m_evTimestamp.Year;
      p += sizeof(unsigned short);
      *(unsigned short *)p = m_evTimestamp.Month;
      p += sizeof(unsigned short);
      *(unsigned short *)p = m_evTimestamp.Day;
      p += sizeof(unsigned short);
      *(unsigned short *)p = m_evTimestamp.Hour;
      p += sizeof(unsigned short);
      *(unsigned short *)p = m_evTimestamp.Minute;
      p += sizeof(unsigned short);
      *(unsigned short *)p = m_evTimestamp.Second;
      p += sizeof(unsigned short);
      *(unsigned short *)p = m_evTimestamp.Milliseconds;
      p += sizeof(unsigned short);
      *(unsigned short *)p = 0; // reserved
      p += sizeof(unsigned short);

      for (int j=0 ; j<m_waveDepth ; j++) {
         // save binary time as 32-bit float value
         if (m_waveDepth == 2048) {
            t = (m_time[j]+m_time[j+1])/2;
            j++;
         } else
            t = m_time[j];
         *(float *)p = t;
         p += sizeof(float);
      }
      for (int i=0 ; i<4 ; i++) {
         if (m_chnOn[i]) {
            sprintf((char *)p, "C%03d", i+1);
            p += 4;
            for (int j=0 ; j<m_waveDepth ; j++) {
               // save binary date as 16-bit value: 0 = -0.5V, 65535 = +0.5V
               if (m_waveDepth == 2048) {
                  // in cascaded mode, save 1024 values as averages of the 2048 values
                  d = (unsigned short)(((m_waveform[i][j]+m_waveform[i][j+1])/2000.0 + 0.5) * 65535);
                  *(unsigned short *)p = d;
                  p += sizeof(unsigned short);
                  j++;
               } else {
                  d = (unsigned short)((m_waveform[i][j]/1000.0 + 0.5) * 65535);
                  *(unsigned short *)p = d;
                  p += sizeof(unsigned short);
               }
            }
         }
      }
      int size = p-buffer;
      int n = write(fd, buffer, size);
      if (n != size)
         return -1;
   }

   return 1;
}

/*------------------------------------------------------------------*/

void Osci::SetSamplingSpeed(double freq)
{
   for (int i=0 ; i< m_drs->GetNumberOfBoards() ; i++) {
      DRSBoard *b = m_drs->GetBoard(i);

      b->SetFrequency(freq, true);
      m_samplingSpeed = b->GetFrequency();
      wxMilliSleep(10);

      if (b->GetDRSType() == 4 && !b->IsPLLLocked()) {
         wxString str;

#ifdef HAVE_VME
         if (b->GetTransport() == 1)
            str.Printf(_T("PLLs did not lock on VME board slot %2d %s, serial #%d"), 
               (b->GetSlotNumber() >> 1)+2, ((b->GetSlotNumber() & 1) == 0) ? "upper" : "lower", 
               b->GetBoardSerialNumber());
         else
#endif
            str.Printf(_T("PLLs did not lock on USB board #%d, serial #%d"), 
               b->GetSlotNumber(), b->GetBoardSerialNumber());

         wxMessageBox(str, _T("DRS Oscilloscope Error"), wxOK | wxICON_STOP, NULL);
      }
   }
}

/*------------------------------------------------------------------*/

double Osci::GetSamplingSpeed()
{ 
   if (m_drs->GetNumberOfBoards() > 0)
      return m_drs->GetBoard(m_board)->GetFrequency();
   return m_samplingSpeed;
}

/*------------------------------------------------------------------*/

double Osci::GetMaxSamplingSpeed()
{ 
   for (int i=0 ; i< m_drs->GetNumberOfBoards() ; i++)
      if (m_drs->GetBoard(i)->GetDRSType() == 2)
         return 4;
   return 5;
}

/*------------------------------------------------------------------*/

double Osci::GetMinSamplingSpeed()
{ 
   for (int i=0 ; i< m_drs->GetNumberOfBoards() ; i++)
      if (m_drs->GetBoard(i)->GetDRSType() == 4) {
         if (m_drs->GetBoard(i)->GetBoardSerialNumber() == 2146 ||
             m_drs->GetBoard(i)->GetBoardSerialNumber() == 2205 ||
             m_drs->GetBoard(i)->GetBoardSerialNumber() == 2208)
            return 0.5; // special modified boards for RFBeta & Slow Muons
         return 0.7;
      }
   return 0.5;
}

/*------------------------------------------------------------------*/

int Osci::GetWaveformDepth(int channel)
{ 
   if (channel == 3 && m_clkOn && m_waveDepth > kNumberOfBins)
      return m_waveDepth - kNumberOfBins; // clock chnnael has only 1024 bins

   return m_waveDepth; 
}

/*------------------------------------------------------------------*/

float *Osci::GetTime(int channel)  
{ 
   if (channel == 3 && m_clkOn)
      return (float *)m_timeClk;

   return (float *)m_time; 
}

/*------------------------------------------------------------------*/

bool Osci::IsTCalibrated(void)
{
   return m_drs->GetBoard(m_board)->IsTimingCalibrationValid();
}

/*------------------------------------------------------------------*/

bool Osci::IsVCalibrated(void)
{
   return m_drs->GetBoard(m_board)->IsVoltageCalibrationValid();
}

/*------------------------------------------------------------------*/

bool Osci::GetTimeCalibration(int mode, float *time, bool force)
{
   if (!force && !IsTCalibrated())
      return false;

   m_drs->GetBoard(m_board)->GetTimeCalibration(m_chip, mode, time, force);
   return true;
}

/*------------------------------------------------------------------*/

void Osci::SetTriggerLevel(double level, bool negative)
{
   m_trgLevel = level;
   m_trgNegative = negative;

   for (int i=0 ; i< m_drs->GetNumberOfBoards() ; i++)
      m_drs->GetBoard(i)->SetTriggerLevel(level, negative);
}

/*------------------------------------------------------------------*/

void Osci::SetTriggerDelay(int delay)
{
   m_trgDelay = delay;

   for (int i=0 ; i< m_drs->GetNumberOfBoards() ; i++)
      m_drs->GetBoard(i)->SetTriggerDelayPercent(delay);
}

/*------------------------------------------------------------------*/

int Osci::GetTriggerDelay()
{
   if (m_drs->GetNumberOfBoards() > 0)
      return m_drs->GetBoard(m_board)->GetTriggerDelay();
   else
      return 0;
}

/*------------------------------------------------------------------*/

double Osci::GetTriggerDelayNs()
{
   if (m_drs->GetNumberOfBoards() > 0)
      return m_drs->GetBoard(m_board)->GetTriggerDelayNs();
   else
      return 0;
}

/*------------------------------------------------------------------*/

void Osci::SetTriggerSource(int source)
{
   m_trgSource = source;

   for (int i=0 ; i< m_drs->GetNumberOfBoards() ; i++) {
      if (m_drs->GetBoard(i)->GetBoardType() == 8) {
         m_drs->GetBoard(i)->EnableTrigger(1, 0); // enable trigger
         m_drs->GetBoard(i)->SetTriggerSource(1 << source); // simple or of single channel
      } else {
         if (source == 4)
            m_drs->GetBoard(i)->EnableTrigger(1, 0); // external trigger
         else {
            m_drs->GetBoard(i)->EnableTrigger(0, 1); // analog trigger
            m_drs->GetBoard(i)->SetTriggerSource(source);
         }
      }
   }

}

/*------------------------------------------------------------------*/

void Osci::SetTriggerConfig(int tc)
{
   for (int i=0 ; i< m_drs->GetNumberOfBoards() ; i++) {
      if (m_drs->GetBoard(i)->GetBoardType() == 8) {
         m_drs->GetBoard(i)->EnableTrigger(1, 0); // enable trigger
         m_drs->GetBoard(i)->SetTriggerSource(tc);
      }
   }
}

/*------------------------------------------------------------------*/

void Osci::SetChnOn(int chn, bool flag)
{
   m_chnOn[chn] = flag; 
}

/*------------------------------------------------------------------*/

void Osci::SetClkOn(bool flag)
{
   m_clkOn = flag; 
   for (int i=0 ; i< m_drs->GetNumberOfBoards() ; i++) {
      m_drs->GetBoard(i)->EnableTcal(flag ? 1 : 0, 0);
      if (m_drs->GetBoard(i)->GetBoardType() == 5 || m_drs->GetBoard(i)->GetBoardType() == 7 || m_drs->GetBoard(i)->GetBoardType() == 8)
         m_drs->GetBoard(i)->SelectClockSource(0); // select sync. clock
   }
}

/*------------------------------------------------------------------*/

void Osci::SetRefclk(bool flag)
{
   m_refClk = flag; 
   // only change clock of current board and not all boards to allow daisy-chaining
   if (m_drs->GetNumberOfBoards()) {
      if (m_drs->GetBoard(m_board)->GetBoardType() == 6 ||
          m_drs->GetBoard(m_board)->GetBoardType() == 8)
            m_drs->GetBoard(m_board)->SetRefclk(flag);
   }
}

/*------------------------------------------------------------------*/

void Osci::SetCalibVoltage(bool flag, double voltage)
{
   m_calibOn = flag;
   for (int i=0 ; i< m_drs->GetNumberOfBoards() ; i++)
      m_drs->GetBoard(i)->EnableAcal(flag, voltage);
}

/*------------------------------------------------------------------*/

void Osci::SetInputRange(double center)
{
   m_inputRange = center;
   for (int i=0 ; i< m_drs->GetNumberOfBoards() ; i++)
      m_drs->GetBoard(i)->SetInputRange(center);
}

/*------------------------------------------------------------------*/

double Osci::GetCalibratedInputRange()
{
   return m_drs->GetBoard(m_board)->GetCalibratedInputRange();
}

/*------------------------------------------------------------------*/

void Osci::CorrectTriggerPoint(double t)
{
   int i, n, min_i;
   double min_dt, dt;
   double trigPoint[2*kNumberOfBins];
   float  *pt;

   if (m_trgSource == 3 && m_clkOn)
      pt = m_timeClk;
   else
      pt = m_time;

   if (m_trgSource < 4) {
      // search and store all points
      for (i = n = 0 ; i<m_waveDepth-1 ; i++) {
         if (m_trgNegative) {
            if (m_waveform[m_trgSource][i] >= m_trgLevel*1000 &&
                m_waveform[m_trgSource][i+1] < m_trgLevel*1000) {

               dt = pt[i+1] - pt[i];
               dt = dt * 1 / (1 + (m_trgLevel*1000-m_waveform[m_trgSource][i+1])/(m_waveform[m_trgSource][i]-m_trgLevel*1000));
               trigPoint[n++] = pt[i] + dt;
            }
         } else {
            if (m_waveform[m_trgSource][i] <= m_trgLevel*1000 &&
                m_waveform[m_trgSource][i+1] > m_trgLevel*1000) {

               dt = pt[i+1] - pt[i];
               dt = dt * 1 / (1 + (m_waveform[m_trgSource][i+1]-m_trgLevel*1000)/(m_trgLevel*1000-m_waveform[m_trgSource][i]));
               trigPoint[n++] = pt[i] + dt;
            }
         }
      }

      /* search trigger point closest to trigger */
      min_dt = 1e6;
      min_i  = -1;
      for (i=0 ; i<n ; i++) {
         if (fabs(trigPoint[i] - t) < min_dt) {
            min_dt = fabs(trigPoint[i] - t);
            min_i = i;
         }
      }

      /* correct times to shift waveform to trigger piont */
      if (n > 0) {
         dt = trigPoint[min_i] - t;
         for (i=0 ; i<m_waveDepth ; i++)
            m_time[i] -= dt;
         for (i=0 ; i<kNumberOfBins ; i++)
            m_timeClk[i] -= dt;
      }
   }
}

/*------------------------------------------------------------------*/

int spos[1024];

void Osci::RemoveSpikes(bool cascading)
{
   int i, j, k, l, c, ofs, nChn;
   double a, b;
   int sp[8][10];
   int rsp[10], rot_sp[10];
   int n_sp[8], n_rsp;
   float wf[8][2048];
   int  nNeighbor, nSymmetric;

   nChn = cascading ? 8 : 4;

   /* rotate waveform back relative to cell #0 */
   if (cascading) {
      if (m_rotated) {
         for (i=0 ; i<nChn ; i++)
            for (j=0 ; j<1024 ; j++)
               wf[i][(j+m_triggerCell) % 1024] = m_waveform[i/2][(i%2)*1024+j];
      } else {
         for (i=0 ; i<nChn ; i++)
            for (j=0 ; j<1024 ; j++)
               wf[i][j] = m_waveform[i/2][(i%2)*1024+j];
      }
   } else {
      if (m_rotated) {
         for (i=0 ; i<nChn ; i++)
            for (j=0 ; j<1024 ; j++)
               wf[i][(j+m_triggerCell) % 1024] = m_waveform[i][j];
      } else {
         for (i=0 ; i<nChn ; i++)
            for (j=0 ; j<1024 ; j++)
               wf[i][j] = m_waveform[i][j];
      }
   }


   memset(sp, 0, sizeof(sp));
   memset(n_sp, 0, sizeof(n_sp));
   memset(rsp, 0, sizeof(rsp));
   n_rsp = 0;

   /* find spikes with special high-pass filter */
   for (j=0 ; j<1024 ; j++) {
      for (i=0 ; i<nChn ; i++) {
         if (-wf[i][j]+wf[i][(j+1) % 1024]+wf[i][(j+2) % 1024]-wf[i][(j+3) % 1024] > 20) {
            if (n_sp[i] < 10) // record maximum of 10 spikes
               sp[i][n_sp[i]++] = j;
            spos[j]++;
         }
         if (-wf[i][j]+wf[i][(j+1) % 1024]+wf[i][(j+2) % 1024]-wf[i][(j+3) % 1024] < -20) {
            if (n_sp[i] < 10) // record maximum of 10 spikes
               sp[i][n_sp[i]++] = j;
            spos[j]++;
         }
      }
   }

   /* find spikes at cell #0 and #1023 */
   for (i=0 ; i<nChn ; i++) {
      if (wf[i][0]+wf[i][1]-2*wf[i][2] > 20) {
         if (n_sp[i] < 10)
            sp[i][n_sp[i]++] = 0;
      }
      if (-2*wf[i][1021]+wf[i][1022]+wf[i][1023] > 20) {
         if (n_sp[i] < 10)
            sp[i][n_sp[i]++] = 1020;
      }
   }

   /* go through all spikes and look for symmetric spikes and neighbors */
   for (i=0 ; i<nChn ; i++) {
      for (j=0 ; j<n_sp[i] ; j++) {
         /* check if this spike has a symmetric partner in any channel */
         for (k=nSymmetric=0 ; k<nChn ; k++) {
               for (l=0 ; l<n_sp[k] ; l++)
                  if (sp[i][j] == (1020-sp[k][l]+1024) % 1024) {
                     nSymmetric++;
                     break;
                  }
            }

         /* check if this spike has same spike in any other channels */
         for (k=nNeighbor=0 ; k<nChn ; k++)
            if (i != k) {
               for (l=0 ; l<n_sp[k] ; l++)
                  if (sp[i][j] == sp[k][l]) {
                     nNeighbor++;
                     break;
                  }
            }

         if (nSymmetric + nNeighbor >= 2) {
            /* if at least two matching spikes, treat this as a real spike */
            for (k=0 ; k<n_rsp ; k++)
               if (rsp[k] == sp[i][j])
                  break;
            if (n_rsp < 10 && k == n_rsp)
               rsp[n_rsp++] = sp[i][j];
         }
      }
   }

   /* rotate spikes according to trigger cell */
   if (m_rotated) {
      for (i=0 ; i<n_rsp ; i++)
         rot_sp[i] = (rsp[i] - m_triggerCell + 1024) % 1024;
   } else {
      for (i=0 ; i<n_rsp ; i++)
         rot_sp[i] = rsp[i];
   }

   /* recognize spikes if at least one channel has it */
   for (k=0 ; k<n_rsp ; k++) {
      for (i=0 ; i<nChn ; i++) {

         if (cascading) {
            c = i/2;
            ofs = (i%2)*1024;
         } else {
            c = i;
            ofs = 0;
         }
         if (k < n_rsp-1 && rsp[k] == 0 && rsp[k+1] == 1020) {
            /* remove double spike */
            j = rot_sp[k] > rot_sp[k+1] ? rot_sp[k+1] : rot_sp[k];
            a = m_waveform[c][ofs+(j+1) % 1024];
            b = m_waveform[c][ofs+(j+6) % 1024];
            if (fabs(a-b) < 15) {
               m_waveform[c][ofs+(j+2) % 1024] = a + 1*(b-a)/5;
               m_waveform[c][ofs+(j+3) % 1024] = a + 2*(b-a)/5;
               m_waveform[c][ofs+(j+4) % 1024] = a + 3*(b-a)/5;
               m_waveform[c][ofs+(j+5) % 1024] = a + 4*(b-a)/5;
            } else {
               m_waveform[c][ofs+(j+2) % 1024] -= 14.8f;
               m_waveform[c][ofs+(j+3) % 1024] -= 14.8f;
               m_waveform[c][ofs+(j+4) % 1024] -= 14.8f;
               m_waveform[c][ofs+(j+5) % 1024] -= 14.8f;
            }
         } else {
            /* remove single spike */
            a = m_waveform[c][ofs+rot_sp[k]];
            b = m_waveform[c][ofs+(rot_sp[k]+3) % 1024];

            if (fabs(a-b) < 15) {
               m_waveform[c][ofs+(rot_sp[k]+1) % 1024] = a + 1*(b-a)/3;
               m_waveform[c][ofs+(rot_sp[k]+2) % 1024] = a + 2*(b-a)/3;
            } else {
               m_waveform[c][ofs+(rot_sp[k]+1) % 1024] -= 14.8f;
               m_waveform[c][ofs+(rot_sp[k]+2) % 1024] -= 14.8f;
            }
         }
      }
      if (k < n_rsp-1 && rsp[k] == 0 && rsp[k+1] == 1020)
         k++; // skip second half of double spike
   }

   /* uncomment to show unfixed spikes
   m_skipDisplay = true;
   for (i=0 ; i<1024 ; i++)
      for (j=0 ; j<nChn ; j++) {
         if (m_waveform[j][i] > 10 || m_waveform[j][i] < -10) {
            m_skipDisplay = false;
            break;
         }
   }
   */
}
