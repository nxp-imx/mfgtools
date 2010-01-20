// ADSTkConfigure.h: interface for the CADSTkConfigure class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ADSTKCONFIGURE_H)
#define AFX_ADSTKCONFIGURE_H

//#include "ConfigurePage.h"
//#include "MXDefine.h"
#include "MemoryInit.h"

using namespace std;

#define DEFAULT_CONF_FILE	"config\\ADSToolkit.cfg"
#define DEFAULT_INI_FILE    "config\\ConfigInit.ini"
#define FLASH_MODEL_ENTRY	"[Flash Model]"
#define USB_CONNECT_TIMEOUT	20 //connect timeout count used in re-connect usb case
#define USB_OPEN_USR_TIMEOUT 30  // in secs
#define USB_TRANS_USR_TIMEOUT 30000 //in msecs

typedef enum
{
	MEMORY_START_ADDR = 0,
	MEMORY_END_ADDR,
	DEFAULT_RKL_ADDR,
	DEFAULT_HWC_ADDR,
	DEFAULT_CSF_ADDR,
	DEFAULT_CHANNEL_ADDR,						
	DEFAULT_NOR_FLASH_ADDR,
	INT_RAM_START,
	INT_RAM_END,
	NR_MEMORY_ADDR,

} MEMORY_ADDR_ID_T;

typedef enum 
{
	ChannelType_UART = 0,
	ChannelType_USB,
} CHANNEL_T;

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CADSTkConfigure  
{
public:
	BOOL isChannelEnabled();
	unsigned long GetMemoryAddr(MEMORY_ADDR_ID_T id);
	void GetChannel(int *type, int *id, unsigned long *handle);
	stMemoryInit * GetMemoryInitScript(int *lines);
	int GetMXType();
	UINT GetRAMKNLAddr();
	BOOL GetSecurity();
	BOOL GetByPass();
	void SetChannel(int type, int id, unsigned long handle);

	CADSTkConfigure();
	virtual ~CADSTkConfigure();
	void SetMemoryAddr(unsigned long mmAddrs[]);
	void SetMemoryType(stMemoryInit *pScript, int lines);
	void SetMXType(int type);
	void SetRAMKNLAddr(UINT RAMKNLAddr);
	MM_T GetRAMType();
	void SetRAMType(MM_T type);
	void SetSecurity(BOOL bSecurity);
	void SetByPass(BOOL bBypass);
//	friend class CConfigurePage;

private:
	unsigned long m_uChannelHandle;
	int m_iScriptLine;	// memory script size on heap
	unsigned long m_uMemAddrs[NR_MEMORY_ADDR];
	int m_iMXType;
	UINT m_RAMKNLAddr;
	BOOL m_bByPass;
	BOOL m_bSecurity;
	stMemoryInit *m_pScript;	// used on heap
	int m_iChannelType;		// USB or UART
	int m_iChannelID;		// USB default ID or UART com port id
	MM_T m_iMemoryType;
};

#endif // !defined(AFX_ADSTKCONFIGURE_H)
