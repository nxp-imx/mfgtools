#include "stdafx.h"
#include <string>

#pragma once
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
		
		
		
		void Empty(){ clear(); return; }
		const bool IsEmpty() { return empty(); }
		int GetLength() { return this->length(); }
		const T * GetBuffer() { return c_str(); }
		int Compare(const T* str) const{ return compare(str); }
		int CompareNoCase(const T* str) { return _tcsnicmp(c_str(), str, length()); }
		int Find(T ch) const {return find(ch);};
		int Find(const T * lpszSub) const{ return find(lpszSub); }
		int Find(T ch, int nStart) const{ return find(ch, nStart); }
		int Find(const T * pstr, int nStart) const { return find(pstr, nStart); }
		T GetAt(int nIndex) const{ return at(nIndex);}

		void Format(const T *fmt, ...){
			T *ret;
			va_list ap;

			va_start(ap, fmt);
			//vasprintf(&ret, fmt, ap);
			va_end(ap);

			CTString<T> str(ret);
			this->assign(str);
			free(ret);

			return;

		}


		void AppendFormat(const T *fmt, ...){

			T *ret;
			va_list ap;

			va_start(ap, fmt);
			//vasprintf(&ret, fmt, ap);
			va_end(ap);

			CTString<T>  str(ret);
			this->append(str);
			free(ret);

			return;



		}

		
		 T * GetBufferSetLength(int length){
			this->resize(length);
			return (T*) this->data();
		}

		
		void TrimLeft(){
			if (empty())
				return;
			while (*this->begin()  == _T('\t') || 
				*this->begin() == _T('\n') || *this->begin() == _T('\r')){
				this->erase(this->begin());
			}
			return;
		}

		
		void TrimRight(){
			if (empty())
				return;
			while (*this->end() == _T('\t') || *this->end() == _T('\n') || *this->end() == _T('\r')){
				this->erase(this->end());
			}
			return;
		}

		
		void TrimLeft(T  chr){
			while (this->at(this->begin()) == chr){
				this->erase(this->begin());
			}
			return;
		}

		
		void TrimRight(T  chr){
			while (this->at(this->end()) == chr){
				this->erase(this->end());
			}
			return;
		}

		
		void TrimLeft(const T * chr){
			bool present = true;
			int i;
			while (present){
				present = false;
				for (i = 0; i < _tcslen(chr); i++){
					if (this->at(this->begin()) == chr[i]){
						present = true;
						this->erase(this->begin());
						break;
					}
				}
			}
			return;


		}

		
		void TrimRight(const T *chr){
			bool present = true;
			int i;
			while (present){
				present = false;
				for (i = 0; i < _tcslen(chr); i++){
					if (this->at(this->end()) == chr[i]){
						present = true;
						this->erase(this->end());
						break;
					}
				}
			}
			return;

		}

		int Replace(T chOld, T chNew){
			int count = 0;
			for (int i = 0; i < length(); i++){
				if (at(i) == chOld){
					at(i) = chNew;
					count++;
				}
			}

			return count;
		}

		
		int Replace(const T * lpszOld, const T * lpszNew){
			bool found = true;
			int count = 0;
			int pos;
			while (found){
				pos = this->find(lpszOld);
				if (pos != -1){
					this->erase(pos, pos + _tcslen(lpszOld));
					this->insert(pos + 1, lpszNew);
					count++;
				}
				else{
					found = false;
				}


			}
			return count;


		}

		
		void MakeUpper(){
			for (int i = 0; i < length(); i++){
				at(i) = _totupper(at(i));
			}
		}

		
		void ReleaseBuffer(int nlength = -1){
			if (nlength == -1){
				this->assign(this->c_str()); // essentially resizing the string as needed
				return;
			}
			else{
				this->resize(nlength);
				return;
			}
		}

		
		CTString<T> Mid(int nFirst) const{
			return	this->substr(nFirst, this->length() - nFirst);
		}

		
		CTString<T> Mid(int nFirst, int nCount) const{
			return this->substr(nFirst, nCount);
		}

		
		CTString<T> Left(int nCount) const{
			return this->substr(0, nCount);
		}

		
		CTString<T> Right(int nCount) const{
			if (nCount > length())
				return *this;
			return this->substr(nCount, this->length() - nCount);
		}

		
		int ReverseFind(T ch) const{
			return (int)this->rfind(ch);
		}

};



#ifdef WINVER
typedef CTString<TCHAR> CString;
typedef CTString<char> CAnsiString;
#else
typedef CTString<char> CString;
#endif
