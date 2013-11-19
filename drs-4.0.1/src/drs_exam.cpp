#include <math.h>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>

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


typedef struct {
    unsigned short Year;
    unsigned short Month;
    unsigned short Day;
    unsigned short Hour;
    unsigned short Minute;
    unsigned short Second;
    unsigned short Milliseconds;
} TIMESTAMP;

void GetTimeStamp(TIMESTAMP &ts);

unsigned char buffer[100000];
int SaveWaveforms(int fd);

DRS *drs;
DRSBoard *b;
TIMESTAMP evTimestamp;

float time_array[1024];
float waveform[8][1024];
int evSerial; 
int numEvents=1000;
int waveDepth=1024;
bool chnOn[4];
double center=0.0; //zero point
double triglevel=10; //trigger level (in mV)


/*------------------------------------------------------------------*/

int main( int argc, char *argv[] )
{
    const char* filename="default.dat";
    filename=argv[1];

    //printf("filename %s",filename);
    if (argc != 2)
    {
        printf("Need to Specify Output Filename\n");
        return 0;
    }

    chnOn[0]=1; //Channel 1 ON/OFF
    chnOn[1]=1;
    chnOn[2]=0;
    chnOn[3]=0;
    int i, j, nBoards;

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
    if (nBoards == 0) 
    {
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

    //set input range center
    b->SetInputRange(0);

    /* use following line to set range to 0..1V */
    //b->SetInputRange(0.5);

    //use following lines to enable hardware trigger 
    b->EnableTrigger(1, 0);           // enable hardware trigger
    b->SetTriggerSource(1<<0);        // set CH1 as source
    b->SetTriggerLevel(-0.05, false);     // trig level, rising or falling edge
    b->SetTriggerDelayNs(200);             // zero ns trigger delay


    /* use following lines to enable the external trigger */
    //if (b->GetBoardType() == 8) {     // Evaluaiton Board V4
    //   b->EnableTrigger(1, 0);           // enable hardware trigger
    //   b->SetTriggerSource(1<<4);        // set external trigger as source
    //} else {                          // Evaluation Board V3
    //   b->EnableTrigger(1, 0);           // lemo on, analog trigger off
    // }


        //open output file
        int WFfd;
        WFfd = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0644);
    
    //Collect Events
    for (j=0 ; j<numEvents ; j++) {

        /* start board (activate domino wave) */
        b->StartDomino();


        /* wait for trigger */
        while (b->IsBusy());

        
        //Save Waveforms
        SaveWaveforms(WFfd); 

        //Print some status
        if (j % 100 == 0)
            printf("\rEvent #%d read successfully\n", j);
        
    }

    //Close Waveform File
    close(WFfd);

    /* delete DRS object -> close USB connection */
    delete drs;
}

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

int SaveWaveforms(int fd)
{
    //char str[80];
    unsigned char *p;
    unsigned short d;
    float t;

    if (fd == 0)
        return 0;
    
    GetTimeStamp(evTimestamp); 

    p = buffer;
    memcpy(p, "EHDR", 4);
    p += 4;
    *(int *)p = evSerial;
    p += sizeof(int);
    *(unsigned short *)p = evTimestamp.Year;
    p += sizeof(unsigned short);
    *(unsigned short *)p = evTimestamp.Month;
    p += sizeof(unsigned short);
    *(unsigned short *)p = evTimestamp.Day;
    p += sizeof(unsigned short);
    *(unsigned short *)p = evTimestamp.Hour;
    p += sizeof(unsigned short);
    *(unsigned short *)p = evTimestamp.Minute;
    p += sizeof(unsigned short);
    *(unsigned short *)p = evTimestamp.Second;
    p += sizeof(unsigned short);
    *(unsigned short *)p = evTimestamp.Milliseconds;
    p += sizeof(unsigned short);
    *(unsigned short *)p = 0; // reserved
    p += sizeof(unsigned short);

    b->GetTime(0, b->GetTriggerCell(0), time_array);
    for (int j=0 ; j<waveDepth ; j++) 
    {
        // save binary time as 32-bit float value
        if (waveDepth == 2048) 
        {
            t = (time_array[j]+time_array[j+1])/2;
            j++;
        } 

        else
            t = time_array[j];
        *(float *)p = t;
        p += sizeof(float);
    }

    b->TransferWaves(0, 8);
    for (int i=0 ; i<4 ; i++) 
    {
        if (chnOn[i]) 
        {
            b->GetWave(0, i*2, waveform[i]);

            sprintf((char *)p, "C%03d", i+1);
            p += 4;
            for (int j=0 ; j<waveDepth ; j++)
            {
                // save binary date as 16-bit value: 0 = -0.5V, 65535 = +0.5V
                if (waveDepth == 2048) 
                {
                    // in cascaded mode, save 1024 values as averages of the 2048 values
                    d = (unsigned short)(((waveform[i][j]+waveform[i][j+1])/2000.0 + 0.5) * 65535);
                    *(unsigned short *)p = d;
                    p += sizeof(unsigned short);
                    j++;
                } 
                else 
                {
                    d = (unsigned short)((waveform[i][j]/1000.0 + 0.5) * 65535);
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

    return 1;
}
