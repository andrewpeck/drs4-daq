/*
 * Osci.h
 * DRS oscilloscope header file
 * $Id: Osci.h 20380 2012-11-13 13:38:01Z ritt $
 */

typedef struct {
   unsigned short Year;
   unsigned short Month;
   unsigned short Day;
   unsigned short Hour;
   unsigned short Minute;
   unsigned short Second;
   unsigned short Milliseconds;
} TIMESTAMP;

#define TM_AUTO    0
#define TM_NORMAL  1

/*------------------------------------------------------------------*/

class Osci;

class OsciThread : public wxThread
{
public:
   OsciThread(Osci *o);
   bool IsIdle();
   void *Entry();
   void ResetSW();
   void Enable(bool flag);
   bool IsFinished() { return m_finished; }

private:
   Osci *m_osci;
   wxStopWatch m_sw1, m_sw2;
   bool m_enabled;
   bool m_active;
   bool m_finished;
};

/*------------------------------------------------------------------*/

class Osci
{
public:
   Osci(double samplingSpeed = 5, bool mthread = true);
   ~Osci();
   int       ScanBoards();
   int       GetNumberOfBoards()   { return m_drs->GetNumberOfBoards(); }
   DRSBoard *GetBoard(int i)       { return m_drs->GetBoard(i); }
   DRSBoard *GetCurrentBoard()     { return m_drs->GetBoard(m_board); }
   DRS      *GetDRS()              { return m_drs; }
   bool      GetError(char *str, int size) { return m_drs->GetError(str, size); }
   void      CheckTimingCalibration();
   void      SelectSource(int board, int firstChannel, int chnSection);
   void      SetRunning(bool flag);
   void      Enable(bool flag);
   bool      IsRunning()           { return m_running; }
   void      SetSingle(bool flag);
   bool      IsSingle()            { return m_single; }
   void      SetArmed(bool flag)   { m_armed = flag; }
   bool      IsArmed()             { return m_armed; }
   bool      IsIdle();
   int       GetWaveformDepth(int channel);
   double    GetWaveformLength()   { return m_waveDepth / GetSamplingSpeed(); }
   float    *GetWaveform(int i)    { return (float *)m_waveform[i]; }
   float    *GetTime(int channel);
   int       GetChip()             { return m_chip; }
   void      SetSamplingSpeed(double freq);
   double    GetSamplingSpeed();
   double    GetMinSamplingSpeed();
   double    GetMaxSamplingSpeed();
   bool      IsTCalibrated();
   bool      IsVCalibrated();
   bool      GetTimeCalibration(int mode, float *time, bool force=false);
   void      Start();
   void      Stop();
   void      DrainEvents();
   void      SingleTrigger();
   void      ReadWaveforms();
   int       SaveWaveforms(MXML_WRITER *, int);
   bool      HasTriggered();
   bool      HasNewEvent();
   void      SetTriggerLevel(double level, bool negative);
   void      SetTriggerDelay(int delay);
   int       GetTriggerDelay();
   double    GetTriggerDelayNs();
   void      SetTriggerMode(int mode) { m_trgMode = mode; }
   int       GetTriggerMode() { return m_trgMode; }
   void      SetTriggerSource(int source);
   int       GetTriggerSource() { return m_trgSource; }
   void      SetTriggerConfig(int tc);
   void      SetRefclk(bool flag);
   void      SetChnOn(int chn, bool flag);
   void      SetClkOn(bool flag);
   void      SetEventSerial(int serial) { m_evSerial = serial; }
   void      SetCalibVoltage(bool flag, double voltage);
   void      SetInputRange(double center);
   double    GetInputRange() { return m_inputRange; }
   double    GetCalibratedInputRange();
   void      SetCalibrated(bool flag) { m_calibrated = flag; }
   void      SetCalibrated2(bool flag) { m_calibrated2 = flag; }
   void      SetTCalOn(bool flag) { m_tcalon = flag; }
   bool      IsTCalOn() { return m_tcalon; }
   void      SetRotated(bool flag) { m_rotated = flag; }
   void      SetSpikeRemoval(bool flag) { m_spikeRemoval = flag; }
   void      CorrectTriggerPoint(double t);
   void      RemoveSpikes(bool cascading);
   bool      SkipDisplay(void) { return m_skipDisplay; }
   int       GetChnSection(void) { return m_chnSection; }
   char      *GetDebugMsg(void) { return m_debugMsg; }

private:
   DRS      *m_drs;
   OsciThread *m_thread;
   bool      m_running;
   bool      m_single;
   bool      m_armed;
   double    m_samplingSpeed;
   float     m_waveform[4][2048];
   float     m_refwaveform[2048];
   unsigned char m_wavebuffer[9 * 1024*2 + 4];
   float     m_time[2048];
   float     m_timeClk[1024];
   int       m_triggerCell;
   int       m_writeSR;
   int       m_waveDepth;
   int       m_trgMode;
   int       m_trgSource;
   bool      m_trgNegative;
   int       m_trgDelay;
   double    m_trgLevel;
   bool      m_chnOn[4];
   bool      m_clkOn;
   bool      m_refClk;
   bool      m_calibOn;
   int       m_evSerial;
   TIMESTAMP m_evTimestamp;
   bool      m_calibrated;
   bool      m_calibrated2;
   bool      m_tcalon;
   bool      m_rotated;
   int       m_nDRS;
   int       m_board;
   int       m_chip;
   int       m_chnOffset;
   int       m_chnSection;
   bool      m_spikeRemoval;
   double    m_inputRange;
   bool      m_skipDisplay;
   char      m_debugMsg[256];
};

/*------------------------------------------------------------------*/

