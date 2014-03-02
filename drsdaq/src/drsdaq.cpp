//------------------------------------------------------------------------------
// System Headers
//------------------------------------------------------------------------------
#include <vector>
#include <iostream>
#include <math.h>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
// Local Headers
//------------------------------------------------------------------------------
#include "strlcpy.h"
#include "DRS.h"
#include "rapidxml.hpp"
#include "progressbar.h"
#define O_BINARY 0
#define DIR_SEPARATOR '/'

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

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------
int SaveWaveforms(int fd);
int init();

//------------------------------------------------------------------------------
// Vars
//------------------------------------------------------------------------------

unsigned char buffer[100000];

DRS *drs;
DRSBoard *board;
TIMESTAMP evTimestamp;

int     evSerial=0;         //event number
float   time_array[1024];   //timebinarray
float   waveform[8][1024];  //8x1024 array (8 channels x 1024 samples)
int     numEvents=1000;     //number of events to record
int     waveDepth=1024;     //waveform depth of the evaluation board
double  center=0.0;         //zero point
double  triglevel=-0.02;    //trigger level (in VOLTS)
bool    chnOn[4]={};        //channel-On bitmask
int     trigsource=0;       //choose a trigger source
double  freq=5;             //sampling frequency

/*------------------------------------------------------------------*/

int main( int argc, char *argv[] )
{
    //Take in filename from argv
    const char* filename;
    filename=argv[1];
    if (argc != 2) {
        printf("Need to Specify Output Filename\n");
        return 0;
    }


    int i, j, nBoards;

    // do initial scan for boards
    drs = new DRS();

    // show any found board(s)
    for (i=0 ; i<drs->GetNumberOfBoards() ; i++) {
        board = drs->GetBoard(i);
        printf("Found DRS4 evaluation board, \
                serial #%d, firmware revision %d\n", 
                board->GetBoardSerialNumber(), board->GetFirmwareVersion());
    }

    // exit if no board found
    nBoards = drs->GetNumberOfBoards();
    if (nBoards == 0) {
        printf("No DRS4 evaluation board found\n");
        return 0;
    }

    //initialize board
    init();

    //parse XML config file for run options
    ParseOptions(); 

    //print options for current run
    cout << "Running with options: \n";
    cout << "Trigger Level:"        << triglevel        << "\n";
    cout << "Trigger Source:"       << trigsource       << "\n";
    cout << "Zero-point:"           << center           << "\n";
    cout << "Number of events:"     << numEvents        << "\n";
    cout << "Sampling frequency:"   << freq << "GHz"    << "\n";
    cout << "Channel 0 on/off:"     << chnOn[0]         << "\n";
    cout << "Channel 1 on/off :"    << chnOn[1]         << "\n";
    cout << "Channel 2 on/off :"    << chnOn[2]         << "\n";
    cout << "Channel 3 on/off :"    << chnOn[3]         << "\n";

    //trigger options
    board->EnableTrigger(1, 0);             // enable hardware trigger
    board->SetTriggerSource(7);             // set trigger source
    board->SetTriggerLevel(-0.05, false);   // trig level, edge
    board->SetTriggerDelayNs(0);            // trigger delay

    /* NOTE: 
     * SetTriggerSource accepts an INT, determined by 12-bit bitmask
     *
     * 1<<0 = CHN1 OR
     * 1<<1 = CHN2 OR
     * 1<<2 = CHN3 OR
     * 1<<3 = CHN4 OR
     * 
     * 1<<(0+8) = CHN1 AND
     * 1<<(1+8) = CHN2 AND
     * 1<<(2+8) = CHN3 AND
     * 1<<(4+8) = CHN4 AND
     *
     * OR and AND triggering is achieved by taking the bitwise OR of 
     * whatever channels are desired... 
     *
     * e.g.:
     * To trig on CHN1 or CHN2 
     *      SetTriggerSource(1<<0 | 1<<1)
     *
     * To Trig on CHN1 and CHN2 
     *      SetTriggerSource( 1<<(0+8) | 1<<(1+8) )
     *
     * 6 = 0110 = 0100 | 0010        (Trig on CHN2 AND 3)
     * 7 = 0111 = 0100 | 0010 | 0001 (Trig on CHN1 and 2 and 3)
     *
     */


    // open output file
    int WFfd;
    WFfd = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0644);

    // Collect Events 
    for (int j=0 ; j<numEvents ; j++) {

        // start board (activate domino wave)
        board->StartDomino();

        // wait for trigger
        while (board->IsBusy());

        // Save Waveforms
        evSerial=j;
        SaveWaveforms(WFfd); 

        // Print some status progress
        if (j % 100 == 0)
            printf("\nEvent #%d read successfully\n", j);
    }

    //Close Waveform File
    close(WFfd);

    // delete DRS object, close USB connection
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
    *(unsigned short *)p = 0; // reserved bit
    p += sizeof(unsigned short);

    board->GetTime(0, board->GetTriggerCell(0), time_array);

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

    // transfer wave 
    board->TransferWaves(0, 8);
    
    // loop over active channels
    for (int i=0 ; i<4 ; i++) {
        if (chnOn[i]) {
            board->GetWave(0, i*2, waveform[i]);
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
            } // close for j<waveDepth
        } // close if chnOn[i]
    } // close for i<4

    int size = p-buffer;
    int n = write(fd, buffer, size);
    if (n != size)
        return -1;

    return 1;
}

//------------------------------------------------------------------------------
// Parses XML configuration file
//------------------------------------------------------------------------------
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
    root_node = doc.first_node("DRS4");

    for (xml_node<> * RunConfig = root_node->first_node("RunConfig"); RunConfig; RunConfig = RunConfig->next_sibling()) {
        triglevel=atof(RunConfig->first_attribute("triglevel")->value());
        trigsource=atoi(RunConfig->first_attribute("trigsource")->value());
        center=atoi(RunConfig->first_attribute("center")->value());
        numEvents=atoi(RunConfig->first_attribute("numEvents")->value());
        freq=atof(RunConfig->first_attribute("freq")->value());
        chnOn[0]=atoi(RunConfig->first_attribute("chnOn0")->value());
        chnOn[1]=atoi(RunConfig->first_attribute("chnOn1")->value());
        chnOn[2]=atoi(RunConfig->first_attribute("chnOn2")->value());
        chnOn[3]=atoi(RunConfig->first_attribute("chnOn3")->value());
    }
}

//------------------------------------------------------------------------------
// Initializes DRS board
//------------------------------------------------------------------------------
int init()
{
    // continue working with first board only
    board = drs->GetBoard(0);
    // initialize board
    board->Init();
    // enable transparent mode needed for analog trigger
    board->SetTranspMode(1);
    // set input range to -0.5V ... +0.5V - default
    board->SetInputRange(center);
    // use following line to set range to 0..1V
    //board->SetInputRange(0.5);
    return 1;
}
