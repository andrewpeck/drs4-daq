/********************************************************************\
Name:         main.cpp
Created by:   Nam, adapted from Stefan Ritt's drs_exam.cpp
Contents:     Simple example application to read out a DRS4
evaluation board
\********************************************************************/

#include "main.h"
#include "progressbar.h"
#include "DRS.h"
//#include "Osci.h"

//class OsciThread
//{
//    OsciThread(Osci *o);
//    bool IsIdle();
//    void *Entry();
//    void ResetSW();
//    void Enable(bool flag);
//    bool IsFinished() { return m_finished; }
//
//    Osci *m_osci;
//    bool m_enabled;
//    bool m_active;
//    bool m_finished;
//};

DRS *drs;
DRSBoard *b;

int m_WFfd = 0;
DRS m_drs;
//OsciThread m_thread;
bool m_running = false;
bool m_single = false;
bool m_armed = false;
double m_samplingSpeed;
int m_triggerCell = 0;
int m_writeSR = 0;
int m_waveDepth = 1024;
bool m_trgNegative = false;
int m_trgDelay = 0;
double m_trgLevel = 0;
bool m_chnOn[4]={};
bool m_clkOn = false;
bool m_refClk = false;
bool m_calibOn = false;
int m_evSerial = 0;
bool m_calibrated = true;
bool m_calibrated2 = true;
bool m_tcalon = true;
bool m_rotated = true;
int m_nDRS = 0;
int m_board = 0;
int m_chip = 0;
int m_chnOffset = 0;
int m_chnSection = 0;
bool m_spikeRemoval = false;
double m_inputRange = 0;
bool m_skipDisplay = false;

float time_array[1024];
float wave_array[8][1024];
FILE  *f;
TEvent *evt;
TFile *rootfile;
TTree *t;
uint32_t NumEvents;
int			runNo;
int			trigsource;
double	triglevel;
double 	trigdelay;
bool		trigedge;
double	freq;
double	midpoint;
char		filename[2000];
std::string	dirstr;
std::string	filestr;
fstream	logfile;


/*------------------------------------------------------------------*/
int main(int argc, char** argv)
{
    //Logging for each run
    logfile.open("output/runlog.txt",fstream::app|fstream::out);
    time_t ltime;
    ltime = time(NULL);
    logfile<< Timestamp();
    logfile<<"    Run started\n";
    logfile<<"\tCommand: ";
    for(int i = 0; i < argc; i++) logfile << argv[i]<<" "; 
    logfile << "\n";

    //Initialization, if no board is found, exit
    if (!Scan()) 
        return 0;

    //Set Defaults
    freq 			= 1.0;      //Sampling Rate
    midpoint	    = 0.0;      //Voltage Midpoint
    trigsource 	    = 1;        //Trigger Channel
    triglevel	 	= -0.1;     //Trigger Level
    trigdelay	 	= 0;        //Trigger Delay
    trigedge 		= false;    
    NumEvents           = 3000;     //Default No. of Events to Store
    dirstr = "./output/";
    filestr = "test.root";

    //Get Options 
    //c:calibrate, i: info, h: help
    //f: freq, v: range (voltage), o: output file name
    //d: trigdelay, r: run #, s: trigger source, l: trig. level
    int c;
    while((c = getopt(argc,argv,"ciho:r:n:s:l:f:v:d:?")) != -1)
        switch (c)
        {
            case 'i': 
                Info();
                return 0;
                break;
            case 'c':
                Calibrate();
                return 0;
                break;
            case 'h':
                ShowHelp();
                return 0;
                break;
            case 'o':
                filestr = optarg;
                dirstr	= "";
                break;
            case 'r':
                runNo = atoi(optarg);
                char cstr[1000];
                sprintf(cstr,"run%05d.root",runNo);
                filestr = "";
                filestr = filestr.append(cstr);
                break;
            case 'f':
                freq = atof(optarg);
                break;
            case 'v':
                midpoint = atof(optarg);
                break;
            case 'n':
                NumEvents = atoi(optarg);
                break;
            case 's':
                trigsource = atoi(optarg);
                break;
            case 'l':
                triglevel = atof(optarg);
                break;
            case 'd':
                trigdelay = atof(optarg);
                break;
            case '?':
                ShowHelp();
                return 0;
                break;
        }

    filestr = dirstr.append(filestr);
    logfile<<"\tOutput file: "<<filestr<<std::endl;
    strcpy(filename,filestr.c_str());


    //Measure();

    m_WFfd = open(filename.char_str(), O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0644);
    //SaveWaveforms(m_WFfd);

    //Close log file
    logfile<< Timestamp();
    logfile<<"    Run finished\n";
    logfile.close();
    /* delete DRS object -> close USB connection */
    delete drs;
    return 0;
}


int Scan()
{
    drs = new DRS();
    /* do initial scan, exit if no board found */
    int nBoards = drs->GetNumberOfBoards();
    if (nBoards == 0) {
        printf("No DRS4 evaluation board found\n");
        logfile<<"\tNo DRS4 evaluation board found\n";
        logfile<< Timestamp();
        logfile<<"    Run finished\n";
        logfile.close();
        return 0;
    }
    /* show any found board(s) */
    for (int i=0 ; i<drs->GetNumberOfBoards() ; i++) {
        b = drs->GetBoard(i);
        printf("Found DRS4 evaluation board, serial #%d, firmware revision %d\n", 
                b->GetBoardSerialNumber(), b->GetFirmwareVersion());
    }

    return 1;
}

int Init(int boardNo)
{
    /* continue working with first board only */
    b = drs->GetBoard(boardNo);
    /* initialize board */
    b->Init();
    /* enable transparent mode needed for analog trigger */
    b->SetTranspMode(1);
    /* set sampling frequency, default: 1GHz*/
    b->SetFrequency(freq, true);
    /* set input range to -0.5V ... +0.5V - default*/
    b->SetInputRange(midpoint);
    /* use following line to set range to 0..1V */
    //b->SetInputRange(0.5);
    return 1;
}

void ReadWaveforms()
{
    m_samplingSpeed=freq;
    //unsigned char *pdata;
    //unsigned short *ptc;
    int size = 0;

    m_skipDisplay = false;
    m_armed = false;
    m_evSerial++;
    if (m_drs->GetNumberOfBoards() == 0) {
        for (int w=0 ; w<4 ; w++)
            for (int i=0 ; i<GetWaveformDepth(w) ; i++) 
            {
                m_waveform[w][i] = sin(i/m_samplingSpeed/10*M_PI+w*M_PI/4)*100;
                m_waveform[w][i] += ((double)rand()/RAND_MAX-0.5)*5;
                m_time[i] = 1/m_samplingSpeed*i;
            }
        m_waveDepth = kNumberOfBins;
        GetTimeStamp(m_evTimestamp);
    } 
    else 
    {
        int ofs = m_chnOffset;
        int chip = m_chip;

        if (m_drs->GetBoard(m_board)->GetBoardType() == 5 || m_drs->GetBoard(m_board)->GetBoardType() == 7 || m_drs->GetBoard(m_board)->GetBoardType() == 8) 
        {
            // get waveforms directly from device
            m_drs->GetBoard(m_board)->TransferWaves(m_wavebuffer, 0, 8);
            m_triggerCell = m_drs->GetBoard(m_board)->GetStopCell(chip);
            m_writeSR = m_drs->GetBoard(m_board)->GetStopWSR(chip);
            GetTimeStamp(m_evTimestamp);

            m_waveDepth = m_drs->GetBoard(m_board)->GetChannelDepth();
            m_drs->GetBoard(m_board)->GetTime(0, m_triggerCell, m_time, m_tcalon, m_rotated);
            if (m_clkOn && GetWaveformDepth(0) > kNumberOfBins) 
            {
                for (int i=0 ; i<kNumberOfBins ; i++)
                    m_timeClk[i] = m_time[i] + GetWaveformLength()/2;
            } 
            else 
            {
                for (int i=0 ; i<kNumberOfBins ; i++)
                    m_timeClk[i] = m_time[i];
            }

            if (m_drs->GetBoard(m_board)->GetChannelCascading() == 2) 
            {
                m_drs->GetBoard(m_board)->GetWave(m_wavebuffer, 0, 0, m_waveform[0], m_calibrated, m_triggerCell, m_writeSR, !m_rotated, 0, m_calibrated2);
                m_drs->GetBoard(m_board)->GetWave(m_wavebuffer, 0, 1, m_waveform[1], m_calibrated, m_triggerCell, m_writeSR, !m_rotated, 0, m_calibrated2);
                m_drs->GetBoard(m_board)->GetWave(m_wavebuffer, 0, 2, m_waveform[2], m_calibrated, m_triggerCell, m_writeSR, !m_rotated, 0, m_calibrated2);
                if (m_clkOn)
                    m_drs->GetBoard(m_board)->GetWave(m_wavebuffer, 0, 8, m_waveform[3], m_calibrated, m_triggerCell, 0, !m_rotated);
                else
                    m_drs->GetBoard(m_board)->GetWave(m_wavebuffer, 0, 3, m_waveform[3], m_calibrated, m_triggerCell, m_writeSR, !m_rotated, 0, m_calibrated2);
                if (m_spikeRemoval)
                    RemoveSpikes(true);
            } 
            else 
            {
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
        } 
    }

    /* extrapolate the first two samples (are noisy) */
    for (int i=0 ; i<4 ; i++) {
        m_waveform[i][1] = 2*m_waveform[i][2] - m_waveform[i][3];
        m_waveform[i][0] = 2*m_waveform[i][1] - m_waveform[i][2];
    }

/* auto-restart in running mode */
if (m_thread == NULL && m_running)
    Start();
} 

unsigned char buffer[100000];
int SaveWaveforms(int fd)
{
    char str[80];
    unsigned char *p;
    unsigned short d;
    float t;

    if (fd == 0)
        return 0;

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

    for (int i=0 ; i<4 ; i++) 
    {
        if (m_chnOn[i]) 
        {
            sprintf((char *)p, "C%03d", i+1);
            p += 4;
            for (int j=0 ; j<m_waveDepth ; j++)
            {
                // save binary date as 16-bit value: 0 = -0.5V, 65535 = +0.5V
                if (m_waveDepth == 2048) 
                {
                    // in cascaded mode, save 1024 values as averages of the 2048 values
                    d = (unsigned short)(((m_waveform[i][j]+m_waveform[i][j+1])/2000.0 + 0.5) * 65535);
                    *(unsigned short *)p = d;
                    p += sizeof(unsigned short);
                    j++;
                } 
                else 
                {
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

    return 1;
}

void SetTrigger(int chn,double level, double delay,bool edge)
{
    if(chn>0)
    {
        level = triglevel;
        edge  = trigedge;
        delay = trigdelay;
        b->EnableTrigger(1, 0);           // enable hardware trigger
        b->SetTriggerSource(1<<(chn-1));        
        b->SetTriggerLevel(level, edge);  // 0.1 V, negative trigedge, the number has no effect 
        b->SetTriggerDelayNs(delay);
    }
}

int Info()
{
    Init();
    printf("==============================\n");
    printf("Mezz. Board index:    %d\n", 0);
#ifdef HAVE_VME
    if (b->GetTransport() == TR_VME) {
        printf("Slot:                 %d", (b->GetSlotNumber() >> 1)+2);
        if ((b->GetSlotNumber() & 1) == 0)
            printf(" upper\n");
        else
            printf(" lower\n");
    }
#endif
    printf("DRS type:             DRS%d\n", b->GetDRSType());
    printf("Board type:           %d\n", b->GetBoardType());
    printf("Serial number:        %04d\n", b->GetBoardSerialNumber());
    printf("Firmware revision:    %d\n", b->GetFirmwareVersion());
    printf("Temperature:          %1.1lf C\n", b->GetTemperature());
    if (b->GetDRSType() == 4) {
        printf("Input range:          %1.2lgV...%1.2lgV\n", 
                b->GetInputRange()-0.5, b->GetInputRange()+0.5);
        printf("Calibrated range:     %1.2lgV...%1.2lgV\n", b->GetCalibratedInputRange()-0.5,
                b->GetCalibratedInputRange()+0.5);
        //printf("Calibrated frequency: %1.3lf GHz\n", b->GetCalibratedFrequency());

        if (b->GetTransport() == TR_VME) {
            printf("Multi Buffer WP:      %d\n", b->GetMultiBufferWP());
            printf("Multi Buffer RP:      %d\n", b->GetMultiBufferRP());
        }
    }
    int i;
    double freq;

    printf("Status reg.:          %08X\n", b->GetStatusReg());
    if (b->GetStatusReg() & BIT_RUNNING)
        puts("  Domino wave running");
    if (b->GetDRSType() == 4) {
        if (b->GetBoardType() == 5) {
            if (b->GetStatusReg() & BIT_PLL_LOCKED0)
                puts("  PLL locked");
        } else if (b->GetBoardType() == 6) {
            i = 0;
            if (b->GetStatusReg() & BIT_PLL_LOCKED0) i++;
            if (b->GetStatusReg() & BIT_PLL_LOCKED1) i++;
            if (b->GetStatusReg() & BIT_PLL_LOCKED2) i++;
            if (b->GetStatusReg() & BIT_PLL_LOCKED3) i++;
            if (i == 4)
                puts("  All PLLs locked");
            else if (i == 0)
                puts("  No PLL locked");
            else
                printf("  %d PLLs locked\n", i);
            if (b->GetStatusReg() & BIT_LMK_LOCKED)
                puts("  LMK PLL locked");
        }
    } else {
        if (b->GetStatusReg() & BIT_NEW_FREQ1)
            puts("  New Freq1 ready");
        if (b->GetStatusReg() & BIT_NEW_FREQ2)
            puts("  New Freq2 ready");
    }

    printf("Control reg.:         %08X\n", b->GetCtrlReg());
    if (b->GetCtrlReg() & BIT_MULTI_BUFFER)
        puts("  Multi-buffering enabled");
    if (b->GetDRSType() == 4) {
        if (b->GetConfigReg() & BIT_CONFIG_DMODE)
            puts("  DMODE circular");
        else
            puts("  DMODE single shot");
    } else {
        if (b->GetCtrlReg() & BIT_DMODE)
            puts("  DMODE circular");
        else
            puts("  DMODE single shot");
    }
    if (b->GetCtrlReg() & BIT_LED)
        puts("  LED");
    if (b->GetCtrlReg() & BIT_TCAL_EN)
        puts("  TCAL enabled");
    if (b->GetDRSType() == 4) {
        if (b->GetCtrlReg() & BIT_TRANSP_MODE)
            puts("  TRANSP_MODE enabled");
    } else {
        if (b->GetCtrlReg() & BIT_FREQ_AUTO_ADJ)
            puts("  FREQ_AUTO_ADJ enabled");
    }
    if (b->GetCtrlReg() & BIT_ENABLE_TRIGGER1)
        puts("  Hardware trigger enabled");
    if (b->GetDRSType() == 4) {
        if (b->GetCtrlReg() & BIT_READOUT_MODE)
            puts("  Readout from stop");
        if (b->GetCtrlReg() & BIT_ENABLE_TRIGGER2)
            puts("  Internal trigger enabled");
    } else {
        if (b->GetCtrlReg() & BIT_LONG_START_PULSE)
            puts("  LONG_START_PULSE");
    }
    if (b->GetCtrlReg() & BIT_DELAYED_START)
        puts("  DELAYED_START");
    if (b->GetCtrlReg() & BIT_ACAL_EN)
        puts("  ACAL enabled");
    if (b->GetDRSType() < 4)
        if (b->GetCtrlReg() & BIT_TRIGGER_DELAYED)
            puts("  DELAYED_TRIGGER selected");
    if (b->GetBoardType() != 5)
        printf("Trigger bus:          %08X\n", b->GetTriggerBus());
    if (b->GetDRSType() == 4) {
        if (b->GetRefclk() == 1) {
            if (b->IsPLLLocked() && b->IsLMKLocked()) {
                b->ReadFrequency(0, &freq);
                printf("Frequency:            %1.3lf GHz\n", freq);
            } else {
                if (!b->IsPLLLocked())
                    printf("Frequency:            PLL not locked\n");
                else
                    printf("Frequency:            LMK chip not locked\n");
            }
        } else {
            if (b->IsPLLLocked()) {
                b->ReadFrequency(0, &freq);
                printf("Frequency:            %1.3lf GHz\n", freq);
            } else {
                printf("Frequency:            PLL not locked\n");
            }
        }
    } else {
        if (b->IsBusy()) {
            b->ReadFrequency(0, &freq);
            printf("Frequency0:           %1.4lf GHz\n", freq);
            b->ReadFrequency(1, &freq);
            printf("Frequency1:           %1.4lf GHz\n", freq);
        } else
            puts("Domino wave stopped");
    }
    logfile<< Timestamp();
    logfile<<"    Run finished\n";
    logfile.close();
    return 0;
}

int Calibrate()
{
    char line[80];
    float freq,range;

    Init();

    printf("\nEnter calibration frequency [GHz]: ");
    fgets(line, sizeof(line), stdin);
    freq = atof(line);
    printf("freq: %f\n",freq);
    b->SetFrequency(freq, true);
    logfile<<"\tVoltage calibration at "<<freq<< " GHz\n";

    printf("Enter midpoint of voltage range [V]: ");
    fgets(line, sizeof(line), stdin);
    range = atof(line);
    b->SetInputRange(range);
    logfile<<"\tRange: "<<midpoint-0.5<<" V - "<<midpoint+0.5<<"V\n";

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

    ProgressBar p;

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

    logfile<< Timestamp();
    logfile<<"    Run finished\n";
    logfile.close();
    return 0;
}

int Measure()
{
    Init();
    SetTrigger(trigsource,triglevel,trigdelay,trigedge);
    TEvent *evt = new TEvent();

    /*
    //Open ROOT file
    if (filestr == "./output/test.root") 
    rootfile = new TFile (filename,"recreate");
    else
    {
    rootfile = new TFile (filename,"create");
    if (rootfile->IsZombie())
    {
    printf("Run aborted!\n");
    logfile<<"\tRun aborted!\n";
    return 0;
    }
    }

    t = new TTree ("t","Waveform tree");
    t->Branch("evt",&evt,6400,0);
    */

    uint32_t  j;
    for (j=0 ; j<NumEvents ; j++) 
    {
        // start board (activate domino wave)
        b->StartDomino();
        if (trigsource>0)
        {
            //printf("Waiting for trigger...");
            //fflush(stdout);
        }
        else 
            b->SoftTrigger(); //if use soft trigger

        // wait for trigger
        while (b->IsBusy());

        //read all waveforms
        b->TransferWaves(0, 8);

        //assign event number
        evt = new TEvent(j);

        // read time (X) array in ns
        b->GetTime(0, b->GetTriggerCell(0), time_array);

        //Note: On the evaluation board input #1 is connected to channel 0 and 1 of
        //The DRS chip, input #2 is connected to channel 2 and 3 and so on. So to
        //Get the input #2 we have to read DRS channel #2, not #1 */

        // decode waveform (Y) array first channel in mV
        b->GetWave(0, 0, wave_array[0]);

        // decode waveform (Y) array second channel in mV
        //b->GetWave(0, 2, wave_array[1]);


        //copy from b to evt
        evt->WriteWave(0,wave_array[0]);
        evt->WriteWave(1,wave_array[1]);

        t->Fill();

        if(j%1000 == 0) 
        {
            t->AutoSave("SaveSelf"); //autosave after 1000 evt 
            printf("\rEvent #%d read successfully\n", j+1);
        }
    }

    printf("\rEvent #%d read successfully\n", j);

    t->Write();
    rootfile->Close();
    return 1;
}
