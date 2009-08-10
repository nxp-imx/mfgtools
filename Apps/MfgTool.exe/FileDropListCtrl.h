#if !defined(AFX_FileDropListCtrl_H__55F59001_AD5F_11D3_9DA4_0008C711C6B6__INCLUDED_)
#define AFX_FileDropListCtrl_H__55F59001_AD5F_11D3_9DA4_0008C711C6B6__INCLUDED_

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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


typedef HRESULT (CALLBACK FAR * LPFN_DROP_FILES_CALLBACK)(PVOID pCallerClass, CString&, UINT&);
//
// Declare your - optional - callback function like this:
//		HRESULT CALLBACK MyFunc(CListCtrl* pList,
//								const CString& csPathname,
//								const UINT& iPathType)
//
//		- pList is a pointer to the List control calling you
//		- csPathname is the fully qualified path of the file/folder dropped
//		- iPathType is the type of path dropped (file or folder)
//					CFileDropListCtrl::DL_FOLDER_TYPE
//					CFileDropListCtrl::DL_FILE_TYPE

/////////////////////////////////////////////////////////////////////////////

class CFileDropListCtrl : public CListCtrl
{
public:
	CFileDropListCtrl();
	virtual ~CFileDropListCtrl();

	enum DLDropFlags
	{
		DL_ACCEPT_FILES =       0x01,	// Allow files to be dropped
		DL_ACCEPT_FOLDERS =		0x02,	// Allow folders to be droppped
		DL_ALLOW_DUPLICATES =	0x04,	// Allow a pathname to be dropped even if its already in the list (ignored if you specify a callback function)
		DL_FILTER_EXTENSION =	0x10,	// Only accept files with the specified extension. Specify in csFileExt
		DL_USE_CALLBACK =		0x20,	// Receive a callback for each item dropped, specified in pfnCallback (you have responsibility for inserting items into the list)

		DL_FOLDER_TYPE =		0x40,	// Returned to the callback function - indicating the type of path dropped
		DL_FILE_TYPE =			0x80
	};

	struct DROPLISTMODE { 
		UINT iMask;								// Specifies what type of items to accept - a combination of the above flags
		CString csFileExt;						// The file extension on which to filter. Use the format ".extension". Ignored unless DL_FILTER_EXTENSION is specified
		PVOID pCallerClass;						// Caller provided class ptr
		LPFN_DROP_FILES_CALLBACK pfnCallback;	// Address of your callback function. Ignored unless DL_USE_CALLBACK is specified
	}; 



	BOOL SetDropMode(CFileDropListCtrl::DROPLISTMODE& dropMode);
	DROPLISTMODE GetDropMode() { return m_dropMode; };
	void TreeDropFiles(HDROP dropInfo);

//	void SetMouseMoveCallback(MOUSEMOVECALLBACK& mmcb);
//	MOUSEMOVECALLBACK	m_MMCallBack;

protected:
	//{{AFX_MSG(CFileDropListCtrl)
	afx_msg void OnDropFiles(HDROP dropInfo);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
//	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	virtual int InsertPathname(CString& csFilename);

private:
	DROPLISTMODE m_dropMode;
	BOOL m_bMustUninitOLE;

	CString ExpandShortcut(CString& csFilename);
	BOOL ValidatePathname(CString& csPathname, UINT& iPathType);

	// Prevent default compiler generation of these copy constructors (no bitwise copy)
	CFileDropListCtrl& operator=(CFileDropListCtrl& x);
	CFileDropListCtrl(CFileDropListCtrl& x);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FileDropListCtrl_H__55F59001_AD5F_11D3_9DA4_0008C711C6B6__INCLUDED_)
