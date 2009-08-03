// StDevice.cpp: implementation of the CStDevice class.
//
//////////////////////////////////////////////////////////////////////

#include "StHeader.h"
#include "StVersionInfo.h"
#include "StUpdater.h"
#include "StDevice.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStDevice::CStDevice(CStUpdater* _p_updater, string _name):CStBase(_name)
{
	m_p_updater = _p_updater;
}

CStDevice::~CStDevice()
{

}

CStUpdater*	CStDevice::GetUpdater()
{
	return m_p_updater;
}