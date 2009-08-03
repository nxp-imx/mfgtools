// StFwVersion.cpp: implementation of the CStFwVersion class.
//
//////////////////////////////////////////////////////////////////////


#include "StdAfx.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
//#include <fstream>
#include <memory>
using namespace std;

#include "StFwVersion.h"


#define MAX_CHARS_PER_LINE			80
#define PRODUCT_VERSION_STRING		"Product Version:"
#define COMPONENT_VERSION_STRING	"Component Version:"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStFwVersion::CStFwVersion(CString& _fname, PUCHAR _pResBuffer, ULONG _uResLen)
{
	m_filename	    = _fname;
    m_res_ptr       = _pResBuffer;
    m_res_length    = _uResLen;

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

	m_allocated_header_size = max(sizeof(STMP36XX_HEADER), sizeof(STMP37XX_HEADER));
	m_pHeader = (UCHAR *) new UCHAR[m_allocated_header_size];

    if ( m_res_ptr == NULL )
    {
    	//
	    // open the file in text mode. return error on fail to open or read.
    	//
	    m_fw_file.open(m_filename, ios::in | ios::binary);
    	if(m_fw_file.fail())
	    {
		    goto done;
    	}

    	m_fw_file.seekg(ios::beg);
    }
	if ( m_fw_file.is_open() )
	{
		// read file header
		m_fw_file.read((char *)m_pHeader, m_allocated_header_size);
	}
	else
		memcpy_s(m_pHeader, m_allocated_header_size, m_res_ptr, m_allocated_header_size);

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
done:
	if( m_fw_file.is_open() )
	{
		m_fw_file.close();
	}

}



CStFwVersion::~CStFwVersion()
{
	if ( m_pHeader )
		delete m_pHeader;
}


void CStFwVersion::GetProjectVersion(CString& _ver)
{
	m_project_version.GetVersionString(_ver);
	return;
}

void CStFwVersion::GetComponentVersion(CString& _ver)
{
	 m_component_version.GetVersionString(_ver);
	return;
}

//-------------------------------------------------------------------
// ValidateHeaderTag()
//
// Check if tag field is STMP or RSRC, set file type string.
// This is for SDK4.0 and above.  Only 4 bytes are stored in the
// tag field.
//-------------------------------------------------------------------
void CStFwVersion::ValidateHeaderTag()
{
    m_file_type = FW_FILE_TYPE_UNKNOWN;
	PSTMP37XX_HEADER p37xxHeader = (PSTMP37XX_HEADER) m_pHeader;
	PSTMP36XX_HEADER p36xxHeader = (PSTMP36XX_HEADER) m_pHeader;

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

void CStFwVersion::Extract35xxVersionInformation()
{
	BOOL						prod_ver_found = FALSE, comp_ver_found = FALSE, done = FALSE;
	string						filename;
	CHAR						*line = NULL;
	EXTRACT_VERSION_STATUS		version_status;
	
	m_version_status = NO_VERSION_FOUND;

	if( !m_fw_file.is_open() && !m_res_ptr)
    {
	    goto done;
    }

	if( m_fw_file.is_open() )
	{
		m_fw_file.seekg(ios::beg);
		line = new CHAR[MAX_CHARS_PER_LINE];
	}
	else
		line = (CHAR *)m_res_ptr;

	while ( !done )
	{
		if ( m_fw_file.is_open()  )
		{
			if ( m_fw_file.eof() )
			{
				done = TRUE;
				break;
			}

			memset(line, 0, MAX_CHARS_PER_LINE);
			m_fw_file.getline(line, MAX_CHARS_PER_LINE);

			if( m_fw_file.fail() )
			{
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
			done = TRUE;
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
	
	return;
}

EXTRACT_VERSION_STATUS CStFwVersion::ExtractVersionFromLine(char* _line, size_t /*_len*/)
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

void CStFwVersion::Extract36xxVersionFromHeader()
{
	PSTMP36XX_HEADER pHeader = (PSTMP36XX_HEADER) m_pHeader;
	
	m_version_status = NO_VERSION_FOUND;

	if( !pHeader)
    {
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

	return;
}

void CStFwVersion::Extract37xxVersionFromHeader()
{
	PSTMP37XX_HEADER pHeader = (PSTMP37XX_HEADER) m_pHeader;
	
	m_version_status = NO_VERSION_FOUND;

	if( !pHeader)
    {
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

	return;
}