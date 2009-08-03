// AutoPlayReject.h : header file
//

#pragma once

#include <shobjidl.h>

// CAutoPlayReject
class CAutoPlayReject
{
// Construction
public:
	CAutoPlayReject();	// standard constructor
	~CAutoPlayReject();
	
	HRESULT Register(LPCTSTR AutoPlayDrvList = NULL);
	HRESULT Unregister(void);
	
	static CString GetDrvList(void){return m_DrvList;};
	
// Implementation
protected:
    DWORD m_ROT;
	bool m_Registered;

	static CString m_DrvList;

	static const CLSID m_CancelAutoplayClsId;

    void DoVista();
   	DWORD m_VistaDefaultDisableAutoplay;

	class CQueryCancelAutoplay : public IQueryCancelAutoPlay
	{
	public:
		// IUnknown interface
		STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();
		// IQueryCancelAutoPlay interface
		STDMETHODIMP AllowAutoPlay(LPCWSTR pszPath, DWORD dwContentType,
			LPCWSTR pszLabel, DWORD dwSerialNumber);
		CQueryCancelAutoplay() : _cRef(1) { }
		~CQueryCancelAutoplay() { }
	private:
		ULONG _cRef;
	};
};