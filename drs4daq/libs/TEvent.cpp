#include "TEvent.h"
ClassImp(TEvent);

void TEvent::ShowWave(int ch)
{
	TH1D *h1 = new TH1D("h1","a pulse",1024,0,1023);
	for(int i=0;i<1024;i++)
	{
		h1->Fill(i,fpWaveArray[ch][i]);
	}
	h1->Draw();
}

TEvent::TEvent()
{
	fId = 0;
	for (int i=0;i<4;i++)
	{
		for (int j=0;j<1024;j++)
			fpWaveArray[i][j]=0;
	}
}

void TEvent::WriteWave(int chn,float *pwave)
{
	for (int i=0;i<1024;i++)
		fpWaveArray[chn][i] = pwave[i];
}

void TEvent::GetWave(int chn,float *pwave)
{
	for (int i=0;i<1024;i++)
		pwave[i] = fpWaveArray[chn][i];
}
