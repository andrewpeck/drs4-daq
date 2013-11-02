#include <iostream>
#include <string>
#include <fstream>

using namespace std;

void plotTimingResolution(){

  //the default ROOT styles are ugly, this will make it look better
  gROOT->SetStyle("Plain");
  gStyle->SetTitleBorderSize(0);
  gStyle->SetPalette(1);

  double efficiency, avgPEleft, avgPEleftErr, avgPEright, avgPErightErr, sigmaConstL, sigmaConstLerr, sigmaConstR, sigmaConstRerr, sigmaFracL, sigmaFracLerr, sigmaFracR, sigmaFracRerr, sigmaCorrL, sigmaCorrLerr, sigmaCorrR, sigmaCorrRerr;

  ifstream inFile("sigmaEffPoints.dat");

  //variables 
  int pointNum = 0;	
  //this example just uses TGraph
  TGraphErrors* constL = new TGraphErrors();
  TGraphErrors* constR = new TGraphErrors();
  TGraphErrors* fracL = new TGraphErrors();
  TGraphErrors* fracR = new TGraphErrors();
  TGraphErrors* corrL = new TGraphErrors();
  TGraphErrors* corrR = new TGraphErrors();
  
  inFile >> efficiency >> avgPEleft >> avgPEleftErr >> avgPEright >> avgPErightErr >> sigmaConstL >> sigmaConstLerr >> sigmaConstR >> sigmaConstRerr >> sigmaFracL >> sigmaFracLerr >> sigmaFracR >> sigmaFracRerr >> sigmaCorrL >> sigmaCorrLerr >> sigmaCorrR >> sigmaCorrRerr;
  while(!inFile.eof())
    {
      if(pointNum%2 == 1){
	constL->SetPoint(pointNum/2-1,avgPEleft,sigmaConstL);
	constL->SetPointError(pointNum/2-1,avgPEleftErr,sigmaConstLerr);
	constR->SetPoint(pointNum/2-1,avgPEright,sigmaConstR);
	constR->SetPointError(pointNum/2-1,avgPErightErr,sigmaConstRerr);
	fracL->SetPoint(pointNum/2-1,avgPEleft,sigmaFracL);
	fracL->SetPointError(pointNum/2-1,avgPEleftErr,sigmaFracLerr);
	fracR->SetPoint(pointNum/2-1,avgPEright,sigmaConstR);
	fracR->SetPointError(pointNum/2-1,avgPErightErr,sigmaFracRerr);
	corrL->SetPoint(pointNum/2-1,avgPEleft,sigmaCorrL);
	corrL->SetPointError(pointNum/2-1,avgPEleftErr,sigmaCorrLerr);
	corrR->SetPoint(pointNum/2-1,avgPEright,sigmaCorrR);
	corrR->SetPointError(pointNum/2-1,avgPErightErr,sigmaCorrRerr);
      }
      pointNum++;
      inFile >> efficiency >> avgPEleft >> avgPEleftErr >> avgPEright >> avgPErightErr >> sigmaConstL >> sigmaConstLerr >> sigmaConstR >> sigmaConstRerr >> sigmaFracL >> sigmaFracLerr >> sigmaFracR >> sigmaFracRerr >> sigmaCorrL >> sigmaCorrLerr >> sigmaCorrR >> sigmaCorrRerr;
     }
  //output the plots
  TGraphErrors* graphArray[6] = {constL,constR,fracL,fracR,corrL,corrR};
  TCanvas* graphCanvas = new TCanvas("graphCanvas","Plot Canvas",0,0,1000,600);
  graphCanvas->Divide(2,3);

  constL->SetTitle("constant timing, left");
  constR->SetTitle("constant timing, right");
  fracL->SetTitle("fractional timing, left");
  fracR->SetTitle("fractional timing, right");
  corrL->SetTitle("corrected timing, left");
  corrR->SetTitle("corrected timing, right");

  for(unsigned int i=0; i<6; i++){
    graphArray[i]->GetXaxis()->SetTitle("avg # photoelectrons");
    graphArray[i]->GetXaxis()->CenterTitle();
    graphArray[i]->GetYaxis()->SetTitle("sigma (ns)");
    graphArray[i]->GetYaxis()->CenterTitle();

    graphCanvas->cd(i+1);
    graphArray[i]->Draw("AP*");
  }
  
  graphCanvas->Print("resolutionGraphs.png","png");
}
