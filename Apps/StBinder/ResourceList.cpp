/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "StdAfx.h"
#include ".\resourcelist.h"
#include "..\\..\\common\\updater_res.h"
#include "..\\..\\common\\updater_restypes.h"

CResourceList::CResourceList(CString _groupName)
{
    m_ListHead = NULL;
	m_ListTail = NULL;
    m_count = 0;
	m_count_resource_ids = 20;
	//m_resource_ids = (int *)_malloc_dbg (m_count_resource_ids+2 * sizeof(int), _NORMAL_BLOCK, NULL, NULL);
	m_resource_ids = (int *)VirtualAlloc(NULL, (m_count_resource_ids+1) * sizeof(int), MEM_COMMIT, PAGE_READWRITE);
	int *pTmp = m_resource_ids;
	if ( pTmp )
		for ( int i = 0; i <= m_count_resource_ids; ++i )
		{
			*pTmp = i;
			++pTmp;
		}

	m_GroupName = _groupName;
}

CResourceList::~CResourceList(void)
{
	EmptyTheList();
	if ( m_resource_ids )
		VirtualFree( m_resource_ids, 0, MEM_RELEASE );
}

void CResourceList::GetResourceGroupName( CString& _csGrpName )
{
	_csGrpName = m_GroupName;
}

void CResourceList::EmptyTheList(  )
{
    if ( m_ListHead )
    {
        while ( m_ListHead->pNext )
        {
            PRESOURCE_DATA pEntry = m_ListHead;

            m_ListHead = pEntry->pNext;
            delete (pEntry);
        }
        delete ( m_ListHead );
    }
    m_ListHead = NULL;
	m_ListTail = NULL;
    m_count = 0;
}

PRESOURCE_DATA CResourceList::FindEntry( USHORT _index )
{
    PRESOURCE_DATA pEntry = m_ListHead;

    if ( pEntry )
    {
        USHORT index = 0;
        
        while ( pEntry )
        {
            if ( index == _index )
                break;

            ++index;
            pEntry = pEntry->pNext;
        }
    }
    return pEntry;
}

int CResourceList::NextAvailableResourceID()
{
	int id = -1;
	int i;
	int *pTmp;
retry:
	pTmp = m_resource_ids;
	++pTmp; // skip zero
	for ( i = 1; i <= m_count_resource_ids; ++i )
		if ( *pTmp != 0 )
		{
			id = *pTmp;
			break;
		}
		else
			++pTmp;

	if ( id < 0 )
	{
		pTmp = (int *)VirtualAlloc(NULL, (m_count_resource_ids+10+1) * sizeof(int), MEM_COMMIT, PAGE_READWRITE);
		if ( pTmp )
		{
			int *pInit = pTmp;
			pInit += m_count_resource_ids+1;
			memcpy( pTmp, m_resource_ids, (m_count_resource_ids+1) * sizeof(int));
			for ( i = m_count_resource_ids+1; i < m_count_resource_ids+10; ++i )
			{
				*pInit = i;
				++pInit;
			}
			VirtualFree( m_resource_ids, 0, MEM_RELEASE );
			m_resource_ids = pTmp;
			m_count_resource_ids += 10;
		}
		goto retry;
	}

	if ( id > 0 )
		ReserveResourceID( id );

	return id;
}

void CResourceList::ReserveResourceID( int _id )
{
	if ( _id > 0 && _id <= m_count_resource_ids)
	{
		int *pTmp = m_resource_ids;
		pTmp += _id;
		*pTmp = 0;
	}
}

void CResourceList::FreeResourceID( int _id )
{
	if ( _id > 0  && _id <= m_count_resource_ids )
	{
		int *pTmp = m_resource_ids;
		pTmp += _id;
		*pTmp = _id;
	}
}

USHORT CResourceList::AddResource(CString& _pathName, DWORD _dwSize, CString& _szVersion, USHORT _tagId, SYSTEMTIME& _dateTime)
{
    USHORT index = NO_INDEX;

    PRESOURCE_DATA pNewEntry = new (RESOURCE_DATA);
    if ( pNewEntry )
    {
		int posFname;
		// get filename; get resource id
		posFname = _pathName.ReverseFind(0x5c);
		pNewEntry->szFileName = _pathName.Tokenize(L"\\", posFname);

        pNewEntry->bBoundResource = FALSE;
        pNewEntry->bMarkedForDeletion = FALSE;
        pNewEntry->bMarkedForReplacement = FALSE;
        pNewEntry->szResourcePathName = _pathName;
        pNewEntry->dwResourceSize = _dwSize;
        pNewEntry->sResourceDateTime = _dateTime;
        pNewEntry->szVersionInfo = _szVersion;
        pNewEntry->usTagId = _tagId;
		pNewEntry->iResourceId = -1;
        pNewEntry->pNext = NULL;
        if ( m_ListHead == NULL )
        {
            m_ListHead = pNewEntry;
            m_ListTail = m_ListHead;
        }
        else
        {
            m_ListTail->pNext = pNewEntry;
            m_ListTail = pNewEntry;
        }

        index = m_count;
        ++m_count;

    }

    return index;
}

USHORT CResourceList::AddResource(CString& _fileName, CString& _resName, DWORD _dwSize, CString& _szVersion, USHORT _tagId, int _resId)
{
    USHORT index = NO_INDEX;

    PRESOURCE_DATA pNewEntry = new (RESOURCE_DATA);
    if ( pNewEntry )
    {
        pNewEntry->bBoundResource = TRUE;
        pNewEntry->bMarkedForDeletion = FALSE;
        pNewEntry->bMarkedForReplacement = FALSE;
		pNewEntry->szFileName = _fileName;
        pNewEntry->szResourcePathName = _resName;
        pNewEntry->dwResourceSize = _dwSize;
        pNewEntry->iResourceId = _resId;
        pNewEntry->szVersionInfo = _szVersion;
        pNewEntry->usTagId = _tagId;
        pNewEntry->pNext = NULL;
        if ( m_ListHead == NULL )
        {
            m_ListHead = pNewEntry;
            m_ListTail = m_ListHead;
        }
        else
        {
            m_ListTail->pNext = pNewEntry;
            m_ListTail = pNewEntry;
        }

		ReserveResourceID( _resId );
        index = m_count;
        ++m_count;
    }

    return index;
}

void CResourceList::RemoveResource(USHORT _index)
{
    if ( m_ListHead )
    {
        PRESOURCE_DATA pEntry = m_ListHead;
        PRESOURCE_DATA pPrev = NULL;
        USHORT index = 0;
        
        while ( pEntry )
        {
            if ( index == _index )
            {
                if (pEntry->bBoundResource)
                {
                    if ( pEntry->bMarkedForDeletion == FALSE )
                    {
                        pEntry->bMarkedForDeletion = TRUE;
						if ( pEntry->bMarkedForReplacement )
							pEntry->bMarkedForReplacement = FALSE;
                    }
                }
                else
                {
                    if ( pPrev )
                    {
                        pPrev->pNext = pEntry->pNext;
                    }
                    else // deleting the head
                    {
                        m_ListHead = pEntry->pNext;
						if ( !m_ListHead )
							m_ListTail = NULL;
                    }

					if ( m_ListTail == pEntry )
						m_ListTail = pPrev;

					FreeResourceID( pEntry->iResourceId );
                    delete ( pEntry );
                    --m_count;
                }
                break;
            }

            // keep going
            ++index;
            pPrev = pEntry;
            pEntry = pEntry->pNext;
        }
    }
}

USHORT CResourceList::GetCount()
{
    return m_count;
}


BOOL CResourceList::GetAtIndex(USHORT _index, CString& _pathName, DWORD& _pdwSize, CString& _szVersion, USHORT& _tagId, SYSTEMTIME& _DateTime)
{
    BOOL returnStatus = FALSE;

    PRESOURCE_DATA pEntry = FindEntry(_index);

    if( pEntry )
    {
        _pathName       = pEntry->szResourcePathName;
        _pdwSize        = pEntry->dwResourceSize;
        _DateTime       = pEntry->sResourceDateTime;
        _szVersion      = pEntry->szVersionInfo;
        _tagId          = pEntry->usTagId;
        returnStatus    = TRUE;
     }

    return returnStatus;
}

BOOL CResourceList::GetAtIndex(USHORT _index, CString& _pathName, DWORD& _pdwSize, CString& _szVersion, USHORT& _tagId)
{
    BOOL returnStatus = FALSE;

    PRESOURCE_DATA pEntry = FindEntry(_index);

    if( pEntry )
    {
        _pathName       = pEntry->szResourcePathName;
        _pdwSize        = pEntry->dwResourceSize;
        _szVersion      = pEntry->szVersionInfo;
        _tagId          = pEntry->usTagId;
        returnStatus    = TRUE;
     }

    return returnStatus;
}

BOOL CResourceList::IsBoundResource ( USHORT _index )
{
    BOOL returnStatus = FALSE;
    PRESOURCE_DATA pEntry = FindEntry(_index);

    if ( pEntry )
        returnStatus = pEntry->bBoundResource;

    return returnStatus;
}

void CResourceList::SetBoundResource ( USHORT _index, BOOL _state )
{
    PRESOURCE_DATA pEntry = FindEntry(_index);

    if ( pEntry )
       pEntry->bBoundResource = _state;
}

void CResourceList::SetPathname(USHORT _index, CString& _pathname)
{
    PRESOURCE_DATA pEntry = FindEntry(_index);

    if (  pEntry )
        pEntry->szResourcePathName = _pathname;
}

BOOL CResourceList::IsMarkedForDeletion(USHORT _index)
{
    PRESOURCE_DATA pEntry = FindEntry(_index);

    BOOL returnStatus = FALSE;

    if ( pEntry )
        if (pEntry->bMarkedForDeletion)
            returnStatus = TRUE;

    return returnStatus;
}

BOOL CResourceList::IsMarkedForReplacement(USHORT _index)
{
    PRESOURCE_DATA pEntry = FindEntry(_index);

    BOOL returnStatus = FALSE;

    if ( pEntry )
        if (pEntry->bMarkedForReplacement)
            returnStatus = TRUE;

    return returnStatus;
}

void CResourceList::SetForReplacement(USHORT _index, BOOL _state)
{
    PRESOURCE_DATA pEntry = FindEntry(_index);

    if ( pEntry )
    {
        pEntry->bMarkedForReplacement = _state;
		if ( pEntry->bMarkedForDeletion )
			pEntry->bMarkedForDeletion = FALSE;
    }

    return;
}

int CResourceList::GetResId(USHORT _index)
{
    PRESOURCE_DATA pEntry = FindEntry(_index);

    int resId = -1;

    if ( pEntry )
        resId = pEntry->iResourceId;

    return resId;
}


CString CResourceList::GetFileName(USHORT _index)
{
    PRESOURCE_DATA pEntry = FindEntry(_index);

    if ( pEntry )
		return pEntry->szFileName;

	return NULL;
}


int CResourceList::SetResId(USHORT _index, int _resId)
{
    PRESOURCE_DATA pEntry = FindEntry(_index);

    int resId = -1;

    if ( pEntry )
        pEntry->iResourceId = _resId;

    return resId;
}
