// System Headers
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <cmath>

// ROOT Headers
#include "TROOT.h"
#include "TTree.h"
#include "TFile.h"
#include "TRandom3.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TProfile.h"
#include "TChain.h"

//using namespace std;

void IntegrateSignal();
void ClearValues();

// Tree data
int	eventNum;		//event number from geant4
int	processNum;		//number in which this was processed (basically a better event number)
double	primaryEnergy;
double	gunTheta;
double	gunPhi;

double 	hitPointX;
double 	hitPointY;

//scintillator info
std::vector<double> scintEdep;	//energy deposits in the scint slabs
std::vector<double> scintTimes;	//times for hits in each scint slab
double 	scintEdepTotal;	//total energy deposits in the scint slabs
double 	scintEdepMax;	//largest energy deposits in the scint slabs
double 	scintTimeMax;	//time when the max scint hit occured
double	scintTimeAvg;
  
//PMT info
std::vector<double>	pmtTimes[2];
std::vector<double> 	pmtEnergies[2];
std::vector<double>	pmtLgTimes[2];

int	pmtHitCount[2];
int	enteredLG[2];
double 	peakVoltage[2];
double	totalCharge[2];
double	CTHtime[2]; //time constant threshold is crossed
double	FTHtime[2]; //time fraction of peak is crossed

double  singlePulseInterp[500]; //interpolated single pulse waveform for integration

TTree* tree1;

//global variable defaults
double eff = 1.0; //superficial efficiency
double threshold = 13.0; //constant threshold (mV) average 60mV so this is 10%

TROOT root("root","ASCII2ROOT");

int main(int argc, const char* argv[])
{  
  
  //allow vectors 
  gROOT->ProcessLine("#include <vector>");

  std::string inFileName, outFileName, voltageFile;

  if(!(argc<5)){ // else use default values
    
    // attempt to read threshold from file
    ifstream voltIn;
    double threshold;
    voltIn.open(argv[4]);
    if(voltIn >> threshold);
    std::cout << "voltage read: " << threshold << std::endl;
  //constant threshold (mV) average 60mV so this is 10%
  }


  // superficial efficiency, either default or
  if(argc < 4)
    eff = 1.0;
  else { // take argument, and record to data file, for use with script
    eff = atof(argv[3])/100.0;
    std::ofstream pointsFileS;
    pointsFileS.open("sigmaEffPoints.dat", std::ofstream::app);
    if(pointsFileS.is_open()) {
      pointsFileS << eff << "	"; 
    }
  }

  // Get the output file name from user.  If not specified,
  // use the default
  if (argc < 3) 
    outFileName = "dataTree.root";
  else
    outFileName = argv[2];
  
  // Get the input file name from user.  If not specified, use the default
  if (argc < 2)
    inFileName = "../data/*.dat";
  else
    inFileName = argv[1];

  // Create output file
  TFile tree_file(outFileName.c_str(),"RECREATE");

  // Set up the Tree
  //   D double
  //   F float
  //   I int32_t
  tree1 = new TTree("dataTree","dataTree");
  
  tree1->Branch("eventNum",&eventNum,"eventNum/I");
  tree1->Branch("processNum", &processNum,"processNum/I");
  tree1->Branch("primaryEnergy",&primaryEnergy,"primaryEnergy/D");
  tree1->Branch("gunTheta", &gunTheta, "gunTheta/D");
  tree1->Branch("gunPhi", &gunPhi, "gunPhi/D");
  tree1->Branch("hitPointX", &hitPointX, "hitPointX/D");
  tree1->Branch("hitPointY", &hitPointY, "hitPointY/D");
  tree1->Branch("scintEdepTotal", &scintEdepTotal, "scintEdepTotal/D");
  tree1->Branch("scintEdepMax", &scintEdepMax, "scintEdepMax/D");  
  tree1->Branch("scintTimeAvg", &scintTimeAvg, "scintTimeAvg/D");
  tree1->Branch("scintTimeMax", &scintTimeMax, "scintTimeMax/D");
  tree1->Branch("pmtHitCount", &pmtHitCount, "pmtHitCount[2]/I");
  tree1->Branch("pmtEnergies", &pmtEnergies, "pmtEnergies[2]/D");
  tree1->Branch("enteredLG", &enteredLG, "enteredLG[2]/I");
  tree1->Branch("peakVoltage", &peakVoltage, "peakVoltage[2]/D");
  tree1->Branch("totalCharge", &totalCharge, "totalCharge[2]/D");
  tree1->Branch("CTHtime", &CTHtime, "CTHtime[2]/D");
  tree1->Branch("FTHtime", &FTHtime, "FTHtime[2]/D");
  
  ClearValues();
  processNum = 0;
  
  //set default values
  //set up values for integration
  // voltage array in mV for a single pulse
  double singlePulseWF[50]={-0.103201, -0.0856226, -0.0623353, -0.0318365, 0.00632456, 0.00121639, 0.0363726, 0.24701, 0.826487, 1.93781, 3.46396, 4.8977, 5.85909, 6.27976, 6.18391, 5.78517, 5.16994, 4.43181, 3.67775, 3.00993, 2.42054, 1.87366, 1.40702, 1.01309, 0.743704, 0.580994, 0.468164, 0.40987, 0.366901, 0.268344, 0.143344, 0.0581575, 0.0598101, 0.133728, 0.191421, 0.155513, 0.085952, 0.0345698, -0.0267283, -0.0486634, -0.0111033, 0.0264568, 0.0326166, 0.0270577, 0.0323161, 0.0106815, -0.0238738, 0.00166711, 0.0317152, 0.0198462}; //mV
  double scale = 3.0/3.8; // scale the default gain (3.8E6) to some other gain, will change each bin height	
  for (int t=0; t<50; t++) 
    singlePulseWF[t]=singlePulseWF[t]*scale;
  //make linear interpolation over the pulse shape, 10 bins per 0.5 ns -> 1 bin = 50 ps //need an array with 50*10=500 elements
  
  for(int i=0; i<50; i++)
    { //linear interpolate
      double slope = (singlePulseWF[i+1]-singlePulseWF[i])/10; 
      //linear interpolation slope = rise/bin; 
      for(int k=0;k<10;k++) //fill 10 bins
	singlePulseInterp[10*i+k] = slope*k+singlePulseWF[i]; 
      //linear interpolation: same slope per 0.5ns
    }

  int numLineEntries = 15;
  double lineData[numLineEntries];
  for (int i=0;i<numLineEntries;i++)
    lineData[i]=-1;
  std::string curLine;
  std::string temp;
 
  int processCounter = 0;
  TRandom3* rand0 = new TRandom3();
  rand0->SetSeed(0); //create random UUID seed

  ifstream fp(inFileName.c_str());
  if(!fp.is_open())
    {
      std::cout << "Error opening input file!" << std::endl;
      return EXIT_FAILURE;
    }

  // loop over the current line of data, filling an array
  getline(fp,curLine);

  while(!fp.eof())
    { //loop over all lines in the file
      
      std::stringstream parLine(curLine);
      // loop over the current line of data, filling an array
      for(int i=0; i<numLineEntries;i++) //mark 
	{
	  if(parLine >> temp)
	    {
	      lineData[i] = atof(temp.c_str());
	    }
	  else lineData[i] = -10;
	}//end loop over this line of data
	  
      int line0 = static_cast<int>(lineData[0]);

      //process this line of data

      if(line0 == 11111)
	std::cout << "beginning of run!" << std::endl;

      //read start of event code, and primary track information
      else if(line0==22222)
	{
	  eventNum = static_cast<int>(lineData[1]);
	  
	  primaryEnergy = lineData[2];
	  gunTheta = lineData[3];
	  gunPhi = lineData[4];
	  hitPointX = lineData[5];
	  hitPointY = lineData[6];
	
	  enteredLG[0] = lineData[7];
	  enteredLG[1] = lineData[8];
	  
	  //charge = lineData[9];
	  
	  processCounter++;
	  std::cout << "process number set to " << processCounter << std::endl;
	}
      	  
      //if this is scintillator energy deposit type of event, process it
      else if(line0%100==50)
	{  
	  scintEdep.push_back(lineData[1]);
	  scintEdepTotal+=lineData[1];
	  scintTimes.push_back(lineData[2]);
	}
      
      //process pmt hits
      else if(line0%100==75)
	{ //copy number is that of pmt
	  double randNum = rand0->Uniform();
	  if(randNum < eff)
	    {
	      int pmtNum = (line0-line0%100)/100;
	      pmtHitCount[pmtNum]++;
	      pmtTimes[pmtNum].push_back(lineData[1]);
	      pmtEnergies[pmtNum].push_back(lineData[2]);
	      pmtLgTimes[pmtNum].push_back(lineData[3]);
	    }
	}
      
      //if this is the end of the event, then fill the root tree
      else if(line0==88888)
	{
	  //find the max energy deposit for this event in each scintillator
	  if(!scintEdep.empty())
	    scintEdepMax = *max_element(scintEdep.begin(), scintEdep.end());
	  if(!scintTimes.empty())
	    {
	      scintTimeMax = *max_element(scintTimes.begin(), scintTimes.end());
	      int timeEntries = scintTimes.size();
	      double timeTotal = 0;
	      for(int i=0; i<timeEntries; i++)
		timeTotal += scintTimes.at(i);
	      scintTimeAvg = timeTotal/timeEntries;	
	    }
	  //integrate to find max voltage, threshold crossing time
	  IntegrateSignal();

	  tree1->Fill();
	      
	  ClearValues();
	}//end end-of-event
	  
      else if(line0 == 99999)
	std::cout << "end of run!" << std::endl;
      else 
	std::cout << "Unknown copy number!!! " << line0 <<std::endl;

      getline(fp,curLine);
    } //end loop over all lines in the file
    
  //write tree file
  tree_file.Write();
  tree_file.Close();
  std::cout << "Data written to '" << outFileName << "'" << std::endl;
  
  return EXIT_SUCCESS;
}

void IntegrateSignal()
{

  //per photon data
  double pulseArray[2][4520]; // add up single photoelectron pulse, spans 200ns from start of event + 25 ns allowed for the pmt pulse + 1ns allowed for PE transit time 1 bin = 50 ps
  for(int n=0;n<2;n++){
    for(int i=0;i<4520;i++)
      pulseArray[n][i] = 0;}
  TRandom3* rand1 = new TRandom3(); // single PE transition time
  rand1->SetSeed(0);
  double pmtTime = -1;
  double transit = 0; //rand1->Gaus(0,0.175), mean = 0 (because you can set arbitrary offset), standard deviation = 0.175 ns
  //single photoelectron pulse 1 bin = 0.5 ns, total 50 bins, spans 25 ns
    
  //per event data
  bool pmtDataExists[2] = {false,false};
  bool CTHcrossed[2] = {false,false};
  bool FTHcrossed[2] = {false,false};
  double fraction = 0.2; // fractional threshold
  double stopTime = 40+scintTimeAvg; // common stop time in ns

  for(int n=0;n<2;n++) // n is pmtNum
    {
      // pmt data
      for (int op=0; op < pmtHitCount[n]; op++) {
	pmtTime = pmtTimes[n].at(op); //pmtTimes in units of ns
	transit = rand1->Gaus(0, 0.175); // standard deviation of 175ps
	double findBin = (pmtTime+transit)/0.05; //divide by 50ps
	int bin = round(findBin); 
	if ((bin<=0)||(bin>4000)) { //will overflow array 
	  std::cout<<" bin "<<bin<<" findBin "<<findBin<<" pmtTime "<<pmtTime<<std::endl;
	  std::cout<<"will overflow, ignore"<<std::endl;
	} //overflow
	else {	
	  //add pulse		  
	  pmtDataExists[n]=true; 
	  for (int j=0; j<500; j++)
	    pulseArray[n][j+bin]+=singlePulseInterp[j];	 
	} //add pulse
      } // end pmt data 
    } // for pmtNum loop

  // process pmt data 
  for (int n=0; n<2; n++) { // n is pmtNum
    totalCharge[n]=0;
    if (pmtDataExists[n]==true){
      //find peakVoltage and charge
      for (int bin=0; bin<4520; bin++) 
	{//loop over pulse
	  totalCharge[n]+=pulseArray[n][bin];
	  if (pulseArray[n][bin]>peakVoltage[n])
	    peakVoltage[n]=pulseArray[n][bin];
	} //loop over pulse					
	//find constant threshold time
      for (int bin=0; bin<4520; bin++) {//loop over pulse
	if ((CTHcrossed[n]==false) && (pulseArray[n][bin]>=threshold)) {
	  CTHcrossed[n] = true;
	  CTHtime[n] = (double)bin*0.05; // convert to ns
	  CTHtime[n] = stopTime - CTHtime[n]; // subtract from common stop
	} //find CT time
	
      } //loop over pulse
      
      
	//find constant fraction time
      for (int bin=0; bin<4520; bin++) {//loop over pulse									
	if (FTHcrossed[n]==false && pulseArray[n][bin]>=peakVoltage[n]*fraction)
	  {
	    FTHcrossed[n] = true;
	    FTHtime[n] = (double)bin*0.05; // convert to ns
	    FTHtime[n] = stopTime - FTHtime[n]; // subtract from common stop
	  } //find CF time
      } //loop over pulse

      //charge
      totalCharge[n]=totalCharge[n]*1.e-3; //convert mV to V
      totalCharge[n]=totalCharge[n]*50.*1.e-12; //convert bin width to 50 ps, now with unit V*s
      totalCharge[n]=totalCharge[n]/50.; //divide by 50 Ohm, now with unit Amp*s = Coulomb

    } // pmtData exists
  } // loop over pmts [n]

  delete rand1;
} // end process pmt data

void ClearValues()
{
  primaryEnergy=-10;
  gunTheta = -10;
  gunPhi = -10;
  hitPointX = 0;
  hitPointY = 0;
  scintEdepTotal=-10;
  scintEdepMax=-1;
  scintEdep.clear();
  scintTimes.clear();
  scintTimeMax = -1;
  scintTimeAvg = -1;
  for(int i=0;i<2;i++)
    {
      pmtHitCount[i] = 0;
      pmtTimes[i].clear();
      pmtEnergies[i].clear();
      enteredLG[i] = 0;
      peakVoltage[i] = -10;
      totalCharge[i] = 0;
      CTHtime[i] = -1; // this value will remain if threshold not crossed
      FTHtime[i] = -1; // '                                             '
    }
}
