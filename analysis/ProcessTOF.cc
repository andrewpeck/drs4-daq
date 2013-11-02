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

void ClearValues();

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
  

TTree* tree1;

//global variable defaults
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
  
  ClearValues();
    
  unsigned int numLineEntries = 10;
  double lineData[numLineEntries];
  for (unsigned int i=0; i<numLineEntries; i++)
    lineData[i]=-1;
  string curLine;
  string temp;
 
  int 	processCounter = 0;
  int	totalEvents = 0;
  int	hitMiddle = 0; // the number that hit the middle
  int	numTriggers = 0; // the number that activate the trigger
    
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
	  cout << "PMT hit!" << endl;
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
}
