// include std libraries
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// include ROOT libraries
#include "TH1.h"
#include "TH2.h"
#include "TF1.h"
#include "TTree.h"
#include "TChain.h"
#include "TGraph.h"
#include "TStyle.h"
#include "TFolder.h"
#include "TCanvas.h"
#include "TRandom.h"
#include "TMath.h"
#include "TFile.h"
#include "TSystem.h"
#include "TProfile.h"

using namespace std;

int unpacker(char* fname)
{

    ifstream file;		// read file directly

    if (fname != NULL)
    {
        file.open (fname, ios::in | ios::binary);
        cout << ">> Opening file " << fname << " ......" << endl;
        cout << endl;
        if (!file.is_open ())
        {			// terminate if the file can't be opened
            cerr << "!! File open error:" << fname << endl;
            return 1;
        }
    }

    else
    {				// terminate if there is no input file or more than 1 input file
        cerr << "!! No input file" << endl;
        return 1;
    }

    // automatically change XXXX.dat to XXXX.root
    int file_len = strlen (fname);
    string filename = fname;
    filename.replace (file_len - 3, 3, "root");

    char EventHeader[5];
    int SerialNumber;
    char Date[17];
    float EventTime[1024];
    unsigned short ChannelDataRaw[1024];
    char ChannelHeader[5];
    //bool endoffile = false;

    file.read ((char *) &EventHeader, 4);
    EventHeader[4] = '\0';

    //bool loopchannel = true;
    //while(true)
    //{
    file.read ((char *) &SerialNumber, 4);
    file.read ((char *) &Date, 16);
    Date[16] = '\0';
    file.read ((char *) &EventTime, 4096);

    cout << EventHeader << SerialNumber << " " << EventTime << "\n";

    file.read ((char *) &ChannelHeader, 4);
    ChannelHeader[4] = '\0';
    file.read ((char *) &ChannelDataRaw, 2048);
    cout << ChannelHeader << "  " << ChannelDataRaw << "\n";

    cout << "\n";

    file.read ((char *) &EventHeader, 4);
    EventHeader[4] = '\0';
    file.read ((char *) &SerialNumber, 4);
    file.read ((char *) &Date, 16);
    file.read ((char *) &EventTime, 4096);
    cout << EventHeader << SerialNumber << " " << EventTime << "\n";
    file.read ((char *) &ChannelHeader, 4);
    ChannelHeader[4] = '\0';
    file.read ((char *) &ChannelDataRaw, 2048);
    cout << ChannelHeader << "  " << ChannelDataRaw << "\n";
    /*
    //while (loopchannel)
    //{
    file.read ((char *) &ChannelHeader, 4);
    ChannelHeader[4] = '\0';
    if (strcmp (ChannelHeader, "EHDR") == 0)
    {
    break;
    }

    else if (file.eof ())
    {
    endoffile = true;
    break;
    }

    else
    {
    //                cout << "CHANNELHEADER: " << ChannelHeader << "\n";
    file.read ((char *) &ChannelDataRaw, 2048);
    //                cout << "CHANNELDATA: " << ChannelDataRaw << "\n";
    cout << ChannelHeader << " " << ChannelDataRaw << "\n";
    }

    }


    if (file.eof ())
    {
        endoffile = true;
        break;
    }

    */
    //}
    file.close();
    return 0;
}

//void test500ped(void) {
//  UShort_t ped[1024]; // an array of 16 bit unsigned integers
//  FILE *fin = fopen("test500ped.dat", "r");
//  if (!fin) {
//    printf("Error : test500ped.dat not found!\n");
//    return;
//  }
//  TFile *fout = TFile::Open("test500ped.root", "recreate");
//  TTree *tree = new TTree("tree", "A TTree from test500ped.dat");
//  // "==> Case A" in http://root.cern.ch/root/html/TTree.html
//  tree->Branch("ped", ped, "ped[1024]/s"); // "/s" = "UShort_t"
//  while ( sizeof(ped) == fread(ped, 1, sizeof(ped), fin) ) {
//#if 0 /* 0 or 1 */
//    // swap the high and low bytes (endianness correction)
//    for (Int_t i = 0; i < ((Int_t)sizeof(ped)); i += 2) {
//      UChar_t c = *((UChar_t *)ped + i);
//      *((UChar_t *)ped + i) = *((UChar_t *)ped + i + 1);
//      *((UChar_t *)ped + i + 1) = c;
//    }
//#endif /* 0 or 1 */
//    tree->Fill();
//  }
//  fclose(fin); // no longer needed
//  tree->Write();
//  delete fout; // automatically deletes the "tree", too
//  return;
//}
