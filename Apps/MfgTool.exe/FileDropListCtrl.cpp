/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
/////////////////////////////////////////////////////////////////////////////
//
//	CFileFileDropListCtrl - Enhanced CListCtrl that accepts and
//							filters dropped files/folders.
//
//	Jan 2000, Stuart Carter, stuart.carter@hotmail.com
//
//	You're free to use, modify and distribute this code
//	as long as credit is given...
//
//		Thanks to:
//		Handling of droppped files modified from:
//			CDropEdit, 1997 Chris Losinger
//			http://www.codeguru.com/editctrl/filedragedit.shtml
//
//		Shortcut expansion code modified from :
//			CShortcut, 1996 Rob Warner
//
//
//	History:
//
//	Version	Date		Author				Description
//	-------	----------	-------------------	--------------------------------
//	1.0		20-01-2000	Stuart Carter		Initial
//

#include "stdafx.h"
//#include "mm_callback.h"
#include "FileDropListCtrl.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <afxdisp.h>	// OLE stuff
#include <shlwapi.h>	// Shell functions (PathFindExtension() in this case)
#include <afxpriv.h>	// ANSI to/from Unicode conversion macros

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

CFileDropListCtrl::CFileDropListCtrl()
{
	//
	// Default drop mode
	//
	m_dropMode.iMask = DL_ACCEPT_FILES | DL_ACCEPT_FOLDERS;
	m_dropMode.pfnCallback = NULL;

	//
	// Initialize OLE libraries
	//
	m_bMustUninitOLE = FALSE;
/*
	_AFX_THREAD_STATE* pState = AfxGetThreadState();
	if (!pState->m_bNeedTerm) // TRUE if OleUninitialize needs to be called
	{
		HRESULT hr = CoInitializeEx (NULL, COINIT_MULTITHREADED); //::AfxOleInit(); //::OleInitialize(NULL);
		if (FAILED(hr))
			// Or something of your choosing...
			AfxMessageBox(_T("OLE initialization failed.\n\nMake sure that the OLE libraries are the correct version."));
		else
			m_bMustUninitOLE = TRUE;
	}
*/
}

CFileDropListCtrl::~CFileDropListCtrl()
{
	if(m_bMustUninitOLE)
	{
		::AfxOleTerm(TRUE); //::OleUninitialize();
	}
}

BEGIN_MESSAGE_MAP(CFileDropListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CFileDropListCtrl)
	ON_WM_DROPFILES()
	ON_WM_CREATE()
//	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////

int CFileDropListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	//
	// Register ourselves as a drop target for files
	//
	DragAcceptFiles(TRUE);
	
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//	PUBLIC SetDropMode()
//
//	Specify how the control will react to dropped files/folders.
//
//	Return value:
//		FALSE:	the structure was not populated correctly, 
//				the default settings will be used.
//		TRUE:	changes to the drop mode accepted
//
//
//

BOOL CFileDropListCtrl::SetDropMode(CFileDropListCtrl::DROPLISTMODE& dropMode)
{
	//
	// If they want to use a callback, ensure they also
	// specified the address of a function...
	//
	if(dropMode.iMask & DL_USE_CALLBACK && dropMode.pfnCallback == NULL)
	{
		// Must specify a function if using DL_USE_CALLBACK flag
		ASSERT(FALSE);
		return FALSE;
	}

	m_dropMode = dropMode;
	return TRUE;
}
/*
void CFileDropListCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	m_MMCallBack.pfnCallback(m_MMCallBack.pCallerClass, m_MMCallBack.id);

	CListCtrl::OnMouseMove(nFlags, point);
}


void CFileDropListCtrl::SetMouseMoveCallback(MOUSEMOVECALLBACK& pMMCB)
{
	m_MMCallBack.id = pMMCB.id;
	m_MMCallBack.pCallerClass = pMMCB.pCallerClass;
	m_MMCallBack.pfnCallback = pMMCB.pfnCallback;
}
*/

/////////////////////////////////////////////////////////////////////////////
//
//	Handle the WM_DROPFILES message
//

void CFileDropListCtrl::OnDropFiles(HDROP dropInfo)
{
	//
	// Get the number of pathnames (files or folders) dropped
	//
	UINT nNumFilesDropped = DragQueryFile(dropInfo, 0xFFFFFFFF, NULL, 0);

	//
	// Iterate through the pathnames and process each one
	//
	TCHAR szFilename[MAX_PATH + 1];
	CString csPathname;
	CString csExpandedFilename;

	for (UINT nFile = 0 ; nFile < nNumFilesDropped; nFile++)
	{
		//
		// Get the pathname
		//
		DragQueryFile(dropInfo, nFile, szFilename, MAX_PATH + 1);

		//
		// It might be shortcut, so try and expand it
		//
		csPathname = szFilename;
		csExpandedFilename = ExpandShortcut(csPathname);
		if(!csExpandedFilename.IsEmpty())
		{
			csPathname = csExpandedFilename;
		}


		//
		// Now see if its something we allow to be dropped
		//
		UINT iPathType = 0;
		if(ValidatePathname(csPathname, iPathType))
		{
			//
			// By default, we insert the filename into the list
			// ourselves, but if our parent wants to do something flashy
			// with it (maybe get the filesize and insert that as an extra
			// column), they will have installed a callback for each
			// droppped item
			//
			if(m_dropMode.iMask & DL_USE_CALLBACK)
			{
				//
				// Let them know about this list control and the item
				// droppped onto it
				//
				if(m_dropMode.pfnCallback)
					m_dropMode.pfnCallback(m_dropMode.pCallerClass, csPathname, iPathType);
			}
			else
			{
				InsertPathname(csPathname);
			}
		}
	}


	//
	// Free the dropped-file info that was allocated by windows
	//
	DragFinish(dropInfo);
}

void CFileDropListCtrl::TreeDropFiles(HDROP dropInfo)
{
	OnDropFiles(dropInfo);
}

//////////////////////////////////////////////////////////////////
//
//	ExpandShortcut()
//
//	Uses IShellLink to expand a shortcut.
//
//	Return value:
//		the expanded filename, or "" on error or if filename
//		wasn't a shortcut
//
//	Adapted from CShortcut, 1996 by Rob Warner
//	rhwarner@southeast.net
//	http://users.southeast.net/~rhwarner

CString CFileDropListCtrl::ExpandShortcut(CString& csFilename)
{
	USES_CONVERSION;		// For T2COLE() below
	CString csExpandedFile;

	//
    // Make sure we have a path
	//
	if(csFilename.IsEmpty())
	{
		ASSERT(FALSE);
		return csExpandedFile;
	}

	//
    // Get a pointer to the IShellLink interface
	//
    HRESULT hr;
    IShellLink* pIShellLink;

    hr = ::CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
							IID_IShellLink, (LPVOID*) &pIShellLink);

    if (SUCCEEDED(hr))
    {

		//
        // Get a pointer to the persist file interface
		//
		IPersistFile* pIPersistFile;
        hr = pIShellLink->QueryInterface(IID_IPersistFile, (LPVOID*) &pIPersistFile);

        if (SUCCEEDED(hr))
        {
			//
            // Load the shortcut and resolve the path
			//
            // IPersistFile::Load() expects a UNICODE string
			// so we're using the T2COLE macro for the conversion
			//
			// For more info, check out MFC Technical note TN059
			// (these macros are also supported in ATL and are
			// so much better than the ::MultiByteToWideChar() family)
			//
            hr = pIPersistFile->Load(T2COLE(csFilename), STGM_READ);
			
			if (SUCCEEDED(hr))
			{
				WIN32_FIND_DATA wfd;
				hr = pIShellLink->GetPath(csExpandedFile.GetBuffer(MAX_PATH),
										  MAX_PATH,
										  &wfd,
										  SLGP_UNCPRIORITY);

				csExpandedFile.ReleaseBuffer(-1);
            }
            pIPersistFile->Release();
        }
        pIShellLink->Release();
    }

    return csExpandedFile;
}


//////////////////////////////////////////////////////////////////
//
//	ValidatePathname()
//
//	Checks if a pathname is valid based on these options set:
//		Allow directories to be dropped
//		Allow files to be dropped
//		Only allow files with a certain extension to be dropped
//
//	Return value:
//		TRUE:	the pathname is suitable for selection, or
//		FALSE:	the pathname failed the checks.
//
//		If successful, iPathType specifies the type of path
//		validated - either a file or a folder.

BOOL CFileDropListCtrl::ValidatePathname(CString& csPathname, UINT& iPathType)
{
	//
	// Get some info about that path so we can filter out dirs
	// and files if need be
	//
	BOOL bValid = FALSE;

	struct _stat buf;
	int result = _tstat( csPathname, &buf );
	if( result == 0 )
	{
		//
		// Do we have a directory? (if we want dirs)
		//
		if ((m_dropMode.iMask & DL_ACCEPT_FOLDERS) &&
			((buf.st_mode & _S_IFDIR) == _S_IFDIR)) 
	    {
			bValid = TRUE;
			iPathType = DL_FOLDER_TYPE;
		} 
	    else if ((m_dropMode.iMask & DL_ACCEPT_FILES) &&
				((buf.st_mode & _S_IFREG) == _S_IFREG)) 
	    {
			// 
			// We've got a file and files are allowed.
			//
			iPathType = DL_FILE_TYPE;

			//
			// Now if we are filtering out specific types,
			// check the file extension.
			//
			if(m_dropMode.iMask & DL_FILTER_EXTENSION)
			{
				LPTSTR pszFileExt = PathFindExtension(csPathname);
//				if(CString(pszFileExt).CompareNoCase(m_dropMode.csFileExt) == 0)

				if(m_dropMode.csFileExt.Find(pszFileExt, 0) >= 0)
				{
					bValid = TRUE;
				}
			}
			else
			{
				bValid = TRUE;
			}
		}
	}

	return bValid;
}


//////////////////////////////////////////////////////////////////
//
//	InsertPathname()
//
//	This is used to insert a dropped item when a callback function
//	hasn't been specified.
//
//	It also checks if duplicate files are allowed to be inserted
//	and does the necessary.
//

int CFileDropListCtrl::InsertPathname(CString& csFilename)
{
	if(!(m_dropMode.iMask & DL_ALLOW_DUPLICATES))
	{
		//
		// We don't allow duplicate pathnames, so
		// see if this one is already in the list.
		//
		LVFINDINFO lvInfo;
		lvInfo.flags = LVFI_STRING;
		lvInfo.psz = csFilename;

		if(FindItem(&lvInfo, -1) != -1)
			return -1;
	}

	return InsertItem(0, csFilename);
}
