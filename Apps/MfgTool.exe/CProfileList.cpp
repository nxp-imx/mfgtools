
#include "stdafx.h"
#include "PlayerProfile.h"
#include "CProfileList.h"



CProfileList::CProfileList(void)
{
	m_ListHead	= NULL;
	m_ListTail	= NULL;
	m_Count		= 0;
}

CProfileList::~CProfileList(void)
{
	while( m_ListTail )
	{
		PPROFILELISTITEM pItem = m_ListTail;
		m_ListTail = (PPROFILELISTITEM) m_ListTail->pPrev;
		if (pItem->pProfile)
			delete pItem->pProfile;
		delete pItem;
	}
}

void CProfileList::Add(CPlayerProfile *_newProfile)
{
	PPROFILELISTITEM pProfileItem;

	pProfileItem = new PROFILELISTITEM;

	pProfileItem->pNext = NULL;
	pProfileItem->pPrev = NULL;
	pProfileItem->pProfile = _newProfile;

	if(	m_ListTail ) // add to end of list
	{
		m_ListTail->pNext = pProfileItem;
		pProfileItem->pPrev = m_ListTail;
		m_ListTail = pProfileItem;
	}
	else // new list
	{
		m_ListHead = pProfileItem;
		m_ListTail = m_ListHead;
	}
	++m_Count;
}

void CProfileList::Remove(int _index)
{
	int i = 0;
	PPROFILELISTITEM pProfileItem = m_ListHead;
	PPROFILELISTITEM pProfilePrevItem = NULL;
	PPROFILELISTITEM pProfileNextItem = NULL;

	while( i < _index && pProfileItem)
	{
		pProfilePrevItem = pProfileItem;
		pProfileItem = (PPROFILELISTITEM) pProfileItem->pNext;
		++i;
	}

	if( i == 0 && pProfileItem)  // removing head item
	{
		if( m_ListHead == m_ListTail ) // only one item in list
			m_ListTail = NULL;

		m_ListHead = (PPROFILELISTITEM) m_ListHead->pNext;

		if (pProfileItem->pProfile)
			delete pProfileItem->pProfile;

		delete pProfileItem;
		--m_Count;
	}
	else if( i == _index && pProfileItem) 
	{
		pProfileNextItem = (PPROFILELISTITEM) pProfileItem->pNext;
		pProfilePrevItem->pNext = pProfileItem->pNext;
		if( pProfileNextItem ) // removing mid-list item
			pProfileNextItem->pPrev = pProfilePrevItem;
		else	// removing tail item
			m_ListTail = pProfilePrevItem;

		if (pProfileItem->pProfile)
			delete pProfileItem->pProfile;

		delete pProfileItem;
		--m_Count;
	}
}

void CProfileList::RemoveAll()
{
	while( m_ListTail )
	{
		PPROFILELISTITEM pItem = m_ListTail;
		m_ListTail = (PPROFILELISTITEM) m_ListTail->pPrev;
		if (pItem->pProfile)
			delete pItem->pProfile;
		delete pItem;
	}
	m_ListHead = NULL;
	m_Count = 0;
}

CPlayerProfile * CProfileList::Get(int _index)
{
	int i = 0;
	CPlayerProfile *pProfile = NULL;
	PPROFILELISTITEM pProfileItem = m_ListHead;

	while( i < _index && pProfileItem)
	{
		pProfileItem = (PPROFILELISTITEM)pProfileItem->pNext;
		++i;
	}

	if( i == _index && pProfileItem)
		pProfile = pProfileItem->pProfile;

	return pProfile;
}

CPlayerProfile * CProfileList::Find(CString _csProfileName)
{
	CPlayerProfile *pProfile = NULL;
	PPROFILELISTITEM pProfileItem = m_ListHead;

	while( pProfileItem )
	{
		if( _csProfileName == pProfileItem->pProfile->GetName() )
			break;
		pProfileItem = (PPROFILELISTITEM)pProfileItem->pNext;
	}

	if( pProfileItem)
		pProfile = pProfileItem->pProfile;

	return pProfile;
}


int CProfileList::GetCount()
{
	return m_Count;
}


