#pragma once

#define NO_INDEX        -1


class CFileList
{
public:
	CFileList(void);
	CFileList(CString _csRootPath);
	~CFileList(void);
	
	typedef enum FileListAction {EXISTS_IN_TARGET=0, COPY_TO_TARGET, DELETE_FROM_TARGET, CREATE_DIR, IN_EDIT_ADD, IN_EDIT_DELETE, IN_EDIT_CREATE_DIR, RESOURCE_FILE, IGNORE_FILEITEM};

typedef struct _fileListItem
	{
		int			m_action;
		int			m_currentAction;
		CString		m_csFilePathName;
		CString		m_csFileName;
		PVOID		m_pNext;
		DWORD		m_dwAttr;
		FILETIME	m_timestamp;
		DWORD		m_dwFileSize;
		CFileList	*m_pSubFolderList;
		int			m_resId;
	} FILEITEM, *PFILEITEM;

protected:

	PFILEITEM	m_ListHead;
	PFILEITEM	m_ListTail;
	int			m_ListCount;

	void		EmptyTheList(void);
	void		EmptySubFolder(CFileList *_pFileList);
	void		EnumCommitEdits(CFileList *_pFileList);
	void		EnumRemoveEdits(CFileList *_pFileList);
	int			EnumGetTotalCount(CFileList *_pFileList);

public:
	CString		m_csRootPath;
	int			AddFile(CString _pathName, int _action);
	int			AddFile(CString _pathName, int _action, int _resId);
	void		RemoveFile(CString _pathName);
	void		RemoveFile(int _index);
	void		RemoveAll(void);
	PFILEITEM	GetAt(int _index);
	PFILEITEM	FindFile(CString _pathName, int& _index);
	int			GetCount(void) { return m_ListCount; };
	int			GetTotalCount(void);
	CString		GetPathNameAt(int _index);
	CString		GetFileNameAt(int _index);
	void		CommitEdits(void);
	void		RemoveEdits(void);
	BOOL		ChangeFile(int _index, CString _pathName);

};
