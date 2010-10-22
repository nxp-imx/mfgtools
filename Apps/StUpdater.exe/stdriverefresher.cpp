/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "StGlobals.h"
#include "StProgress.h"
#include ".\stdriverefresher.h"
#include <cfgmgr32.h>
#include <setupapi.h>
#include <devguid.h>


CStDriveRefresher::CStDriveRefresher(CStError* _p_error)
{
	m_p_error = _p_error;
	HMODULE h_setupapi = 0, h_cfgmgr32 = 0; 
	FARPROC fn = 0;
	m_platform = CStGlobals::GetPlatform();

	h_setupapi = LoadLibrary( L"SETUPAPI.DLL" );
	if( !h_setupapi )
	{
		m_last_error = STERR_FAILED_TO_LOAD_SETUPAPI_LIB;
		m_system_last_error = CStGlobals::GetLastError();

		goto end_of_construction;
	}

	fn = GetProcAddress(h_setupapi, "SetupDiGetClassDevsA");
	if( !fn )
	{
		m_last_error = STERR_MISSING_API_IN_SETUPAPI_LIB;
		m_system_last_error = CStGlobals::GetLastError();

		goto end_of_construction;
	}

	fn = GetProcAddress(h_setupapi, "SetupDiEnumDeviceInfo");
	if( !fn )
	{
		m_last_error = STERR_MISSING_API_IN_SETUPAPI_LIB;
		m_system_last_error = CStGlobals::GetLastError();

		goto end_of_construction;
	}

	fn = GetProcAddress(h_setupapi, "SetupDiGetDeviceRegistryPropertyA");
	if( !fn )
	{
		m_last_error = STERR_MISSING_API_IN_SETUPAPI_LIB;
		m_system_last_error = CStGlobals::GetLastError();

		goto end_of_construction;
	}

	fn = GetProcAddress(h_setupapi, "SetupDiGetDeviceInstanceIdA");
	if( !fn )
	{
		m_last_error = STERR_MISSING_API_IN_SETUPAPI_LIB;
		m_system_last_error = CStGlobals::GetLastError();

		goto end_of_construction;
	}

	fn = GetProcAddress(h_setupapi, "SetupDiDestroyDeviceInfoList");
	if( !fn )
	{
		m_last_error = STERR_MISSING_API_IN_SETUPAPI_LIB;
		m_system_last_error = CStGlobals::GetLastError();

		goto end_of_construction;
	}

	if( ( m_platform == OS_2K ) || ( m_platform == OS_XP ) || ( m_platform == OS_XP64 ))
	{
		fn = GetProcAddress(h_setupapi, "SetupDiSetClassInstallParamsW");
		if( !fn )
		{
			m_last_error = STERR_MISSING_API_IN_SETUPAPI_LIB;
			m_system_last_error = CStGlobals::GetLastError();

			goto end_of_construction;
		}

		fn = GetProcAddress(h_setupapi, "SetupDiChangeState");
		if( !fn )
		{
			m_last_error = STERR_MISSING_API_IN_SETUPAPI_LIB;
			m_system_last_error = CStGlobals::GetLastError();

			goto end_of_construction;
		}
	}

	if( ( m_platform == OS_ME ) || ( m_platform == OS_98 ) )
	{
		h_cfgmgr32 = LoadLibrary( L"CFGMGR32.DLL" );
		if( !h_cfgmgr32 )
		{
			m_last_error = STERR_FAILED_TO_LOAD_CFGMGR32_LIB;
			m_system_last_error = CStGlobals::GetLastError();

			goto end_of_construction;
		}

		fn = GetProcAddress(h_cfgmgr32, "CM_Disable_DevNode");
		if( !fn )
		{
			m_last_error = STERR_MISSING_API_IN_CFGMGR32_LIB;
			m_system_last_error = CStGlobals::GetLastError();

			goto end_of_construction;
		}

		fn = GetProcAddress(h_cfgmgr32, "CM_Enable_DevNode");
		if( !fn )
		{
			m_last_error = STERR_MISSING_API_IN_CFGMGR32_LIB;
			m_system_last_error = CStGlobals::GetLastError();

			goto end_of_construction;
		}

		fn = GetProcAddress(h_cfgmgr32, "CM_Locate_DevNodeA");
		if( !fn )
		{
			m_last_error = STERR_MISSING_API_IN_CFGMGR32_LIB;
			m_system_last_error = CStGlobals::GetLastError();

			goto end_of_construction;
		}
	}

end_of_construction:

	m_p_error->SaveStatus(this);
	
	if( h_setupapi )
		FreeLibrary( h_setupapi );
	if( h_cfgmgr32 )
		FreeLibrary( h_cfgmgr32 );
}

CStDriveRefresher::~CStDriveRefresher(void)
{
}

ST_ERROR CStDriveRefresher::RefreshDevice( USHORT _usb_vendor_id, USHORT _usb_product_id, CStProgress* _p_progress )
{
	char		device_string[64];
	
	ConstructDeviceString( _usb_vendor_id, _usb_product_id, device_string, 64 );

	return DisableEnableDevice( device_string, _p_progress );
}

CString CStDriveRefresher::ConstructDeviceString( USHORT _usb_vendor_id, USHORT _usb_product_id, char* _device_string, rsize_t _bufsize )
{
	
	sprintf_s( _device_string, _bufsize, "USB\\VID_%04x&PID_%04x", _usb_vendor_id, _usb_product_id );

	return CString(_device_string);
}

ST_ERROR CStDriveRefresher::DisableEnableDevice(const char* _device_string, CStProgress* _p_progress )
{
	ST_ERROR			err = STERR_NONE;
	DEVINST				device_inst_id = 0;
    HDEVINFO			DeviceInfoSet = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA		DeviceInfoData;
    DWORD				buffersize = 1024, 
						RequiredSize = 0;
	LPSTR				p = NULL;
	LPSTR				buffer = NULL;
	char				DeviceInstanceId[1024];
	int					loop=0;

    //
    // Create a Device Information Set with all present devices.
    //
	switch ( m_platform )
	{
	case OS_98 :
		DeviceInfoSet = SetupDiGetClassDevsA(
				(LPGUID)&GUID_DEVCLASS_HDC, 
				0,
				0, 
				DIGCF_PRESENT
			); 
		break;
	default:
		DeviceInfoSet = SetupDiGetClassDevsA(
				(LPGUID)&GUID_DEVCLASS_USB, 
				0,
				0, 
				DIGCF_PRESENT
			); 
		break;
	}
    if( DeviceInfoSet == INVALID_HANDLE_VALUE )
    {
		wchar_t	 more_error_info[MAX_PATH];

		m_system_last_error = CStGlobals::GetLastError();
		err = STERR_FAILED_TO_GET_DEVICE_INFO_SET;

		wsprintf( more_error_info, L"0x%x", this->GetSystemLastError() );
		m_p_error->SaveStatus(this, more_error_info);
        return err;
    }
    
	_p_progress->SetCurrentTask( TASK_TYPE_DEVICE_REFRESH, 5 );
	_p_progress->UpdateProgress();

	//
    //  Enumerate through all Devices.
    //
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for( int i=0; SetupDiEnumDeviceInfo(DeviceInfoSet,i,&DeviceInfoData); i++ )
    {
        DWORD DataT;
        
		if (!buffer)
		{
			buffer = (LPSTR)LocalAlloc(LPTR,buffersize);
			if( !buffer )
			{
				err = STERR_NO_MEMORY;
				goto cleanup_DeviceInfo;
			}
		}
	
        //
        // We won't know the size of the HardwareID buffer until we call
        // this function. So call it with a null to begin with, and then 
        // use the required buffer size to Alloc the nessicary space.
        // Keep calling we have success or an unknown failure.
        //
        while( !SetupDiGetDeviceRegistryPropertyA(
				DeviceInfoSet,
				&DeviceInfoData,
				SPDRP_HARDWAREID,
				&DataT,
				(PBYTE)buffer,
				buffersize,
				&buffersize
			))
        {
			if( CStGlobals::GetLastError() == ERROR_INVALID_DATA )
            {
                //
                // May be a Legacy Device with no HardwareID. Continue.
                //
                break;
            }
            else if( CStGlobals::GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                //
                // We need to change the buffer size.
                //
                if( buffer ) 
                    LocalFree(buffer);
                buffer = (LPSTR)LocalAlloc( LPTR, buffersize );
				if( !buffer )
				{
					err = STERR_NO_MEMORY;
					goto cleanup_DeviceInfo;
				}
            }
            else
            {
                //
                // Unknown Failure.
                //
				m_system_last_error = CStGlobals::GetLastError();
				err = STERR_FAILED_GET_DEVICE_REGISTRY_PROPERTY;
                goto cleanup_DeviceInfo;
            }            
        }

        if( CStGlobals::GetLastError() == ERROR_INVALID_DATA ) 
            continue;
        
        //
        // Compare each entry in the buffer multi-sz list with our HardwareID.
        //
        for( p=buffer; *p&&(p<&buffer[buffersize]);p+=strlen(p)+sizeof(char) )
        {
			if( !_strnicmp( _device_string, p, strlen( _device_string ) ) )
			{
				_p_progress->UpdateProgress();
				if( SetupDiGetDeviceInstanceIdA(
					DeviceInfoSet,
					&DeviceInfoData,
					DeviceInstanceId,
					buffersize,
					&RequiredSize
					))
				{
					if( ( m_platform == OS_XP ) || ( m_platform == OS_2K ) || ( m_platform == OS_XP64 ) )
					{
						SP_PROPCHANGE_PARAMS params;

						params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
						params.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
						params.HwProfile = 0;
						params.Scope = DICS_FLAG_GLOBAL;
						params.StateChange = DICS_DISABLE;
						
						if( !SetupDiSetClassInstallParamsW( DeviceInfoSet,	
							&DeviceInfoData, &params.ClassInstallHeader, sizeof(SP_PROPCHANGE_PARAMS)) )
						{
							err = STERR_ERROR_IN_SETUPAPI_API;
							m_system_last_error = ::GetLastError();
							goto cleanup_DeviceInfo;
						}

						if( !SetupDiChangeState( DeviceInfoSet, &DeviceInfoData ) )
						{
							err = STERR_ERROR_IN_SETUPAPI_API;
							m_system_last_error = ::GetLastError();
							goto cleanup_DeviceInfo;
						}

						params.StateChange = DICS_ENABLE;
						if( !SetupDiSetClassInstallParamsW( DeviceInfoSet,	
							&DeviceInfoData, &params.ClassInstallHeader, sizeof(SP_PROPCHANGE_PARAMS)) )
						{
							err = STERR_ERROR_IN_SETUPAPI_API;
							m_system_last_error = ::GetLastError();
							goto cleanup_DeviceInfo;
						}
						if( !SetupDiChangeState( DeviceInfoSet, &DeviceInfoData ) )
						{
							err = STERR_ERROR_IN_SETUPAPI_API;
							m_system_last_error = ::GetLastError();
							goto cleanup_DeviceInfo;
						}
					}
					else if ( ( m_platform == OS_98 ) || ( m_platform == OS_ME ) )
					{
						CONFIGRET ret=0;
						
						if( (ret = CM_Locate_DevNodeA(&device_inst_id, DeviceInstanceId, CM_LOCATE_DEVNODE_NORMAL)) == CR_SUCCESS )
						{
							_p_progress->UpdateProgress();
							for( loop = 0; loop < 10; loop ++ ) //try 10 times before giving up
							{
								ret = CM_Disable_DevNode( device_inst_id, 0 );
								if( ret == CR_SUCCESS )
								{
									break;
								}
								::Sleep( 250 );
							}
							if( ret != CR_SUCCESS )
							{
								err = CONFIGRET_to_ST_ERROR( ret );
								goto cleanup_DeviceInfo;
							}
							_p_progress->UpdateProgress();
							for( loop = 0; loop < 10; loop ++ ) //try 10 times before giving up
							{
								ret = CM_Enable_DevNode( device_inst_id, 0 );
								if( ret == CR_SUCCESS )
								{
									break;
								}
								::Sleep( 250 );
							}
							if( ret != CR_SUCCESS )
							{
								err = CONFIGRET_to_ST_ERROR( ret );
								goto cleanup_DeviceInfo;
							}
						}
						else
						{
							//err = CONFIGRET_to_ST_ERROR( ret );
							//goto cleanup_DeviceInfo;
							continue;
						}
					}
				}
				break;
			}
        }
    }

    
    //
    //  Cleanup.
    //    
cleanup_DeviceInfo:

    if( buffer ) 
		LocalFree( buffer );

    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	_p_progress->UpdateProgress();

	if( err != STERR_NONE )
	{
		wchar_t	 more_error_info[MAX_PATH];
		wsprintf( more_error_info, L"0x%x", this->GetSystemLastError() );
		m_p_error->SaveStatus(this, more_error_info);
	}
	return err;
}

ST_ERROR CStDriveRefresher::CONFIGRET_to_ST_ERROR( DWORD _cfg_err )
{
	ST_ERROR err = STERR_NONE;

	switch( _cfg_err )
	{
		case CR_SUCCESS                  :
			err = STERR_NONE;
			break;
		case CR_OUT_OF_MEMORY            :
			err = STERR_NO_MEMORY;
			break;
		case CR_INVALID_POINTER          :
		case CR_INVALID_FLAG             :
		case CR_INVALID_DEVINST          :
		case CR_INVALID_RES_DES          :
		case CR_INVALID_LOG_CONF         :
		case CR_INVALID_ARBITRATOR       :
		case CR_INVALID_NODELIST         :
		case CR_DEVNODE_HAS_REQS         :
		case CR_INVALID_RESOURCEID       :
		case CR_DLVXD_NOT_FOUND          :  
		case CR_NO_SUCH_DEVINST          :
		case CR_NO_MORE_LOG_CONF         :
		case CR_NO_MORE_RES_DES          :
		case CR_ALREADY_SUCH_DEVNODE     :
		case CR_INVALID_RANGE_LIST       :
		case CR_INVALID_RANGE            :
		case CR_FAILURE                  :
		case CR_NO_SUCH_LOGICAL_DEV      :
		case CR_CREATE_BLOCKED           :
		case CR_NOT_SYSTEM_VM            :  
		case CR_REMOVE_VETOED            :
		case CR_APM_VETOED               :
		case CR_INVALID_LOAD_TYPE        :
		case CR_BUFFER_SMALL             :
		case CR_NO_ARBITRATOR            :
		case CR_NO_REGISTRY_HANDLE       :
		case CR_REGISTRY_ERROR           :
		case CR_INVALID_DEVICE_ID        :
		case CR_INVALID_DATA             :
		case CR_INVALID_API              :
		case CR_DEVLOADER_NOT_READY      :
		case CR_NEED_RESTART             :
		case CR_NO_MORE_HW_PROFILES      :
		case CR_DEVICE_NOT_THERE         :
		case CR_NO_SUCH_VALUE            :
		case CR_WRONG_TYPE               :
		case CR_INVALID_PRIORITY         :
		case CR_NOT_DISABLEABLE          :
		case CR_FREE_RESOURCES           :
		case CR_QUERY_VETOED             :
		case CR_CANT_SHARE_IRQ           :
		case CR_NO_DEPENDENT             :
		case CR_SAME_RESOURCES           :
		case CR_NO_SUCH_REGISTRY_KEY     :
		case CR_INVALID_MACHINENAME      :  
		case CR_REMOTE_COMM_FAILURE      :  
		case CR_MACHINE_UNAVAILABLE      :  
		case CR_NO_CM_SERVICES           :  
		case CR_ACCESS_DENIED            :  
		case CR_CALL_NOT_IMPLEMENTED     :
		case CR_INVALID_PROPERTY         :
		case CR_DEVICE_INTERFACE_ACTIVE  :
		case CR_NO_SUCH_DEVICE_INTERFACE :
		case CR_INVALID_REFERENCE_STRING :
		case CR_INVALID_CONFLICT_LIST    :
		case CR_INVALID_INDEX            :
		case CR_INVALID_STRUCTURE_SIZE   :
		default:
			err = STERR_ERROR_IN_CFGMGR32_API;
			break;
	}

	m_system_last_error = _cfg_err;
	return err;
}
