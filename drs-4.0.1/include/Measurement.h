/*
$Id: Measurement.h 18557 2011-10-28 13:21:44Z ritt $
*/

class DOScreen;

/** Implementing MeasureDialog_fb */
class Measurement
{
protected:
   wxString m_name;
   int      m_index;
   double   m_value;
   double   *m_statArray;
   int      m_statIndex;
   int      m_nMeasured;
   int      m_nStat;
   double   m_vsum;
   double   m_vvsum;
   double   m_min;
   double   m_max;
   
public:
   /** Constructor & Desctructor */
   Measurement(int index);
   ~Measurement();

   wxString GetName();
   wxString GetUnit();
   double Measure(double *x1, double *y1, double *x2, double *y2, int n, bool update, DOScreen *s);
   void Measure(double *x1, double *y1, double *x2, double *y2, int n);
   wxString GetString();
   wxString GetStat();
   void SetNStat(int n);
   int GetNStat() { return m_nStat; }
   int GetNMeasured() { return m_nMeasured; }
   void ResetStat();
   double *GetArray() { return m_statArray; }

   static const int N_MEASUREMENTS = 10;

private:
   double MLevel(double *x1, double *y1, int n, DOScreen *s);
   double MPkPk(double *x1, double *y1, int n, DOScreen *s);
   double MRMS(double *x1, double *y1, int n, DOScreen *s);
   double MFreq(double *x1, double *y1, int n, DOScreen *s);
   double MPeriod(double *x1, double *y1, int n, DOScreen *s);
   double MRise(double *x1, double *y1, int n, DOScreen *s);
   double MFall(double *x1, double *y1, int n, DOScreen *s);
   double MPosWidth(double *x1, double *y1, int n, DOScreen *s);
   double MNegWidth(double *x1, double *y1, int n, DOScreen *s);
   double MChnDelay(double *x1, double *y1, double *x2, double *y2, int n, DOScreen *s);
};
