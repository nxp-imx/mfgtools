#include "StdAfx.h"
#include ".\regscrublist.h"

CRegScrubList::CRegScrubList(void)
{
	m_RemoveDeviceCount = 0;
	m_pRemoveDeviceListHead = NULL;
	m_pRemoveDeviceListTail = NULL;
}

CRegScrubList::~CRegScrubList(void)
{
	if ( m_pRemoveDeviceListHead )
	{
		PREMOVEDEVICEITEM pNext, pPrev;

		pPrev = m_pRemoveDeviceListHead;
		pNext = (PREMOVEDEVICEITEM)m_pRemoveDeviceListHead->pNext;
		while (pPrev)
		{

			free (pPrev);
			pPrev = pNext;
			if (pNext)
				pNext = (PREMOVEDEVICEITEM)pNext->pNext;
		}
	}
}


/*****************************************************************************/
PREMOVEDEVICEITEM CRegScrubList::InsertItem(CString OrgMfg, CString mfg, CString OrgProduct, CString scsiproduct, CString usbvendor, CString usbproduct)
{

	if ( m_pRemoveDeviceListHead == NULL )
	{
		m_pRemoveDeviceListHead = (PREMOVEDEVICEITEM) new ( REMOVEDEVICEITEM );
		if ( m_pRemoveDeviceListHead == NULL )
		{
			ATLTRACE(_T("\r\n *** ERROR *** new() failed, ErrorCode = %d"), GetLastError());
			return NULL;
		}
		m_pRemoveDeviceListTail = m_pRemoveDeviceListHead;
		m_pRemoveDeviceListTail->pNext = NULL;
		m_pRemoveDeviceListTail->pPrev = NULL;
	}
	else
	{
		PREMOVEDEVICEITEM pNewItem;

		pNewItem = (PREMOVEDEVICEITEM) new( REMOVEDEVICEITEM );
		if (pNewItem == NULL)
		{
			ATLTRACE(_T("\r\n *** ERROR *** new() failed, ErrorCode = %d"), GetLastError());
			return NULL;
		}

		pNewItem->pPrev = m_pRemoveDeviceListTail;
		pNewItem->pNext = NULL;
		m_pRemoveDeviceListTail->pNext = pNewItem;
		m_pRemoveDeviceListTail = pNewItem;
	}

	m_pRemoveDeviceListTail->OrgMfg		= OrgMfg;
	m_pRemoveDeviceListTail->Mfg		= mfg;
	m_pRemoveDeviceListTail->OrgProduct	= OrgProduct;
	m_pRemoveDeviceListTail->Product	= scsiproduct;
	m_pRemoveDeviceListTail->USBVid		= usbvendor;
	m_pRemoveDeviceListTail->USBPid		= usbproduct;

	++m_RemoveDeviceCount;

    return m_pRemoveDeviceListTail;
}


/*****************************************************************************/

void CRegScrubList::RemoveItem(PREMOVEDEVICEITEM pItem)
{
	if ( pItem )
	{
		PREMOVEDEVICEITEM pListItem = m_pRemoveDeviceListHead;

		while ( pListItem )
		{
			if ( pListItem == pItem )
			{
				PREMOVEDEVICEITEM pPrevItem, pNextItem;

				if ( pListItem == m_pRemoveDeviceListHead )
				{
					pNextItem = (PREMOVEDEVICEITEM)m_pRemoveDeviceListHead->pNext;

					if (m_pRemoveDeviceListTail == m_pRemoveDeviceListHead)
						m_pRemoveDeviceListTail = (PREMOVEDEVICEITEM)m_pRemoveDeviceListTail->pNext;

					delete ( m_pRemoveDeviceListHead );

					m_pRemoveDeviceListHead = pNextItem;
					if (m_pRemoveDeviceListHead)
						m_pRemoveDeviceListHead->pPrev = NULL;
				}
				else
				{
					pPrevItem = (PREMOVEDEVICEITEM)pListItem->pPrev;
					pNextItem = (PREMOVEDEVICEITEM)pListItem->pNext;

					pPrevItem->pNext = pNextItem;

					if ( pNextItem )
						pNextItem->pPrev = pListItem->pPrev;
					else
						m_pRemoveDeviceListTail = (PREMOVEDEVICEITEM)pListItem->pPrev;

					delete ( pListItem );
				}
				--m_RemoveDeviceCount;
				pListItem = NULL; // our work is done here
			}
			else
				pListItem = (PREMOVEDEVICEITEM)pListItem->pNext;
		}
	}
}

/*****************************************************************************/

PREMOVEDEVICEITEM CRegScrubList::GetHead (void)
{
	return (m_pRemoveDeviceListHead);
}


/*****************************************************************************/

PREMOVEDEVICEITEM CRegScrubList::GetTail (void)
{
	return (m_pRemoveDeviceListTail);
}


/*****************************************************************************/

PREMOVEDEVICEITEM CRegScrubList::GetNext (PREMOVEDEVICEITEM pCurrent)
{
	return ((PREMOVEDEVICEITEM)pCurrent->pNext);
}


/*****************************************************************************/

PREMOVEDEVICEITEM CRegScrubList::GetPrev (PREMOVEDEVICEITEM pCurrent)
{
	return ((PREMOVEDEVICEITEM)pCurrent->pPrev);
}
