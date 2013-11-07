{
	gROOT->ProcessLine(".L ../obj/TEvent.so");
	TFile *f = new TFile("../output/run00024.root");
	TTree *t = (TTree*) f->Get("t");
	TEvent *evt = new TEvent();
	t->SetBranchAddress("evt",&evt);

	TCanvas *c1 = new TCanvas("c1","c1",20,20,500,300);
	TH1D *h1 = new TH1D("h1","Waveform",1024,0,1023);
	h1->SetLineColor(kRed);
	h1->SetStats(0);
	int nevt = t->GetEntries();

	TString input;
	for(int i=0;i<nevt;i++)
	{
		t->GetEntry(i);
		float wave[1024];
		evt->GetWave(1,wave);
		for (int j=0;j<1024;j++)
			h1->Fill(j,wave[j]);
		h1->Draw();
		c1->Update();
		//c1->WaitPrimitive();
		cout <<"Press q to quit, Enter to go to next event"<<endl;
		char c;
		c = getchar();

		//cin >> input;
		//if(input == "q")
		if(c == 'q')
			break;
		else 
			h1->Reset("ICESM");
	}
}
