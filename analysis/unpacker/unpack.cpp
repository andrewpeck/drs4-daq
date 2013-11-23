//System Headers
#include <string.h>
#include <TH2.h>
#include <stdio.h>
#include <TString.h>
#include <iostream>
#include <ctime>

// ROOT Headers
#include <TROOT.h>
#include <TTree.h>
#include <TBranch.h>
#include <TFile.h>
#include <TRandom3.h>
#include <TH1.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TProfile.h>
#include <TChain.h>

using namespace std; 

double fraction=0.5;  //fractional threshold
double Vthresh=-100;  //fixed threshold

void DecodeWaveform(unsigned short WaveIn[], Double_t WaveOut[], double &Vpeak, int &PeakBin);
void InterpolateWaveform(Double_t ArrA[], Double_t ArrB[]);
void IntegrateCharge(Double_t chn[], int PeakBin, double& Qint);
void PulseTime(Double_t chn[], Double_t t[], double& CTHtime, double& FTHtime, double& CTHWidth, double& FTHWidth, double Vpeak, int PeakBin);

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
    char           chn0_header[4];
    unsigned short chn0[1024];
    char           chn1_header[4];
    unsigned short chn1[1024];
    //   char           chn3_header[4]; //uncomment for channel 2
    //   unsigned short chn3[1024]; //uncomment for channel 2 
    //   char           chn4_header[4]; //uncomment for channel 3 
    //   unsigned short chn4[1024]; //uncomment for channel 3 
};

void unpack(char *filename) 
{
    Header_t header;
    Waveform_t waveform;
    Double_t t[1024];
    Double_t chn0[1024];
    Double_t chn1[1024];
    //Double_t chn2[1024]; //uncomment for channel 2 
    //Double_t chn3[1024]; //uncomment for channel 3
    Int_t n;
    unsigned int eventNum; 

    //change nchn for the number of channels
    int nchn=2;
    double Qint[nchn];
    double Vpeak[nchn];

    double CTHtime[nchn];
    double CTHWidth[nchn];
    double CTHFlightTime;

    double FTHtime[nchn];
    double FTHWidth[nchn];
    double FTHFlightTime;

    int PeakBin[nchn];
    std::time_t EventDate;

    // open the binary waveform file
    FILE *f = fopen(Form("%s.dat", filename), "r");

    //open the root file
    TFile *outfile = new TFile(Form("%s.root", filename), "RECREATE");

    // plant tree
    TTree *rec = new TTree("rec","rec");

    //event branches
    rec->Branch("t", &t   ,"t[1024]/D");  
    rec->Branch("eventNum", &eventNum ,"eventNum/I");
    rec->Branch("EventDate", &EventDate ,"EventDate");

    //channel 0 branches
    rec->Branch("chn0", &chn0, "chn0[1024]/D");
    rec->Branch("Qint0", &Qint[0] ,"Qint0/D");
    rec->Branch("VPeak0", &Vpeak[0],	"VPeak0/D");
    rec->Branch("PeakBin0", &PeakBin[0],	"PeakBin0/I");
    rec->Branch("CTHtime0", &CTHtime[0],	"CTHtime0/D");
    rec->Branch("FTHtime0", &FTHtime[0],	"FTHtime0/D");
    rec->Branch("CTHWidth0", &CTHWidth[0],	"CTHWidth0/D");
    rec->Branch("FTHWidth0", &FTHWidth[0],	"FTHWidth0/D");

    //channel 1 branches
    rec->Branch("chn1", &chn1,"chn1[1024]/D");
    rec->Branch("Qint1", &Qint[1] ,"Qint1/D");
    rec->Branch("VPeak1", &Vpeak[1],	"VPeak1/D");
    rec->Branch("PeakBin1", &PeakBin[1],	"PeakBin1/I");
    rec->Branch("CTHtime1", &CTHtime[1],	"CTHtime1/D");
    rec->Branch("FTHtime1", &FTHtime[1],	"FTHtime1/D");
    rec->Branch("CTHWidth1", &CTHWidth[1],	"CTHWidth1/D");
    rec->Branch("FTHWidth1", &FTHWidth[1],	"FTHWidth1/D");


    //flight time branches
    rec->Branch("CTHFlightTime", &CTHFlightTime,	"CTHFlightTime/D");
    rec->Branch("FTHFlightTime", &FTHFlightTime,	"FTHFlightTime/D");

    // loop over all events in data file
    for (n=0 ; fread(&header, sizeof(header), 1, f) > 0; n++) 
    {
        //date = header.year+header.month+header.day+header.hour+header.minute+header.second+header.millisecond;
        eventNum=header.serial_number;

        // decode time      
        for (Int_t i=0; i<1024; i++)
            t[i] = (Double_t) header.time[i];

        //fill time struct with date
        std::tm tm; 
        tm.tm_year = header.year-1900;
        tm.tm_mon  = header.month-1;
        tm.tm_mday = header.day;
        tm.tm_hour = header.hour;
        tm.tm_min = header.minute;
        tm.tm_sec = (header.second + header.millisecond * 1000);
        EventDate = std::mktime(&tm);

        if (n%1000==0)
            cout << "Processing event " << n << "\n";

        //write zeroes 
        memset(Qint, 0, sizeof(Qint));
        memset(Vpeak, 0, sizeof(Vpeak));
        memset(PeakBin, 0, sizeof(PeakBin));

        // Read in Waveform Data
        fread(&waveform, sizeof(waveform), 1, f);
        
        // decode amplitudes in mV and find Peak Voltage
        DecodeWaveform(waveform.chn0, chn0, Vpeak[0], PeakBin[0]);
        DecodeWaveform(waveform.chn1, chn1, Vpeak[1], PeakBin[1]);

        //if Vthresh undefined, define as avg of first two pulses
        if (Vthresh==0)
            Vthresh=0.5*(Vpeak[0]+Vpeak[1])/2.0;

        //Measure PulseTime (Fractional and Constant Threshold Time)
        PulseTime(chn0, t, CTHtime[0], FTHtime[0], CTHWidth[0], FTHWidth[0], Vpeak[0], PeakBin[0]);
        PulseTime(chn1, t, CTHtime[1], FTHtime[1], CTHWidth[1], FTHWidth[1], Vpeak[1], PeakBin[1]);

        //Integrate Charge, Find Pedestal
        IntegrateCharge(chn0, PeakBin[0], Qint[0]);
        IntegrateCharge(chn1, PeakBin[1], Qint[1]);

        //Calculate deltaT
        CTHFlightTime=(CTHtime[1]-CTHtime[0]);
        FTHFlightTime=(FTHtime[1]-FTHtime[0]);

        //Fill Histograms
        rec->Fill();
    }

    // print number of events

    cout<<n<<" events processed"<<endl;
    cout<<"\""<<Form("%s.root", filename)<<"\" written"<<endl;

    //rec->Draw("chn0:t");
    //rec->Draw("FTHFlightTime");
    rec->Draw("FTHFlightTime","(FTHFlightTime < 10 && FTHFlightTime > -10 && FTHtime0>100 && FTHtime1 > 100)");
    //rec->Draw("FTHFlightTime");
    //rec->Draw("CTHFlightTime","(CTHFlightTime < 10 && CTHFlightTime > -10 && VPeak0<-100)");

    //Write Tree
    rec->Write();
    
    //Close ROOTfile
    outfile->Close();
}


void DecodeWaveform(unsigned short WaveIn[], Double_t WaveOut[], double &Vpeak, int &PeakBin)
{
    for (Int_t i=0; i<1024; i++) 
    {
        // decode amplitudes in mV and find Peak Voltage
        // Values from 0-65535 are 16-Bit DAC counts, measured from 0-1V
        WaveOut[i] = (Double_t) ((WaveIn[i]) / 65535. - 0.5) * 1000;   

        //first and last time samples are noisy---discard them
        if ( (i < 10) || (i>1000) )
            WaveOut[i]=0;

        //Simple Peak Finding
        if (WaveOut[i] < Vpeak) 
        {
            Vpeak=WaveOut[i]; 
            PeakBin=i;
        }

    }
}

void InterpolateWaveform(Double_t ArrA[], Double_t ArrB[])
{
    for (int i=0; i<2048; i++)
    {
        if (i%2==0)
            ArrB[i]=ArrA[i/2];
        else 
            ArrB[i]=(ArrA[(i-1)/2]+ArrA[(i+1)/2])/2.0;
    }
}

void IntegrateCharge(Double_t chn[], int PeakBin, double& Qint)
{
    int n=0;
    int m=0;
    double Qped=0;

    //Integrate Charge and find Qpedestal
    for (Int_t i=10; i<1014; i++)
    {
        if  ( i < PeakBin-15 )
        {
            n++;
            Qped+=chn[i];
        }
        else if ( i < PeakBin+15) 
        {
            Qint+=chn[i];
            m++;
        }
    }
    Qped=Qped/n;
    Qint=(Qint-Qped*m);
}

void PulseTime(Double_t chn[], Double_t t[], double& CTHtime, double& FTHtime, double& CTHWidth, double& FTHWidth, double Vpeak, int PeakBin)
{
    bool fth_rise=false;
    bool fth_fall=false;
    bool cth_rise=false;
    bool cth_fall=false;
    double chnInterp[2048];

    InterpolateWaveform(chn,chnInterp);

    CTHtime=FTHtime=CTHWidth=FTHWidth=0;

    for (Int_t i=10; i<2048; i++) 
    {
        if ((i>2*(PeakBin-50)) && (i<2*(PeakBin+50)))
        {
            //Find Constant Threshold Cross Time & Pulse Width
            if ( (cth_rise==false) && (chnInterp[i] < Vthresh) )
            {
                CTHtime=t[i/2]+t[i%2]/2;
                cth_rise=true;
            }

            if ( (cth_rise==true) && (cth_fall==false) && (chnInterp[i] > (Vthresh) ))
            {
                CTHWidth=t[i/2]+t[i%2]/2-CTHtime;
                cth_fall=true;
            }
            
            //Find Fractional Threshold Cross Time & Pulse Width
            if ( (fth_rise==false) && (chnInterp[i] < (Vpeak*fraction) ))
            {
                FTHtime=t[i/2]+t[i%2]/2; 
                fth_rise=true;
            }

            if ( (fth_rise==true) && (fth_fall==false) && (chnInterp[i] > (Vpeak*fraction) ))
            {
                FTHWidth=t[i/2]+t[i%2]/2-FTHtime;
                fth_fall=true;
            }
        }
    }
}
