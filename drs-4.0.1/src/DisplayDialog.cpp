/*
 * DisplayDialog.cpp
 * Modeless Displayuration Dialog class
 * $Id: DisplayDialog.cpp 15274 2010-05-14 15:11:20Z ritt $
 */

#include "DRSOscInc.h"

DisplayDialog::DisplayDialog( wxWindow* parent )
:
DisplayDialog_fb( parent )
{
   m_frame = (DOFrame *)parent;
   m_osci  = m_frame->GetOsci();
}

void DisplayDialog::OnDateTime( wxCommandEvent& event )
{
   m_frame->SetDisplayDateTime(event.IsChecked());
}

void DisplayDialog::OnShowGrid( wxCommandEvent& event )
{
   m_frame->SetDisplayShowGrid(event.IsChecked());
}

void DisplayDialog::OnLines( wxCommandEvent& event )
{
   m_frame->SetDisplayLines(event.IsChecked());
}

void DisplayDialog::OnDisplayMode( wxCommandEvent& event )
{
   long n;
   m_cbNumber->GetValue().ToLong(&n);

   if (event.GetId() == ID_DISPSAMPLE)
      m_frame->SetDisplayMode(ID_DISPSAMPLE, 0);
   else if (event.GetId() == ID_DISPAVERAGE)
      m_frame->SetDisplayMode(ID_DISPAVERAGE, n);
   else if (event.GetId() == ID_DISPPERSIST)
      m_frame->SetDisplayMode(ID_DISPPERSIST, n);
   else if (event.GetId() == ID_DISPNUMBER)
      m_frame->SetDisplayMode(m_rbShowAverage->GetValue()?ID_DISPAVERAGE:ID_DISPPERSIST, n);
}

void DisplayDialog::OnButton( wxCommandEvent& event )
{
   m_frame->SetMathDisplay(event.GetId(), event.IsChecked());
}

void DisplayDialog::OnClose( wxCommandEvent& event )
{
   this->Hide();
}
