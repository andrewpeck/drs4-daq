/*
 * TriggerDialog.cpp
 * Modal Trigger Configuration Dialog class
 * $Id: TriggerDialog.cpp 17660 2011-05-12 10:14:28Z ritt $
 */

#include "DRSOscInc.h"

TriggerDialog::TriggerDialog( wxWindow* parent )
:
TriggerDialog_fb( parent )
{
   m_frame = (DOFrame *)parent;

   int tc = m_frame->GetTriggerConfig();
   m_cbOR1->SetValue((tc & (1<<0))>0);
   m_cbOR2->SetValue((tc & (1<<1))>0);
   m_cbOR3->SetValue((tc & (1<<2))>0);
   m_cbOR4->SetValue((tc & (1<<3))>0);
   m_cbOREXT->SetValue((tc & (1<<4))>0);

   m_cbAND1->SetValue((tc & (1<<8))>0);
   m_cbAND2->SetValue((tc & (1<<9))>0);
   m_cbAND3->SetValue((tc & (1<<10))>0);
   m_cbAND4->SetValue((tc & (1<<11))>0);
   m_cbANDEXT->SetValue((tc & (1<<12))>0);
}

void TriggerDialog::OnClose( wxCommandEvent& event )
{
   this->Hide();
}

void TriggerDialog::OnButton( wxCommandEvent& event )
{
   m_frame->SetTriggerConfig(event.GetId(), event.IsChecked()); 
}
