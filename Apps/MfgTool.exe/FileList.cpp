/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "FileList.h"

CFileList::CFileList(void)
{
    m_ListHead = NULL;
	m_ListTail = NULL;
	m_ListCount = 0;
	m_csRootPath = _T("");
}

CFileList::CFileList(CString _csRootPath)
{
    m_ListHead = NULL;
	m_ListTail = NULL;
	m_ListCount = 0;
	m_csRootPath = _csRootPath;
}

CFileList::~CFileList(void)
{
	EmptyTheList();
}

void CFileList::EmptyTheList(  )
{
	if( m_ListHead )
	    EmptySubFolder(this);
    m_ListHead = NULL;
	m_ListTail = NULL;
    m_ListCount = 0;
}

void CFileList::EmptySubFolder( CFileList *pFileList )
{
    PFILEITEM pEntry = pFileList->m_ListHead;
    while ( pEntry )
    {
		if ( pEntry->m_pSubFolderList)
		{
			EmptySubFolder(pEntry->m_pSubFolderList);
			delete pEntry->m_pSubFolderList;
			pEntry->m_pSubFolderList = NULL;
		}

        pEntry = (PFILEITEM)pEntry->m_pNext;
        delete (pFileList->m_ListHead);
		pFileList->m_ListHead = pEntry;
    }

	pFileList->m_ListHead = NULL;
	pFileList->m_ListTail = NULL;
    pFileList->m_ListCount = 0;
}

int CFileList::AddFile(CString _pathName, int _action)
{
    int index = NO_INDEX;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

    PFILEITEM pNewEntry = new (FILEITEM);
    if ( pNewEntry )
    {
		// save filename
		pNewEntry->m_csFilePathName = _pathName;

		if( _action != RESOURCE_FILE )
		{
			pNewEntry->m_csFileName = _pathName.Right(_pathName.GetLength() - _pathName.ReverseFind(_T('\\'))-1);
			hFind = FindFirstFile(pNewEntry->m_csFilePathName, &FindFileData);
			if( hFind != INVALID_HANDLE_VALUE )
			{
				pNewEntry->m_dwAttr = FindFileData.dwFileAttributes;
				if( !(pNewEntry->m_dwAttr & FILE_ATTRIBUTE_DIRECTORY) )
				{
					pNewEntry->m_timestamp = FindFileData.ftLastWriteTime;
					pNewEntry->m_dwFileSize = FindFileData.nFileSizeLow;
				}
				else
					pNewEntry->m_dwFileSize = 0;
			}
			pNewEntry->m_resId = 0;
		}

		pNewEntry->m_pSubFolderList = NULL;

		pNewEntry->m_action = pNewEntry->m_currentAction = _action;
        pNewEntry->m_pNext = NULL;
        if ( m_ListHead == NULL )
        {
            m_ListHead = pNewEntry;
            m_ListTail = m_ListHead;
        }
        else
        {
            m_ListTail->m_pNext = pNewEntry;
            m_ListTail = pNewEntry;
        }

        index = m_ListCount;
        ++m_ListCount;

    }

    return index;
}

int CFileList::AddFile(CString _pathName, int _action, int _resId)
{
    int index = NO_INDEX;
	PFILEITEM pNewEntry = new (FILEITEM);
    if ( pNewEntry )
    {
		// save filename
		pNewEntry->m_csFilePathName = _pathName;
		pNewEntry->m_csFileName = _pathName;
		pNewEntry->m_pSubFolderList = NULL;

		pNewEntry->m_action = pNewEntry->m_currentAction = _action;
        pNewEntry->m_pNext = NULL;
		pNewEntry->m_resId = _resId;

        if ( m_ListHead == NULL )
        {
            m_ListHead = pNewEntry;
            m_ListTail = m_ListHead;
        }
        else
        {
            m_ListTail->m_pNext = pNewEntry;
            m_ListTail = pNewEntry;
        }

        index = m_ListCount;
        ++m_ListCount;
	}
	return index;
}

void CFileList::RemoveFile(CString _pathName)
{
    if ( m_ListHead )
    {
        PFILEITEM pEntry = m_ListHead;
        PFILEITEM pPrev = NULL;
        int index = 0;
        
        while ( pEntry )
        {
			if ( pEntry->m_csFilePathName.CompareNoCase(_pathName) == 0 )
            {
				if (pEntry->m_pSubFolderList)
				{
					EmptySubFolder(pEntry->m_pSubFolderList);
					delete(pEntry->m_pSubFolderList);
					pEntry->m_pSubFolderList = NULL;
				}

                if ( pPrev )
                {
                    pPrev->m_pNext = pEntry->m_pNext;
                }
                else // deleting the head
                {
                    m_ListHead = (PFILEITEM)pEntry->m_pNext;
					if ( !m_ListHead )
						m_ListTail = NULL;
                }

				if ( m_ListTail == pEntry )
					m_ListTail = pPrev;

                delete ( pEntry );
                --m_ListCount;
                break;
            }

            // keep going
            ++index;
            pPrev = pEntry;
            pEntry = (PFILEITEM)pEntry->m_pNext;
        }
    }
}

void CFileList::RemoveFile(int _index)
{
    if ( m_ListHead )
    {
        PFILEITEM pEntry = m_ListHead;
        PFILEITEM pPrev = NULL;
        int index = 0;
        
        while ( pEntry )
        {
			if ( index == _index )
            {
				if (pEntry->m_pSubFolderList)
				{
					EmptySubFolder(pEntry->m_pSubFolderList);
					delete(pEntry->m_pSubFolderList);
					pEntry->m_pSubFolderList = NULL;
				}

                if ( pPrev )
                {
                    pPrev->m_pNext = pEntry->m_pNext;
                }
                else // deleting the head
                {
                    m_ListHead = (PFILEITEM)pEntry->m_pNext;
					if ( !m_ListHead )
						m_ListTail = NULL;
                }

				if ( m_ListTail == pEntry )
					m_ListTail = pPrev;

                delete ( pEntry );
                --m_ListCount;
                break;
            }

            // keep going
            ++index;
            pPrev = pEntry;
            pEntry = (PFILEITEM)pEntry->m_pNext;
        }
    }
}

void CFileList::RemoveAll()
{
	PFILEITEM pEntry = m_ListHead;
	while (pEntry)
	{
		if (pEntry && pEntry->m_pSubFolderList)
		{
			EmptySubFolder(pEntry->m_pSubFolderList);
 			delete(pEntry->m_pSubFolderList);
			pEntry->m_pSubFolderList = NULL;
		}
		pEntry = (PFILEITEM)pEntry->m_pNext;
		delete m_ListHead;
		m_ListHead = pEntry;
	}
	m_ListHead = m_ListTail = NULL;
	m_ListCount = 0;
}

CFileList::PFILEITEM CFileList::GetAt(int _index)
{
	PFILEITEM pEntry = m_ListHead;
	int i = 0;
	while (pEntry)
	{
		if (i == _index)
			break;

		++i;
		pEntry = (PFILEITEM)pEntry->m_pNext;
	}

	return pEntry;
}


CFileList::PFILEITEM CFileList::FindFile(CString _pathName, int& _index)
{
	PFILEITEM pEntry = m_ListHead;
	int i = 0;
	while (pEntry)
	{
		if (pEntry->m_csFilePathName.CompareNoCase(_pathName) == 0)
			break;

		++i;
		pEntry = (PFILEITEM)pEntry->m_pNext;
	}

	_index = i;
	return pEntry;
}

CString	CFileList::GetPathNameAt(int _index)
{
	if (_index != NO_INDEX)
	{
		PFILEITEM pItem = GetAt(_index);

		if ( pItem )
			return pItem->m_csFilePathName;
	}
		return NULL;
}

CString	CFileList::GetFileNameAt(int _index)
{
	if (_index != NO_INDEX)
	{
		PFILEITEM pItem = GetAt(_index);
		if ( pItem )
			return pItem->m_csFileName;
	}

	return NULL;
}

BOOL CFileList::ChangeFile(int _index, CString _pathName)
{
	BOOL retStatus = FALSE;

	if (_index != NO_INDEX)
	{
		PFILEITEM pItem = GetAt(_index);
		if ( pItem )
		{
			WIN32_FIND_DATA FindFileData;
			HANDLE hFind;
			CString csFileName = _pathName.Right(_pathName.GetLength() - _pathName.ReverseFind(_T('\\'))-1);
			hFind = FindFirstFile(pItem->m_csFilePathName, &FindFileData);
			if( hFind != INVALID_HANDLE_VALUE )
			{
				pItem->m_dwAttr = FindFileData.dwFileAttributes;
				if( !(pItem->m_dwAttr & FILE_ATTRIBUTE_DIRECTORY) )
				{
					pItem->m_timestamp = FindFileData.ftLastWriteTime;
					pItem->m_dwFileSize = FindFileData.nFileSizeLow;
				}
				else
					pItem->m_dwFileSize = 0;

				pItem->m_resId = 0;
				pItem->m_csFileName = csFileName;
				pItem->m_csFilePathName = _pathName;
				pItem->m_action = IN_EDIT_ADD;
				retStatus = TRUE;
			}
		}
	}
	return retStatus;
}


void CFileList::CommitEdits(void)
{
	EnumCommitEdits(this);
}

void CFileList::EnumCommitEdits(CFileList *_pFileList)
{
	PFILEITEM pEntry = _pFileList->m_ListHead;
	while (pEntry)
	{
		if (pEntry->m_pSubFolderList)
			EnumCommitEdits(pEntry->m_pSubFolderList);

		if (pEntry->m_action == IN_EDIT_ADD)
		{
			pEntry->m_action= pEntry->m_currentAction = COPY_TO_TARGET;
		}
		else if (pEntry->m_action == IN_EDIT_DELETE)
			pEntry->m_action = pEntry->m_currentAction = DELETE_FROM_TARGET;
		else if (pEntry->m_action == IN_EDIT_CREATE_DIR)
			pEntry->m_action = CREATE_DIR;


		pEntry = (PFILEITEM)pEntry->m_pNext;
	}
}

void CFileList::RemoveEdits(void)
{
	EnumRemoveEdits(this);
}

void CFileList::EnumRemoveEdits(CFileList *_pFileList)
{
	PFILEITEM pEntry = _pFileList->m_ListHead;
	while (pEntry)
	{
		if (pEntry->m_pSubFolderList)
		{
			EnumRemoveEdits(pEntry->m_pSubFolderList);
			if (pEntry->m_action == IN_EDIT_CREATE_DIR)
			{
				delete pEntry->m_pSubFolderList;
				pEntry->m_pSubFolderList = NULL;
			}
		}

		if (pEntry->m_action == IN_EDIT_ADD || pEntry->m_action == IN_EDIT_CREATE_DIR)
		{
			PFILEITEM pNext = (PFILEITEM) pEntry->m_pNext;
			RemoveFile(pEntry->m_csFilePathName);
			pEntry = pNext;
			continue;
		}
		else if (pEntry->m_action == IN_EDIT_DELETE)
		{
				pEntry->m_action = pEntry->m_currentAction;
		}

		pEntry = (PFILEITEM)pEntry->m_pNext;
	}
}


int CFileList::GetTotalCount()
{
	return EnumGetTotalCount(this);
}

int CFileList::EnumGetTotalCount(CFileList * _pFileList)
{
	PFILEITEM pEntry = _pFileList->m_ListHead;
	int count = 0;

	while (pEntry)
	{
		if (pEntry->m_pSubFolderList)
			count += EnumGetTotalCount(pEntry->m_pSubFolderList);

		if (pEntry->m_action != DELETE_FROM_TARGET && pEntry->m_action != IN_EDIT_DELETE)
			++count;

		pEntry = (PFILEITEM)pEntry->m_pNext;
	}
	return count;
}
