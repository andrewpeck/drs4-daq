
#ifndef CreatePlots_h
#define CreatePlots_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.

// Fixed size dimensions of array or collections stored in the TTree if any.

class CreatePlots {
public :
  TTree          *fChain;   //!pointer to the analyzed TTree or TChain
  Int_t           fCurrent; //!current Tree number in a TChain

  // Declaration of leaf types
  Int_t           eventNum;
  Int_t           processNum;
  Double_t        primaryEnergy;
  Double_t        gunTheta;
  Double_t        gunPhi;
  Double_t        hitPointX;
  Double_t        hitPointY;
  Double_t        scintEdepTotal;
  Double_t        scintEdepMax;
  Double_t        scintTimeAvg;
  Double_t        scintTimeMax;
  Int_t           pmtHitCount[2];
  Double_t        pmtEnergies[2];
  Int_t           enteredLG[2];
  Double_t        peakVoltage[2];
  Double_t        totalCharge[2];
  Double_t        CTHtime[2];
  Double_t        FTHtime[2];

  // List of branches
  TBranch        *b_eventNum;   //!
  TBranch        *b_processNum;   //!
  TBranch        *b_primaryEnergy;   //!
  TBranch        *b_gunTheta;   //!
  TBranch        *b_gunPhi;   //!
  TBranch        *b_hitPointX;   //!
  TBranch        *b_hitPointY;   //!
  TBranch        *b_scintEdepTotal;   //!
  TBranch        *b_scintEdepMax;   //!
  TBranch        *b_scintTimeAvg;   //!
  TBranch        *b_scintTimeMax;   //!
  TBranch        *b_pmtHitCount;   //!
  TBranch        *b_pmtEnergies;   //!
  TBranch        *b_enteredLG;   //!
  TBranch        *b_peakVoltage;   //!
  TBranch        *b_totalCharge;   //!
  TBranch        *b_CTHtime;   //!
  TBranch        *b_FTHtime;   //!

  CreatePlots(TTree *tree=0);
  virtual ~CreatePlots();
  virtual Int_t    Cut(Long64_t entry);
  virtual Int_t    GetEntry(Long64_t entry);
  virtual Long64_t LoadTree(Long64_t entry);
  virtual void     Init(TTree *tree);
  virtual void     Loop();
  virtual Bool_t   Notify();
  virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef CreatePlots_cxx
CreatePlots::CreatePlots(TTree *tree) : fChain(0) 
{
  // if parameter tree is not specified (or zero), connect the file
  // used to generate this class and read the Tree.
  if (tree == 0) {
    TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("dataTree.root");
    if (!f || !f->IsOpen()) {
      f = new TFile("dataTree.root");
    }
    f->GetObject("dataTree",tree);
      
  }
  Init(tree);
}

CreatePlots::~CreatePlots()
{
  if (!fChain) return;
  delete fChain->GetCurrentFile();
}

Int_t CreatePlots::GetEntry(Long64_t entry)
{
  // Read contents of entry.
  if (!fChain) return 0;
  return fChain->GetEntry(entry);
}
Long64_t CreatePlots::LoadTree(Long64_t entry)
{
  // Set the environment to read one entry
  if (!fChain) return -5;
  Long64_t centry = fChain->LoadTree(entry);
  if (centry < 0) return centry;
  if (fChain->GetTreeNumber() != fCurrent) {
    fCurrent = fChain->GetTreeNumber();
    Notify();
  }
  return centry;
}

void CreatePlots::Init(TTree *tree)
{
  // The Init() function is called when the selector needs to initialize
  // a new tree or chain. Typically here the branch addresses and branch
  // pointers of the tree will be set.
  // It is normally not necessary to make changes to the generated
  // code, but the routine can be extended by the user if needed.
  // Init() will be called many times when running on PROOF
  // (once per file to be processed).

  // Set branch addresses and branch pointers
  if (!tree) return;
  fChain = tree;
  fCurrent = -1;
  fChain->SetMakeClass(1);

  fChain->SetBranchAddress("eventNum", &eventNum, &b_eventNum);
  fChain->SetBranchAddress("processNum", &processNum, &b_processNum);
  fChain->SetBranchAddress("primaryEnergy", &primaryEnergy, &b_primaryEnergy);
  fChain->SetBranchAddress("gunTheta", &gunTheta, &b_gunTheta);
  fChain->SetBranchAddress("gunPhi", &gunPhi, &b_gunPhi);
  fChain->SetBranchAddress("hitPointX", &hitPointX, &b_hitPointX);
  fChain->SetBranchAddress("hitPointY", &hitPointY, &b_hitPointY);
  fChain->SetBranchAddress("scintEdepTotal", &scintEdepTotal, &b_scintEdepTotal);
  fChain->SetBranchAddress("scintEdepMax", &scintEdepMax, &b_scintEdepMax);
  fChain->SetBranchAddress("scintTimeAvg", &scintTimeAvg, &b_scintTimeAvg);
  fChain->SetBranchAddress("scintTimeMax", &scintTimeMax, &b_scintTimeMax);
  fChain->SetBranchAddress("pmtHitCount", pmtHitCount, &b_pmtHitCount);
  fChain->SetBranchAddress("pmtEnergies", pmtEnergies, &b_pmtEnergies);
  fChain->SetBranchAddress("enteredLG", enteredLG, &b_enteredLG);
  fChain->SetBranchAddress("peakVoltage", peakVoltage, &b_peakVoltage);
  fChain->SetBranchAddress("totalCharge", totalCharge, &b_totalCharge);
  fChain->SetBranchAddress("CTHtime", CTHtime, &b_CTHtime);
  fChain->SetBranchAddress("FTHtime", FTHtime, &b_FTHtime);
  Notify();
}

Bool_t CreatePlots::Notify()
{
  // The Notify() function is called when a new file is opened. This
  // can be either for a new TTree in a TChain or when when a new TTree
  // is started when using PROOF. It is normally not necessary to make changes
  // to the generated code, but the routine can be extended by the
  // user if needed. The return value is currently not used.

  return kTRUE;
}

void CreatePlots::Show(Long64_t entry)
{
  // Print contents of entry.
  // If entry is not specified, print current entry
  if (!fChain) return;
  fChain->Show(entry);
}
Int_t CreatePlots::Cut(Long64_t entry)
{
  // This function may be called from Loop.
  // returns  1 if entry is accepted.
  // returns -1 otherwise.
  return 1;
}
#endif // #ifdef CreatePlots_h
