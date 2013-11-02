/*
 * EPThread.h
 * DRS oscilloscope event processor header file
 * $Id: EPThread.h 17273 2011-03-07 10:52:36Z ritt $
 */

class EPThread : public wxThread
{
public:
   EPThread(DOFrame *o);
   ~EPThread();
   void *Entry();
   float *GetTime()            { return m_time; }
   float *GetWaveform(int c)   { return m_waveform[c]; }
   bool IsFinished()           { return m_finished; }

private:
   DOFrame *m_frame;
   Osci    *m_osci;
   bool     m_finished;
   float    m_waveform[4][2048];
   float    m_time[2048];
};
