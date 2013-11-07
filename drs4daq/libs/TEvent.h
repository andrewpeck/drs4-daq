#ifndef TEvent_h
#define TEvent_h

#include "TObject.h"
#include "TString.h"
#include "TH1D.h"
#include "TApplication.h"
#include <iostream>

class TEvent : public TObject {
	private:
		int		fId; //event id
		float	fpWaveArray[4][1024]; 
	public:
		TEvent(); 
		TEvent(int id) {fId = id;}
		~TEvent() {;}
		inline void SetId(int i){fId = i;}
		inline int GetId(){ return fId;}
		inline void ShowId() {printf("Event: %d\n",fId);}
		void WriteWave(int chn,float *pwave);
		void GetWave(int chn, float *pwave);
		void ShowWave(int ch);

		ClassDef(TEvent, 1);
};
#endif
