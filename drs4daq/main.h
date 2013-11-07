#ifndef main_hh
#define main_hh 1

#include <math.h>

#ifdef _MSC_VER
#include <windows.h>
#elif defined(OS_LINUX)
#define O_BINARY 0
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <errno.h>
#define DIR_SEPARATOR '/'
#endif

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <unistd.h> //for getopt
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <assert.h>

#include "strlcpy.h"
#include "DRS.h"

#include "TROOT.h"
#include "TApplication.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TH1D.h"

#include "libs/TEvent.h"
/*------------------------------------------------------------------*/
int Scan();
//Scan for boards
int Init(int boardNo=0);
//Initialize the board, with a sampling frequency and range setting
//default values are: 1GHz, and -0.5 to +0.5 V ( midpoint = 0V)
//Returns 0 if no board is found, 1 if things OK
void SetTrigger(int ch,double level, double delay,bool edge);
//Settings for trigger: source 0: soft trigger, 1: ch1, 2:ch2, 3:ch3, 4:ch4, 5: external
//level in V; delay in ns; edge: true: negative edge, false: pos edge
int ShowHelp();
void ShowEvent();
void ShowEvent()
{
}
int Info();
int Calibrate();
int Measure();
int ShowHelp()
{
	printf("Usage: ./r [option]\n");
	printf("If no option is provided, the program will perform measurement with default settings: sampling rate 1 GHz, input range -0.5 - +0.5 V, software (forced) trigger, delay 0 ns, result will be written to output/test.root\n");
	printf("Available options are:\n");
	printf("\t-h: show this help \n"); 
	printf("\t-i: show some information of the board, for testing purpose\n");
	printf("\t-c: do voltage calibration for the board, it will ask for sampling rate and range \n"); 
//	printf("\t-m: do measurement, optional frequency, trigger source and trigger level can be set using -f, -l, -t option \n"); 
	printf("\t-o: set ouput file name \n");
	printf("\t-r: set run number, then the output file will be output/runXXXXX.root\n");
	printf("\t-l: set trigger level in V \n");
	printf("\t-f: set sampling rate (frequency) in GHz\n");
	printf("\t-t: set trigger source, 1: ch1, 2:ch2, 3:ch3, 4:ch4, 5: external\n");
	printf("\t-i: set midpoint of the input range in V, the range will be [midpoint - 0.5; midpoint +0.5]. Acceptable midpoint is from 0.0 V to 0.5 V\n");
	return 0;
}
std::string Timestamp();
std::string Timestamp()
{
	std::string s="";
	char cstr[500];
	time_t ltime;
	ltime = time(NULL);
//	struct tm *Tm;
//	Tm	  = localtime(&ltime);
//	sprintf(cstr,"%d/%02d/%d, %d:%d:%d",
//			Tm->tm_year + 1900,
//			Tm->tm_mon +1,
//			Tm->tm_mday,
//			Tm->tm_hour,
//			Tm->tm_min,
//			Tm->tm_sec);
	sprintf(cstr,"%s",ctime(&ltime));
	s.append(cstr);
	s.resize(s.size()-1);
	return s;
}
#endif
