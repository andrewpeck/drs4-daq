#include <math.h>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#define O_BINARY 0
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <errno.h>
#define DIR_SEPARATOR '/'
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "strlcpy.h"
#include "DRS.h"
#include "rapidxml.hpp"
#include <vector>
#include <iostream>
#include "progressbar.h"

using namespace rapidxml;
using namespace std;

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
void ParseOptions(void);

unsigned char buffer[100000];
int SaveWaveforms(int fd);
int calibrate();
int init();

DRS *drs;
DRSBoard *b;
TIMESTAMP evTimestamp;

int evSerial=0; //event number
float time_array[1024]; //timebinarray
float waveform[8][1024]; //8x1024 array (8 channels x 1024 samples)
int numEvents=1000;
int waveDepth=1024;
double center=0.0; //zero point
double triglevel=-100; //trigger level (in mV)
bool chnOn[4]={};
int trigsource=0;
bool posneg=0; //1=rising edge 0=falling edge
double freq=5; //sampling frequency

/*------------------------------------------------------------------*/

int main( int argc, char *argv[] )
{
    //calibrate board
    if (strcomp(argv[1],"calibrate")==0)
        calibrate();

    //filename = argument1 
    const char* filename;
    filename=argv[1];
    if (argc != 2) {
        printf("Need to Specify Output Filename\n");
        return 0;
    }


    int i, j, nBoards;

    /* do initial scan */
    drs = new DRS();

    /* show any found board(s) */
    for (i=0 ; i<drs->GetNumberOfBoards() ; i++) {
        b = drs->GetBoard(i);
        printf("Found DRS4 evaluation board, \
                serial #%d, firmware revision %d\n", 
                b->GetBoardSerialNumber(), b->GetFirmwareVersion());
    }

    /* exit if no board found */
    nBoards = drs->GetNumberOfBoards();
    if (nBoards == 0) {
        printf("No DRS4 evaluation board found\n");
        return 0;
    }

    //initialize board
    init();

    //parse XML config file for options
    ParseOptions(); 

    //print options for current run
    cout << "Running with options: \n";
    cout << "Trigger Level:"        << triglevel    << "\n";
    cout << "Trigger Source:"       << trigsource   << "\n";
    cout << "Zero-point:"           << center       << "\n";
    cout << "Number of events:"     << numEvents    << "\n";
    cout << "Sampling frequency:"   << freq <<"GHz" << "\n";
    cout << "Channel 0 on/off:"     << chnOn[0]     << "\n";
    cout << "Channel 1 on/off :"    << chnOn[1]     << "\n";
    cout << "Channel 2 on/off :"    << chnOn[2]     << "\n";
    cout << "Channel 3 on/off :"    << chnOn[3]     << "\n";

    //use following lines to enable hardware trigger 
    b->EnableTrigger(1, 0);             // enable hardware trigger
    b->SetTriggerSource(1<<trigsource); // set trigger source
    //0=ch0 1=ch1 2=ch2 3=ch3 4=EXT
    //DATA WORD SPECIFIES TRIGGERING INFORMATION
    //0000=CH0; 0001=CH1 ETC.
    //0011=CH0+CH1
    //
    b->SetTriggerLevel(-0.20, posneg);  // trig level, edge
    b->SetTriggerDelayNs(150);          // trigger delay

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
        evSerial=j;
        SaveWaveforms(WFfd); 

        //Print some status
        if (j % 100 == 0)
            printf("\nEvent #%d read successfully\n", j);
    }

    //Close Waveform File
    close(WFfd);

    /* delete DRS object -> close USB connection */
    delete drs;
}

void GetTimeStamp(TIMESTAMP &ts)
{
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

    for (int j=0 ; j<waveDepth ; j++) {
        // save binary time as 32-bit float value
        if (waveDepth == 2048) {
            t = (time_array[j]+time_array[j+1])/2;
            j++;
        } 

        else
            t = time_array[j];

        *(float *)p = t;
        p += sizeof(float);
    }

    b->TransferWaves(0, 8);
    for (int i=0 ; i<4 ; i++) {
        if (chnOn[i]) {
            b->GetWave(0, i*2, waveform[i]);
            sprintf((char *)p, "C%03d", i+1);
            p += 4;

            for (int j=0 ; j<waveDepth ; j++) {
                // save binary date as 16-bit value: 0 = -0.5V, 65535 = +0.5V
                if (waveDepth == 2048) {
                    // in cascaded mode, save 1024 values 
                    // as averages of the 2048 values
                    d = (unsigned short)(((waveform[i][j]+waveform[i][j+1]) \
                         /2000.0+0.5)*65535);
                    *(unsigned short *)p = d;
                    p += sizeof(unsigned short);
                    j++;
                } 
                else {
                    d = (unsigned short)((waveform[i][j]/1000.0+0.5)*65535);
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

void ParseOptions(void) //Reads XML config file config.xml
{
    xml_document<> doc; 
    xml_node<> * root_node; 
    ifstream ConfigFile ("config.xml");
    vector<char> buffer((istreambuf_iterator<char>(ConfigFile)), 
            istreambuf_iterator<char>());
    buffer.push_back('\0');

    // Parse the buffer using the xml file parsing library into doc 
    doc.parse<0>(&buffer[0]);

    // Find our root node
    root_node = doc.first_node("DRS4Config");

    for (xml_node<> * RunConfig = root_node->first_node("RunConfig"); 
            RunConfig; RunConfig = RunConfig->next_sibling()) {

        triglevel=atof(RunConfig->first_attribute("triglevel")->value());
        trigsource=atoi(RunConfig->first_attribute("trigsource")->value());
        center=atoi(RunConfig->first_attribute("center")->value());
        numEvents=atoi(RunConfig->first_attribute("numEvents")->value());
        freq=atof(RunConfig->first_attribute("freq")->value());
        posneg=atoi(RunConfig->first_attribute("posneg")->value());
        chnOn[0]=atoi(RunConfig->first_attribute("ChnOn0")->value());
        chnOn[1]=atoi(RunConfig->first_attribute("ChnOn1")->value());
        chnOn[2]=atoi(RunConfig->first_attribute("ChnOn2")->value());
        chnOn[3]=atoi(RunConfig->first_attribute("ChnOn3")->value());
    }
}


int calibrate()
{
    ProgressBar p;

    char line[80];
    float freq,range;

    //Initialize DRS4 board
    init();

    printf("\nEnter calibration frequency [GHz]: ");
    fgets(line, sizeof(line), stdin);
    freq = atof(line);
    printf("freq: %f\n",freq);
    b->SetFrequency(freq, true);
    cout << "\tVoltage calibration at "<<freq<< " GHz\n";

    printf("Enter center of voltage range [V]: ");
    fgets(line, sizeof(line), stdin);
    range = atof(line);
    b->SetInputRange(range);
    cout <<"\tRange: "<<center-0.5<<" V - "<<center+0.5<<"V\n";

    //printf("Enter mode [1]024 or [2]048 bin mode: ");
    //fgets(line, sizeof(line), stdin);
    //int cascading;
    //cascading = atoi(line);
    //if (cascading == 2)
    b->SetChannelConfig(0, 8, 4);
    //else
    //	b->SetChannelConfig(0, 8, 8);

    printf("\nPlease make sure that no input signal are present then hit any key\r");
    fflush(stdout);
    while (!kbhit());
    printf("                                                                  \r");
    while (kbhit())
        getchar();

    if (b->GetTransport() == TR_VME)
        printf("Creating Calibration of Board in VME slot %2d %s, serial #%04d\n",  
                (b->GetSlotNumber() >> 1)+2, ((b->GetSlotNumber() & 1) == 0) ? "upper" : "lower", 
                b->GetBoardSerialNumber());
    else
        printf("Creating Calibration of Board on USB, serial #%04d\n",  
                b->GetBoardSerialNumber());

    b->SoftTrigger();
    b->CalibrateVolt(&p);
    printf("\nDone! \n");

    cout <<"Calibration finished\n";
    return 0;
}


int init()
{
    /* continue working with first board only */
    b = drs->GetBoard(0);
    /* initialize board */
    b->Init();
    /* enable transparent mode needed for analog trigger */
    b->SetTranspMode(1);
    /* set input range to -0.5V ... +0.5V - default*/
    b->SetInputRange(center);
    /* use following line to set range to 0..1V */
    //b->SetInputRange(0.5);
    return 1;
}
