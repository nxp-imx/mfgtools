// StProgress.cpp: implementation of the CStProgress class.
//
//////////////////////////////////////////////////////////////////////
#include "StHeader.h"
#include "StGlobals.h"
#include "StProgress.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//
// synchronizing progress bar with tasks running.
// ----------------------------------------------
// all progress bar threads wait for m_start_event to signal.
// m_start_event signals when someone calls CStProgress::UpdateProgress(task).
// the caller after finishing the task will wait on m_end_event to complete the progress bar if it has not completed.
// the thread that completed the progress bar will signal m_end_event
// the calling will resume the next task.
//


CStProgress::CStProgress(string _name)
{
	m_in_progress = FALSE;
}

CStProgress::~CStProgress()
{
}

ST_ERROR CStProgress::Begin()
{ 
	m_in_progress = TRUE; 
	return STERR_NONE;
}

BOOL CStProgress::InProgress()
{ 
	return m_in_progress; 
}

void CStProgress::Relax()
{
	m_in_progress = FALSE;
}
