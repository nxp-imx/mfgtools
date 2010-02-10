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

MM_T CADSTkConfigure::GetRAMType()
{
	return m_iMemoryType;
}

void CADSTkConfigure::SetRAMType(MM_T type)
{
	m_iMemoryType = type;
}

char* CADSTkConfigure::GetMemoryTypeName(int mx_Type, MM_T mm_Type)
{
	char typeName[256]; 
	ZeroMemory(typeName,256);
	switch(mm_Type)
	{
		case MM_DDR:
			strcpy_s(typeName, "ddr");
			break;
	    case MM_SDRAM:
			strcpy_s(typeName, "sdr");
			break;	    
	    case MM_DDR2:
			strcpy_s(typeName, "ddr2");
			break;
	    case MM_MDDR:
			strcpy_s(typeName, "mddr");
			break;
		case MM_CUSTOM: 
			break;
	}
	
	switch(mx_Type)
	{
		case MX_MX25_TO1:
			strcat_s(typeName, "Mx25");
			break;
		case MX_MX25_TO11:
			strcat_s(typeName, "Mx25_To11");
			break;
		case MX_MX27_TO1:
			strcat_s(typeName, "Mx27");
			break;
		case MX_MX27_TO2:
			strcat_s(typeName, "Mx27");
			break;
		case MX_MX31_TO1:
			strcat_s(typeName, "Mx31");
			break;
		case MX_MX31_TO2:
			strcat_s(typeName, "Mx31");
			break;
		case MX_MX31_TO201:
			strcat_s(typeName, "Mx31");
			break;
		case MX_MX32:
			strcat_s(typeName, "Mx32");
			break;
		case MX_MX35_TO1:
			strcat_s(typeName, "Mx35");
			break;
		case MX_MX35_TO2:
			strcat_s(typeName, "Mx35");
			break;
		case MX_MX37:
			strcat_s(typeName, "Mx37");
			break;
		case MX_MX51_TO1:
			strcat_s(typeName, "Mx51");
			break;
		case MX_MX51_TO2:
			strcat_s(typeName, "Mx51_To2");
			break;
	}

	return typeName;
}

//
void CADSTkConfigure::SetMemoryType(int mx_Type,MM_T mm_Type,CString cFilePath)
{
	int nameFind,strBegin,strEnd,dataBegin,dataEnd,lines;
	char cMXTypeName[256];
	ZeroMemory(cMXTypeName, 256);
	CStringT<char,StrTraitMFC<char> > mmString,csDataString,csTemp,csData;
	CFile memoryFile;
	CFileException fileException;

	// Open memory file
	if( !memoryFile.Open(cFilePath, CFile::modeRead | CFile::shareDenyNone, &fileException) )
	{
		TRACE( _T("Can't open file %s, error = %u\n"), cFilePath, fileException.m_cause );
		AfxMessageBox(_T("Failed to open Mx Memory initialization file.\n\nMake sure there is the file. "), MB_OK);                
        return;
	}

	// Read memory file
	memoryFile.Read(mmString.GetBufferSetLength(memoryFile.GetLength()), memoryFile.GetLength());
	mmString.ReleaseBuffer();

	// Close file handl
	memoryFile.Close();

	// Get the memerytype name
	strcpy_s(cMXTypeName, GetMemoryTypeName(mx_Type, mm_Type));
	
	nameFind = mmString.Find(cMXTypeName, 0);
	if(nameFind == -1)
	{
		TRACE( _T("There is no the defines in the file %s \n"), cMXTypeName);
        return;
	}
	strBegin = mmString.Find( "{" , nameFind) + 1;// string begins after the first "{" and befor the second "{"
	strEnd = mmString.Find( "};" , nameFind);
	csDataString = mmString.Mid(strBegin, strEnd - strBegin);

	// Judge how much lines
	strEnd = strBegin = 0;
	// Get the valid data string
	while((strBegin = csDataString.Find( "//{" , strEnd)) != -1)
	{
		strEnd = csDataString.Find( "}," , strBegin) + 2;
		csDataString.Delete(strBegin,strEnd - strBegin);
		strEnd = strBegin = 0;
	}
	strEnd = strBegin = lines = 0;
	while( (strEnd = csDataString.Find( "}," , strBegin)) != -1)
	{
		strBegin = strEnd + 2;//string begins after "},"
		lines++;
	}

	if (m_pScript)
	{
		delete [] m_pScript;
	}
	m_pScript = new stMemoryInit[lines];
	m_iScriptLine = lines;
	// get the items
	strEnd = strBegin = 0;
	for (int i=0; i<lines; i++)
	{
		// Get each string item:{0xXXXX,0xXXXX,XX}
		strBegin = csDataString.Find( "{" , strEnd) + 1;//string begins after "{"
		strEnd = csDataString.Find( "}," , strBegin);
		csTemp= csDataString.Mid(strBegin, strEnd - strBegin);

		// Set each string item to m_pScript structure
		dataBegin = dataEnd = 0;
		dataBegin = csTemp.Find( "0x" , dataEnd);
		dataEnd = csTemp.Find( "," , dataBegin);
		csData = csTemp.Mid(dataBegin, dataEnd - dataBegin);
		m_pScript[i].addr = strtoul(csData,NULL,16);
		dataBegin = csTemp.Find( "0x" , dataEnd);
		dataEnd = csTemp.Find( "," , dataBegin);
		csData = csTemp.Mid(dataBegin, dataEnd - dataBegin);
		m_pScript[i].data = strtoul(csData,NULL,16);
		csTemp.Delete(0,dataEnd + 1);
		m_pScript[i].format= strtoul(csTemp,NULL,10);;
	}
	
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