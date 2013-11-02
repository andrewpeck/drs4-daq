#ifndef __TriggerDialog__
#define __TriggerDialog__

/*
$Id: TriggerDialog.h 17646 2011-05-11 15:21:02Z ritt $
*/

class DOFrame;

#include "DRSOsc.h"

/** Implementing TriggerDialog_fb */
class TriggerDialog : public TriggerDialog_fb
{
protected:
   // Handlers for TriggerDialog_fb events.
   void OnClose( wxCommandEvent& event );
   void OnButton( wxCommandEvent& event );
   
public:
   /** Constructor */
   TriggerDialog( wxWindow* parent );

private:
   DOFrame *m_frame;
};

#endif // __TriggerDialog__
