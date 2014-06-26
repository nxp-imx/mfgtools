#include "CString.h"
template <class T>
void CTString::Format(const T *, ...){
	char *ret;
	va_list ap;

	va_start(ap, fmt);
	vasprintf(&ret, fmt, ap);
	va_end(ap);

	std::string str(ret);
	this.assign(str);
	free(ret);

	return;

}

template <class T>
void CTString::AppendFormat(const T *, ...){

	char *ret;
	va_list ap;

	va_start(ap, fmt);
	vasprintf(&ret, fmt, ap);
	va_end(ap);

	std::string str(ret);
	this.append(str);
	free(ret);

	return;



}

template <class T>
const T * CTString::GetBufferSetLength(int length){
	this.resize(length);
	return this.data();
}

template <class T>
void CTString::TrimLeft(){
	while (this->at(this->begin()) == '\t' || this->at(this->begin()) == '\n' || this->at(this->begin()) == '\r'){
		this->erase(this->begin());
	}
	return;
}

template <class T>
void CTString::TrimRight(){
	while (this->at(this->end()) == '\t' || this->at(this->end()) == '\n' || this->at(this->end()) == '\r'){
		this->erase(this->end());
	}
	return;
}

template <class T>
void CTString::TrimLeft(T  chr){
	while (this->at(this->begin()) == chr ){
			this->erase(this->begin());
		}
		return;
}

template <class T>
void CTString::TrimRight(T  chr){
	while (this->at(this->end()) == chr){
			this->erase(this->end());
		}
		return;
}

template <class T>
void CTString::TrimLeft(const T * chr){
	bool present = true;
	int i;
	while (present){
		present = false;
		for (i = 0; i < strlen(chr); i++){
			if (this->at(this->begin()) == chr[i]){
				present = true;
				this->erase(this->begin());
				break;
			}
		}
	}
	return;


}

template <class T>
void CTString::TrimRight(const T *chr){
	bool present = true;
	int i;
	while (present){
		present = false;
		for (i = 0; i < strlen(chr); i++){
			if (this->at(this->end()) == chr[i]){
				present = true;
				this->erase(this->end());
				break;
			}
		}
	}
	return;

}


//template<class T>
//int CTString::Find(T ch) const{
//
//
//
//
//
//
//}
//
//template<class T>
//int Find(const T * lpszSub) const{
//
//
//
//
//}
//
//template<class T>
//int Find(T ch, int nStart) const{
//
//
//
//
//
//}
//
//template<class T>
//int Find(const T * pstr, int nStart) const{
//
//
//
//
//}

template<class T>
int CTString::Replace(T chOld, T chNew){
	int count = 0;
	for (int i = 0; i < this.length(); i++){
		if (this[i] == chOld){
			this[i] = chNew;
			count++;
		}
	}

	return count;
}

template<class T>
int CTString::Replace(const T * lpszOld, const T * lpszNew){
	bool found = true;
	int count = 0;
	int pos;
	while (found){
		pos = this.find(lpszOld);
		if (pos != -1){
			this.erase(pos, pos + strlen(lpszOld));
			this.insert(pos + 1, lpszNew);
			count++;
		}
		else{
			found = false;
		}


	}
	return count;


}
template <class T>
void CTString::MakeUpper(){
	for (int i = 0; i < length(); i++){
		this->[i] = _totupper(this->[i]);
	}
}