/********************************************************************\

Name:         drs_exam.cpp
Created by:   Stefan Ritt

Contents:     Simple example application to read out a DRS4
evaluation board

$Id: drs_exam.cpp 20380 2012-11-13 13:38:01Z ritt $

\********************************************************************/

#include <math.h>

#ifdef _MSC_VER

#include <windows.h>

#elif defined(OS_LINUX)

#define O_BINARY 0

#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <errno.h>

#define DIR_SEPARATOR '/'

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "strlcpy.h"
#include "DRS.h"

#include "Osci.h"

#ifdef USE_DRS_MUTEX 
#include "wx/wx.h"    // must be before <windows.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <algorithm>
#include <sys/stat.h>
#include "strlcpy.h"
#include "DRS.h"

/*------------------------------------------------------------------*/

int main()
{
// 
   m_evSerial = 0;
//

	int i, j, nBoards;
	DRS *drs;
	DRSBoard *b;
	float time_array[1024];
	float wave_array[8][1024];
	FILE  *f;

	/* do initial scan */
	drs = new DRS();

	/* show any found board(s) */
	for (i=0 ; i<drs->GetNumberOfBoards() ; i++) {
		b = drs->GetBoard(i);
		printf("Found DRS4 evaluation board, serial #%d, firmware revision %d\n", 
				b->GetBoardSerialNumber(), b->GetFirmwareVersion());
	}

	/* exit if no board found */
	nBoards = drs->GetNumberOfBoards();
	if (nBoards == 0) {
		printf("No DRS4 evaluation board found\n");
		return 0;
	}

	/* continue working with first board only */
	b = drs->GetBoard(0);

	/* initialize board */
	b->Init();

	/* set sampling frequency */
	b->SetFrequency(5, true);

	/* enable transparent mode needed for analog trigger */
	b->SetTranspMode(1);

	/* set input range to -0.5V ... +0.5V */
	b->SetInputRange(0);

	/* use following line to set range to 0..1V */
	//b->SetInputRange(0.5);

	/* use following lines to enable hardware trigger on CH1 at 50 mV positive edge */
	if (b->GetBoardType() == 8) {     // Evaluaiton Board V4
		b->EnableTrigger(1, 0);           // enable hardware trigger
		b->SetTriggerSource(1<<0);        // set CH1 as source
	} else {                          // Evaluation Board V3
		b->EnableTrigger(0, 1);           // lemo off, analog trigger on
		b->SetTriggerSource(0);           // use CH1 as source
	}
	b->SetTriggerLevel(-0.05, false);     // 0.05 V, positive edge
	b->SetTriggerDelayNs(0);             // zero ns trigger delay

	/* use following lines to enable the external trigger */
	//if (b->GetBoardType() == 8) {     // Evaluaiton Board V4
	//   b->EnableTrigger(1, 0);           // enable hardware trigger
	//   b->SetTriggerSource(1<<4);        // set external trigger as source
	//} else {                          // Evaluation Board V3
	//   b->EnableTrigger(1, 0);           // lemo on, analog trigger off
	// }

	/* open file to save waveforms */
	f = fopen("data.txt", "w");
	if (f == NULL) {
		perror("ERROR: Cannot open file \"data.txt\"");
		return 1;
	}

	/* repeat ten times */
	for (j=0 ; j<10000 ; j++) {

		/* start board (activate domino wave) */
		b->StartDomino();

		/* wait for trigger */
		while (b->IsBusy());

		/* read all waveforms */
		b->TransferWaves(0, 8);

		/* read time (X) array in ns */
		b->GetTime(0, b->GetTriggerCell(0), time_array);

		/* decode waveform (Y) array first channel in mV */
		b->GetWave(0, 0, wave_array[0]);

		/* decode waveform (Y) array second channel in mV
Note: On the evaluation board input #1 is connected to channel 0 and 1 of
the DRS chip, input #2 is connected to channel 2 and 3 and so on. So to
get the input #2 we have to read DRS channel #2, not #1 */
		b->GetWave(0, 2, wave_array[1]);

		
		char str[80];
		unsigned char *p;
		unsigned short d;
		float t;
		unsigned char buffer[100000];
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

			int size = p-buffer;
			int n = write(fd, buffer, size);
			if (n != size)
				return -1;

			/* print some progress indication */
			if (j % 100 == 0)
				printf("\rEvent #%d read successfully\n", j);
		}

		fclose(f);

		/* delete DRS object -> close USB connection */
		delete drs;
	}
