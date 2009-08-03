#include "fileversioninfo.h"

class CStVersion
{
public:
	CStVersion(LPTSTR _fname);
    virtual ~CStVersion();

	void StGetProductVersion(CString& _ver);
	void StGetFileVersion(CString& _ver);
	void StGetFileDate(CString& _date);
	void StGetFileSize(CString& _filesize);
	void StGetOriginalFileName(CString& _originalfilename);
	BOOL ApplyVersionChanges(HANDLE _hTargetUpdate, LPVOID _data) { return m_FileVersionInfo.ApplyVersionChanges(_hTargetUpdate, _data); };


private:
//	BOOL					IsInfFile(LPTSTR _fname);
//	CString					GetInfVersion(LPTSTR _fname);

	CFileVersionInfo		m_FileVersionInfo;
	CString					m_csFilename;
	DWORD					m_dwFilesize;

	CString					m_csFilesize;
	CString					m_csDate;
};