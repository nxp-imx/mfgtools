// StBase.h: interface for the StBase class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STBASE_H__C6DD00FD_38FC_438D_BD15_685AE4AEDAD6__INCLUDED_)
#define AFX_STBASE_H__C6DD00FD_38FC_438D_BD15_685AE4AEDAD6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _DEBUG
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

class CStBase  
{
public:

	CStBase(string name="CStBase");
	CStBase(const CStBase&);
	virtual ~CStBase();
	CStBase& operator=(const CStBase&);

	virtual ST_ERROR GetLastError() const;
	virtual long GetSystemLastError() const;
	string GetObjName() const;

protected:

	ST_ERROR	m_last_error;
	long		m_system_last_error;
	string		m_obj_name;
};

class CStBaseToResource :
	public CStBase
{
public:
	CStBaseToResource(void);
	virtual ~CStBaseToResource(void);

	virtual ST_ERROR GetErrorMessage(USHORT _sense_code, wstring& _msg, BOOL _for_logging_only=FALSE)=0;
	virtual ST_ERROR GetResourceString( int _res_id, wstring& _res_string )=0;
	virtual LPVOID GetDefaultCfgResource(int _res_id, DWORD _size)=0;

};

class CStBaseToCmdLineProcessor :
	public CStBase
{
public:
	CStBaseToCmdLineProcessor(void);
	virtual ~CStBaseToCmdLineProcessor(void);

	virtual BOOL Fat16(){ return FALSE; }

#ifdef _DEBUG
	virtual BOOL GenXRFiles()=0;
#endif
};

class CStBaseToLogger :
	public CStBase
{
public:
	CStBaseToLogger(void);
	virtual ~CStBaseToLogger(void);

	virtual ST_ERROR Log(CString _text) { return STERR_NONE; };
};
#endif // !defined(AFX_STBASE_H__C6DD00FD_38FC_438D_BD15_685AE4AEDAD6__INCLUDED_)
