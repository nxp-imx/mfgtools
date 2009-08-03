#include "StdAfx.h"
#include "resource.h"
//#include "mm_callback.h"
#include "StGrpTreeCtrl.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <afxdisp.h>	// OLE stuff
#include <shlwapi.h>	// Shell functions (PathFindExtension() in this case)
#include <afxpriv.h>	// ANSI to/from Unicode conversion macros

CStGrpTreeCtrl::CStGrpTreeCtrl(void)
{
}

CStGrpTreeCtrl::~CStGrpTreeCtrl(void)
{
}


BEGIN_MESSAGE_MAP(CStGrpTreeCtrl, CTreeCtrl)
	//{{AFX_MSG_MAP(CStGrpTreeCtrl)
	ON_WM_DROPFILES()
//	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////



BOOL CStGrpTreeCtrl::SetDropMode(CStGrpTreeCtrl::DROPTREEMODE& dropMode)
{
	m_dropMode = dropMode;

	// Register Tree control as a drop target	
	m_CTreeDropTarget.Register(this);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//	Handle the WM_DROPFILES message
//

void CStGrpTreeCtrl::OnDropFiles(HDROP dropInfo)
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
			//
			// Let them know about this list control and the item
			// droppped onto it
			//
			if(m_dropMode.pfnCallback)
				m_dropMode.pfnCallback(this, m_dropMode.pCallerClass, csPathname, iPathType);
		}
	}


	//
	// Free the dropped-file info that was allocated by windows
	//
	DragFinish(dropInfo);
}


BOOL CStGrpTreeCtrl::ValidatePathname(CString& csPathname, UINT& iPathType)
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

CString CStGrpTreeCtrl::ExpandShortcut(CString& csFilename)
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

/*
void CStGrpTreeCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	m_MMCallBack.pfnCallback(m_MMCallBack.pCallerClass, m_MMCallBack.id);

	CTreeCtrl::OnMouseMove(nFlags, point);
}


void CStGrpTreeCtrl::SetMouseMoveCallback(MOUSEMOVECALLBACK& pMMCB)
{
	m_MMCallBack.id = pMMCB.id;
	m_MMCallBack.pCallerClass = pMMCB.pCallerClass;
	m_MMCallBack.pfnCallback = pMMCB.pfnCallback;
}
*/
