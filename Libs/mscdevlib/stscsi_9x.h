/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StScsi_9x.h: interface for the CStScsi_9x class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STSCSI_9X_H__FC04488C_1B3F_43DE_AB7D_957F7B48E326__INCLUDED_)
#define AFX_STSCSI_9X_H__FC04488C_1B3F_43DE_AB7D_957F7B48E326__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stscsi.h"

typedef void *LPSRB;
typedef DWORD (*GETASPI32SUPPORTINFO)(void);
typedef DWORD (*SENDASPI32COMMAND)(LPSRB);

#pragma pack(1)

typedef struct _DRIVE_MAP_INFO {
	BYTE dmiAllocationLength;
    BYTE dmiInfoLength;
    BYTE dmiFlags;
    BYTE dmiInt13Unit;
    DWORD dmiAssociatedDriveMap;
    DWORD dmiPartitionStartRBA;
	DWORD reserved;
} DRIVE_MAP_INFO;

#pragma pack()

class CStScsi_9x : public CStScsi  
{

public:
	
	CStScsi_9x(CStUpdater *pUpdater, string name="CStScsi_9x");
	virtual ~CStScsi_9x();

	ST_ERROR SendCommand( CStByteArray* command_arr, UCHAR cdb_len, BOOL direction_out, 
							 CStByteArray& response_arr, ULONG ulTimeOut);
	ST_ERROR Initialize(USHORT driveToFind, USHORT UpgradeOrNormal);

	virtual ST_ERROR Open();
	virtual ST_ERROR Lock(BOOL _media_new);
	virtual ST_ERROR Unlock(BOOL _media_new);
	virtual ST_ERROR AcquireFormatLock(BOOL _media_new);
	virtual ST_ERROR ReleaseFormatLock(BOOL _media_new);
	virtual ST_ERROR ReadGeometry( PDISK_GEOMETRY _p_dg );
	virtual ST_ERROR Dismount();
	virtual ST_ERROR Close();

private:

    UCHAR					m_adapter_id;
	UCHAR					m_lun;
	HINSTANCE				m_handle_dll_wnaspi32;
	GETASPI32SUPPORTINFO	m_p_fn_get_aspi32_support_info;
	SENDASPI32COMMAND		m_p_fn_send_aspi32_command;

	ST_ERROR GetNumAdapters(UCHAR& _count);
	ST_ERROR GetPhysicalDriveNumber(UCHAR _ha_id, UCHAR& _drive_number);
	ST_ERROR PhysicalDriveNumberToLogicalDriveLetter(UCHAR _phy, wchar_t& _drive_letter);
	ST_ERROR GuessTheRightDriveLetter( wstring drive_letters, wchar_t& m_drive_letter );
	ST_ERROR GetDriveMap(wchar_t _drive_letter, DRIVE_MAP_INFO& _map);
	ST_ERROR AcquirePhysicalLock(WORD _permissions);
	ST_ERROR AcquireLogicalLock(WORD _permissions);

protected:

	long ScsiInquiry(PINQUIRYDATA inquiry);
	ST_ERROR Lock(WORD Permissions, UCHAR _lock_type);

};

#endif // !defined(AFX_STSCSI_9X_H__FC04488C_1B3F_43DE_AB7D_957F7B48E326__INCLUDED_)
