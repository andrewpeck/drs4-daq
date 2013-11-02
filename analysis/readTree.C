#define CreatePlots_cxx
#include "CreatePlots.hh"
#include <TH1.h>
#include <TH2.h>
#include <TF1.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TProfile.h>

#include <fstream>
#include <cmath>
#include "float.h"

void CreatePlots::Loop()
{
  //initialize
  if (fChain == 0) return;
  TFile plotFile("plotTree.root","RECREATE");
  
  ////////////// set up style 
  gROOT->SetStyle("Plain");
  gStyle->SetTitleBorderSize(0);
  gStyle->SetPalette(1);
  //gStyle->SetOptFit(1);

  //////////////// SET UP HISTOGRAMS AND PROFILES ///////////////////////

  //instantiate plot parameters
  const char* name;
  const char* title;
  int nXBins, nYBins;
  double xLow, xHigh, yLow, yHigh;
  TAxis* xAxis;
  TAxis* yAxis;
  double stopTime = 40; // common stop time used in analysis

  // peak voltage and total charge histograms
  TH1D* chargeHist[2];
  chargeHist[0]= new TH1D(name="charge1", title="left PMT: total charge (C) Histogram", nXBins=50, xLow=0, xHigh=50.e-12);
  chargeHist[1]= new TH1D("charge2", "right PMT: total charge (C) Histogram", 50, 0, 50.e-12);
  TH1D* peakVoltageHist[2];
  peakVoltageHist[0]= new TH1D("peak1", "left PMT: peak voltage (mV) Histogram", 100, 0, 250);
  peakVoltageHist[1]= new TH1D("peak2", "right PMT: peak voltage (mV) Histogram", 100, 0, 250);

  // timing histograms
  TH1D* CTHtimeHist[2];
  TH1D* FTHtimeHist[2];
  TH1D* corrTimeHist[2];
  CTHtimeHist[0]=new TH1D("constTimeLeft", "left PMT: constant threshold timing (ns)", 40, 27, 38);
  CTHtimeHist[1]=new TH1D("constTimeRight", "right PMT: constant threshold timing (ns)", 40, 27, 38);
  FTHtimeHist[0]=new TH1D("fracTimeLeft", "left PMT: fractional threshold timing (ns)", 40, 27, 38);
  FTHtimeHist[1]=new TH1D("fracTimeRight", "right PMT: fractional threshold timing (ns)", 40, 27, 38);
  corrTimeHist[0]=new TH1D("corrTimeLeft", "left PMT: corrected threshold timing (ns)", 40, 27, 38);
  corrTimeHist[1]=new TH1D("corrTimeRight", "right PMT: corrected threshold timing (ns)", 40, 27, 38);

  //hits over secant theta profile
  TProfile* hitsOverST[2];
  hitsOverST[0] = new TProfile(name="hitsOverSTleft",title="#photoelectrons over sec(theta) vs x",nXBins=25,xLow=-250,xHigh=250,yLow=0,yHigh=175);
  hitsOverST[1] = new TProfile(name="hitsOverSTright",title,nXBins,xLow,xHigh,yLow,yHigh);
  for(unsigned int n=0;n<2;n++)
    {
      xAxis = hitsOverST[n]->GetXaxis();
      yAxis = hitsOverST[n]->GetYaxis();
      xAxis->SetTitle("x (mm)");
      yAxis->SetTitle("# photoelectrons / sec(theta)");
      hitsOverST[n]->SetMarkerSize(2);
      hitsOverST[n]->SetMarkerColor(2+n); 
      hitsOverST[n]->SetMarkerStyle(7);
      hitsOverST[n]->SetStats(false);
    }

  /////////////// PMT Hits V Sec Theta //////////////////////////
  TProfile* pmtHitsVSecThetaProf[2];
  pmtHitsVSecThetaProf[0] = new TProfile(name="pmtHitsVSecTheta0",title="# photoelectrons vs sec(theta) for -2.5cm<x<2.5cm",nXBins=50,xLow=1,xHigh=5,yLow=0,yHigh=100);
  name = "pmtHitsVSecTheta1";
  pmtHitsVSecThetaProf[1] = new TProfile(name,title,nXBins,xLow,xHigh,yLow,yHigh);

  pmtHitsVSecThetaProf[0]->GetXaxis()->SetTitle("sec(theta)");
  pmtHitsVSecThetaProf[0]->GetYaxis()->SetTitle("# photoelectrons");;
  
  for(unsigned int n=0;n<2;n++)
    {
      pmtHitsVSecThetaProf[n]->SetMarkerSize(1);
      pmtHitsVSecThetaProf[n]->SetMarkerColor(2+n); 
      pmtHitsVSecThetaProf[n]->SetMarkerStyle(34);
      pmtHitsVSecThetaProf[n]->SetStats(false);
    }

  //////////// PMT Hits V X -position along paddle /////////////////
  TProfile* pmtHitsVx[2];
  pmtHitsVx[0] = new TProfile(name="PMThitsVxLeft",title="#photoelectrons vs x, left PMT",nXBins=50,xLow=-250,xHigh=250,yLow=0,yHigh=175);
  pmtHitsVx[1] = new TProfile(name="PMThitsVxRight",title="#photoelectrons vs x, right PMT",nXBins,xLow,xHigh,yLow,yHigh);
  for(unsigned int n=0;n<2;n++)
    {
      xAxis = pmtHitsVx[n]->GetXaxis();
      yAxis = pmtHitsVx[n]->GetYaxis();
      xAxis->SetTitle("x (mm)");
      yAxis->SetTitle("# photoelectrons");
      pmtHitsVx[n]->SetMarkerSize(2);
      pmtHitsVx[n]->SetMarkerColor(2+n); 
      pmtHitsVx[n]->SetMarkerStyle(7);
      pmtHitsVx[n]->SetStats(false);
    }

  ////////// PMT Hits vs y-position along width of paddle /////////////////
  TProfile* pmtHitsVy[2];
  pmtHitsVy[0] = new TProfile(name="pmtHitsVyLeft",title="# photoelectrons vs y for -10cm<x<10cm in left PMT",nXBins=50,xLow=-80,xHigh=80,yLow=0,yHigh=125);
  pmtHitsVy[1] = new TProfile(name="pmtHitsVyRight",title="# photoelectrons vs y for -10cm<x<10cm in right PMT",nXBins,xLow,xHigh,yLow,yHigh);
  
  for(unsigned int n=0;n<2;n++)
    {
      xAxis = pmtHitsVy[n]->GetXaxis();
      yAxis = pmtHitsVy[n]->GetYaxis();
      xAxis->SetTitle("y (mm)");
      yAxis->SetTitle("# photoelectrons");
      pmtHitsVy[n]->SetMarkerSize(2);
      pmtHitsVy[n]->SetMarkerColor(2+n); 
      pmtHitsVy[n]->SetMarkerStyle(7);
      pmtHitsVy[n]->SetStats(false);
    }
  ////////// TDC (timing) vs ADC (charge) ///////////////////////////
  // timing before constant threshold is passed
  TH2D* ConstTimingVChargeH[3]; // 3 -> both PMTs
  ConstTimingVChargeH[0] = new TH2D("ConstTimingVChargeHL",title="constant TDC timing vs ADC charge, left PMT",nXBins=50,xLow=0,xHigh=70e-12,nYBins=50,yLow=27,yHigh=35);
  ConstTimingVChargeH[1] = new TH2D("ConstTimingVChargeHR",title="constant TDC timing vs ADC charge, right PMT",nXBins=50,xLow=0,xHigh=70e-12,nYBins=50,yLow=27,yHigh=35);
  ConstTimingVChargeH[2] = new TH2D("ConstTimingVChargeHBoth",title="constant TDC timing vs ADC charge, both PMTs",nXBins=50,xLow=0,xHigh=70e-12,nYBins=50,yLow=27,yHigh=35);
      
  for(unsigned int N=0; N<3; N++){
    ConstTimingVChargeH[N]->GetXaxis()->SetTitle("total charge (coulombs)");
    ConstTimingVChargeH[N]->GetYaxis()->SetTitle("time (ns)");
  } 

  // timing before fractional threshold is crossed
  TH2D* FtimingVChargeH[2];
  FtimingVChargeH[0] = new TH2D(name="FtimingVChargeHLeft",title="fractional TDC timing vs ADC charge",nXBins,xLow,xHigh,nYBins,yLow,yHigh);
  FtimingVChargeH[1] = new TH2D(name="FtimingVChargeHRight",title,nXBins,xLow,xHigh,nYBins,yLow,yHigh);
  xAxis = FtimingVChargeH[0]->GetXaxis();
  yAxis = FtimingVChargeH[0]->GetYaxis();
  xAxis->SetTitle("total charge (coulombs)");
  yAxis->SetTitle("time (ns)");
    
  // histogram for slew-corrected timing
  TH2D* corrTimingVChargeH[3];
  corrTimingVChargeH[0] = new TH2D("corrTimingVChargeHLeft",title="corrected TDC timing vs ADC charge, left",nXBins=50,xLow=5e-12,xHigh=70e-12,nYBins=50,yLow=30,yHigh=36);
  corrTimingVChargeH[1] = new TH2D("corrTimingVChargeHRight",title="corrected TDC timing vs ADC charge, right",nXBins=50,xLow=5e-12,xHigh=70e-12,nYBins=50,yLow=30,yHigh=36);
  corrTimingVChargeH[2] = new TH2D("corrTimingVChargeHBoth",title="corrected TDC timing vs ADC charge, both",nXBins=50,xLow=5e-12,xHigh=70e-12,nYBins=50,yLow=30,yHigh=36);
  
  corrTimingVChargeH[0]->GetXaxis()->SetTitle("total charge (coulombs)");
  corrTimingVChargeH[0]->GetYaxis()->SetTitle("time (ns)");
  
  // histogram for difference between measured and actual X
  TH1D* deltaXHist = new TH1D("deltaX","x resolution histogram",nXBins=50,xLow=-250,xHigh=250);
  deltaXHist->SetMinimum(0);
  xAxis = deltaXHist->GetXaxis();
  xAxis->SetTitle("delta X (mm)");
  TH1D* deltaXHist_corr = new TH1D("deltaX_corr","x resolution histogram, slewing corrected",nXBins=50,xLow=-250,xHigh=250);
  deltaXHist_corr->SetMinimum(0);
  xAxis = deltaXHist_corr->GetXaxis();
  xAxis->SetTitle("delta X (mm)");

  //////////// difference in timings vs x hit position //////////////
  TH2D* deltaTvsX = new TH2D(name="deltaTvsX",title="x vs TDC0-TDC1",nXBins=100,xLow=-10,xHigh=10,nYBins=100,yLow=-250,yHigh=250);
  xAxis = deltaTvsX->GetXaxis();
  yAxis = deltaTvsX->GetYaxis();
  xAxis->SetTitle("delta T (ns)");
  yAxis->SetTitle("x (mm)");
  deltaTvsX->SetMarkerSize(2);
  deltaTvsX->SetMarkerColor(2); 
  deltaTvsX->SetMarkerStyle(7);
  deltaTvsX->SetStats(false);
  //corrected
  TH2D* deltaTcorrVsX = new TH2D(name="deltaTcorrVsX",title="x vs corrected TDC0-TDC1",nXBins=100,xLow=-15,xHigh=15,nYBins=100,yLow=-250,yHigh=250);
  xAxis = deltaTcorrVsX->GetXaxis();
  yAxis = deltaTcorrVsX->GetYaxis();
  xAxis->SetTitle("delta T (ns)");
  yAxis->SetTitle("x (mm)");
  deltaTcorrVsX->SetMarkerSize(2);
  deltaTcorrVsX->SetMarkerColor(2); 
  deltaTcorrVsX->SetMarkerStyle(7);
  deltaTcorrVsX->SetStats(false);

  ////// average scint time vs average tdc time ///////
  TH2D* scintVsTDCtime = new TH2D(name="scintVsTDCtime",title="scintillator hit time vs average TDC time",nXBins=100,xLow=stopTime-13,xHigh=stopTime-5,nYBins=100,yLow=3.3,yHigh=3.7);
  
  xAxis = scintVsTDCtime->GetXaxis();
  yAxis = scintVsTDCtime->GetYaxis();
  xAxis->SetTitle("average TDC time (ns)");
  yAxis->SetTitle("scint hit time (ns)");
  scintVsTDCtime->SetMarkerSize(2);
  scintVsTDCtime->SetMarkerColor(2); 
  scintVsTDCtime->SetMarkerStyle(7);
  scintVsTDCtime->SetStats(true);
  // corrected: 
  TH2D* scintVsTDCtime_corr = new TH2D(name="scintVsTDCtime_corr",title="scintillator hit time vs average corrected TDC time",nXBins=100,xLow=28,xHigh=36,nYBins=100,yLow=3.25,yHigh=3.75);
  
  xAxis = scintVsTDCtime_corr->GetXaxis();
  yAxis = scintVsTDCtime_corr->GetYaxis();
  xAxis->SetTitle("average corrected TDC time (ns)");
  yAxis->SetTitle("scint hit time (ns)");
  scintVsTDCtime_corr->SetMarkerSize(2);
  scintVsTDCtime_corr->SetMarkerColor(2); 
  scintVsTDCtime_corr->SetMarkerStyle(7);
  scintVsTDCtime_corr->SetStats(true);

  // timing histograms, for sigma
  TH1D* leftTDCconstHist = new TH1D("leftTDCconstHist","constant threshold timing, left",100,0,40);
  TH1D* rightTDCconstHist = new TH1D("rightTDCconstHist","constant threshold timing, right",100,0,40);
  TH1D* leftTDCfracHist = new TH1D("leftTDCfractHist","fractional threshold timing, left",100,0,40);
  TH1D* rightTDCfracHist = new TH1D("rightTDCfracHist","fractional threshold timing, right",100,0,40);
TH1D* leftTDCcorrHist = new TH1D("leftTDCcorrHist","corrected threshold timing, left",100,0,40);
  TH1D* rightTDCcorrHist = new TH1D("rightTDCcorrHist","corrected threshold timing, right",100,0,40);

  //// delta T histograms, constant, fractional, and corrected /////////////
  TH1D* deltaTconstHist = new TH1D("deltaTconstHist","constant threshold timing differences",100,-12.5,12.5);
  TH1D* deltaTfracHist = new TH1D("deltaTfracHist","fraction threshold timing differences",100,-12.5,12.5);
  TH1D* deltaTcorrHist = new TH1D("deltaTcorrHist","slewing corrected threshold timing differences",100,-12.5,12.5);

  /// average number of photoelectrons histogram ///
  TH1D* leftPEHist = new TH1D("leftPEHist","average number of photoelectrons",100,0,100);
  TH1D* rightPEHist = new TH1D("rightPEHist","average number of photoelectrons",100,0,100);
  TH1D* avgPEHist = new TH1D("avgPEHist","average number of photoelectrons",100,0,100);

  TH1D* primaryEnergyHist = new TH1D("primaryEnergyHist","histogram of primary energy", 100, 0, 5.5e10);
  
  // end set up histograms //

  ///////// LOOP THROUGH TREE ////////////////////
  double chargeMin[2] = {DBL_MAX,DBL_MAX};
  double chargeMax[2] = {DBL_MIN,DBL_MIN};
  
  Long64_t nentries = fChain->GetEntries();
  Long64_t nbytes = 0, nb = 0;
  for (Long64_t jentry=0; jentry<nentries; jentry++) 
    {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry);
      nbytes += nb;
      // if (Cut(ientry) < 0) continue;
      
      // fill before applying angle cut
      if(hitPointX >= -25 && hitPointX <= 25){
	for(unsigned int n=0; n<2; n++)
	  pmtHitsVSecThetaProf[n]->Fill(secTheta,pmtHitCount[n]);}	  
      // apply cut for angle
      if(gunTheta > .949)
	continue;
      
      /////// FILL HISTOGRAMS ///////////
      // loop through PMTs
      for (unsigned int n=0;n<2;n++)
	{
	  
	  if(totalCharge[n] > chargeMax[n])
	    chargeMax[n] = totalCharge[n];
	  if(0 < totalCharge[n] && totalCharge[n] < chargeMin[n])
	    chargeMin[n] = totalCharge[n];
	  peakVoltageHist[n]->Fill(peakVoltage[n]);
	  chargeHist[n]->Fill(totalCharge[n]);
	  
	  if(CTHtime[n]!=-1) //check that threshold is passed before filling
	    CTHtimeHist[n]->Fill(CTHtime[n]);
	  
	  if(FTHtime[n]!=-1)
	    FTHtimeHist[n]->Fill(FTHtime[n]);
	  
	  double hitsOverSecTheta = pmtHitCount[n]*cos(gunTheta);// 1/sec = cos
	  hitsOverST[n]->Fill(hitPointX,hitsOverSecTheta);
	  
	  pmtHitsVx[n]->Fill(hitPointX,pmtHitCount[n]);
	  
	  if(hitPointX>=-100 && hitPointX<=100)
	    pmtHitsVy[n]->Fill(hitPointY,pmtHitCount[n]);
	  
	  if(hitPointX >= -25 && hitPointX <= 25 && CTHtime[n]!=-1)
	    {
	      ConstTimingVChargeH[n]->Fill(totalCharge[n],CTHtime[n]);
	      FtimingVChargeH[n]->Fill(totalCharge[n],FTHtime[n]);
	      ConstTimingVChargeH[2]->Fill(totalCharge[n],CTHtime[n]);
	    }

	} //end loop through PMTs

      //fill plots not specific to a PMT:
      double timeDiff_const = CTHtime[1]-CTHtime[0]; // right minus left because of sign change
      deltaTvsX->Fill(timeDiff_const,hitPointX);
      double timeDiff_frac = FTHtime[1]-FTHtime[0];
      // difference between hit position and measured x based on time difference
      double avgTime = (CTHtime[0]+CTHtime[1])/2;
      scintVsTDCtime->Fill(avgTime, scintTimeAvg);
      
      leftTDCconstHist->Fill(CTHtime[0]);
      rightTDCconstHist->Fill(CTHtime[1]);
      leftTDCfracHist->Fill(FTHtime[0]);
      rightTDCfracHist->Fill(FTHtime[1]);

      leftPEHist->Fill(pmtHitCount[0]);
      rightPEHist->Fill(pmtHitCount[1]);
      
      if(gunTheta < .506 && hitPointX > -25 && hitPointX < 25){
	deltaTconstHist->Fill(timeDiff_const);
	deltaTfracHist->Fill(timeDiff_frac); }

      if(hitPointX > -25 && hitPointX < 25)
	avgPEHist->Fill((pmtHitCount[0]+pmtHitCount[1])/2);

      primaryEnergyHist->Fill(primaryEnergy);
   
    } // end loop through tree first time
  
  /////// fit plots to find necessary parameters /////////////
  double correctedTiming[2];
  ///// fit parameters to be filled with TFit ///////
  double xParams[2];
  int wtopx, wtopy, ww, wh; // canvas parameters

  /// Delta T vs X, Xmeasured ///
  TCanvas* resCanvas = new TCanvas("resCanvas","resolution canvas",wtopx=10,wtopy=10,ww=700,wh=500);
  resCanvas->Divide(2,2);
  resCanvas->cd(1);
  deltaTvsX->SetMarkerSize(7);
  TF1* xFit = new TF1("xFit","[0]+[1]*x",-3,3);
  TProfile* deltaTvsX_prof = deltaTvsX->ProfileX();
  xFit->SetParameter(0,0);
  xFit->SetParameter(1,.01);
  xFit->SetLineWidth(2);
  deltaTvsX_prof->Fit("xFit","R");
  for(unsigned int i=0;i<2;i++)
    xParams[i] = xFit->GetParameter(i);
  //////////////// slewing fit and canvas ////////////////////////////
  TF1* slewFit[3]; // 3 
  TProfile* CtimingVChargeF[3];
  const char* slewFitName[3] = {"slewFitL","slewFitR","slewFitBoth"};
  TCanvas* slewCanvas = new TCanvas("slewCanvas","slewing correction",10,10,1000,800);
  slewCanvas->Divide(3,2);
  for(unsigned int N=0; N<3; N++)
    {
      CtimingVChargeF[N] = ConstTimingVChargeH[N]->ProfileX();
      //CtimingVChargeF[N]->SetBins(100,chargeMin[N],chargeMax[N]);
      slewFit[N] = new TF1(slewFitName[N], "[0]-[1]/(x+[2])");
      slewFit[N]->SetParameter(0,30);
      //slewFit[N]->SetParLimits(0,8,12);
      slewFit[N]->SetParameter(1,9e-11);
      slewFit[N]->SetParameter(2,2e-11);
      slewFit[N]->SetLineWidth(1);
      slewCanvas->cd(N+1);
      CtimingVChargeF[N]->Fit(slewFitName[N]);
    } 
  
  ///////// loop through tree again to fill corrected values ///////
  for (Long64_t jentry=0; jentry<nentries; jentry++) 
    {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry);
      nbytes += nb;
      if(gunTheta > .949) continue;
      
      for(unsigned int n=0; n<2; n++) // loop through pmts
	{
	  correctedTiming[n] = CTHtime[n]+slewFit[n]->GetParameter(1)/(slewFit[n]->GetParameter(2)+totalCharge[n]);
	  
	  if(CTHtime[n]!=-1)
	    corrTimeHist[n]->Fill(correctedTiming[n]);
	  
	  if(hitPointX >= -25 && hitPointX <= 25 && CTHtime[n]!=-1)
	    {
	      corrTimingVChargeH[n]->Fill(totalCharge[n],correctedTiming[n]);
	      corrTimingVChargeH[2]->Fill(totalCharge[n],correctedTiming[n]);
	    }
	} // end loop through pmts

      double timeDiff_corr = correctedTiming[1]-correctedTiming[0];
      deltaTcorrVsX->Fill(timeDiff_corr,hitPointX);
      if(gunTheta < .506 && hitPointX > -25 && hitPointX < 25)
	deltaTcorrHist->Fill(timeDiff_corr);
      
      double timeDiff = CTHtime[1]-CTHtime[0];
      double xMeas = xParams[0] + timeDiff*xParams[1];
      double deltaX = xMeas - hitPointX;
      deltaXHist->Fill(deltaX);
      
      double avgTime_corr = (correctedTiming[0]+correctedTiming[1])/2;
      scintVsTDCtime_corr->Fill(avgTime_corr, scintTimeAvg);

      leftTDCcorrHist->Fill(correctedTiming[0]);
      rightTDCcorrHist->Fill(correctedTiming[1]);
	
    } // end loop through tree for corrected values

  // fit corrected delta t vs x
  TProfile* deltaTcorrVsX_prof = deltaTcorrVsX->ProfileX();
  resCanvas->cd(2);
  deltaTcorrVsX_prof->Fit("xFit","R");
  for(unsigned int i=0;i<2;i++)
    xParams[i] = xFit->GetParameter(i);

  //now one more loop is required
  for (Long64_t jentry=0; jentry<nentries; jentry++) 
    {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry);
      nbytes += nb;
      if(gunTheta > .949) continue;

      for(unsigned int n=0; n<2; n++)
	correctedTiming[n] = CTHtime[n]+slewFit[n]->GetParameter(1)/(slewFit[n]->GetParameter(2)+totalCharge[n]);

      // fill corrected delta x
      double timeDiff_corr = correctedTiming[1]-correctedTiming[0];
      double xMeas_corr = xParams[0] + timeDiff_corr*xParams[1];
      double deltaX_corr = xMeas_corr - hitPointX;
      deltaXHist_corr->Fill(deltaX_corr);

    }

  
  ////// set up canvases and print plots to file /////////////////
  
  resCanvas->cd(3);
  deltaXHist->Draw();
  resCanvas->cd(4);
  deltaXHist_corr->Draw();
  resCanvas->Print("deltaTvsX.png","png");

  // canvas for peak voltage and total charge
  TCanvas* peakChargeCanvas = new TCanvas("peakChargeCanvas","charge & peak",wtopx=10,wtopy=10,ww=700,wh=500);
  peakChargeCanvas->Divide(2, 2);
  
  // canvas for threshold times
  TCanvas* timeCanvas = new TCanvas("timeCanvas","resolution",10,10,700,700);
  timeCanvas->Divide(2, 3);
  
  TCanvas* canvasTDCADC = new TCanvas("TDCADC","TDC vs ADC",10,10,1000,800);
  canvasTDCADC->Divide(2,2);
  
  //draw peak, charge, timings
  for (unsigned int n=0; n<2; n++) 
    {
      peakChargeCanvas->cd(n+1); 
      chargeHist[n]->Draw();
      peakChargeCanvas->cd(n+3); 
      peakVoltageHist[n]->Draw();
      timeCanvas->cd(n+1); 
      CTHtimeHist[n]->Draw();
      timeCanvas->cd(n+3); 
      FTHtimeHist[n]->Draw();
      timeCanvas->cd(n+5);
      corrTimeHist[n]->Draw();
      canvasTDCADC->cd(n+1);
      ConstTimingVChargeH[n]->Draw("COLZ");
      canvasTDCADC->cd(n+3);
      FtimingVChargeH[n]->Draw("COLZ");
    }
  peakChargeCanvas->Print("PeakVoltageTotalCharge.png","png");
  timeCanvas->Print("ThresholdTiming.png","png");
  canvasTDCADC->Print("TDCvADC.png","png");

  //draw hits over sec theta 
  TCanvas* canvas = new TCanvas("canvas","hitsOverST",0,0,1000,500);
  canvas->Divide(2,1);
  for(unsigned int n=0; n<2; n++){
    canvas->cd(n+1);
    hitsOverST[n]->Draw();
  }
  canvas->Print("hitsOverST.png","png");

  //draw hits vs sec theta
  canvas->SetTitle("pmtHitsVSecTheta");
  for(unsigned int n=0; n<2; n++) {
    canvas->cd(n+1);
    pmtHitsVSecThetaProf[n]->Draw(); }
  canvas->Print("pmtHitsVSecTheta.png","png");

  //draw pmt hits vs x position along paddle
  canvas->SetTitle("pmtHitsVx");
  for(unsigned int n=0; n<2; n++) {
    canvas->cd(n+1);
    pmtHitsVx[n]->Draw(); }
  canvas->Print("pmtHitsVx.png","png");

  //draw pmt hits vs y
  canvas->SetTitle("pmtHitsVy");
  for(unsigned int n=0; n<2; n++) {
    canvas->cd(n+1);
    pmtHitsVy[n]->Draw(); }
  canvas->Print("pmtHitsVy.png","png");
  
  for(unsigned int N=0; N<3; N++)
    { 
      // fit timing
      if(N!=2)//remove
	{std::cout << "chargemin " << chargeMin[N] << std::endl;
	  std::cout<<"chargemax" << chargeMax[N] << std::endl;}
      CtimingVChargeF[N]->SetStats(false);
      CtimingVChargeF[N]->SetMarkerColor(2);
      CtimingVChargeF[N]->SetLineColor(2);
      // draw corrected timing
      slewCanvas->cd(N+4);
      corrTimingVChargeH[N]->Draw("COLZ");
    }
  slewCanvas->Print("TDCvADC_fit.png","png");

  ///// scint hit time Vs average TDC time /////
  TCanvas* scintTimeCanvas = new TCanvas("scintTimeCanvas","average time canvas", 10, 10, 700, 500);
  scintTimeCanvas->Divide(1,2);
  scintTimeCanvas->cd(1);
  scintVsTDCtime->Draw();
  scintTimeCanvas->cd(2);
  scintVsTDCtime_corr->Draw();
  scintTimeCanvas->Print("scintVsTDCtime.png","png");

  /////// delta T histograms /////
  TF1* gausFit = new TF1("gausFit","gaus",0,50);
  TCanvas* deltaTCanvas = new TCanvas("deltaTCanvas","timing difference canvas", 10, 10, 1010, 710);
  deltaTCanvas->Divide(3,1);
  deltaTCanvas->cd(1);
  deltaTconstHist->Fit("gausFit");
  deltaTCanvas->cd(2);
  deltaTfracHist->Draw();
  deltaTCanvas->cd(3);
  deltaTcorrHist->Draw();
  deltaTCanvas->Print("deltaTHists.png","png");

  // timing fits
  TF1* gausFitConstL = new TF1("gausFitConstL","gaus",0,50);
  TF1* gausFitConstR = new TF1("gausFitConstR","gaus",0,50);
  TF1* gausFitFracL = new TF1("gausFitFracL","gaus",0,50);
  TF1* gausFitFracR = new TF1("gausFitFracR","gaus",0,50);
  TF1* gausFitCorrL = new TF1("gausFitCorrL","gaus",0,50);
  TF1* gausFitCorrR = new TF1("gausFitCorrR","gaus",0,50);

  leftTDCconstHist->Fit(gausFitConstL);
  rightTDCconstHist->Fit(gausFitConstR);
  leftTDCfracHist->Fit(gausFitFracL);
  rightTDCfracHist->Fit(gausFitFracR);
  
  leftTDCcorrHist->Fit(gausFitCorrL);
  rightTDCcorrHist->Fit(gausFitCorrR);

  ///// average number of photoelectrons histogram /////
  TCanvas* avgPECanvas = new TCanvas("avgPECanvas","average PE canvas", 10, 10, 700, 500);
  avgPECanvas->cd(1);
  avgPEHist->Draw();
  avgPECanvas->Print("avgPEhist.png","png");

  /// primary energy histogram ////
  TCanvas* primaryEnergyCanvas = new TCanvas("primaryEnergyCanvas","primary energy canvas", 10, 10, 700, 500);
  primaryEnergyCanvas->cd(1);
  primaryEnergyHist->Draw();
  primaryEnergyCanvas->Print("primaryEnergyhist.png","png");

  ///// close canvases
  peakChargeCanvas->Close();
  timeCanvas->Close();
  canvas->Close();
  canvasTDCADC->Close();
  slewCanvas->Close();
  resCanvas->Close();
  scintTimeCanvas->Close();
  deltaTCanvas->Close();
  avgPECanvas->Close();
  primaryEnergyCanvas->Close();
  ////// end draw plots ////////////

  // output means and standard deviations of a few plots
  double threshVoltL = .2*peakVoltageHist[0]->GetBinCenter(peakVoltageHist[0]->GetMaximumBin());
  double threshVoltR = .2*peakVoltageHist[1]->GetBinCenter(peakVoltageHist[1]->GetMaximumBin());
  std::cout << "new threshold voltage: " <<  threshVoltL  << " " << threshVoltR << std::endl;
  std::cout << "avg # photoelectrons, mean: " << avgPEHist->GetMean() << std::endl; 
  std::cout << "rms : " << avgPEHist->GetRMS() << std::endl;
  std::cout << "delta t, sigma: " << gausFit->GetParameter(2) << std::endl;
  std::cout << "delta t, err: " << gausFit->GetParError(2) << std::endl;

  std::ofstream voltFileS;
  voltFileS.open("voltage.dat");
  if(voltFileS.is_open())
    voltFileS << .5*(threshVoltL+threshVoltR) << std::endl;

  std::ofstream pointsFileS;
  pointsFileS.open("sigmaEffPoints.dat", std::ofstream::app);
  if(pointsFileS.is_open()) {
    pointsFileS << leftPEHist->GetMean() << "	" << leftPEHist->GetRMS() << "	"; 
    pointsFileS << rightPEHist->GetMean() << "	" << rightPEHist->GetRMS() << "	";
    pointsFileS << gausFitConstL->GetParameter(2) << "	" << gausFitConstL->GetParError(2) << "	";
    pointsFileS << gausFitConstR->GetParameter(2) << "	" << gausFitConstR->GetParError(2) << "	";
    pointsFileS << gausFitFracL->GetParameter(2) <<"	" << gausFitFracL->GetParError(2) << "	";
    pointsFileS << gausFitFracR->GetParameter(2) << "	" << gausFitFracR->GetParError(2) << "	";
    pointsFileS << gausFitCorrL->GetParameter(2) << "	" << gausFitCorrL->GetParError(2) << "	";
    pointsFileS << gausFitCorrR->GetParameter(2) << "	" << gausFitCorrR->GetParError(2) << std::endl;
  }

  std::cout << "new threshold voltage: " <<   .2*peakVoltageHist[0]->GetBinCenter(peakVoltageHist[0]->GetMaximumBin()) << " " << .2*peakVoltageHist[1]->GetBinCenter(peakVoltageHist[1]->GetMaximumBin()) << std::endl;

  plotFile.Write();
  plotFile.Close();  
}

// pmt hits => # photoelectrons

//   In a ROOT session, you can do:
//      Root > .L CreatePlots.C
//      Root > CreatePlots t
//      Root > t.GetEntry(12); // Fill t data members with entry number 12
//      Root > t.Show();       // Show values of entry 12
//      Root > t.Show(16);     // Read and show values of entry 16
//      Root > t.Loop();       // Loop on all entries
//

//     This is the loop skeleton where:
//    jentry is the global entry number in the chain
//    ientry is the entry number in the current Tree
//  Note that the argument to GetEntry must be:
//    jentry for TChain::GetEntry
//    ientry for TTree::GetEntry and TBranch::GetEntry
//
//       To read only selected branches, Insert statements like:
// METHOD1:
//    fChain->SetBranchStatus("*",0);  // disable all branches
//    fChain->SetBranchStatus("branchname",1);  // activate branchname
// METHOD2: replace line
//    fChain->GetEntry(jentry);       //read all branches
//by  b_branchname->GetEntry(ientry); //read only this branch

int main()
{
  /*
  TFile* dataFile = new TFile("dataTree.root");
  TTree* tree1 = (TTree*)dataFile->Get("tree1");
  CreatePlots cp = CreatePlots(tree1);*/

  CreatePlots cp;
  cp.Loop();
  return 0;
}

void CreatePlots()
{
  main();
}
