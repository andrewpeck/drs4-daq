//System Headers
#include <string.h>
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
#include <TH2.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TProfile.h> 
#include <TChain.h>

using namespace std; 

double fraction=0.5;  //fractional threshold
double Vthresh=-50;  //fixed threshold

void DecodeWaveform(unsigned short WaveIn[], Double_t WaveOut[], double &Vpeak, int &PeakBin);
void InterpolateWaveform(Double_t ArrA[], Double_t ArrB[]);
void IntegrateCharge(Double_t chn[], int PeakBin, double& Qint);
void PulseTime(Double_t chn[], Double_t t[], double& CTHtime, double& FTHtime, double& CTHWidth, double& FTHWidth, double Vpeak, int PeakBin);
void MakePlots(char* filename);

#define NCHN 2

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
#if NCHN>0
    char           chn0_header[4];
    unsigned short chn0[1024];
#endif
#if NCHN>1
    char           chn1_header[4];
    unsigned short chn1[1024];
#endif
#if NCHN>2
    char           chn2_header[4];
    unsigned short chn2[1024];
#endif
#if NCHN>3
    char           chn3_header[4]; 
    unsigned short chn3[1024]; 
#endif
};

int unpack(char *filename) {

    gROOT->SetStyle("Plain");
    gStyle->SetTitleBorderSize(0);
    gStyle->SetPalette(1);


    Header_t header;
    Waveform_t waveform;


    Double_t t[1024];
#if NCHN>0
    Double_t chn0[1024];
#endif

#if NCHN>1
    Double_t chn1[1024];
#endif

#if NCHN>2
    Double_t chn2[1024];
#endif

#if NCHN>3
    Double_t chn3[1024];
#endif

    unsigned int eventNum; 

    double Qint[NCHN];
    double Vpeak[NCHN];

    double CTHtime[NCHN];
    double CTHWidth[NCHN];
    double CTHFlightTime;

    double FTHtime[NCHN];
    double FTHWidth[NCHN];
    double FTHFlightTime;

    int PeakBin[NCHN];
    std::time_t EventDate;

    // open the binary waveform file
    FILE *f; 
    if ( (f=fopen(Form("%s.dat",filename),"r")) == NULL) {
        cout << "File not found\n";
        return 1; 
    }
    else {
        cout << "File opened\n";
    }

    TFile *outfile = new TFile(Form("%s.root", filename), "RECREATE");

    // plant tree
    TTree *rec = new TTree("rec","rec");

    //event branches
    rec->Branch("t", &t   ,"t[1024]/D");  
    rec->Branch("eventNum", &eventNum ,"eventNum/I");
    rec->Branch("EventDate", &EventDate ,"EventDate");

    //channel 0 branches
#if NCHN > 0
    rec->Branch("chn0", &chn0, "chn0[1024]/D");
    rec->Branch("Qint0", &Qint[0] ,"Qint0/D");
    rec->Branch("Vpeak0", &Vpeak[0],	"Vpeak0/D");
    rec->Branch("PeakBin0", &PeakBin[0],	"PeakBin0/I");
    rec->Branch("CTHtime0", &CTHtime[0],	"CTHtime0/D");
    rec->Branch("FTHtime0", &FTHtime[0],	"FTHtime0/D");
    rec->Branch("CTHWidth0", &CTHWidth[0],	"CTHWidth0/D");
    rec->Branch("FTHWidth0", &FTHWidth[0],	"FTHWidth0/D");
#endif

    //channel 1 branches
#if NCHN > 1
    rec->Branch("chn1", &chn1,"chn1[1024]/D");
    rec->Branch("Qint1", &Qint[1] ,"Qint1/D");
    rec->Branch("Vpeak1", &Vpeak[1],	"Vpeak1/D");
    rec->Branch("PeakBin1", &PeakBin[1],	"PeakBin1/I");
    rec->Branch("CTHtime1", &CTHtime[1],	"CTHtime1/D");
    rec->Branch("FTHtime1", &FTHtime[1],	"FTHtime1/D");
    rec->Branch("CTHWidth1", &CTHWidth[1],	"CTHWidth1/D");
    rec->Branch("FTHWidth1", &FTHWidth[1],	"FTHWidth1/D");
#endif

    //channel 2 branches
#if NCHN > 2
    rec->Branch("chn2", &chn2,              "chn2[1024]/D");
    rec->Branch("Qint2", &Qint[2],          "Qint2/D");
    rec->Branch("Vpeak2", &Vpeak[2],	    "Vpeak2/D");
    rec->Branch("PeakBin2", &PeakBin[2],	"PeakBin2/I");
    rec->Branch("CTHtime2", &CTHtime[2],	"CTHtime2/D");
    rec->Branch("FTHtime2", &FTHtime[2],	"FTHtime2/D");
    rec->Branch("CTHWidth2", &CTHWidth[2],	"CTHWidth2/D");
    rec->Branch("FTHWidth2", &FTHWidth[2],	"FTHWidth2/D");
#endif

    //channel 3 branches
#if NCHN > 3
    rec->Branch("chn3", &chn3,              "chn3[1024]/D");
    rec->Branch("Qint3", &Qint[3],          "Qint3/D");
    rec->Branch("Vpeak3", &Vpeak[3],	    "Vpeak3/D");
    rec->Branch("PeakBin3", &PeakBin[3],	"PeakBin3/I");
    rec->Branch("CTHtime3", &CTHtime[3],	"CTHtime3/D");
    rec->Branch("FTHtime3", &FTHtime[3],	"FTHtime3/D");
    rec->Branch("CTHWidth3", &CTHWidth[3],	"CTHWidth3/D");
    rec->Branch("FTHWidth3", &FTHWidth[3],	"FTHWidth3/D");
#endif


    //flight time branches
    rec->Branch("CTHFlightTime", &CTHFlightTime,	"CTHFlightTime/D");
    rec->Branch("FTHFlightTime", &FTHFlightTime,	"FTHFlightTime/D");

    // loop over all events in data file
    Int_t n; 
    for (n=0; fread(&header, sizeof(header), 1, f) > 0; n++) {

        //extract event number
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

        //some progress notification
        if (n%1000==0)
            cout << "Processing event " << n << "\n";

        //write zeroes 
        memset(Qint, 0, sizeof(Qint));
        memset(Vpeak, 0, sizeof(Vpeak));
        memset(PeakBin, 0, sizeof(PeakBin));

        // Read in Waveform Data
        fread(&waveform, sizeof(waveform), 1, f);

        // decode amplitudes in mV and find Peak Voltage
#if NCHN > 0
        DecodeWaveform(waveform.chn0, chn0, Vpeak[0], PeakBin[0]);
#endif

#if NCHN > 1
        DecodeWaveform(waveform.chn1, chn1, Vpeak[1], PeakBin[1]);
#endif

#if NCHN > 2
        DecodeWaveform(waveform.chn2, chn2, Vpeak[2], PeakBin[2]);
#endif

#if NCHN > 3
        DecodeWaveform(waveform.chn3, chn3, Vpeak[3], PeakBin[3]);
#endif

        //if Vthresh undefined, define as 1/2 avg of first two pulses
        if (Vthresh==0)
            Vthresh=0.25*(Vpeak[0]+Vpeak[1]);

        //Measure PulseTime (Fractional and Constant Threshold Time)
#if NCHN > 0
        PulseTime(chn0, t, CTHtime[0], FTHtime[0], CTHWidth[0], FTHWidth[0], Vpeak[0], PeakBin[0]);
#endif

#if NCHN > 1
        PulseTime(chn1, t, CTHtime[1], FTHtime[1], CTHWidth[1], FTHWidth[1], Vpeak[1], PeakBin[1]);
#endif

#if NCHN > 2
        PulseTime(chn2, t, CTHtime[2], FTHtime[2], CTHWidth[2], FTHWidth[2], Vpeak[2], PeakBin[2]);
#endif

#if NCHN > 3
        PulseTime(chn3, t, CTHtime[3], FTHtime[3], CTHWidth[3], FTHWidth[3], Vpeak[3], PeakBin[3]);
#endif

        //Integrate Charge, Find Pedestal
#if NCHN > 0
        IntegrateCharge(chn0, PeakBin[0], Qint[0]);
#endif 

#if NCHN > 1
        IntegrateCharge(chn1, PeakBin[1], Qint[1]);
#endif 

#if NCHN > 2
        IntegrateCharge(chn2, PeakBin[2], Qint[2]);
#endif 

#if NCHN > 3
        IntegrateCharge(chn3, PeakBin[3], Qint[3]);
#endif 


        //Calculate time-of-flight
        CTHFlightTime=(CTHtime[1]-CTHtime[0]);
        FTHFlightTime=(FTHtime[1]-FTHtime[0]);

        //Fill Histograms
        rec->Fill();
    }

    // print number of events

    cout<<n<<" events processed"<<endl;
    cout<<"\""<<Form("%s.root", filename)<<"\" written"<<endl;

    //Write Tree
    rec->Write();

    //Close ROOTfile
    outfile->Close();

    //make some extra plots
    MakePlots(filename);
    return 0;
}

void DecodeWaveform(unsigned short WaveIn[], Double_t WaveOut[], double &Vpeak, int &PeakBin) {
    for (Int_t i=0; i<1024; i++) 
    {
        // decode amplitudes in mV and find Peak Voltage
        // Values from 0-65535 are 16-Bit DAC counts, measured from 0-1V
        WaveOut[i] = (Double_t) ((WaveIn[i]) / 65535. - 0.5) * 1000;   

        //first and last time samples are noisy---discard them
        if ( (i < 10) || (i>1000) )
            WaveOut[i]=0;

        //Simple Peak Finding
        if (WaveOut[i] < Vpeak) {
            Vpeak=WaveOut[i]; 
            PeakBin=i;
        }
    }
}

void InterpolateWaveform(Double_t ArrA[], Double_t ArrB[]) {
    for (int i=0; i<2048; i++) {
        if (i%2==0)
            ArrB[i]=ArrA[i/2];
        else 
            ArrB[i]=(ArrA[(i-1)/2]+ArrA[(i+1)/2])/2.0;
    }
}

void IntegrateCharge(Double_t chn[], int PeakBin, double& Qint) {
    int n=0;
    int m=0;
    double Qped=0;

    //Integrate Charge and find Qpedestal
    for (Int_t i=10; i<1014; i++) {
        if  ( i < PeakBin-15 ) {
            n++;
            Qped+=chn[i];
        }
        else if ( i < PeakBin+15) {
            Qint+=chn[i];
            m++;
        }
    }
    Qped=Qped/n;
    Qint=(Qint-Qped*m);
}

void PulseTime(Double_t chn[], Double_t t[], double& CTHtime, double& FTHtime, double& CTHWidth, double& FTHWidth, double Vpeak, int PeakBin) {

    bool fth_rise, fth_fall, cth_rise, cth_fall;
    fth_rise=fth_fall=cth_rise=cth_fall=false;
    double chnInterp[2048];

    InterpolateWaveform(chn,chnInterp);

    CTHtime=FTHtime=CTHWidth=FTHWidth=0;

    for (Int_t i=20; i<2028; i++)  //discard first and last 10 time bins
    {
        if ((i>2*(PeakBin-30)) && (i<2*(PeakBin+30))) {

            //Find Constant Threshold Cross Time 
            if ( (cth_rise==false) && (chnInterp[i] < Vthresh) ) {
                CTHtime=t[i/2]+t[i%2]/2;
                cth_rise=true;
            }

            //CTH Pulse Width
            if ((cth_rise==true) && (cth_fall==false) \
                    && (chnInterp[i]>(Vthresh) )) {
                CTHWidth=t[i/2]+t[i%2]/2-CTHtime;
                cth_fall=true;
            }

            //Find Fractional Threshold Cross Time
            if ((fth_rise==false)&&(chnInterp[i]<(Vpeak*fraction))) {
                FTHtime=t[i/2]+t[i%2]/2; 
                fth_rise=true;
            }

            //FTH Pulse width
            if ((fth_rise==true) && (fth_fall==false) \
                    && (chnInterp[i]>(Vpeak*fraction))) {
                FTHWidth=t[i/2]+t[i%2]/2-FTHtime;
                fth_fall=true;
            }
        }
    }
}

void MakePlots(char* filename) {
    //open the root file
    TFile *rootfile = new TFile(Form("%s.root", filename),"UPDATE");
    TTree *rec = (TTree *)rootfile->Get("rec");


    //TH1F *h1 = new TH1F("h1","Fractional Threshold Flighttime (Vpeak<-10)",43,-2,2);
    //TH1F *h1 = new TH1F("h1","Fractional Threshold Flighttime (Vpeak<-10)",123,-6,6);
    TH1F *h1 = new TH1F("h1","Fractional Threshold Flighttime (Vpeak<-10)",101,-5,5);
    h1->GetXaxis()->SetTitle("time (ns)");
    rec->Draw("FTHFlightTime>>h1","(Vpeak0<-10 && Vpeak1<-10)");
    h1->Fit("gaus");

    TH1F *h2 = new TH1F("h2","Constant Threshold Flighttime (Vpeak<-10)",101,-5,5);
    h2->GetXaxis()->SetTitle("time (ns)");
    rec->Draw("CTHFlightTime>>h2","(Vpeak0<-10 && Vpeak1<-10)");

    TH1F *h3 = new TH1F("h3","Fractional Threshold Flighttime (Vpeak<-100)",101,-5,5);
    h3->GetXaxis()->SetTitle("time (ns)");
    rec->Draw("FTHFlightTime>>h3","(Vpeak0<-100 && Vpeak1<-100)");

    TF1 *sinefit = new TF1("sinefit","[0]*sin([1]*x+[2])", 0,200);
    sinefit->SetParameter(0, 80);
    sinefit->SetParameter(1, 0.15);
    sinefit->SetParameter(2, -8.2);

#if NCHN>0
    TH2F *ch0 = new TH2F("ch0","channel0 trace",225,0,225,520,-500,20);
    ch0->GetXaxis()->SetTitle("time (ns)");
    ch0->GetYaxis()->SetTitle("Voltage (mv)");
    rec->Draw("chn0:t>>ch0","(Vpeak0<-10)");
    //ch0->Fit("sinefit");
    ch0->Write();
#endif

#if NCHN>1
    TH2F *ch1 = new TH2F("ch1","channel1 trace",225,0,225,520,-500,20);
    ch1->GetXaxis()->SetTitle("time (ns)");
    ch1->GetYaxis()->SetTitle("Voltage (mv)");
    rec->Draw("chn1:t>>ch1","(Vpeak1<-10)");
    //ch1->Fit("sinefit");
    ch1->Write();
#endif

#if NCHN>2
    TH2F *ch2 = new TH2F("ch2","channel2 trace",225,0,225,620,-310,310);
    ch2->GetXaxis()->SetTitle("time (ns)");
    ch2->GetYaxis()->SetTitle("Voltage (mv)");
    rec->Draw("chn2:t>>ch2","(Vpeak2<-10)");
    ch2->Fit("sinefit");
    ch2->Write();
#endif

#if NCHN>3
    TH2F *ch3 = new TH2F("ch3","channel3 trace",225,0,225,620,-310,310);
    ch3->GetXaxis()->SetTitle("time (ns)");
    ch3->GetYaxis()->SetTitle("Voltage (mv)");
    rec->Draw("chn3:t>>ch3","(Vpeak3<-10)");
    ch3->Fit("sinefit");
    ch3->Write();
#endif

    TProfile *hprof = new TProfile("hprof", "Flight time dependence on peak voltage",100,0,400,0,100);
    hprof->GetXaxis()->SetTitle("Peak Voltage (-mV)");
    hprof->GetYaxis()->SetTitle("abs(Flight Time) (ns)");
    rec->Draw("abs(FTHFlightTime):abs(0.5*(Vpeak1+Vpeak0))>>hprof",
            "(Vpeak0<0 && Vpeak1<0)");


    h1->Write();
    h2->Write();
    h3->Write();
    hprof->Write();

    rootfile->Close();
}
