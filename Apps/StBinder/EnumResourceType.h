#pragma once

typedef HRESULT (CALLBACK FAR * LPFN_ENUM_NAMERESTYPE_CALLBACK)(PVOID pCallerClass, HMODULE _hModule, CString& _resName, int _iResId, WORD& _langId);
typedef HRESULT (CALLBACK FAR * LPFN_ENUM_IDRESTYPE_CALLBACK)(PVOID pCallerClass, HMODULE _hModule, int _iResId, WORD& _langId);
//
// Declare your - optional - callback function like this:
//		HRESULT CALLBACK MyFunc(PVOID pCallerClass,
//								const CString& csResName
//								const WORD& wLangId)
//
/////////////////////////////////////////////////////////////////////////////
class CEnumResourceType
{
public:
	CEnumResourceType(PVOID _pCallerClass, LPFN_ENUM_NAMERESTYPE_CALLBACK _pfnCallback, HMODULE _hModule, LPTSTR _szResType);
	CEnumResourceType(PVOID _pCallerClass, LPFN_ENUM_IDRESTYPE_CALLBACK _pfnCallback, HMODULE _hModule, LPTSTR _szResType);
public:
	~CEnumResourceType(void);

	void Begin();
	static BOOL EnumResNameProc(HMODULE _hModule,
								LPCTSTR _lpszType,
							    LPTSTR _lpszName,
								LONG_PTR _lParam);
	static BOOL EnumResLanguageProc(HANDLE _hModule,
							    LPCTSTR _lpszType,
							    LPCTSTR _lpszName,
							    WORD _wIDLanguage,
								LONG_PTR _lParam);
	static void RollYourOwnEnumLanguages(HMODULE _hModule,
								LPCTSTR _lpszType,
							    int	_iResId,
								LONG_PTR _lParam);
	PVOID	m_pCallerClass;
	LPFN_ENUM_NAMERESTYPE_CALLBACK m_pfnNameIdCallback;
	LPFN_ENUM_IDRESTYPE_CALLBACK m_pfnResIdCallback;
	LPTSTR	m_szResType;
	HMODULE	m_hModule;

	PSTFWRESINFO	m_pFwResInfo;
	int				m_iFwResInfoCount;
};
