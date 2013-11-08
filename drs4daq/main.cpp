/********************************************************************\
Name:         test.cpp
Created by:   Nam, adapted from Stefan Ritt's drs_exam.cpp
Contents:     Simple example application to read out a DRS4
evaluation board
\********************************************************************/
#include "main.h"
#include "progressbar.h"

DRS *drs;
DRSBoard *b;
float time_array[1024];
float wave_array[8][1024];
FILE  *f;
TEvent *evt;
TFile *rootfile;
TTree *t;
uint32_t evtNo;
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
	if (!Scan()) return 0;
	//Default sampling rate is 1GHz, input range: -0.5 to 0.5 V
	freq 				= 1.0;
	midpoint		= 0.0;
	trigsource 	= 1;
	triglevel	 	= -0.1;
	trigdelay	 	= 0; //0 ns
	trigedge 		= false; // rising edge
	evtNo				= 1000;
	dirstr = "./output/";
	filestr = "test.root";

	int c;
	//c:calibrate, i: info, h: help
	//f: freq, v: range (voltage), o: output file name
	//d: trigdelay, r: run #, s: trigger source, l: trig. level
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
				evtNo = atoi(optarg);
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
	Measure();
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

	uint32_t  j;
	for (j=0 ; j<evtNo ; j++) 
	{
		/* start board (activate domino wave) */
		b->StartDomino();
		if (trigsource>0)
		{
			/* wait for trigger */
			//printf("Waiting for trigger...");
			//fflush(stdout);
		}
		else b->SoftTrigger(); //if use soft trigger
		while (b->IsBusy());
		/* read all waveforms */
		b->TransferWaves(0, 8);
		evt = new TEvent(j);
		/* read time (X) array in ns */
		//b->GetTime(0, b->GetTriggerCell(0), time_array);
		/* decode waveform (Y) array first channel in mV */
		b->GetWave(0, 0, wave_array[0]);
		evt->WriteWave(0,wave_array[0]);
		/* decode waveform (Y) array second channel in mV
Note: On the evaluation board input #1 is connected to channel 0 and 1 of
the DRS chip, input #2 is connected to channel 2 and 3 and so on. So to
get the input #2 we have to read DRS channel #2, not #1 */
		b->GetWave(0, 2, wave_array[1]);
		evt->WriteWave(1,wave_array[1]);
		/* print some progress indication */
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
