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

using namespace std;

void IntegrateSignal();
void ClearValues();
void MakePlots();  

// Tree data
int	eventNum; // event number from geant4
double	primaryEnergy;
double	gunTheta;
double	gunPhi;

double 	startPointX;
double 	startPointY;
double 	startPointZ;

//scintillator info
vector<double> scintEdep[16];	//energy deposits in the scint slabs
vector<double> scintTimes[16];	//times for hits in each scint slab
double 	scintEdepTotal[16];	//total energy deposits in the scint slabs
double 	scintEdepMax[16];	//largest energy deposits in the scint slabs
double 	scintTimeMax[16];	//time when the max scint hit occured
double	scintTimeAvg[16];
  
//PMT info
vector<double>	pmtTimes[32];
vector<double> 	pmtEnergies[32];

int	pmtHitCount[32];
double 	peakVoltage[32];
double	totalCharge[32];
double	CTHtime[32]; //time constant threshold is crossed
double	FTHtime[32]; //time fraction of peak is crossed

double  singlePulseInterp[500]; //interpolated single pulse waveform for integration

TTree* tree1;

bool pmtHasBeenHit;

//global variable defaults
double threshold = 13.0; //constant threshold (mV) average 60mV so this is 10%
double eDepMin = 1e5; //at least 1MeV of energy deposited for scint hit to register

TROOT root("root","ASCII2ROOT");

int main(int argc, const char* argv[])
{  
  
  //allow vectors 
  gROOT->ProcessLine("#include <vector>");

  string inFileName, outFileName, voltageFile;

  // Get the output file name from user.  If not specified,
  // use the default
  if (argc < 3) 
    outFileName = "treeTOF.root";
  else
    outFileName = argv[2];
  
  // Get the input file name from user.  If not specified, use the default
  if (argc < 2)
    inFileName = "test.dat";
  else
    inFileName = argv[1];

  // Create output file
  TFile tree_file(outFileName.c_str(),"RECREATE");

  tree1 = new TTree("dataTree","dataTree");
  
  tree1->Branch("eventNum",&eventNum,"eventNum/I");
  tree1->Branch("primaryEnergy",&primaryEnergy,"primaryEnergy/D");
  tree1->Branch("gunTheta", &gunTheta, "gunTheta/D");
  tree1->Branch("gunPhi", &gunPhi, "gunPhi/D");
  tree1->Branch("startPointX", &startPointX, "startPointX/D");
  tree1->Branch("startPointY", &startPointY, "startPointY/D");
  tree1->Branch("startPointZ", &startPointY, "startPointZ/D");
  tree1->Branch("scintEdepTotal", &scintEdepTotal, "scintEdepTotal[16]/D");
  tree1->Branch("scintEdepMax", &scintEdepMax, "scintEdepMax[16]/D");  
  tree1->Branch("scintTimeAvg", &scintTimeAvg, "scintTimeAvg[16]/D");
  tree1->Branch("scintTimeMax", &scintTimeMax, "scintTimeMax[16]/D");
  tree1->Branch("pmtHitCount", &pmtHitCount, "pmtHitCount[32]/I");
  tree1->Branch("pmtEnergies", &pmtEnergies, "pmtEnergies[32]/D");
  tree1->Branch("peakVoltage", &peakVoltage, "peakVoltage[32]/D");
  tree1->Branch("totalCharge", &totalCharge, "totalCharge[32]/D");
  tree1->Branch("CTHtime", &CTHtime, "CTHtime[32]/D");
  tree1->Branch("FTHtime", &FTHtime, "FTHtime[32]/D");
  
  ClearValues();
    
  //set up default values for integration
  // voltage array in mV for a single pulse
  double singlePulseWF[50]={-0.103201, -0.0856226, -0.0623353, -0.0318365, 0.00632456, 0.00121639, 0.0363726, 0.24701, 0.826487, 1.93781, 3.46396, 4.8977, 5.85909, 6.27976, 6.18391, 5.78517, 5.16994, 4.43181, 3.67775, 3.00993, 2.42054, 1.87366, 1.40702, 1.01309, 0.743704, 0.580994, 0.468164, 0.40987, 0.366901, 0.268344, 0.143344, 0.0581575, 0.0598101, 0.133728, 0.191421, 0.155513, 0.085952, 0.0345698, -0.0267283, -0.0486634, -0.0111033, 0.0264568, 0.0326166, 0.0270577, 0.0323161, 0.0106815, -0.0238738, 0.00166711, 0.0317152, 0.0198462}; //mV

  double scale = 3.0/3.8; // scale the default gain (3.8E6) to some other gain, will change each bin height	
  for (unsigned int t=0; t<50; t++) 
    singlePulseWF[t]=singlePulseWF[t]*scale;
  //make linear interpolation over the pulse shape, 10 bins per 0.5 ns -> 1 bin = 50 ps //need an array with 50*10=500 elements
  
  for(unsigned int i=0; i<50; i++)
    { //linear interpolate
      double slope = (singlePulseWF[i+1]-singlePulseWF[i])/10; 
      //linear interpolation slope = rise/bin; 
      for(int k=0;k<10;k++) //fill 10 bins
	singlePulseInterp[10*i+k] = slope*k+singlePulseWF[i]; 
      //linear interpolation: same slope per 0.5ns
    }

  unsigned int numLineEntries = 10;
  double lineData[numLineEntries];
  for (unsigned int i=0; i<numLineEntries; i++)
    lineData[i]=-1;
  string curLine;
  string temp;
 
  int processCounter = 0;
  int totalEvents = 0;
  int	hitMiddle = 0; // the number that hit the middle
  int	numTriggers = 0; // the number that activate the trigger
  pmtHasBeenHit = false;
  
  ifstream fp(inFileName.c_str());
  if(!fp.is_open())
    {
      cout << "Error opening input file!" << endl;
      return EXIT_FAILURE;
    }

  // loop over the current line of data, filling an array
  getline(fp,curLine);

  while(!fp.eof())
    { //loop over all lines in the file
      
      stringstream parLine(curLine);
      // loop over the current line of data, filling an array
      for(unsigned int i=0; i<numLineEntries;i++) //mark 
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
	cout << "beginning of run!" << endl;

      //read start of event code, and primary track information
      else if(line0==22222)
	{
	  eventNum = static_cast<int>(lineData[1]);
	  
	  primaryEnergy = lineData[2];
	  gunTheta = lineData[3];
	  gunPhi = lineData[4];
	  startPointX = lineData[5];
	  startPointY = lineData[6];
	  startPointZ = lineData[7];

	  processCounter++;
	  //cout << "process number set to " << processCounter << endl;
	}
      	  
      //if this is scintillator energy deposit type of event, process it
      else if(line0%100==50)
	{  
	  int paddleNum = (line0-line0%100)/100;
	  scintEdep[paddleNum].push_back(lineData[1]);
	  scintEdepTotal[paddleNum] += lineData[1];
	  scintTimes[paddleNum].push_back(lineData[2]);
	}
      
      //process pmt hits
      else if(line0%100==75)
	{ //copy number is that of pmt
	  int pmtNum = (line0-line0%100)/100;
	  pmtHitCount[pmtNum]++;
	  pmtTimes[pmtNum].push_back(lineData[1]);
	  pmtEnergies[pmtNum].push_back(lineData[2]);
	}
      
      //if this is the end of the event, then fill the root tree
      else if(line0==88888)
	{
	  for(unsigned int i=0; i<16; i++){
	  //find the max energy deposit for this event in each scintillator
	  if(!scintEdep[i].empty())
	    scintEdepMax[i] = *max_element(scintEdep[i].begin(), scintEdep[i].end());
	  if(!scintTimes[i].empty())
	    {
	      scintTimeMax[i] = *max_element(scintTimes[i].begin(), scintTimes[i].end());
	      int timeEntries = scintTimes[i].size();
	      double timeTotal = 0;
	      for(int j=0; j<timeEntries; j++)
		timeTotal += scintTimes[i].at(j);
	      scintTimeAvg[i] = timeTotal/timeEntries;	
	    }
	  
	  }

	  for(unsigned int i=0; i<32; i++)
	    {
	      if(!pmtTimes[i].empty())
		pmtHasBeenHit = true;
	    }
	  if(pmtHasBeenHit)
	    IntegrateSignal(); //integrate to find max voltage, threshold crossing time
	  if((scintEdepTotal[12]>eDepMin||scintEdepTotal[13]>eDepMin)&&(scintEdepTotal[14]>eDepMin||scintEdepTotal[15]>eDepMin))
	    {
	      //if min energy is deposited in 1 scint from each of the 2 middle layers
	      hitMiddle++;
	      int numPaddlesHit = 0;
	      for(unsigned int j=0; j<16; j++)
		{
		  if(scintEdepTotal[j] > eDepMin)
		    numPaddlesHit++;
		}
	      if(numPaddlesHit > 5) // at least one other paddle hit
		numTriggers++; // add one to the trigger
	    }
	  tree1->Fill();
	  ClearValues();
	}//end end-of-event
	  
      else if(line0 == 99999)
	{
	  totalEvents = lineData[1];
	  cout << "end of run!" << endl;
	}
      else 
	cout << "Unknown copy number!!! " << line0 <<endl;

      getline(fp,curLine);
    } //end loop over all lines in the file
    
  cout << "total number of events: " << totalEvents << endl;
  cout << "number that hit opening aperture: " << hitMiddle << endl;
  cout << "number that activated trigger: " << numTriggers << endl;



  //write tree file
  tree_file.Write();
  tree_file.Close();
  cout << "Data written to '" << outFileName << "'" << endl;
  
  return EXIT_SUCCESS;
}

void IntegrateSignal()
{

  //per photon data
  double pulseArray[32][4520]; // add up single photoelectron pulse, spans 200ns from start of event + 25 ns allowed for the pmt pulse + 1ns allowed for PE transit time 1 bin = 50 ps
  for(unsigned int n=0; n<32; n++){
    for(unsigned int i=0;i<4520;i++)
      pulseArray[n][i] = 0;}
  TRandom3* rand1 = new TRandom3(); // single PE transition time
  rand1->SetSeed(0); //create random UUID seed
  double pmtTime = -1;
  double transit = 0; //rand1->Gaus(0,0.175), mean = 0 (because you can set arbitrary offset), standard deviation = 0.175 ns
  //single photoelectron pulse 1 bin = 0.5 ns, total 50 bins, spans 25 ns
    
  //per event data
  bool pmtDataExists[32] = {false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false};
  bool CTHcrossed[32] = {false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false};
  bool FTHcrossed[32] = {false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false};
  double fraction = 0.2; // fractional threshold
  double stopTime = 40; // common stop time in ns

  for(unsigned int n=0; n<32; n++) // n is pmtNum
    {
      // pmt data
      for (int op=0; op < pmtHitCount[n]; op++) {
	pmtTime = pmtTimes[n].at(op); //pmtTimes in units of ns
	transit = rand1->Gaus(0, 0.175); // standard deviation of 175ps
	double findBin = (pmtTime+transit)/0.05; //divide by 50ps
	int bin = round(findBin); 
	if ((bin<=0)||(bin>4000)) { //will overflow array 
	  cout<<" bin "<<bin<<" findBin "<<findBin<<" pmtTime "<<pmtTime<<endl;
	  cout<<"will overflow, ignore"<<endl;
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
  for (unsigned int n=0; n<32; n++) { // n is pmtNum
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
} // end process pmt data

void ClearValues()
{
  primaryEnergy = -10;
  gunTheta = -10;
  gunPhi = -10;
  startPointX = 0;
  startPointY = 0;
  startPointZ = 0;
  for(unsigned int i=0; i<16; i++)
    {
      scintEdepTotal[i] = -10;
      scintEdepMax[i] = -1;
      scintEdep[i].clear();
      scintTimes[i].clear();
      scintTimeMax[i] = -1;
      scintTimeAvg[i] = -1;
    }
  for(unsigned int i=0; i<32; i++)
    {
      pmtHitCount[i] = 0;
      pmtTimes[i].clear();
      pmtEnergies[i].clear();
      peakVoltage[i] = -10;
      totalCharge[i] = 0;
      CTHtime[i] = -1; // this value will remain if threshold not crossed
      FTHtime[i] = -1; // '                                             '
    }
  pmtHasBeenHit = false;
}
