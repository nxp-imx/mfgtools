/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
/////////////////////////////////////////////////////////////////////////////
//
// MRUCombo.cpp: Implementation file for the CMRUComboBox class.
//
// Written by Michael Dunn <mdunn at inreach dot com>
//
/////////////////////////////////////////////////////////////////////////////
//
// Revision history:
//
//  9/9/1998: First release.
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MRUCombo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MRUC_DEFAULT_MRU_SIZE   10


/////////////////////////////////////////////////////////////////////////////
// CMRUComboBox constructor & destructor


//////////////////////////////////////////////////////////////////////////
//
// Function:    CMRUComboBox()
//
// Description:
//  Class constructor.
//
// Notes:
//  Calls base class constructor and initializes member variables.
//
//////////////////////////////////////////////////////////////////////////

CMRUComboBox::CMRUComboBox() : CComboBox(),
    m_bRefreshAfterAdd   ( FALSE ),
    m_bSaveAfterAdd      ( FALSE ),
    m_bSaveOnDestroy     ( TRUE ),
    m_nMaxMRUSize        ( MRUC_DEFAULT_MRU_SIZE ),
    m_pMRU               ( NULL ),
    m_bParamsChanged     ( FALSE )
{
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    CMRUComboBox()
//
// Description:
//  Class destructor.
//
//////////////////////////////////////////////////////////////////////////

CMRUComboBox::~CMRUComboBox()
{
                                        // Save the MRU if we need to.
    if ( m_bSaveOnDestroy )
        {
        if ( !SaveMRU() )
            {
            TRACE0("CMRUComboBox -- Warning - SaveMRU() in destructor failed. MRU was not saved.\n");
            }
        }

                                        // Free up the CRecentFileList object.
    if ( NULL != m_pMRU )
        {
        delete m_pMRU;
        }
}


BEGIN_MESSAGE_MAP(CMRUComboBox, CComboBox)
	//{{AFX_MSG_MAP(CMRUComboBox)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnCbnDropdown)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMRUComboBox MRU operations


//////////////////////////////////////////////////////////////////////////
//
// Function:    AddToMRU()
//
// Description:
//  Adds a string to the MRU list.
//
// Input:
//  szNewItem: [in] The string to add.
//
// Returns:
//  TRUE if successful, FALSE if not.
//
//////////////////////////////////////////////////////////////////////////

BOOL CMRUComboBox::AddToMRU ( LPCTSTR szNewItem )
{
                                        // String can't be a null pointer!
    ASSERT ( NULL != szNewItem );

                                        // Allocate a new CRecentFileList 
                                        // if necessary.
    if ( NULL == m_pMRU )
        {
        if ( !AllocNewMRU() )
            {
            TRACE0("CMRUComboBox -- AllocNewMRU() failed in AddToMRU().\n");
            return FALSE;
            }
        }

                                        // Add it to the MRU list.
    m_pMRU->Add ( szNewItem );

                                        // Automagically refresh the combobox?
    if ( m_bRefreshAfterAdd )
        {
        RefreshCtrl();
        }

                                        // Automagically save the MRU?
    if ( m_bSaveAfterAdd )
        {
        SaveMRU();
        }


    return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    EmptyMRU()
//
// Description:
//  Removes all strings from the MRU list.
//
// Input:
//  Nothing.
//
// Returns:
//  Nothing.
//
//////////////////////////////////////////////////////////////////////////

void CMRUComboBox::EmptyMRU()
{
int i;

    if ( NULL != m_pMRU )
        {
                                        // Remove all strings from the MRU list.
                                        // Go in reverse order to keep the
                                        // strings from changing positions 
                                        // while we're doing our dirty work.
        for ( i = m_pMRU->GetSize() - 1; i >= 0; i-- )
            {
            m_pMRU->Remove(i);
            }
        }
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    LoadMRU()
//
// Description:
//  Loads an MRU from the registry or the app's INI file.
//
// Input:
//  Nothing.
//
// Returns:
//  TRUE if successful, FALSE if not.
//
//////////////////////////////////////////////////////////////////////////

BOOL CMRUComboBox::LoadMRU()
{
                                        // We always allocate a new
                                        // CRecentFileList object when loading.
    if ( !AllocNewMRU() )
        {
        TRACE0("CMRUComboBox -- AllocNewMRU() failed in LoadMRU().\n");
        return FALSE;
        }

    m_pMRU->ReadList();

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    SaveMRU()
//
// Description:
//  Writes the MRU to the registry or app's INI file.
//
// Input:
//  Nothing.
//
// Returns:
//  TRUE if successful, FALSE if not.
//
//////////////////////////////////////////////////////////////////////////

BOOL CMRUComboBox::SaveMRU()
{
                                        // If we haven't created a 
                                        // CRecentFileList yet, then there's
                                        // nothing to save.
    if ( NULL == m_pMRU )
        {
        TRACE0("CMRUComboBox -- SaveMRU failed - no CRecentFileList created.\n");
        return FALSE;
        }

                                        // Check that the CRecentFileList 
                                        // parameters are kosher.
    if ( !VerifyMRUParams() )
        {
        TRACE0("CMRUComboBox -- SaveMRU() failed - params not set.\n");
        return FALSE;
        }

                                        // If the registry key/value strings
                                        // have been changed, we need to make
                                        // a new CRecentFileList.
    if ( m_bParamsChanged )
        {
        if ( !AllocNewMRU() )
            {
            TRACE0("CMRUComboBox -- SaveMRU failed - couldn't reallocate CRecentFileList with new MRU params.\n");
            return FALSE;
            }
        }

    m_pMRU->WriteList();

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CMRUComboBox combobox control operations


//////////////////////////////////////////////////////////////////////////
//
// Function:    RefreshCtrl()
//
// Description:
//  Sets the contents of the combobox according to the MRU list.
//
// Input:
//  Nothing.
//
// Returns:
//  Nothing.
//
//////////////////////////////////////////////////////////////////////////

void CMRUComboBox::RefreshCtrl()
{
CString cstrComboText;

                                        // Save the contents of the edit
                                        // portion of the combobox.
    GetWindowText ( cstrComboText );

    ResetContent();

    for ( int i = 0; i < m_pMRU->GetSize(); i++ )
        {
                                        // Don't add empty strings to the combobox.
        if ( (*m_pMRU)[i].GetLength() > 0 )
            {
            if ( AddString ( (*m_pMRU)[i] ) < 0 )
                {
                TRACE1("CMRUComboBox -- Warning - RefreshCtrl() couldn't add MRU item %d to combobox.\n",
                       i );
                }
            }
        }

                                        // Restore the editbox text.
    SetWindowText ( cstrComboText );
}


/////////////////////////////////////////////////////////////////////////////
// CMRUComboBox data accessor functions


//////////////////////////////////////////////////////////////////////////
//
// Function:    SetMRURegKey()
//
// Description:
//  Sets the registry key (or INI file section) in which the MRU will be
//  saved.
//
// Input:
//  szRegKey: [in] The key/section name.
//
// Returns:
//  Nothing.
//
//////////////////////////////////////////////////////////////////////////

void CMRUComboBox::SetMRURegKey ( LPCTSTR szRegKey )
{
                                        // The key name can't be a null string.
    ASSERT ( NULL != szRegKey );

//    try
//        {
                                        // Store the reg key name & set the
                                        // changed flag.
        m_cstrRegKey = szRegKey;

        m_bParamsChanged = TRUE;
//        }
//    catch ( CMemoryException )
//        {
//        TRACE0("CMRUComboBox -- Memory exception in CMRUComboBox::SetMRURegKey()!\n");
//        throw;
//        }
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    GetMRURegKey
//
// Description:
//  Returns the current registry key or INI file section in which the MRU
//  will be saved.
//
// Input:
//  Nothing.
//
// Returns:
//  The key name.
//
//////////////////////////////////////////////////////////////////////////

const CString& CMRUComboBox::GetMRURegKey() const
{
    return m_cstrRegKey;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    SetMRUValueFormat()
//
// Description:
//  Sets the format to be used for writing MRU items to the registry or
//  the app's INI file.
//
// Input:
//  szValueFormat: [in] The format to use.
//
// Returns:
//  TRUE if the format is acceptable (i.e., contains "%d"), FALSE if not.
//
//////////////////////////////////////////////////////////////////////////

BOOL CMRUComboBox::SetMRUValueFormat ( LPCTSTR szValueFormat )
{
BOOL bRetVal = FALSE;

                                        // The key name can't be a null string.
    ASSERT ( NULL != szValueFormat );


                                        // Check that the format strings 
                                        // contains "%d"
    if ( NULL == _tcsstr ( szValueFormat, _T("%d") ) )
        {
        TRACE0("CMRUComboBox -- SetMRUValueFormat() returning FALSE - argument didn't contain \"%d\"\n");
        return FALSE;
        }
    else
        {
//        try
//            {
                                        // Save the format string and set the
                                        // changed flag.
            m_cstrRegValueFormat = szValueFormat;
            m_bParamsChanged = TRUE;
            bRetVal = TRUE;
//            }
//        catch ( CMemoryException )
//            {
//            TRACE0("CMRUComboBox -- Memory exception in CMRUComboBox::SetMRUValueFormat()!\n");
//            throw;
//            }
        }

    return bRetVal;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    GetMRUValueFormat
//
// Description:
//  Returns the current registry value format string.
//
// Input:
//  Nothing.
//
// Returns:
//  The format string.
//
//////////////////////////////////////////////////////////////////////////

const CString& CMRUComboBox::GetMRUValueFormat() const
{
    return m_cstrRegValueFormat;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    SetMaxMRUSize()
//
// Description:
//  Sets the max number of entries that the MRU can hold.
//
// Input:
//  nMaxSize: [in] The new MRU size.
//
// Returns:
//  The previous MRU size, or 0 if there was no previous MRU allocated, or
//  -1 on error.
//
// Notes:
//  This function always reallocates a CRecentFileList so that the size
//  change (and registry string changes, if any) takes place immediately.
//
//////////////////////////////////////////////////////////////////////////

int CMRUComboBox::SetMaxMRUSize ( int nMaxSize )
{
int nRetVal = m_nMaxMRUSize;

                                        // New size needs to be a positive
                                        // number.
    ASSERT ( nMaxSize >= 1 );

    if ( nMaxSize <= 0 )
        return -1;


    m_nMaxMRUSize = nMaxSize;

    if ( NULL == m_pMRU )
        {
        nRetVal = 0;                    // no previous size
        }

    if ( !AllocNewMRU() )
        {
        nRetVal = -1;                   // error!!
        TRACE0("CMRUComboBox -- SetMaxMRUSize() failed - couldn't allocate new CRecentFileList.\n");
        }

    return nRetVal;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    GetMaxMRUSize
//
// Description:
//  Returns the current max MRU size.
//
// Input:
//  Nothing.
//
// Returns:
//  The current max size.
//
//////////////////////////////////////////////////////////////////////////

int CMRUComboBox::GetMaxMRUSize() const
{
    return m_nMaxMRUSize;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    SetAutoSaveOnDestroy()
//
// Description:
//  Sets whether the CMRUComboBox will automatically save the MRU when
//  the object is destroyed.
//
// Input:
//  bAutoSave: [in] Flag: enable auto-saving?
//
// Returns:
//  The previous value of this setting.
//
//////////////////////////////////////////////////////////////////////////

BOOL CMRUComboBox::SetAutoSaveOnDestroy ( BOOL bAutoSave )
{
BOOL bRetVal = m_bSaveOnDestroy;

    m_bSaveOnDestroy = bAutoSave;

    return bRetVal;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    SetAutoSaveAfterAdd()
//
// Description:
//  Sets whether the CMRUComboBox will automatically save the MRU after
//  an item is added successfully.
//
// Input:
//  bAutoSave: [in] Flag: enable auto-saving?
//
// Returns:
//  The previous value of this setting.
//
//////////////////////////////////////////////////////////////////////////

BOOL CMRUComboBox::SetAutoSaveAfterAdd ( BOOL bAutoSave )
{
BOOL bRetVal = m_bSaveAfterAdd;

    m_bSaveAfterAdd = bAutoSave;

    return bRetVal;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    SetAutoRefreshAfterAdd()
//
// Description:
//  Sets whether the CMRUComboBox will automatically refresh the combobox
//  control after an item is added successfully.
//
// Input:
//  bAutoSave: [in] Flag: enable auto-refresh?
//
// Returns:
//  The previous value of this setting.
//
//////////////////////////////////////////////////////////////////////////

BOOL CMRUComboBox::SetAutoRefreshAfterAdd ( BOOL bAutoSave )
{
BOOL bRetVal = m_bRefreshAfterAdd;

    m_bRefreshAfterAdd = bAutoSave;

    return bRetVal;
}


/////////////////////////////////////////////////////////////////////////////
// CMRUComboBox misc. functions


//////////////////////////////////////////////////////////////////////////
//
// Function:    VerifyMRUParams()
//
// Description:
//  Checks the registry and size parameters and makes sure they're valid
//  for CRecentFileList.
//
// Input:
//  Nothing. (uses member variables)
//
// Returns:
//  TRUE if the params are OK, FALSE if not.
//
//////////////////////////////////////////////////////////////////////////

BOOL CMRUComboBox::VerifyMRUParams() const
{
BOOL bRetVal = TRUE;

                                        // 1. The registry key string must be
                                        //    non-empty.
    if ( m_cstrRegKey.IsEmpty() || 0 == m_cstrRegKey.GetLength() )
        {
        TRACE0("CMRUComboBox -- VerifyMRUParams() - registry key name not set.\n");
        bRetVal = FALSE;
        }

                                        // 2. The reg value must be non-empty
                                        //    and contain "%d"
    if ( m_cstrRegValueFormat.IsEmpty() || 
         0 == m_cstrRegValueFormat.GetLength() )
        {
        TRACE0("CMRUComboBox -- VerifyMRUParams() - registry value format not set.\n");
        bRetVal = FALSE;
        }
    else if ( -1 == m_cstrRegValueFormat.Find ( _T("%d") ) )
        {
        TRACE0("CMRUComboBox -- VerifyMRUParams() - registry value format doesn't contain \"%d\"\n");
        bRetVal = FALSE;
        }

                                        // 3. The Max MRU size must be > 0.
    if ( m_nMaxMRUSize <= 0 )
        {
        TRACE0("CMRUComboBox -- VerifyMRUParams() - max MRU size is set to <= 0\n");
        bRetVal = FALSE;
        }


    return bRetVal;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    AllocNewMRU()
//
// Description:
//  Allocates a new CRecentFileList, and copies the contents of the previous
//  MRU (if any) to the new one.
//
// Input:
//  Nothing.
//
// Returns:
//  TRUE if successful, FALSE if not.
//
//////////////////////////////////////////////////////////////////////////

BOOL CMRUComboBox::AllocNewMRU()
{
CString* acstrOldList = NULL;
int      nItemsToCopy;
int      i;

                                        // Make sure the MRU params are OK.
    if ( !VerifyMRUParams() )
        {
        TRACE0("CMRUComboBox -- AllocNewMRU() returning FALSE - MRU list params invalid or not set.\n");
        return FALSE;
        }


//    try
//        {
                                        // Figuring out how many strings to
                                        // copy: The lesser of the new MRU 
                                        // size and the previous MRU's size.
                                        // Of course, if there was no previous
                                        // MRU, then nothing will be copied.
        nItemsToCopy = m_nMaxMRUSize;

        if ( NULL != m_pMRU )
            {
            nItemsToCopy = __min ( m_nMaxMRUSize, m_pMRU->GetSize() );

                                        // Save the contents of the old MRU list.
            acstrOldList = new CString [ nItemsToCopy ];

            for ( i = 0; i < nItemsToCopy; i++ )
                {
                acstrOldList[i] = (*m_pMRU)[i];
                }

                                        // Nuke the old CRecentFileList object...
            delete m_pMRU;
            }

                                        // and make a new one!
        m_pMRU = new CRecentFileList ( 1, m_cstrRegKey,
                                       m_cstrRegValueFormat, m_nMaxMRUSize );


        // Copy the MRU strings if there was a previous MRU.  We add
        // the strings in reverse numerical order so they end up in the same
        // order as they were in the old MRU.

        if ( NULL != acstrOldList )
            {
            for ( i = nItemsToCopy - 1; i >= 0; i-- )
                {
                m_pMRU->Add ( acstrOldList[i] );
                }

            delete [] acstrOldList;
            }
//        }
//    catch ( CMemoryException )
//        {
//        TRACE0("CMRUComboBox -- Memory exception in AllocNewMRU()!\n");
//        
//        if ( NULL != m_pMRU )
//            {
//            delete m_pMRU;
//            }
//
//        throw;
//        }

                                        // Reset the changed flag.
    m_bParamsChanged = FALSE;

    return TRUE;
}

void CMRUComboBox::OnCbnDropdown()
{
    // Reset the dropped width
    int nNumEntries = GetCount();
    int nWidth = 0;
    CString str;
	CRect rect;

    CClientDC dc(this);
    int nSave = dc.SaveDC();
    dc.SelectObject(GetFont());

    for (int i = 0; i < nNumEntries; i++)
    {
        GetLBText(i, str);
        int nLength = dc.GetTextExtent(str).cx;
        nWidth = max(nWidth, nLength);
    }
	nWidth += 2*::GetSystemMetrics(SM_CXEDGE) + 4;
	
	// check if the current height is large enough for the items in the list
	GetDroppedControlRect(&rect);
	if (rect.Height() <= nNumEntries*GetItemHeight(0))
		nWidth +=::GetSystemMetrics(SM_CXVSCROLL);

    dc.RestoreDC(nSave);
    SetDroppedWidth(nWidth);
}
