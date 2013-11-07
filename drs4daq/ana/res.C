void res()
{
	gROOT->ProcessLine(".L ../obj/TEvent.so");
	gStyle->SetOptFit(0111);
	TH1 *h1 = new TH1D("h1","PHS, E1, run 6",500,0,500);
	TH1 *h2 = new TH1D("h2","PHS, E2, run 7",500,0,500);
	TH1 *h3 = new TH1D("h3","PHS, E3, run 8",500,0,500);
	TH1 *h4 = new TH1D("h4","PHS, E4, run 9",500,0,500);
	TH1 *htmp = new TH1D("htmp","PHS",1000,0,500);
	h1->SetLineColor(kBlue);
	h2->SetLineColor(kBlue);
	h3->SetLineColor(kBlue);
	h4->SetLineColor(kBlue);
	makehist(6,h1);
	makehist(7,h2);
	makehist(8,h3);
	makehist(9,h4);

	TSpectrum *s = new TSpectrum(10);
	h = h1->Clone();
	s->Search(h1,5,"nobackground",0.10);
	float *xpeak = s->GetPositionX();
	h1->Fit("gaus","","",xpeak[0],500);
	h = h2->Clone();
	s->Search(h2,5,"nobackground",0.10);
	float *xpeak = s->GetPositionX();
	h2->Fit("gaus","","",xpeak[0],500);
	h = h1->Clone();
	s->Search(h3,5,"nobackground",0.10);
	float *xpeak = s->GetPositionX();
	h3->Fit("gaus","","",xpeak[0],500);
	h = h4->Clone();
	s->Search(h4,5,"nobackground",0.10);
	float *xpeak = s->GetPositionX();
	h4->Fit("gaus","","",xpeak[0],500);
	
	TCanvas *c1 = new TCanvas("c1","c1",20,20,1000,800);
	c1->Divide(2,2);
	c1->cd(1); h1->Draw();
	c1->cd(2); h2->Draw();
	c1->cd(3); h3->Draw();
	c1->cd(4); h4->Draw();
	TString pdfFile = TString::Format("ResolutionE.pdf"); 
	TString pngFile = TString::Format("ResolutionE.png"); 
	c1->Print(pdfFile);
	c1->Print(pngFile);
}
void makehist(int runNo,TH1 *h)
{
	TString filename = TString::Format("../output/run%05d.root",runNo);
	TFile *f = new TFile(filename);
	TTree *t = (TTree*) f->Get("t");
	TEvent *evt = new TEvent();
	t->SetBranchAddress("evt",&evt);

	int nevt = t->GetEntries();

	for(int i=0;i<nevt;i++)
	{
		t->GetEntry(i);
		float wave[1024];
		evt->GetWave(1,wave);
		float max = wave[0];
		for (int j=0;j<1024;j++)
		{
			//h1->Fill(j,wave[j]);
			//cout<<h1->GetMaximum()<<endl;
			if (wave[j]>max) max = wave[j];
		}
		h->Fill(max);
	}
	//h->Draw();
	f->Close();
}
