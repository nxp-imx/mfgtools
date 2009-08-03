// StArray.h: interface for the StArray class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STARRAY_H__8DB0925B_E1B3_49F7_B93F_D610D00A4A8F__INCLUDED_)
#define AFX_STARRAY_H__8DB0925B_E1B3_49F7_B93F_D610D00A4A8F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#include "StHeader.h"

template<class T> 
class CStArray : public CStBase {

public:

	CStArray(size_t count=4, string name="CStArray");
	CStArray(const CStArray& ar);
	virtual ~CStArray();
	CStArray& operator=(const CStArray&);

	virtual size_t GetCount() const;
	virtual T* GetAt(size_t _pos) const;
	virtual ST_ERROR GetAt(size_t _pos, T& rT) const;
	virtual ST_ERROR SetAt(size_t _pos, T t);
	virtual ST_ERROR InitializeElementsToZero();
  	virtual ST_ERROR InitializeElementsTo(T t);
	virtual ST_ERROR Reverse();

	virtual ST_ERROR Init(size_t _count);
	virtual void Trash();

	T *m_p_t;
	size_t m_count;

};

template<class T> 
CStArray<T>::CStArray(size_t _count, string _name):CStBase(_name)
{
	Init(_count);
}
 
template<class T> 
CStArray<T>::CStArray(const CStArray& _ar):CStBase( _ar )
{
	m_p_t = NULL;
	*this = _ar;
}

template<class T>
CStArray<T>::~CStArray()
{
	Trash();
}

template<class T>
CStArray<T>& CStArray<T>::operator=(const CStArray& _ar)
{
	if( this == &_ar )
	{
		return *this;
	}

	Trash();
	Init(_ar.GetCount());

	for(size_t index=0; index<_ar.GetCount(); index++)
	{
		T t;
		_ar.GetAt(index, t);
		SetAt(index, t);
	}
	
	return *this;
}

template<class T>
ST_ERROR CStArray<T>::Init(size_t _count)
{
	m_count = _count;
	m_p_t = new T [m_count];

	if ( m_p_t == NULL )
	{
		m_last_error = STERR_NO_MEMORY;
		return m_last_error;
	}
	InitializeElementsToZero();
	m_last_error = STERR_NONE;
	return m_last_error;
}

template<class T>
void CStArray<T>::Trash()
{
	m_count = 0;
	delete[] m_p_t;
	m_p_t = NULL;
}

template<class T>
size_t CStArray<T>::GetCount() const
{
	return m_count;
}

template<class T>
T* CStArray<T>::GetAt(size_t _pos) const
{
	if((_pos >= 0) && (_pos < m_count))
	{
		return &m_p_t[_pos];
	}
	return NULL;
}

template<class T>
ST_ERROR CStArray<T>::GetAt(size_t _pos, T& _rT) const
{
	if((_pos >= 0) && (_pos < m_count))
	{
		_rT = m_p_t[_pos];
		return STERR_NONE;
	}
	return STERR_INVALID_POS_IN_ARRAY;
}

template<class T>
ST_ERROR CStArray<T>::InitializeElementsToZero()
{
	for(size_t index = 0; index < GetCount(); index ++)
	{
		m_p_t[index] = (T)0;
	}
	return STERR_NONE;
}

template<class T>
ST_ERROR CStArray<T>::InitializeElementsTo(T t)
{
	for(size_t index = 0; index < GetCount(); index ++)
	{
		m_p_t[index] = t;
	}
	return STERR_NONE;
}

template<class T>
ST_ERROR CStArray<T>::SetAt(size_t _pos, T _t)
{
	if((_pos >= 0) && (_pos < m_count))
	{
		m_p_t[_pos] = _t;
		return STERR_NONE;
	}
	return STERR_INVALID_POS_IN_ARRAY;
}

template<class T>
ST_ERROR CStArray<T>::Reverse()
{
	CStArray<T> arr_temp(GetCount());
	T t;
	size_t index_temp;
	int index;

	//first write to arr_temp in reverse order
	for(index = (int)GetCount()-1, index_temp=0; index >= 0; index --, index_temp++)
	{
		GetAt((size_t)index, t);
		arr_temp.SetAt(index_temp, t);
	}

	//copy arr_temp to this.
	for(index = 0; index < (int)GetCount(); index ++)
	{
		arr_temp.GetAt(index, t);
		SetAt(index, t);
	}
	return STERR_NONE;
}
#endif // !defined(AFX_STARRAY_H__8DB0925B_E1B3_49F7_B93F_D610D00A4A8F__INCLUDED_)
