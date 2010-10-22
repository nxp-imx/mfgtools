/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#define NO_TAGID        0
#define NO_INDEX        0xFFFF

typedef struct _STVERSIONINFO
{
	USHORT	m_high;
	USHORT	m_mid;
	USHORT	m_low;
} STVERSIONINFO, *PSTVERSIONINFO;

typedef struct _RESOURCE_DATA
{
    _RESOURCE_DATA *pNext;

    BOOL            bBoundResource;
    BOOL            bMarkedForDeletion;
    BOOL            bMarkedForReplacement;
    CString         szVersionInfo;
    int             iResourceId;
	CString			szFileName;
    CString         szResourcePathName;
    DWORD           dwResourceSize;
    USHORT          usTagId;
    SYSTEMTIME      sResourceDateTime;
} RESOURCE_DATA, *PRESOURCE_DATA;

class CResourceList
{
private:
    PRESOURCE_DATA  FindEntry( USHORT _index );

    PRESOURCE_DATA  m_ListHead;
    PRESOURCE_DATA  m_ListTail;
	CString			m_GroupName;

	int				*m_resource_ids;
	int				m_count_resource_ids;
    USHORT          m_count;

public:
    CResourceList(CString _groupName);
    ~CResourceList(void);

	void	GetResourceGroupName( CString& _csGrpName );
	void	EmptyTheList();
    USHORT  AddResource(CString& _pathName, DWORD _dwSize, CString& _szVersion, USHORT _tagId, SYSTEMTIME& _datetime);
    USHORT  AddResource(CString& _fileName, CString& _resName, DWORD _dwSize, CString& _szVersion, USHORT _tagId, int _resId);

	int		NextAvailableResourceID();
	void	ReserveResourceID( int _id );
	void	FreeResourceID( int _id );

    void    RemoveResource(USHORT _index);
    USHORT  GetCount();
    BOOL    GetAtIndex(USHORT _index, CString& _pathName, DWORD& _pdwSize, CString& _szVersion, USHORT& _tagId, SYSTEMTIME& _pDateTime);
    BOOL    GetAtIndex(USHORT _index, CString& _pathName, DWORD& _pdwSize, CString& _szVersion, USHORT& _tagId);
    BOOL    IsBoundResource(USHORT _index);
    BOOL    IsMarkedForDeletion(USHORT _index);
    BOOL    IsMarkedForReplacement(USHORT _index);
    void    SetForReplacement(USHORT _index, BOOL _state);
    void    SetBoundResource(USHORT _index, BOOL _state);
    void    SetPathname(USHORT _index, CString& _pathName);
    int     GetResId(USHORT _index);
	CString GetFileName(USHORT _index);
    int     SetResId(USHORT _index, int _resId);
	LANGID	GetLangID() { return m_LangID; }
	void	SetLangID(LANGID _langID) { m_LangID = _langID; }

	LANGID	m_LangID;


};
