#include <string.h>
#include <stdio.h>
#include <TString.h>
#include <iostream>
#include <ctime>

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
    char           chn0_header[4];
    unsigned short chn0[1024];
    char           chn1_header[4];
    unsigned short chn1[1024];
    //   char           chn3_header[4];
    //   unsigned short chn3[1024];
    //   char           chn4_header[4];
    //   unsigned short chn4[1024];
};


using namespace std; 

double fraction=0.3; 
double Vthresh=-5.0;

void IntegrateCharge(Double_t chn[], Double_t t[], int PeakBin, double& Qint, double& Qped)
{
    int n=0;
    //Integrate Charge
    for (Int_t i=10; i<1024; i++)
    {
        if ( i < PeakBin-20)
        {
            n++;
            Qped+=chn[i];
        }

        //   if ( (i > PeakBin-10) && (i<PeakBin+10) )
        Qint+=chn[i];

    }
    //Qped=Qped/n;
    //Qint=Qint-Qped*20;
}

void PulseTime(Double_t chn[], Double_t t[], double& CTHtime, double& FTHtime, double Vpeak, int PeakBin)
{
    bool FTHCrossed=false;
    bool CTHCrossed=false;
    CTHtime=0;
    FTHtime=0;

    for (Int_t i=10; i<1000; i++) 
    {
        if ( (i > 0) && (i<1024) )
        {
            //Find Constant Threshold Cross Time
            if (CTHCrossed==false)
            {
                if (chn[i] < Vthresh) 
                {
                    CTHtime=t[i];
                    CTHCrossed=true;
                }
            }

            //Find Fractional Threshold Cross Time
            if (FTHCrossed==false)
            {
                if (chn[i] < (Vpeak*fraction))
                {
                    FTHtime=t[i];
                    FTHCrossed=true;
                }
            }
        }
    }
}

void unpack(char *filename) {
    Header_t header;
    Waveform_t waveform;
    Double_t t[1024];
    Double_t chn0[1024];
    Double_t chn1[1024];
    //Double_t chn3[1024];
    //Double_t chn4[1024];
    Int_t n;
    unsigned int eventNum; 

    double Qint[2];
    double Qped[2];
    double Vpeak[2];
    double CTHtime[2];
    double FTHtime[2];
    double CTHFlightTime;
    double FTHFlightTime;
    int PeakBin[2];
    std::time_t EventDate;

    // open the binary waveform file
    FILE *f = fopen(Form("%s.dat", filename), "r");

    //open the root file
    TFile *outfile = new TFile(Form("%s.root", filename), "RECREATE");

    // plant tree
    TTree *rec = new TTree("rec","rec");

    //grow branches
    rec->Branch("t", &t   ,"t[1024]/D");  
    rec->Branch("eventNum", &eventNum ,"eventNum/I");
    rec->Branch("EventDate", &EventDate ,"EventDate");

    rec->Branch("chn0", &chn0, "chn0[1024]/D");
    rec->Branch("IntegratedCharge0", &Qint[0] ,"IntegratedCharge0/D");
    rec->Branch("PeakVoltage0", &Vpeak[0],	"PeakVoltage0/D");
    rec->Branch("PeakBin0", &PeakBin[0],	"PeakBin0/I");
    rec->Branch("CTHtime0", &CTHtime[0],	"CTHtime0/D");
    rec->Branch("FTHtime0", &FTHtime[0],	"FTHtime0/D");

    rec->Branch("chn1", &chn1,"chn1[1024]/D");
    rec->Branch("IntegratedCharge1", &Qint[1] ,"IntegratedCharge1/D");
    rec->Branch("PeakVoltage1", &Vpeak[1],	"PeakVoltage1/D");
    rec->Branch("PeakBin1", &PeakBin[1],	"PeakBin1/I");
    rec->Branch("CTHtime1", &CTHtime[1],	"CTHtime1/D");
    rec->Branch("FTHtime1", &FTHtime[1],	"FTHtime1/D");

    rec->Branch("CTHFlightTime", &CTHFlightTime,	"CTHFlightTime/D > -10");
    rec->Branch("FTHFlightTime", &FTHFlightTime,	"FTHFlightTime/D");

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
        memset(Qped, 0, sizeof(Qped));
        memset(Vpeak, 0, sizeof(Vpeak));
        memset(PeakBin, 0, sizeof(PeakBin));

        // Read in Waveform Data
        fread(&waveform, sizeof(waveform), 1, f);

        // decode amplitudes in mV and find Peak Voltage
        for (Int_t i=0; i<1024; i++) 
        {
            chn0[i] = (Double_t) ((waveform.chn0[i]) / 65535. - 0.5) * 1000;   


            if (chn0[i] < Vpeak[0]) 
            {
                Vpeak[0]=chn0[i];
                PeakBin[0]=i;
            }

            chn1[i] = (Double_t) ((waveform.chn1[i]) / 65535. - 0.5) * 1000;   

            if (chn1[i] < Vpeak[1])
            {
                Vpeak[1]=chn1[i];
                PeakBin[1]=i;
            }
            
            if ( (i < 300) || (i>900) )
            {
                chn0[i]=0;
                chn1[i]=0;
            }
        }

        //Measure PulseTime (Fractional and Constant Threshold Time)
        PulseTime(chn0, t, CTHtime[0], FTHtime[0], Vpeak[0], PeakBin[0]);
        PulseTime(chn1, t, CTHtime[1], FTHtime[1], Vpeak[1], PeakBin[1]);

        //Integrate Charge, Find Pedestal
        IntegrateCharge(chn0, t, PeakBin[0], Qint[0], Qped[0]);
        IntegrateCharge(chn1, t, PeakBin[1], Qint[1], Qped[1]);

        CTHFlightTime=CTHtime[1]-CTHtime[0];
        FTHFlightTime=FTHtime[1]-FTHtime[0];

        //Fill Histograms
        rec->Fill();
    }

    // draw channel #4
    //rec->Draw("chn0:t>>hnew");
    //TH2F *C1trace = (TH2F*)gDirectory->Get("hnew");
    //rec->Branch("C1trace","TH2F",&C1trace,32000,0);

    
    //TH2F *htemp = (TH2F*)gPad->GetPrimitive("htemp");
    //htemp->GetEntries();


    //rec->Draw("CTHFlightTime>>hist_name","(CTHFlightTime<10 && CTHFlightTime>-10)", "goff");
    //hist_name.Draw();


    // print number of events
    cout<<n<<" events processed"<<endl;
    cout<<"\""<<Form("%s.root", filename)<<"\" written"<<endl;

    rec->Draw("FTHFlightTime","(FTHFlightTime<10 && FTHFlightTime>-10)");
    
    rec->Write();
    

    outfile->Close();

    TFile *fi = new TFile(Form("%s.root", filename), "RECREATE");
    TTree *T     = new TTree("T","test");
    TH2F *hpxpy  = new TH2F("hpxpy","py vs px",40,-4,4,40,-4,4);
    T->Branch("hpxpy","TH2F",&hpxpy,32000,0);
    Float_t px, py, pz;
    for (Int_t i = 0; i < 25000; i++) {
        if (i%1000 == 0) printf("at entry: %d\n",i);
        gRandom->Rannor(px,py);
        pz = px*px + py*py;
        hpxpy->Fill(px,py);
        T->Fill();
    }
    T->Print();
    fi->Write();

    //TFile fi("ht.root","recreate");
    //TTree *T     = new TTree("T","test");
    //TH2F *hpxpy  = new TH2F("hpxpy","py vs px",40,-4,4,40,-4,4);
    //T->Branch("hpxpy","TH2F",&hpxpy,32000,0);
    //Float_t px, py, pz;
    //for (Int_t i = 0; i < 25000; i++) {
    //    if (i%1000 == 0) printf("at entry: %d\n",i);
    //    gRandom->Rannor(px,py);
    //    pz = px*px + py*py;
    //    hpxpy->Fill(px,py);
    //    T->Fill();
    //}
    //T->Print();
    //fi.Write();


}
//
