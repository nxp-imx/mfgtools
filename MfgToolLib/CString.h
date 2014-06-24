#include "stdafx.h"
#include <string>


#ifdef WINVER
#define tstring std::basic_string<T, std::char_traits<T>, std::allocator<T> >

template<class T>
class CTString: public std::basic_string<T, std::char_traits<T>, std::allocator<T> >
#else
#define tstring std::string 
#define TCHAR char
class CTString :public std::string
#endif

{

public: CTString() : tstring() { }
		CTString(const tstring& s) : tstring(s) { }
		CTString(const tstring& s, std::size_t n) : tstring(s, n) { }
		CTString(const T * s, std::size_t n) : tstring(s, n) { }
		CTString(const T * s) : tstring(s) { }
		CTString(std::size_t n, T c) : tstring(n, c) { }
	
	
		operator const T * () { return c_str(); }
		operator T * () { return c_str(); }
		operator T * () const { return c_str(); }

		
		void Format(const T *, ...);//////
		void AppendFormat(const T *, ...);//////
		T * GetBufferSetLength(int length);
		void TrimLeft();/////
		void TrimRight();/////
		void TrimLeft(const T *);//////
		void TrimRight(const T *);//////
		const bool IsEmpty() { return empty(); }
		int GetLength() { return this->length(); }
		T * GetBuffer() { return c_str(); }
		int CompareNoCase(const T* str) { return _tcsnicmp(c_str(), str, lenght()); }
		int Find(T ch) const;////
		int Find(const T * lpszSub) const;////
		int Find(T ch, int nStart) const;////
		int Find(const T * pstr, int nStart) const;////
		int Replace(T chOld, T chNew);////
		int Replace(const T * lpszOld, const T * lpszNew);////

};



#ifdef WINVER
typedef CTString<TCHAR> CString;
typedef CTString<char> CAnsiString;
#else
typedef CTString<char> CString;
#endif
