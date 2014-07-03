//#define CString std::string
#include <windows.h>
#include <tchar.h>
#include <string>
#include <string.h>
#include <cstring>  
#include <iostream>
#include <cstdlib>

#include <AtlConv.h>

#include <stdlib.h>
#include <stdio.h>
#include <queue>
#include <cerrno>
#include <math.h>
#include <wchar.h>
#include "sched.h"
#include "semaphore.h"
#include "pthread.h"

#include "CString.h"
#ifdef DEBUG
#define ASSERT assert
#else
#define ASSERT 
#endif


#define TRACE 1? 0: _tprintf

#if 0 

#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif
#define _tcschr strchr
#define _totlower tolower
#define	_tcslen strlen
#define _istspace isspace
#define _ttol atol
#define _tcspbrk strpbrk
#define _tcsnicmp _strnicmp
//#define .Format =sprintf
#define TCHAR char 
#define _T


#define _TCHAR_DEFINED
#define __AFXWIN_H__
#define IsEmpty empty

#endif

typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;

struct thread_msg{
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
	thread_msg() : message(), wParam(), lParam() {}
	thread_msg(UINT msg, WPARAM wPar, LPARAM lPar):message(msg),wParam(wPar),lParam(lPar){}
	

};




//std::string str_format(const std::string fmt_str, ...);


