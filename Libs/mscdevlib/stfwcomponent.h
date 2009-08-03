// StFwComponent.h: interface for the CStFwComponent class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STFWCOMPONENT_H__C01170F6_65CF_42C1_82D4_DA2D3BA71751__INCLUDED_)
#define AFX_STFWCOMPONENT_H__C01170F6_65CF_42C1_82D4_DA2D3BA71751__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

using namespace std;

typedef enum _EXTRACTVERSIONSTATUS {
	PRODUCT_VERSION_FOUND	= 0,
	COMPONENT_VERSION_FOUND = 1,
	ALL_VERSIONS_FOUND		= 2,
	NO_VERSION_FOUND		= 3
} EXTRACT_VERSION_STATUS;

typedef	struct 
{
	DWORD m_romVersion;
	DWORD m_imageSize;
	DWORD m_cipherTextOffset;
	DWORD m_userDataOffset;
	DWORD m_keyTransformCode;
	UCHAR  m_tag[4];
	DWORD m_productVersion[3];
	DWORD m_componentVersion[3];
	DWORD m_reserved;
} STMP36XX_HEADER, *PSTMP36XX_HEADER;

typedef struct 
{
		UCHAR m_digest[20];
		UCHAR m_signature[4];
		UCHAR m_majorVersion;
		UCHAR m_minorVersion;
		USHORT m_flags;
		UINT m_imageBlocks;
		UINT m_firstBootTagBlock;
		UCHAR m_firstBootableSectionID[4]; //???
		USHORT m_keyCount;
		USHORT m_keyDictionaryBlock;
		USHORT m_headerBlocks;
		USHORT m_sectionCount;
		USHORT m_sectionHeaderSize;
		UCHAR m_padding0[2];
		UCHAR m_signature2[4];
		INT64 m_timestamp;
		DWORD m_productVersion[3];
		DWORD m_componentVersion[3];
		USHORT m_driveTag;			//!< Drive tag for the system drive which this boot image belongs to.
		UCHAR m_padding1[6];
} STMP37XX_HEADER, *PSTMP37XX_HEADER;

#define BOOT_LOADER_WORD_SIZE		3
#define RESET_SEQUENCE_SIZE			2
#define RESET_SEQUENCE_WORD0		0xFFFFFF
#define RESET_SEQUENCE_WORD1		0x000000 

#define LEN_SEQ_ARRAY		5
#define FW_STR_FILE_TYPE_UNKNOWN    L"????"
#define FW_STR_FILE_TYPE_FW         L"STMP"
#define FW_STR_FILE_TYPE_RSRC       L"RSRC"
#define FW_FILE_TYPE_UNKNOWN        1
#define FW_FILE_TYPE_FW             2
#define FW_FILE_TYPE_RSRC           3
#define FW_FILE_TYPE_RAW            4
#define FW_FILE_TYPE_FW_35XX		5
#define FW_FILE_TYPE_FW_36XX		6
#define FW_FILE_TYPE_FW_37XX		7

class CStFwComponent : public CStBase
{
public:
	CStFwComponent(CStUpdater *_p_updater, UCHAR drive_index, WORD _langid, string name="CStFwComponent");
//	CStFwComponent(const CStFwComponent& comp);
//	CStFwComponent& operator=(const CStFwComponent& comp);
	virtual ~CStFwComponent();

	CStByteArray  * GetData();
	int GetFileType() { return m_file_type; };
	ST_ERROR GetData(size_t _from, size_t _count, CStByteArray* _p_arr);
	ST_ERROR GetData(size_t _from, size_t _count, PUCHAR _p_arr);
	BOOL IsResourceLoaded(void) { return m_resource_loaded; };
	BOOL IsLoadedFromResource(void) { return m_loaded_from_resource; };
	ST_ERROR OpenFile(void);
	ULONGLONG GetSizeInBytes();
	ULONGLONG GetSizeInSectors(ULONG sector_size);
	ST_ERROR GetProjectVersion(CStVersionInfo& ver);
	ST_ERROR GetComponentVersion(CStVersionInfo& ver);
	string GetFileName() { return m_filename; };
	
private:
	CStUpdater*				m_p_updater;
	UCHAR					m_drive_index;
	CStByteArray*			m_p_arr_data;
	CStVersionInfo			m_project_version;
	CStVersionInfo			m_component_version;
	EXTRACT_VERSION_STATUS	m_version_status;
	UCHAR					m_seq[LEN_SEQ_ARRAY];
	UCHAR					m_seq2[LEN_SEQ_ARRAY];
    UCHAR                   m_rsrc_seq[LEN_SEQ_ARRAY];
	UCHAR *					m_pHeader;
	USHORT					m_allocated_header_size;
    int                     m_file_type;
	size_t					m_data_length;
    UCHAR	                *m_resource_data;
	ifstream				m_fw_file;
	string					m_filename;
    WORD                    m_langid;
	BOOL					m_resource_loaded;
	BOOL					m_loaded_from_resource;

	ST_ERROR PrepareData ();
    void     ValidateHeaderTag();

	ST_ERROR Extract35xxVersionInformation();
	ST_ERROR Extract37xxVersionFromHeader();
	ST_ERROR Extract36xxVersionFromHeader();
	ST_ERROR ExtractBinaryData(CStByteArray** file_data, size_t& length);
	EXTRACT_VERSION_STATUS ExtractVersionFromLine(char* line, size_t len);
	LPVOID LoadFirmwareResource(string _fname, size_t& _length);
	int GetResourceID(string _fname, LANGID _langID);
};

#endif // !defined(AFX_STFWCOMPONENT_H__C01170F6_65CF_42C1_82D4_DA2D3BA71751__INCLUDED_)
