#include <string.h>
#include <stdio.h>
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include <iostream>

// ROOT Headers
#include <TROOT.h>
#include <TTree.h>
#include <TFile.h>
#include <TRandom3.h>
#include <TH1.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TProfile.h>
#include <TChain.h>

struct Header_t {
	char           event_header[4];
	unsigned int   serial_number;
	unsigned short year;
	unsigned short month;
	unsigned short day;
	unsigned short hour;
	unsigned short minute;
	unsigned short second;
	unsigned short millisecond;
	unsigned short reserved1;
	float time[1024];
};

struct Waveform_t {
	char           chn1_header[4];
	unsigned short chn1[1024];
	//   char           chn2_header[4];
	//   unsigned short chn2[1024];
	//   char           chn3_header[4];
	//   unsigned short chn3[1024];
	//   char           chn4_header[4];
	//   unsigned short chn4[1024];
};


using namespace std; 

void decode(char *filename) {
	Header_t header;
	Waveform_t waveform;
	Double_t t[1024], chn1[1024]; //, chn2[1024], chn3[1024], chn4[1024];
	Int_t n;
	unsigned int eventNum; 
	unsigned short EventDate[7];
	double Qint;
	double Vpeak;
	double fraction=0.37; 
	double Vthresh=-10.0;
	int CTHtime;
	int FTHtime;
	int PeakTime;

	// open the binary waveform file
	FILE *f = fopen(Form("%s.dat", filename), "r");

	//open the root file
	TFile *outfile = new TFile(Form("%s.root", filename), "RECREATE");

	// define the rec tree
	TTree *rec = new TTree("rec","rec");
	rec->Branch("t", &t   ,"t[1024]/D");  
	rec->Branch("chn1", &chn1 ,"chn1[1024]/D");
	rec->Branch("eventNum", &eventNum ,"eventNum/I");
	rec->Branch("EventDate", &EventDate ,"EventDate");
	rec->Branch("IntegratedCharge", &Qint ,"IntegratedCharge/D");
	rec->Branch("PeakVoltage", &Vpeak,	"PeakVoltage/D");
	rec->Branch("CTHtime", &CTHtime,	"CTHtime/I");
	rec->Branch("FTHtime", &FTHtime,	"FTHtime/I");

	//   rec->Branch("chn2", &chn2 ,"chn2[1024]/D");
	//   rec->Branch("chn3", &chn3 ,"chn3[1024]/D");
	//   rec->Branch("chn4", &chn4 ,"chn4[1024]/D");

	// loop over all events in data file
	for (n=0 ; fread(&header, sizeof(header), 1, f) > 0; n++) 
	{
		//date = header.year+header.month+header.day+header.hour+header.minute+header.second+header.millisecond;
		eventNum=header.serial_number;

		// decode time      
		for (Int_t i=0; i<1024; i++)
			t[i] = (Double_t) header.time[i];

		//for (Int t i=0; i<16; i++)
		EventDate[0] = header.year;
		EventDate[1] = header.month;
		EventDate[2] = header.day;
		EventDate[3] = header.hour;
		EventDate[4] = header.minute;
		EventDate[5] = header.second;
		EventDate[6] = header.millisecond;

		if (n%1000==0)
			cout << "Processing event " << n << "\n";

		fread(&waveform, sizeof(waveform), 1, f);

		Qint=0;
		Vpeak=0;

		// decode amplitudes in mV
		for (Int_t i=0; i<1024; i++) 
		{
			chn1[i] = (Double_t) ((waveform.chn1[i]) / 65535. - 0.5) * 1000;   
			if (chn1[i] < Vpeak)
			{
				Vpeak=chn1[i];
				PeakTime=i;
			}

			//chn2[i] = (Double_t) ((waveform.chn2[i]) / 65535. - 0.5) * 1000;   
			//chn3[i] = (Double_t) ((waveform.chn3[i]) / 65535. - 0.5) * 1000;   
			//chn4[i] = (Double_t) ((waveform.chn4[i]) / 65535. - 0.5) * 1000;   
		}

		bool FTHCrossed=false;
		bool CTHCrossed=false;

		for (Int_t i=(PeakTime-50); i<PeakTime+50; i++) 
		{
			Qint+=chn1[i];
			if (CTHCrossed==false)
			{
				if (chn1[i] < Vthresh) 
				{
					CTHtime=i;
					CTHCrossed=true;
				}
			}

			if (FTHCrossed==false)
			{
				if (chn1[i] < (Vpeak*fraction))
				{
					FTHtime=i;
					FTHCrossed=true;
				}
			}
		}
		rec->Fill();
	}

	// draw channel #4
	rec->Draw("chn1:t");

	// print number of events
	cout<<n<<" events processed"<<endl;
	cout<<"\""<<Form("%s.root", filename)<<"\" written"<<endl;

	// save and close root file
	rec->Write();
	outfile->Close();
}
