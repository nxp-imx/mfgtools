#ifndef   __MXHIDDEVICE_H__
#define   __MXHIDDEVICE_H__
#pragma once
//#include "MxFwComponent.h"
#include "../../Libs/WinSupport/XMLite.h"

// Code from AtkHostApiClass.h
// Could be moved to SDP protocol or MxRomApi or something like that.
#define ROM_KERNEL_CMD_COMPLETE 0x0
#define ROM_KERNEL_CMD_SIZE 0x10

#define MAX_SIZE_PER_FLASH_COMMAND 0x200000//50k
#define MAX_SIZE_PER_DOWNLOAD_COMMAND 0x200000

/* Command Packet Format: Header(2)+Address(4)+Format(1)+ByteCount(4)+Data(4) */
#define ROM_KERNEL_CMD_RD_MEM 0x0101
#define ROM_KERNEL_CMD_WR_MEM 0x0202
#define ROM_KERNEL_CMD_WR_FILE 0x0404
#define ROM_KERNEL_CMD_ERROR_STATUS 0x0505
#define RAM_KERNEL_CMD_HEADER 0x0606
//#define ROM_KERNEL_CMD_RE_ENUM 0x0909
#define ROM_KERNEL_CMD_DCD_WRITE 0x0A0A
#define ROM_KERNEL_CMD_JUMP_ADDR 0x0B0B

#define ROM_WRITE_ACK   0x128A8A12
#define ROM_STATUS_ACK  0x88888888
#define ROM_STATUS_ACK2 0xF0F0F0F0

#define SDP_CMD_SIZE    16      
#define SDP_REPORT_LENGTH (SDP_CMD_SIZE+1)

#define REPORT_ID_SDP_CMD  1
#define REPORT_ID_DATA     2
#define REPORT_ID_HAB_MODE 3
#define REPORT_ID_STATUS   4

#define RAM_KERNEL_ACK_SIZE 0x08
#define RAM_KERNEL_CMD_SIZE 0x10
#define WR_BUF_MAX_SIZE 0x4000

#define MAX_MODEL_LEN	128
#define MINUM_TRANSFER_SIZE 0x20

// Address ranges for Production parts: 

/// <summary>
/// A MxHidDevice device.
/// </summary>
class MxHidDevice : public Device
{
public: 
    class MemoryInitCommand : public XNode
	{
	public:
		// [XmlAttribute("addr")]
		unsigned int GetAddress()
		{ 
			unsigned int addr = 0; // default to 0

			CString attr = GetAttrValue(_T("addr"));

			if(attr.Left(2) == _T("0x"))
			{
				TCHAR *p;
				addr = _tcstoul(attr.Mid(2),&p,16);
			}
			else
			{
				addr = _tstoi64(attr);
			}

			return addr; 
		};

		// [XmlAttribute("data")]
		unsigned int GetData()
		{ 
			unsigned int data = 0; // default to 0

			CString attr = GetAttrValue(_T("data"));

			if(attr.Left(2) == _T("0x"))
			{
				TCHAR *p;
				data = _tcstoul(attr.Mid(2),&p,16);
			}
			else
			{
				data = _tstoi64(attr);
			}

			return data; 
		};

		// [XmlAttribute("format")]
		unsigned int GetFormat()
		{ 
			unsigned char format = 0; // default to 0

			CString attr = GetAttrValue(_T("format"));

			if(attr.Left(2) == _T("0x"))
			{
				TCHAR *p;
				format = _tcstoul(attr.Mid(2),&p,16);
			}
			else
			{
				format = _tstoi64(attr);
			}

			return format; 
		};

		CString ToString()
		{
			CString str;
			str.Format(_T("addr=\"0x%X\" data=\"0x%X\" format=\"%d\""), GetAddress(), GetData(), GetFormat());
			return str;
		}
	};
    
	struct Response
	{
		unsigned int romResponse;
		struct
		{
			unsigned short ack;		// ack
			unsigned short csum;	// data checksum
			unsigned long len;		// data len
		};
	};

	enum MemorySection { MemSectionOTH = 0x00, MemSectionAPP = 0xAA, MemSectionCSF = 0xCC, MemSectionDCD = 0xEE };
	static MemorySection StringToMemorySection(CString section)
	{
		if ( section.CompareNoCase(_T("DCD")) == 0 )
			return MemSectionDCD;
		else if ( section.CompareNoCase(_T("CSF")) == 0 )
			return MemSectionCSF;
		else if ( section.CompareNoCase(_T("APP")) == 0 )
			return MemSectionAPP;
		else return MemSectionOTH;
	}

	enum MemoryAction { MemAction_None, MemAction_Set, MemAction_Jump };
	static MemoryAction StringToMemoryAction(CString action)
	{
		if ( action.CompareNoCase(_T("Set")) == 0 )
			return MemAction_Set;
		else if ( action.CompareNoCase(_T("Jump")) == 0 )
			return MemAction_Jump;
		else return MemAction_None;
	}

	MxHidDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path);
    virtual ~MxHidDevice();
    //BOOL InitMemoryDevice();  
	BOOL InitMemoryDevice(CString filename);
    BOOL Download(StFwComponent *fwComponent, UINT PhyRAMAddr4KRL, MemorySection loadSection, MemorySection setSection, BOOL HasFlashHeader);

    HANDLE	        m_hid_drive_handle;

private:
    enum HAB_t
	{
		HabUnknown  = -1,
		HabEnabled  = 0x12343412,
		HabDisabled = 0x56787856
	};

    enum ChipFamily_t
    {
        ChipUnknown = 0,
        MX508
    };
    
	enum ChannelType { ChannelType_UART = 0, ChannelType_USB };

	typedef struct _SDPCmd
	{
        short command;
        char format;
        UINT address;
        UINT dataCount;
        UINT data;
	}SDPCmd, * PSDPCmd;

    #pragma pack(1)
    struct _MX_HID_DATA_REPORT
    {
        UCHAR ReportId;
        UCHAR Payload[1];
    };
    #pragma pack()

    int Close();
    int Trash();
    int AllocateIoBuffers();
    void FreeIoBuffers();
	ChipFamily_t GetChipFamily();
    BOOL GetCmdAck(UINT RequiredCmdAck);
	BOOL WriteMemory(UINT address, UINT data, UINT format);
	BOOL ValidAddress(const UINT address, const UINT format) const;
    BOOL Jump(UINT RAMAddress);
	BOOL TransData(UINT address, UINT byteCount, const unsigned char * pBuf);
	BOOL WriteToDevice(const unsigned char *buf, UINT count);
	BOOL ReadFromDevice(PUCHAR buf, UINT count);

    /*
    HANDLE OpenOneDevice (IN HDEVINFO HardwareDeviceInfo,
        IN PSP_INTERFACE_DEVICE_DATA DeviceInfoData,
        OUT CString& devName, OUT DWORD& devInst, rsize_t bufsize);
    HANDLE OpenUsbDevice( LPGUID  pGuid, CString& outNameBuf, DWORD& outDevInst, rsize_t bufsize);
	int SetUsbDeviceId(int dwDevInst);
    HANDLE OpenSpecifiedDevice (IN HDEVINFO HardwareDeviceInfo,
        IN PSP_INTERFACE_DEVICE_DATA DeviceInfoData,
        OUT CString& devName, OUT DWORD& devInst, rsize_t bufsize);
    BOOL GetUsbDeviceFileName( LPGUID  pGuid, CString& outNameBuf, DWORD& outDevInst, rsize_t bufsize);
	BOOL OpenUSBHandle(HANDLE *pHandle, CString pipePath);
	*/
    VOID PackSDPCmd(PSDPCmd pSDPCmd);
    BOOL WriteReg(PSDPCmd pSDPCmd);
    
    static VOID CALLBACK WriteCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,  
        LPOVERLAPPED lpOverlapped);       
    static VOID CALLBACK ReadCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,  
        LPOVERLAPPED lpOverlapped);

    int Init();
    int Read(void* buf, UINT size);
    int Write(UCHAR* buf, ULONG size);
    
	ChipFamily_t _chipFamily;
	HAB_t _habType;

    OVERLAPPED	    m_overlapped;
    CString			m_usb_device_id;
	USHORT          m_vid;
	USHORT          m_pid;

    static HANDLE	m_sync_event_tx;	
    static HANDLE	m_sync_event_rx;	

    HIDP_CAPS				m_Capabilities;
    _MX_HID_DATA_REPORT		*m_pReadReport;
    _MX_HID_DATA_REPORT		*m_pWriteReport;
};
#endif //  __MXHIDDEVICE_H__