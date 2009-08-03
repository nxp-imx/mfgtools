// StFwComponent.cpp: implementation of the CStFwComponent class.
//
//////////////////////////////////////////////////////////////////////

#include "StHeader.h"
#include "StGlobals.h"
#include "ddildl_defs.h"
#include "StByteArray.h"
#include "StVersionInfo.h"
#include "StConfigInfo.h"
#include "StUpdater.h"
#include "StFwComponent.h"
#include "..\\..\\common\\updater_res.h"
#include "..\\..\\common\\updater_restypes.h"

#define MAX_CHARS_PER_LINE			80
#define PRODUCT_VERSION_STRING		"Product Version:"
#define COMPONENT_VERSION_STRING	"Component Version:"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStFwComponent::CStFwComponent(CStUpdater *_p_updater, UCHAR _drive_index, WORD _langid, string _name)
 : CStBase(_name)
 , m_allocated_header_size(0)
 , m_pHeader(NULL)
{
	m_p_updater		= _p_updater;
	m_drive_index	= _drive_index;
    m_langid        = _langid;
	m_p_arr_data	= NULL;
	m_resource_data	= NULL;
	m_resource_loaded = FALSE;
	m_loaded_from_resource = FALSE;

	m_seq[0]=0x1a;
	m_seq[1]=0x53; // S
	m_seq[2]=0x54; // T
	m_seq[3]=0x4d; // M
	m_seq[4]=0x50; // P
	m_seq2[0]=0x73; // 's'
	m_seq2[1]=0x67; // 'g'
	m_seq2[2]=0x74; // 't'
	m_seq2[3]=0x6c; // 'l'
	m_rsrc_seq[0]=0x1a;
	m_rsrc_seq[1]=0x52; // R
	m_rsrc_seq[2]=0x53; // S
	m_rsrc_seq[3]=0x52; // R
	m_rsrc_seq[4]=0x43; // C
	m_file_type = FW_FILE_TYPE_UNKNOWN;

	// See if we have a filename.
	string filename;
	m_p_updater->GetConfigInfo()->GetSystemDriveName(m_drive_index, filename);
	if ( filename.empty() )
	{	m_last_error = STERR_NO_FILE_SPECIFIED;
		return;
	}

	m_allocated_header_size = max(sizeof(STMP36XX_HEADER), sizeof(STMP37XX_HEADER));
	m_pHeader = (UCHAR *) new UCHAR[m_allocated_header_size];

	OpenFile();

	m_p_updater->GetErrorObject()->SaveStatus(this, m_drive_index);
}
/*
CStFwComponent::CStFwComponent(const CStFwComponent& _fw):CStBase( _fw )
{
	*this = _fw;
}

CStFwComponent& CStFwComponent::operator=(const CStFwComponent& _fw)
{
	m_p_updater				= _fw.m_p_updater;
	m_drive_index			= _fw.m_drive_index;
	size_t					index;	

	if( m_p_arr_data )
	{
		delete m_p_arr_data;
	}

	UCHAR byte;
	
	m_p_arr_data			= new CStByteArray(_fw.m_p_arr_data->GetCount());
	
	for( index=0; index < _fw.m_p_arr_data->GetCount(); index++)
	{
		_fw.m_p_arr_data->GetAt(index, byte);
		m_p_arr_data->SetAt(index, byte);
	}

	m_project_version		= _fw.m_project_version;
	m_component_version		= _fw.m_component_version;
	m_version_status		= _fw.m_version_status;
	
	for( index = 0; index < LEN_SEQ_ARRAY; index ++)
	{
		m_seq[index]		= _fw.m_seq[index];
        m_rsrc_seq[index]   = _fw.m_rsrc_seq[index];
	}

    m_file_type             = _fw.m_file_type;

	m_last_error			= _fw.m_last_error;
	m_system_last_error		= _fw.m_system_last_error;
	m_obj_name				= _fw.m_obj_name;

	return *this;	
}
*/
CStFwComponent::~CStFwComponent()
{
	if( m_fw_file.is_open() )
	    m_fw_file.close();

	if( m_p_arr_data )
		delete m_p_arr_data;

	if (m_pHeader)
		delete m_pHeader;
}



ST_ERROR CStFwComponent::OpenFile()
{
	ST_ERROR err = STERR_NONE;

	// Are we already loaded?
	if ( m_resource_loaded )
		if( m_fw_file.is_open() )
		    m_fw_file.close();

	m_loaded_from_resource = FALSE;

	//
	// get the file name from ConfigInfo object.
	//
	m_p_updater->GetConfigInfo()->GetSystemDriveName(m_drive_index, m_filename);

	// Check that the binder has not blocked out local files
	if ( m_p_updater->GetConfigInfo()->GetDefaultLocalResourceUsage() )
		m_fw_file.open(m_filename.c_str(), ios::in | ios::binary);

	if( !m_fw_file.is_open() )
	{
        // Try to load from resources as it may have been bound to the executable.
        m_resource_data = (UCHAR *) LoadFirmwareResource(m_filename, m_data_length);

		if ( m_resource_data )
		{
			m_loaded_from_resource = TRUE;
			m_resource_loaded = TRUE;
		}
	}
	else
		m_resource_loaded = TRUE;

	if ( m_resource_loaded )
	{
		ValidateHeaderTag();

		switch (m_file_type)
		{
		case FW_FILE_TYPE_FW_37XX:
			Extract37xxVersionFromHeader();
			break;
		case FW_FILE_TYPE_FW_36XX:
			Extract36xxVersionFromHeader();
			break;
		case FW_FILE_TYPE_UNKNOWN:
			Extract35xxVersionInformation();
			break;
		}

		m_last_error = PrepareData();
	}
	else
		err = STERR_FAILED_TO_OPEN_FILE;

	return err;
}


ST_ERROR CStFwComponent::PrepareData()
{
	size_t			size=0;
	ST_ERROR 		err=STERR_NONE;
	string			filename, line;

	if( !m_fw_file.is_open() && !m_resource_data  )
    {
	    err = STERR_FAILED_TO_OPEN_FILE;
	    goto done;
    }

	//
	// Create the buffer and read the binary data from the file.
	//
	err = ExtractBinaryData(&m_p_arr_data, size);

done:
	return err;
}

CStByteArray* CStFwComponent::GetData()
{
	return m_p_arr_data;
}

ULONGLONG CStFwComponent::GetSizeInBytes()
{
	return m_p_arr_data == NULL ? 0 : m_p_arr_data->GetCount();
}

ULONGLONG CStFwComponent::GetSizeInSectors(ULONG sector_size)
{
	ULONGLONG sectors=0;

	if( !sector_size )
	{
		return sectors;
	}

	sectors = GetSizeInBytes() / sector_size;
	if( GetSizeInBytes() % sector_size )
	{
		sectors += 1;
	}

	return sectors;
}

ST_ERROR CStFwComponent::GetProjectVersion(CStVersionInfo& _ver)
{
	_ver = m_project_version;
	return STERR_NONE;
}

ST_ERROR CStFwComponent::GetComponentVersion(CStVersionInfo& _ver)
{
	_ver = m_component_version;
	return STERR_NONE;
}

//-------------------------------------------------------------------
// ValidateHeaderTag()
//
// Check if tag field is STMP or RSRC, set file type string.
// This is for SDK4.0 and above.  Only 4 bytes are stored in the
// tag field.
//-------------------------------------------------------------------
void CStFwComponent::ValidateHeaderTag()
{
    m_file_type = FW_FILE_TYPE_UNKNOWN;
	PSTMP37XX_HEADER p37xxHeader = (PSTMP37XX_HEADER) m_pHeader;
	PSTMP36XX_HEADER p36xxHeader = (PSTMP36XX_HEADER) m_pHeader;

	if ( m_fw_file.is_open() )
	{
		m_fw_file.seekg( 0, ios::beg);
	
		// read file header
		m_fw_file.read((char *)m_pHeader, m_allocated_header_size);
		if (m_fw_file.fail())
		{
			return;
		}
	}
	else
		memcpy_s(m_pHeader, m_allocated_header_size, m_resource_data, m_allocated_header_size);


	// check for 37xx
    for (int i=1; i<LEN_SEQ_ARRAY; i++)
    {
        if (p37xxHeader->m_signature[i-1] == m_seq[i] &&
			p37xxHeader->m_signature2[i-1] == m_seq2[i-1])
        {
            m_file_type = FW_FILE_TYPE_FW_37XX;
        }
        else
        {
			m_file_type = FW_FILE_TYPE_UNKNOWN;
            break;
        }
    }

/* RSC files are 3600 flavor
    if (m_file_type == FW_FILE_TYPE_UNKNOWN)
    {
        for (int i=1; i<LEN_SEQ_ARRAY; i++)
        {
            if (p37xxHeader->m_signature[i-1] == m_rsrc_seq[i]  &&
				p37xxHeader->m_signature2[i-1] == m_seq2[i-1])
            {
                m_file_type = FW_FILE_TYPE_FW_37XX;
            }
            else
            {
				m_file_type = FW_FILE_TYPE_UNKNOWN;
                break;
            }
        }
    }
*/
	// check for 36xx
	if (m_file_type == FW_FILE_TYPE_UNKNOWN)
	{
	   for (int i=1; i<LEN_SEQ_ARRAY; i++)
		{
			if (p36xxHeader->m_tag[i-1] == m_seq[i])
		    {
	            m_file_type = FW_FILE_TYPE_FW_36XX;
			}
		    else
	        {
				m_file_type = FW_FILE_TYPE_UNKNOWN;
			    break;
		    }
	    }
	}

    if (m_file_type == FW_FILE_TYPE_UNKNOWN)
    {
        for (int i=1; i<LEN_SEQ_ARRAY; i++)
        {
            if (p36xxHeader->m_tag[i-1] == m_rsrc_seq[i])
            {
                m_file_type = FW_FILE_TYPE_FW_36XX;
            }
            else
            {
				m_file_type = FW_FILE_TYPE_UNKNOWN;
                break;
            }
        }
    }

	// otherwise it is unknown - assume 35xx
}

ST_ERROR CStFwComponent::Extract35xxVersionInformation()
{
	ST_ERROR 					err = STERR_NONE;
	BOOL						prod_ver_found = FALSE, comp_ver_found = FALSE, done = FALSE;
	string						filename;
	CHAR						*line;
	EXTRACT_VERSION_STATUS		version_status;
	
	m_version_status = NO_VERSION_FOUND;

	if( !m_fw_file.is_open() && !m_resource_data)
    {
	    err = STERR_FAILED_TO_OPEN_FILE;
	    goto done;
    }

	if( m_fw_file.is_open() )
	{
		m_fw_file.seekg(ios::beg);
		line = new CHAR[MAX_CHARS_PER_LINE];
	}
	else
		line = (CHAR *)m_resource_data;

	while ( !done )
	{
		if ( m_fw_file.is_open()  )
		{
			if ( m_fw_file.eof() )
			{
				done = TRUE;
				break;
			}

			CStGlobals::MakeMemoryZero((PUCHAR)line, MAX_CHARS_PER_LINE);
			m_fw_file.getline(line, MAX_CHARS_PER_LINE);

			if( m_fw_file.fail() )
			{
				err = STERR_FAILED_TO_READ_FILE_DATA;
				done = TRUE;
				break;
			}
		}

		version_status = ExtractVersionFromLine(line, strlen(line));

		if( version_status == PRODUCT_VERSION_FOUND )
		{
			m_version_status = PRODUCT_VERSION_FOUND;
			prod_ver_found = TRUE;
		}
		
		if( version_status == COMPONENT_VERSION_FOUND )
		{
			m_version_status = COMPONENT_VERSION_FOUND;
			comp_ver_found = TRUE;
		}
		
		if( prod_ver_found && comp_ver_found )
		{
			m_version_status = ALL_VERSIONS_FOUND;
			err = STERR_NONE;
			break;
		}

		if ( !m_fw_file.is_open() )
		{	// bump one line into the resource
			line = strchr(line, 0x0d);
			if (!line)
				done= TRUE;
		}
	}

done:
	
	return err;
}

EXTRACT_VERSION_STATUS CStFwComponent::ExtractVersionFromLine(char* _line, size_t /*_len*/)
{
	char*	sub_str;
	UINT	high=0, low=0, mid=0;

	//
	// call non-unicode c library functions. check for project_version
	//
	sub_str = strstr(_line, PRODUCT_VERSION_STRING);

	if(sub_str == NULL)
	{
		//
		// check for component_version
		//
		sub_str = strstr(_line, COMPONENT_VERSION_STRING);

		if(sub_str == NULL)
		{
			return NO_VERSION_FOUND;
		}

		sub_str += sizeof(COMPONENT_VERSION_STRING)-1;
		if( sscanf_s(sub_str, "%u.%u.%u", &high, &mid, &low) != EOF )
		{
			m_component_version.SetHigh((USHORT)high);
			m_component_version.SetMid((USHORT)mid);
			m_component_version.SetLow((USHORT)low);

			return COMPONENT_VERSION_FOUND;
		}

		return NO_VERSION_FOUND;
	}

	sub_str += sizeof(PRODUCT_VERSION_STRING)-1;
	if( sscanf_s(sub_str, "%u.%u.%u", &high, &mid, &low) != EOF )
	{
		m_project_version.SetHigh((USHORT)high);
		m_project_version.SetMid((USHORT)mid);
		m_project_version.SetLow((USHORT)low);
	
		return PRODUCT_VERSION_FOUND; 
	}

	return NO_VERSION_FOUND;
}

ST_ERROR CStFwComponent::Extract36xxVersionFromHeader()
{
	ST_ERROR	err = STERR_NONE;
	PSTMP36XX_HEADER pHeader = (PSTMP36XX_HEADER) m_pHeader;
	
	m_version_status = NO_VERSION_FOUND;

	if( !pHeader)
    {
	    err = STERR_FAILED_TO_OPEN_FILE;
	    goto done;
    }

	m_project_version.SetHigh(pHeader->m_productVersion[0], true);
	m_project_version.SetMid(pHeader->m_productVersion[1], true);
	m_project_version.SetLow(pHeader->m_productVersion[2], true);
	
	m_component_version.SetHigh(pHeader->m_componentVersion[0], true);
	m_component_version.SetMid(pHeader->m_componentVersion[1], true);
	m_component_version.SetLow(pHeader->m_componentVersion[2], true);
	
	m_version_status = ALL_VERSIONS_FOUND;

done:

	return err;
}

ST_ERROR CStFwComponent::Extract37xxVersionFromHeader()
{
	ST_ERROR	err = STERR_NONE;
	PSTMP37XX_HEADER pHeader = (PSTMP37XX_HEADER) m_pHeader;
	
	m_version_status = NO_VERSION_FOUND;

	if( !pHeader)
    {
	    err = STERR_FAILED_TO_OPEN_FILE;
	    goto done;
    }

	m_project_version.SetHigh(pHeader->m_productVersion[0], true);
	m_project_version.SetMid(pHeader->m_productVersion[1], true);
	m_project_version.SetLow(pHeader->m_productVersion[2], true);
	
	m_component_version.SetHigh(pHeader->m_componentVersion[0], true);
	m_component_version.SetMid(pHeader->m_componentVersion[1], true);
	m_component_version.SetLow(pHeader->m_componentVersion[2], true);

	CHAR updaterDriveIndex;
	m_p_updater->GetConfigInfo()->GetUpdaterDriveIndex(updaterDriveIndex);
	if( m_drive_index != updaterDriveIndex )
		m_p_updater->GetConfigInfo()->SetFirmwareHeaderFlags(pHeader->m_flags);
	
	m_version_status = ALL_VERSIONS_FOUND;

done:

	return err;
}


ST_ERROR CStFwComponent::ExtractBinaryData( CStByteArray** file_data, size_t& length)
{
	USES_CONVERSION;

	size_t index_bin_data=0, index = 0;
	ST_ERROR err = STERR_NONE;
	BOOL binary_data_sequence_found = FALSE;
	UCHAR *data = NULL;
	ULONG size, bytesRead;

	if ( m_fw_file.is_open() )
	{
		filebuf *p_buf = m_fw_file.rdbuf();
	
		size = p_buf->pubseekoff (0, ios::end, ios::in);
		p_buf->pubseekpos (0,ios::in);

		data = new UCHAR[size];
	
		bytesRead = p_buf->_Sgetn_s ((char*)data, size, size);
		
		if ( bytesRead != size )
		{
			CString str, longSize;
			TCHAR testBuff[512];

			str.Format(_T("  ERROR While initializing FirmwareComponent %s; fileSize: 0x%x"), A2W(m_filename.c_str()), size);
			_stprintf_s(testBuff, 512, _T("(%I64u)"), size);
			longSize = testBuff;
			str += longSize;

			str.AppendFormat(_T(", bytesRead: 0x%x"), bytesRead); 
			_stprintf_s(testBuff, 512, _T("(%I64u)"), bytesRead);
			longSize = testBuff;
			str += longSize;

			m_p_updater->GetLogger()->Log(str);
		}
	}
	else
	{
		data = m_resource_data;
		size = (ULONG) m_data_length;
	}

    if ((m_file_type == FW_FILE_TYPE_UNKNOWN) && (m_version_status != NO_VERSION_FOUND))
	{
	    //
	    // We need to separate version information from the data. The version information ends with
	    // sequence "0x1a 0x54 0x54 0x47 0x50" (0x1aSTMP) stored in array m_fw_seq.
	    //
	    for(index = 0; index < size; index ++)
	    {
		    binary_data_sequence_found = TRUE;
		    for( size_t seq_index=0; seq_index<LEN_SEQ_ARRAY; seq_index++ )
		    {
			    if( data[index+seq_index] != m_seq[seq_index] )
			    {
				    binary_data_sequence_found = FALSE;
				    break;
			    }
		    }
		    if( binary_data_sequence_found )
		    {
                m_file_type = FW_FILE_TYPE_FW;
			    index_bin_data = index + LEN_SEQ_ARRAY;
			    break;
		    }
	    }

	    if( binary_data_sequence_found )
	    {
	        //
	        // set the return parameter to the correct _len
	        //
	        length = size - index_bin_data;

//			if ( !m_resource_data )
//			{
		        *file_data = new CStByteArray(length);
		        if( *file_data )
			    {
				    //
					// copy the data
		            //
		            for( index = 0; index < length; index ++ )
			        {
						(*file_data)->SetAt(index, data[index_bin_data+index]);
					}
				}
	            else
		        {
			        length = 0;
				    err = STERR_NO_MEMORY;
		        }
//			}
//			else
//			{
//				*_binary_data = m_resource_data;
//			}
        }
        else
        {
		    //	CStTrace::trace("No binary data found for drive %d", m_drive_index );
		    err = STERR_FAILED_TO_READ_FILE_DATA;
        }
    }
    else
    {
		//
		// data is our binary data, we will simply return data pointer without deleting it.
		// caller is deleting _binary_data pointer !
		//
		length = size;

        *file_data = new CStByteArray(length);
        if( *file_data )
		{
			for( index = 0; index < length; index ++ )
			{
				(*file_data)->SetAt(index, data[index]);
			}
			err = STERR_NONE;
			m_file_type = FW_FILE_TYPE_RAW;
		}
    }

    if ( data && data != m_resource_data )
		delete data;

	return err;
}


ST_ERROR CStFwComponent::GetData(size_t _from_offset, size_t _count, PUCHAR _p_ch)
{
	CStByteArray arr(_count);
	ST_ERROR err = STERR_NONE;

	err = GetData(_from_offset, _count, &arr);
	if( err != STERR_NONE )
		return err;
	
	return arr.Read( _p_ch, _count, 0 );
}

//////////////////////////////////////////////////////////////////////////////////////////
// should always return STERR_NONE; 
// if the _from_offset and _count are invalid then it must fill _arr with 0xFF.
//
ST_ERROR CStFwComponent::GetData(size_t _from_offset, size_t _count, CStByteArray* _p_arr)
{
	size_t index;

	if( _from_offset > GetData()->GetCount() )
	{
		//
		// _from_offset is out of range return arr fill with 0xff.
		//
		for( index = 0; index < _p_arr->GetCount(); index ++ )
		{
			UCHAR uch = 0xFF;
	
			_p_arr->SetAt(index, uch);
		}
		return STERR_NONE;
	}

	if( _count > ( GetData()->GetCount() - _from_offset ) )
	{
		//
		// data is less than asked for so fill the remaining with 0xff
		//
		for( index = 0; index < ( GetData()->GetCount() - _from_offset ); index ++ )
		{
			UCHAR uch;
		
			GetData()->GetAt(index + _from_offset, uch);
			_p_arr->SetAt(index, uch);
		}

		for( ; index < _count; index ++ )
		{
			UCHAR uch = 0xFF;
	
			_p_arr->SetAt(index, uch);
		}
		return STERR_NONE;
	}

	//
	// found _from_offset and _count in range so return the right data.
	//
	for( index = 0; index < _count; index ++ )
	{
		UCHAR uch;
	
		GetData()->GetAt(index + _from_offset, uch);
		_p_arr->SetAt(index, uch);
	}
	
	return STERR_NONE;
}

#include <atlconv.h>


LPVOID CStFwComponent::LoadFirmwareResource(string _fname, size_t& _length)
{
   	HRSRC hResInfo;
	HGLOBAL hRes;
	LPVOID pPtr = NULL;
	int iResId;
USES_CONVERSION;
	wstring wName = A2W(_fname.c_str());

	iResId = GetResourceID( _fname, 0 );

	//hResInfo = FindResource( GetModuleHandle(NULL), MAKEINTRESOURCE(iResId), L"STMPRESTYPE");
	// Here's a trap - note the different parameter order the non-NLS api above
	// and the NLS version below.

	// First try to load the selected language version.
	hResInfo = FindResourceEx( GetModuleHandle(NULL), L_STMP_FW_RESTYPE, MAKEINTRESOURCE(iResId), m_langid );

	    // if that failed, try the primary language id with neutral sublang
    if (!hResInfo)
	{
		iResId = GetResourceID( _fname, MAKELANGID(PRIMARYLANGID(m_langid), SUBLANG_NEUTRAL) );
		hResInfo = FindResourceEx( GetModuleHandle(NULL), L_STMP_FW_RESTYPE, MAKEINTRESOURCE(iResId),
                                MAKELANGID(PRIMARYLANGID(m_langid), SUBLANG_NEUTRAL) );
	}

    // if that failed, try neutral/neutral for the system default
//    if (!hResInfo)
//		hResInfo = FindResourceEx( GetModuleHandle(NULL), L_STMP_FW_RESTYPE, MAKEINTRESOURCE(iResId),
//                              MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL) );

	// If not found load the invariant/neutral version.   Updater.sb and bootmanager should be
	// neutral; others may also be. 
	if (!hResInfo)
	{
		iResId = GetResourceID( _fname, MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL) );
		hResInfo = FindResourceEx( GetModuleHandle(NULL), L_STMP_FW_RESTYPE, MAKEINTRESOURCE(iResId),
								MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL) );
	}

	if ( hResInfo )
    {
		hRes = LoadResource(GetModuleHandle(NULL), hResInfo);
		if ( hRes )
			pPtr = LockResource(hRes);

        _length = SizeofResource(GetModuleHandle(NULL), hResInfo);
    }
//	if (pPtr)
//		::MessageBox(NULL, L"Loaded resource", L"Test", MB_OK);
//	else
//		::MessageBox(NULL, L"Failed to loaded resource", L"Test", MB_OK);

    return pPtr;
}



int CStFwComponent::GetResourceID(string _fname, LANGID _langID)
{
	int resId = IDR_RESID_UNKNOWN;;
	LANGID langID;

	if (_langID)
		langID = _langID;
	else
		langID = m_langid;

	if ( _fname.compare( "stmp3rec.sys" ) == 0 )
		resId = IDR_STMP3REC_SYS;
	else if ( _fname.compare( "stmp3rec.inf" ) == 0 )
		resId = IDR_STMP3REC_INF;
	else if ( _fname.compare( "stmp3rec.cat" ) == 0 )
		resId = IDR_STMP3REC_CAT;
	else if ( _fname.compare( "stmp3recx64.sys" ) == 0 )
		resId = IDR_STMP3RECX64_SYS;
	else if ( _fname.compare( "stmp3recx64.inf" ) == 0 )
		resId = IDR_STMP3RECX64_INF;
	else if ( _fname.compare( "stmp3recx64.cat" ) == 0 )
		resId = IDR_STMP3RECX64_CAT;
	else
	{	// look it up in the resource table
		PSTFWRESINFO	pFwResInfo = NULL;
		int				iFwResInfoCount;
	   	HRSRC hResInfo;
		HGLOBAL hRes;
		LPVOID pPtr = NULL;

	    hResInfo = FindResourceEx( NULL,
	    					L_STMP_RESINFO_TYPE,
		    				MAKEINTRESOURCE(IDR_BOUND_RESOURCE_TABLE_LEN),
			    			MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL));
		if ( hResInfo )
	    {
		    hRes = LoadResource( NULL, hResInfo);
    		if ( hRes )
		   		pPtr = LockResource(hRes);
		   	if ( pPtr )
			{
	       		iFwResInfoCount = *((int *)pPtr);

			    hResInfo = FindResourceEx( NULL,
								L_STMP_RESINFO_TYPE,
								MAKEINTRESOURCE(IDR_BOUND_RESOURCE_TABLE),
								MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL));
			    if ( hResInfo )
			    {
					hRes = LoadResource( NULL, hResInfo);
		    		if ( hRes )
	   					pFwResInfo = (PSTFWRESINFO)LockResource(hRes);

					if ( pFwResInfo )
					{
USES_CONVERSION;
						for (int i = 0; i < iFwResInfoCount; ++i)
							if( (pFwResInfo[i].wLangId == langID) &&
								(_fname.compare(T2A(pFwResInfo[i].szResourceName)) == 0) )
								resId = pFwResInfo[i].iResId;
					}
				}
			}
		}
    }

	return resId;
}

