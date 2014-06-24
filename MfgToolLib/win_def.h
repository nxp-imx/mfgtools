//#define CString std::string
#include <windows.h>
#include <tchar.h>
#include <string>
#include <cstring>  
#include <iostream>
#include <cstdlib>

#include <AtlConv.h>

#include <stdlib.h>
#include <stdio.h>
#include <cerrno>
#include <math.h>
#include <wchar.h>

#include "CString.h"

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
//std::string str_format(const std::string fmt_str, ...);


