/*
 * DRSOscInc.h
 * Collection of all DRS oscilloscope header include files
 * $Id: DRSOscInc.h 18429 2011-09-19 15:24:14Z ritt $
 */

#include "wx/wx.h" 
#include "wx/dcbuffer.h"
#include "wx/print.h"
#include "mxml.h"
#include "DRS.h"
#include "DRSOsc.h"
#include "Osci.h"
#include "Measurement.h"
#include "ConfigDialog.h"
#include "DisplayDialog.h"
#include "AboutDialog.h"
#include "InfoDialog.h"
#include "MeasureDialog.h"
#include "TriggerDialog.h"
#include "DOScreen.h"
#include "DOFrame.h"
#include "EPThread.h"

double ss_nan();
int ss_isnan(double x);
void ss_sleep(int ms);



