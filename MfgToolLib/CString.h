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
	
	
		operator const T * () { return c_str(); }\
		operator const T * () const{ return c_str(); }
		operator T * () { return c_str(); }
		operator T * () const { return c_str(); }
		void operator=(T * buff){	this->assign(buff);return;}
		
		
		void Format(const T *, ...);
		void AppendFormat(const T *, ...);
		T * GetBufferSetLength(int length);
		void TrimLeft();
		void TrimRight();
		void TrimLeft(T chr);
		void TrimRight(T chr);
		void TrimLeft(const T * chr );
		void TrimRight(const T * chr );
		void Empty(){ clear(); return; }
		const bool IsEmpty() { return empty(); }
		int GetLength() { return this->length(); }
		const T * GetBuffer() { return c_str(); }
		void ReleaseBuffer(int nlength = -1);
		int CompareNoCase(const T* str) { return _tcsnicmp(c_str(), str, length()); }
		int Find(T ch) const {return find(ch);};
		int Find(const T * lpszSub) const{ return find(lpszSub); }
		int Find(T ch, int nStart) const{ return find(ch, nStart); }
		int Find(const T * pstr, int nStart) const { return find(pstr, nStart); }
		int Replace(T chOld, T chNew);
		int Replace(const T * lpszOld, const T * lpszNew);
		int Compare(LPCTSTR lpsz) const;///
		int CompareNoCase(LPCTSTR lpsz) const;///
		void MakeUpper();
		CTString<T> Mid(int nFirst) const;
		CTString<T> Mid(int nFirst, int nCount) const;
		CTString<T> Left(int nCount) const;
		CTString<T> Right(int nCount) const;
};



#ifdef WINVER
typedef CTString<TCHAR> CString;
typedef CTString<char> CAnsiString;
#else
typedef CTString<char> CString;
#endif
