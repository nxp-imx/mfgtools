//#define CString std::string
#include <string>
#include <cstring>  
#include <iostream>
#include <cstdlib>
#include "CString.h"


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
typedef const char * LPCTSTR;
typedef char * LPTSTR;

typedef char * LPOLESTR;

#define _TCHAR_DEFINED
#define __AFXWIN_H__
#define IsEmpty empty
//std::string str_format(const std::string fmt_str, ...);


