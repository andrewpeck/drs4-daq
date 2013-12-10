#include "DRS.h"
#include "strlcpy.h"
class ProgressBar : public DRSCallback
{
public:
   void Progress(int prog);
};

void ProgressBar::Progress(int prog)
{
   if (prog == 0)
      printf("[--------------------------------------------------]\r");
   printf("[");
   for (int i=0 ; i<prog/2 ; i++)
      printf("=");
   printf("\r");
   fflush(stdout);
}
int kbhit()
{
   int n;

   ioctl(0, FIONREAD, &n);
   return (n > 0);
}
