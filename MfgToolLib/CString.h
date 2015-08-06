#include "stdafx.h"
#include <string>
#include <stdlib.h>
#include <cstdarg>
#pragma once


#ifndef __linux__
#define tstring std::basic_string<T, std::char_traits<T>, std::allocator<T> >

template<class T>
class CTString: public std::basic_string<T, std::char_traits<T>, std::allocator<T> >
#else
#define tstring std::string
#define TCHAR char
template<class T>
//#define T char
class CTString :public std::string
#endif

{
#ifdef __linux__
#define _vsntprintf vsnprintf
#define _tcsnicmp strncasecmp
#define _tcslen strlen
#define _T(x) x
#endif
public: CTString() : tstring() { }
		
		CTString(const tstring& s) : tstring(s) { }
		CTString(const tstring& s, std::size_t n) : tstring(s, n) { }
		CTString(const T * s, std::size_t n) : tstring(s, n) { }
		CTString(const T * s) : tstring(s?s:(const T *)_T(" ")) { }
		CTString(std::size_t n, T c) : tstring(n, c) { }


		operator const T * () { return this->c_str(); }\
		operator const T * () const{ return this->c_str(); }
		operator T * () { return this->c_str(); }
		operator T * () const { return this->c_str(); }
		void operator=(T * buff){	this->assign(buff);return;}


		void Empty(){ this->clear(); return; }
		const bool IsEmpty() { return this->empty(); }
		int GetLength() { return this->length(); }
		const T * GetBuffer() { return this->c_str(); }
		int Compare(const T* str) const{ return this->compare(str); }
		int CompareNoCase(const T* str) { return _tcsnicmp(this->c_str(), str, this->length()); }
		int Find(T ch) const {return this->find(ch);};
		int Find(const T * lpszSub) const{ return this->find(lpszSub); }
		int Find(T ch, int nStart) const{ return this->find(ch, nStart); }
		int Find(const T * pstr, int nStart) const { return this->find(pstr, nStart); }
		T GetAt(int nIndex) const{ return this->at(nIndex);}

		void Format(const T *fmt, ...){
			size_t buffLen = 512;
			T *ret = new T[buffLen];
			va_list ap;
			int i = 0;
			bool cond = true;
			va_start(ap, fmt);
			int actual = _vsntprintf(ret, buffLen, fmt, ap);
			do{
				if (actual == -1){
					delete ret;
					ret = new T[buffLen*(i + 2)];
					actual = _vsntprintf(ret, actual, fmt, ap);
				}
				else{
					cond = false;
				}
			} while (i < 10 && cond);
				va_end(ap);

			CTString<T> str(ret);
			this->assign(str);
			free(ret);

			return;

		}


		void AppendFormat(const T *fmt, ...){

			size_t buffLen = 512;
			T *ret = new T[buffLen];
			va_list ap;
			int i = 0;
			bool cond = true;
			va_start(ap, fmt);
			int actual = _vsntprintf(ret, buffLen, fmt, ap);
			do{
				if (actual == -1){
					delete ret;
					ret = new T[buffLen*(i+2)];
					actual=_vsntprintf(ret, actual, fmt, ap);
				}
				else{
					cond = false;
				}
			} while (i < 10 && cond);
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
			if (this->empty())
				return;
			while (*this->begin()  == _T('\t') || *this->begin() == _T('\n') || *this->begin() == _T('\r') || *this->begin() == _T(' ')){
				this->erase(this->begin());
				if (this->empty())
					return;
			}
			return;
		}


		void TrimRight(){
			if (this->empty())
				return;
			while (this->back() == _T('\t') || this->back() == _T('\n') || this->back() == _T('\r') || this->back() == _T(' ')){
				this->erase(this->length()-1);
				if (this->empty())
					return;
			}
			return;
		}


		void TrimLeft(T  chr){
			while (this->at(this->begin()) == chr){
				this->erase(this->begin());
				if (this->empty())
					return;
			}
			return;
		}


		void TrimRight(T  chr){
			while (this->back() == chr){
				this->erase(this->length()-1);
				if (this->empty())
					return;
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
			for (int i = 0; i < this->length(); i++){
				if (this->at(i) == chOld){
					this->at(i) = chNew;
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
					this->erase(pos, _tcslen(lpszOld));
					this->insert(pos, lpszNew);
					count++;
				}
				else{
					found = false;
				}


			}
			return count;


		}


		void MakeUpper(){
			for (unsigned int i = 0; i < this->length(); i++){
				this->at(i) = toupper(this->at(i));
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
			if (nCount > this->length())
				return *this;
			return this->substr(this->length()-nCount,  nCount);
		}


		int ReverseFind(T ch) const{
			return (int)this->rfind(ch);
		}

};



#ifndef __linux__
typedef CTString<TCHAR> CString;
typedef CTString<char> CAnsiString;
#else
typedef CTString<char> CString;
#endif
