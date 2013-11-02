/*
 * EPThread.cpp
 * DRS oscilloscope event processing thread
 * $Id: EPThread.cpp 17281 2011-03-07 13:58:24Z ritt $
 */

#include "DRSOscInc.h"
#include "rb.h"

extern wxCriticalSection *g_epcs;

/*------------------------------------------------------------------*/

EPThread::EPThread(DOFrame *f) : wxThread()
{
   m_frame = f;
   m_osci = f->GetOsci();
   m_finished = false;
   Create();
   Run();
} 

/*------------------------------------------------------------------*/

EPThread::~EPThread()
{
} 

/*------------------------------------------------------------------*/

void *EPThread::Entry()
{
   int status;

   do {
      if (m_osci->HasNewEvent()) {
         m_osci->ReadWaveforms();
         if (m_frame->GetRearm()) {
            m_osci->Start();
            m_frame->SetRearm(false);
         }

         if (m_frame->GetTrgCorr())
            m_osci->CorrectTriggerPoint(m_frame->GetTrgPosition());

         g_epcs->Enter();
         status = m_osci->SaveWaveforms(m_frame->GetWFFile(), m_frame->GetWFfd());
         g_epcs->Leave();

         if (status < 0) 
            m_frame->CloseWFFile(true);
      
         m_frame->EvaluateMeasurements();
         m_frame->IncrementAcquisitions();

         // copy event from oscilloscope
         g_epcs->Enter();
         memcpy(m_time, m_osci->GetTime(0), m_osci->GetWaveformDepth(0)*sizeof(float));
         for (int i=0 ; i<4 ; i++)
            memcpy(m_waveform[i], m_osci->GetWaveform(i), m_osci->GetWaveformDepth(i)*sizeof(float));
         g_epcs->Leave();
      } else
         wxThread::Sleep(10);

   } while (!TestDestroy());

   m_finished = true;

   return NULL;
}

