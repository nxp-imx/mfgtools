// StByteArray.h: interface for the CStByteArray class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STBYTEARRAY_H__972C0C8B_9C01_409E_91BA_1C311970F8DC__INCLUDED_)
#define AFX_STBYTEARRAY_H__972C0C8B_9C01_409E_91BA_1C311970F8DC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CStByteArray : public CStArray<UCHAR>  
{
public:
	CStByteArray(size_t size, string name="CStByteArray");
	~CStByteArray();
	
	static ST_ERROR Copy(CStByteArray* _p_srcArray, size_t _srcOffset, CStByteArray* _p_dstArray, size_t _dstOffset, ULONG _length);
	ST_ERROR CStByteArray::CopyTo(CStByteArray* _dstArray, size_t _dstOffset);

	ST_ERROR Read(USHORT&, size_t from_offset);
	ST_ERROR Read(ULONG&, size_t from_offset);
	ST_ERROR Read(ULONG&, size_t from_offset, BOOL noswap);
	ST_ERROR Read(ULONGLONG&, size_t from_offset);
	ST_ERROR Read(ST_BOOLEAN&, size_t from_offset);
	ST_ERROR Read(void*, size_t size, size_t from_offset);

	ST_ERROR Write(USHORT, size_t from_offset);
	ST_ERROR Write(ULONG, size_t from_offset);
	ST_ERROR Write(ULONGLONG, size_t from_offset);
	ST_ERROR Write(ST_BOOLEAN, size_t from_offset);
	ST_ERROR Write( void*, size_t size, size_t from_offset);

	BOOL operator!=(CStByteArray& _arr);
	BOOL operator==(CStByteArray& _arr);
	wstring GetAsString();
};

class CStArrayOfByteArrays : public CStArray<class CStByteArray*>
{
public:
	CStArrayOfByteArrays(size_t size, size_t _size_of_each_bytearray, string name="CStArrayOfByteArrays");
	~CStArrayOfByteArrays();
};

#endif // !defined(AFX_STBYTEARRAY_H__972C0C8B_9C01_409E_91BA_1C311970F8DC__INCLUDED_)
