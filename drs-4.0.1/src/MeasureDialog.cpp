/*
 * ConfigDialog.cpp
 * Modal Measurement Configuration Dialog class
 * $Id: MeasureDialog.cpp 18557 2011-10-28 13:21:44Z ritt $
 */

#include "DRSOscInc.h"

MeasureDialog::MeasureDialog( wxWindow* parent )
:
MeasureDialog_fb( parent )
{
   m_frame = (DOFrame *)parent;
}

void MeasureDialog::OnClose( wxCommandEvent& event )
{
   this->Hide();
}

void MeasureDialog::OnButton( wxCommandEvent& event )
{
   m_frame->SetMeasurement(event.GetId(), event.IsChecked());
}

void MeasureDialog::OnStat( wxCommandEvent& event )
{
   m_frame->SetStat(event.IsChecked());
}

void MeasureDialog::OnHist( wxCommandEvent& event )
{
   m_frame->SetHist(event.IsChecked());
}

void MeasureDialog::OnStatNAverage( wxCommandEvent& event )
{
   wxString str = m_cbNAverage->GetValue();
   char buf[100];
   strcpy( buf, (const char*)str.mb_str(wxConvUTF8) );
   m_frame->SetStatNStat(atoi(buf));
}

void MeasureDialog::OnIndicator( wxCommandEvent& event )
{
   m_frame->SetIndicator(event.IsChecked());
}

void MeasureDialog::OnStatReset( wxCommandEvent& event )
{
   m_frame->StatReset();
}
