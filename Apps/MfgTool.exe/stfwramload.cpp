#include <AFXPRIV.H>
#include "../../Libs/mscdevlib.stmfgtool/StHeader.h"
#include "../../Libs/mscdevlib.stmfgtool/stglobals.h"
#include "../../Libs/mscdevlib.stmfgtool/StByteArray.h"
//#include "StProgress.h"
#include "../../Libs/mscdevlib.stmfgtool/StVersionInfo.h"
//#include "StConfigInfo.h"
#include "../../Libs/mscdevlib.stmfgtool/StUpdater.h"
#include "../../Libs/StDecrypt/StDecrypt.h"

#include ".\stfwramload.h"

#define MAX_CHARS_PER_LINE			80

CStFwRamLoad::CStFwRamLoad(CStUpdater *_p_updater, const char * _profile_path, USHORT _part, int panel_num)
{
	m_p_updater		= _p_updater;
    m_filename      = _profile_path;
    m_filename.append("\\");
	m_drive_index	= 2;
	m_p_arr_data	= NULL;

	m_seq[0]=0x1a;
	m_seq[1]=0x53; // S
	m_seq[2]=0x54; // T
	m_seq[3]=0x4d; // M
	m_seq[4]=0x50; // P
	m_rsrc_seq[0]=0x1a;
	m_rsrc_seq[1]=0x52; // R
	m_rsrc_seq[2]=0x53; // S
	m_rsrc_seq[3]=0x52; // R
	m_rsrc_seq[4]=0x43; // C
	m_file_type = FW_FILE_TYPE_UNKNOWN;    USES_CONVERSION;


    char buf[10];
    m_filename.append(W2A(STATIC_ID_FW_FILENAME));

	_ultoa_s(panel_num+1, buf, 10, 10);
    if (_p_updater->UseMultipleStatiIdFw())
        m_filename.append(buf);

    m_filename.append(W2A(STATIC_ID_FW_EXTENSION));

	ExtractVersionFromHeader();
    ValidateHeaderTag();
    if (m_file_type == FW_FILE_TYPE_UNKNOWN)
    {
        ExtractVersionInformation();
        // Get binary data, skipping header
    	m_last_error = PrepareData();
    }
    else
    {   // Get binary data without skipping header
        m_last_error = PrepareData();
    }
//	ExtractVersionInformation();
//	m_last_error	= PrepareData();
	m_p_updater->GetErrorObject()->SaveStatus(this, m_drive_index);
}

CStFwRamLoad::~CStFwRamLoad(void)
{
}


ST_ERROR CStFwRamLoad::ExtractVersionFromHeader()
{
	ST_ERROR 					err = STERR_NONE;
	ifstream					fw_file;
	
	//
	// get filename from configinfo object.
	//
	//m_p_updater->GetConfigInfo()->GetSystemDriveName(m_drive_index, filename);

	//
	// open the file in binary mode. return error on fail to open or read.
	//
	fw_file.open(m_filename.c_str(), ios::in | ios::binary);
	if(fw_file.fail())
	{
		err = STERR_FAILED_TO_OPEN_FILE;
		goto done;
	}

	m_version_status = NO_VERSION_FOUND;
	fw_file.seekg( 0, ios::beg);
	
	// read file header
	fw_file.read((char *)&m_header, sizeof(m_header));
	if (fw_file.fail())
	{
		err = STERR_FAILED_TO_READ_FILE_DATA;
		goto done;
	}
	
	m_project_version.SetHigh(m_header.m_productVersion[0], true);
	m_project_version.SetMid(m_header.m_productVersion[1], true);
	m_project_version.SetLow(m_header.m_productVersion[2], true);
	
	m_component_version.SetHigh(m_header.m_componentVersion[0], true);
	m_component_version.SetMid(m_header.m_componentVersion[1], true);
	m_component_version.SetLow(m_header.m_componentVersion[2], true);
	
	m_version_status = ALL_VERSIONS_FOUND;

done:
	if( fw_file.is_open() )
	{
		fw_file.close();
	}

	return err;
}


ST_ERROR CStFwRamLoad::ExtractVersionInformation()
{
	ST_ERROR 					err = STERR_NONE;
	ifstream					fw_file;
	BOOL						prod_ver_found = FALSE, comp_ver_found = FALSE;
	CHAR						line[MAX_CHARS_PER_LINE];
	EXTRACT_VERSION_STATUS		version_status;
	
	//
	// get filename from configinfo object.
	//
	//m_p_updater->GetConfigInfo()->GetSystemDriveName(m_drive_index, filename);

	//
	// open the file in text mode. return error on fail to open or read.
	//
	fw_file.open(m_filename.c_str());
	if(fw_file.fail())
	{
		err = STERR_FAILED_TO_OPEN_FILE;
		goto done;
	}

	m_version_status = NO_VERSION_FOUND;
	fw_file.seekg(ios::beg);
	while (!fw_file.eof())
	{
		CStGlobals::MakeMemoryZero((PUCHAR)line, MAX_CHARS_PER_LINE);
		fw_file.getline(line, MAX_CHARS_PER_LINE);

		if( fw_file.fail() )
		{
			err = STERR_FAILED_TO_READ_FILE_DATA;
			goto done;
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
	}

done:
	
	if( fw_file.is_open() )
	{
		fw_file.close();
	}

	return err;
}
/*
ST_ERROR CStFwRamLoad::ExtractVersionInformation(void)
{
	ST_ERROR 					err = STERR_NONE;
	ifstream					fw_file;
	BOOL						prod_ver_found = FALSE, comp_ver_found = FALSE;
	CHAR						line[MAX_CHARS_PER_LINE];
	EXTRACT_VERSION_STATUS		version_status;
	
	//
	// get filename from configinfo object.
	//
//	m_p_updater->GetConfigInfo()->GetSystemDriveName(m_drive_index, filename);

	//
	// open the file in text mode. return error on fail to open or read.
	//
	fw_file.open(m_filename.c_str());
	if(fw_file.fail())
	{
		err = STERR_FAILED_TO_OPEN_FILE;
		goto done;
	}

	m_version_status = NO_VERSION_FOUND;
	fw_file.seekg(ios::beg);
	while (!fw_file.eof())
	{
		CStGlobals::MakeMemoryZero((PUCHAR)line, MAX_CHARS_PER_LINE);
		fw_file.getline(line, MAX_CHARS_PER_LINE);

		if( fw_file.fail() )
		{
			err = STERR_FAILED_TO_READ_FILE_DATA;
			goto done;
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
	}

done:
	
	if( fw_file.is_open() )
	{
		fw_file.close();
	}

	return err;
}
*/


ST_ERROR CStFwRamLoad::PrepareData()
{
	PUCHAR			binary_data=NULL;
	size_t			length=0, index=0;
	ST_ERROR 		err=STERR_NONE;
	ifstream		fw_file;
	string			line;

	//
	// open the file in binary mode. return error on fail to open or read.
	//
	fw_file.open(m_filename.c_str(), ios::in | ios::binary);
	if(fw_file.fail())
	{
		err = STERR_FAILED_TO_OPEN_FILE;
		goto done;
	}

	//
	// Create the buffer and read the binary data from the file.
	//
	err = ExtractBinaryData(fw_file, &binary_data, length);
	if( err != STERR_NONE )
	{
		goto done;
	}

	//
	// construct the member array variable to save the decrypted data. 
	//
	m_p_arr_data = new CStByteArray(length);
	if( !m_p_arr_data )
	{
		err = STERR_NO_MEMORY;
		goto done;
	}

	//
	// m_p_arr_data will always hold the binary data ready to tranfer on a system drive image.
	//
	for(index=0; index<length; index++)
	{
		m_p_arr_data->SetAt(index, binary_data[index]);
	}
	
//	m_p_arr_data->FlipBytes(3, 0, 2);

done:

	if( binary_data )
	{
		delete[] binary_data;
	}

	return err;
}

/*
ST_ERROR CStFwRamLoad::PrepareData(void)
{
	PUCHAR			decrypt_data=NULL, binary_data=NULL;
	size_t			length=0, new_size=0, index=0;
	ST_ERROR 		err=STERR_NONE;
	ifstream		fw_file;
	string			line;
	BOOL			is_data_encrypted = TRUE;

	//
	// get the file name from ConfigInfo object.
	//
//	m_p_updater->GetConfigInfo()->GetSystemDriveName(m_drive_index, filename);
//	m_p_updater->GetConfigInfo()->IsSystemDriveEncrypted(m_drive_index, is_data_encrypted);

	//
	// open the file in binary mode. return error on fail to open or read.
	//
	fw_file.open(m_filename.c_str(), ios::in | ios::binary);
	if(fw_file.fail())
	{
		err = STERR_FAILED_TO_OPEN_FILE;
		goto done;
	}

	//
	// Create the buffer and read the binary data from the file.
	//
	err = ExtractBinaryData(fw_file, &binary_data, length);
	if( err != STERR_NONE )
	{
		goto done;
	}


	//
	// Call decrypt routine.
	//
	if( is_data_encrypted )
	{
		CStDecrypt decrypt_filedata_to_s_records(binary_data, length, &decrypt_data, new_size);
		if( decrypt_filedata_to_s_records.GetLastError() != STERR_NONE )
		{
			err = decrypt_filedata_to_s_records.GetLastError();
			goto done;
		}
	}
	else
	{
		new_size = length;
	}

	//
	// construct the member array variable to save the decrypted data. 
	//
	m_p_arr_data = new CStByteArray(new_size);
	if( !m_p_arr_data )
	{
		err = STERR_NO_MEMORY;
		goto done;
	}

	//
	// m_p_arr_data will always hold the binary data ready to tranfer on a system drive image.
	//
	for(index=0; index<new_size; index++)
	{
		if( is_data_encrypted )
			m_p_arr_data->SetAt(index, decrypt_data[index]);
		else
			m_p_arr_data->SetAt(index, binary_data[index]);
	}
	
//	m_p_arr_data->FlipBytes(3, 0, 2);

done:

	if( binary_data )
	{
		delete[] binary_data;
	}
	if( decrypt_data )
	{
		delete[] decrypt_data;
	}
	return err;
}
*/