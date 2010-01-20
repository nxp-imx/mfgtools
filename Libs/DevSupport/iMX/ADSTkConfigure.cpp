// ADSTkConfigure.cpp: implementation of the CADSTkConfigure class.
//
//////////////////////////////////////////////////////////////////////

#include "../stdafx.h"
#include "ADSTkConfigure.h"
//#include "AtkHostApiClass.h"
//#include <fstream>
//#include <string>

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CADSTkConfigure::CADSTkConfigure()
{
	m_bSecurity = FALSE;
	m_iMXType = MX_UNKNOW;
	m_pScript = NULL;
	m_iScriptLine = 0;
	m_iChannelID = 0;
	m_iChannelType = ChannelType_USB;
	m_bByPass = FALSE;
	m_iMemoryType = MM_DDR;
}

CADSTkConfigure::~CADSTkConfigure()
{
	if (m_pScript)
	{
		delete [] m_pScript;
	}
}

BOOL CADSTkConfigure::GetSecurity()
{
	return m_bSecurity;
}

void CADSTkConfigure::SetSecurity(BOOL bSecurity)
{
	m_bSecurity = bSecurity;
}

BOOL CADSTkConfigure::GetByPass()
{
	return m_bByPass;
}

void CADSTkConfigure::SetByPass(BOOL bByPass)
{
	m_bByPass = bByPass;
}

int CADSTkConfigure::GetMXType()
{
	return m_iMXType;
}

void CADSTkConfigure::SetMXType(int type)
{
	m_iMXType = type;
}

UINT CADSTkConfigure::GetRAMKNLAddr()
{
	return m_RAMKNLAddr;
}

void CADSTkConfigure::SetRAMKNLAddr(UINT RAMKNLAddr)
{
	m_RAMKNLAddr = RAMKNLAddr;
}

MM_T CADSTkConfigure::GetRAMType()
{
	return m_iMemoryType;
}

void CADSTkConfigure::SetRAMType(MM_T type)
{
	m_iMemoryType = type;
}

// you must setup the MX type first
void CADSTkConfigure::SetMemoryType(stMemoryInit *pScript, int lines)
{
	if (m_pScript)
	{
		delete [] m_pScript;
	}
	m_pScript = new stMemoryInit[lines];
	memcpy((void *)m_pScript, (const void *)pScript, sizeof(stMemoryInit)*lines);
	m_iScriptLine = lines;

}

stMemoryInit * CADSTkConfigure::GetMemoryInitScript(int *lines)
{
	*lines = m_iScriptLine;
	return m_pScript;
}

void CADSTkConfigure::SetChannel(int type, int id, unsigned long handle)
{
	m_iChannelType = type;
	m_iChannelID = id;
	m_uChannelHandle = handle;
}

void CADSTkConfigure::GetChannel(int *type, int *id, unsigned long *handle)
{
	*type = m_iChannelType;
	*id = m_iChannelID;
	*handle = m_uChannelHandle;
}

unsigned long CADSTkConfigure::GetMemoryAddr(MEMORY_ADDR_ID_T id)
{
	if (id >= NR_MEMORY_ADDR)
		return ~0UL;
	return m_uMemAddrs[id];
}

void CADSTkConfigure::SetMemoryAddr(unsigned long mmAddrs[])
{
	// copy the memory address to internal value
	for (int i = 0; i < NR_MEMORY_ADDR; i ++)
	{
		m_uMemAddrs[i] = mmAddrs[i];
	}
}

BOOL CADSTkConfigure::isChannelEnabled()
{
	if (m_uChannelHandle == 0)
		return FALSE;
	return TRUE;
}