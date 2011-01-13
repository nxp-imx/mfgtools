/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "stdafx.h"
#include "Device.h"
#include "DeviceClass.h"
#include "ParameterT.h"
#include "Libs\Loki\Singleton.h"
#include "Libs\Loki\Functor.h"

#include <Dbt.h>
#include <mswmdm.h>
#include <SCClient.h>

DEFINE_GUID(CLSID_CANCEL_AUTOPLAY, 0x66a32fe6, 0x229d, 0x427b, 0xa6, 0x8, 0xd2, 0x73, 0xf4, 0xc, 0x3, 0x4c);

//////////////////////////////////////////////////////////////////////
//
// DeviceManager
//
//////////////////////////////////////////////////////////////////////
class DeviceManager : public CWinThread
{
	//! \brief Needed by SingletonHolder to create class.
    friend struct Loki::CreateUsingNew<DeviceManager>;
	DECLARE_DYNCREATE(DeviceManager)

public:
	//! \brief Callback function typedef. i.e. void OnDeviceChangeNotify(const DeviceClass::NotifyStruct& nsInfo)
	typedef Loki::Functor<BOOL, LOKI_TYPELIST_1(const DeviceClass::NotifyStruct&), Loki::ClassLevelLockable> DeviceChangeCallback;
	static const BOOL retUnregisterCallback = TRUE;

    //! \brief Device event types determined in DeviceManager::OnMsgDeviceEvent(WPARAM eventType, LPARAM msg).
    //! \see DeviceClass::NotifyStruct::Event
    //! \see DeviceManager::Observer::Event
	typedef enum DevChangeEvent 
						{UNKNOWN_EVT = 100, EVENT_KILL,
                         HUB_ARRIVAL_EVT, HUB_REMOVAL_EVT,
						 DEVICE_ARRIVAL_EVT, DEVICE_REMOVAL_EVT, 
						 VOLUME_ARRIVAL_EVT, VOLUME_REMOVAL_EVT,
                         /*WMDM_DEVICE_ARRIVAL_EVT, WMDM_DEVICE_REMOVAL_EVT,
                         WMDM_MEDIA_ARRIVAL_EVT, WMDM_MEDIA_REMOVAL_EVT*/
						};
	
	static CString EventToString(int evnt)
	{
		CString str;

		switch(evnt)
		{
			case EVENT_KILL:
				str = _T("EVENT_KILL");
				break;
			case HUB_ARRIVAL_EVT:
				str = _T("HUB_ARRIVAL_EVT");
				break;
			case HUB_REMOVAL_EVT:
				str = _T("HUB_REMOVAL_EVT");
				break;
			case DEVICE_ARRIVAL_EVT:
				str = _T("DEVICE_ARRIVAL_EVT");
				break;
			case DEVICE_REMOVAL_EVT:
				str = _T("DEVICE_REMOVAL_EVT");
				break;
			case VOLUME_ARRIVAL_EVT:
				str = _T("VOLUME_ARRIVAL_EVT");
				break;
			case VOLUME_REMOVAL_EVT:
				str = _T("VOLUME_REMOVAL_EVT");
				break;
			/*case WMDM_DEVICE_ARRIVAL_EVT:
				str = _T("WMDM_DEVICE_ARRIVAL_EVT");
				break;
			case WMDM_DEVICE_REMOVAL_EVT:
				str = _T("WMDM_DEVICE_REMOVAL_EVT");
				break;
			case WMDM_MEDIA_ARRIVAL_EVT:
				str = _T("WMDM_MEDIA_ARRIVAL_EVT");
				break;
			case WMDM_MEDIA_REMOVAL_EVT:
				str = _T("WMDM_MEDIA_REMOVAL_EVT");
				break;*/
			case UNKNOWN_EVT:
			default:
				str = _T("UNKNOWN_EVT");
		}
		
		return str;
	}
	
	typedef enum DeviceChangeNotifyType {
		AnyChange = 0,
		ByDevice,
		ByDeviceClass,
		ByPort,
		HubChange};

	typedef struct Observer
	{
		Observer()
			: NotifyType(AnyChange)
			, DeviceType(DeviceClass::DeviceTypeNone)
			, pDevice(NULL)
			, Hub(_T(""))
			, HubIndex(0)
			, Event(UNKNOWN_EVT){};
		DeviceChangeCallback NotifyFn;
		DeviceChangeNotifyType NotifyType;
		DeviceClass::DeviceType DeviceType;
		Device* pDevice;
		CStdString Hub;
		int32_t HubIndex;
		int32_t Event;
	};

    //! \brief Provides access to the individual DeviceClasses.
	DeviceClass* operator[](DeviceClass::DeviceType devType);
	//! \brief Create the devClasses and start the message thread.
	//! \return Status.
    //! \retval ERROR_SUCCESS               No error occurred.
    //! \retval GetLastError() result       Error has occurred.
    //! \pre Can not have been called previously.
    //! \post Must call Close() to release resources.
    //! \note Runs in the context of the Client thread.
    //! \note Function holds Client thread until the DeviceManager thread has started.
    uint32_t Open();
	//! \brief Gracefully stop the thread.
    //! \note Runs in the context of the Client thread.
	void Close();
	//! \brief Registers a Client Callback Function that gets called for specified device changes.
    //! \note Runs in the context of the Client thread.
	HANDLE Register(const Observer& observer);
	//! \brief Registers a Client Callback Function that gets called for specified device changes.
    //! \note Runs in the context of the Client thread.
	HANDLE Register(DeviceChangeCallback notifyFn, DeviceChangeNotifyType notifyType = AnyChange, LPARAM pData = 0);
	//! \brief Unregisters Client Callback Function.
    //! \note Runs in the context of the Client thread.
	bool Unregister(HANDLE hObserver);
	//! \brief Calls DeviceClass::Refresh() for each DeviceClass held by DeviceManager[].
    //! \note Runs in the context of the Client thread.
    void Refresh();
	//! \brief Calls DeviceClass::ClearFilters() for each DeviceClass held by DeviceManager[].
    //! \note Runs in the context of the Client thread.
    void ClearFilters();
	//! \brief Calls DeviceClass::AddFilter(LPCTSTR,LPCTSTR,LPCTSTR) for each DeviceClass held by DeviceManager.
    //! \note Runs in the context of the Client thread.
    void AddFilter(LPCTSTR vid, LPCTSTR pid = NULL, LPCTSTR instance = NULL);
	//! \brief Calls DeviceClass::AddFilter(uint16_t, uint16_t) for each DeviceClass held by DeviceManager[].
    //! \note Runs in the context of the Client thread.
   	void AddFilter(uint16_t vid, uint16_t pid);
	//! \brief Removes all filters and creates new filters for applicable DeviceClasses plus some SigmaTel standard ones.
    //! \note Runs in the context of the Client thread.
	void UpdateDeviceFilters(LPCTSTR usbVid, LPCTSTR usbPid);
	//! \brief Calls DeviceClass::Devices() for each DeviceClass held by DeviceManager[].
    //! \note Runs in the context of the Client thread.
	std::list<Device*> DeviceManager::Devices();
	//! \note Runs in the context of the Client thread.
	Device* FindDevice(LPCTSTR paramStr);

    //! \note Gets the IWMDeviceManager3.
	//IWMDeviceManager3* GetWmdmDeviceManager() const { return _pWMDevMgr3; }; 

	//! \brief Calls CQueryCancelAutoplay::SetCancelAutoPlay(bool rejectAutoPlay, LPCTSTR driveList)
    //! \note Runs in the context of the Client thread.
	HRESULT SetCancelAutoPlay(bool rejectAutoPlay, LPCTSTR driveList = NULL) 
	{ 
		return _ICancelAutoPlayCallbackObject.SetCancelAutoPlay(rejectAutoPlay, driveList);
	};

private:
    //! \brief The purpose of this hidden window is just to pass WM_DEVICECHANGE messages to the DeviceManager thread.
    //! \see DeviceManager::OnMsgDeviceEvent(WPARAM eventType, LPARAM msg)
	class DevChangeWnd : public CWnd
    {
    public:
        afx_msg BOOL OnDeviceChange(UINT nEventType,DWORD_PTR dwData);
		CStdString DrivesFromMask(ULONG UnitMask);
		DECLARE_MESSAGE_MAP()
    } _DevChangeWnd;
    //! \brief Use this message in the DevChangeWnd::OnDeviceChange() to post messages to the DeviceManager thread.
    //! \see DeviceManager::Close()
    //! \see DeviceManager::OnMsgDeviceEvent(WPARAM eventType, LPARAM msg)
    const static uint32_t WM_MSG_DEV_EVENT = WM_USER+36;
    //! \brief Processes WM_MSG_DEV_EVENT messages for the DeviceManager thread.
    //! \see DeviceManager::Close()
    //! \see DeviceManager::DevChangeWnd::OnDeviceChange(UINT nEventType,DWORD_PTR dwData)
	virtual afx_msg void OnMsgDeviceEvent(WPARAM eventType, LPARAM desc);
	//! \brief Event handle used to notify Client thread about a change in the DeviceManager thread.
    //! \see DeviceManager::OnMsgDeviceEvent()
    //! \see DeviceManager::FindDevice()
    HANDLE _hChangeEvent;

	void Notify(const DeviceClass::NotifyStruct& nsInfo);

	//! \brief Calls DeviceClass::AddUsbDevice() for each DeviceClass held by DeviceManager[].
    //! \note Runs in the context of the DeviceManager thread.
	DeviceClass::NotifyStruct AddUsbDevice(LPCTSTR path);
	//! \brief Calls DeviceClass::RemoveUsbDevice() for each DeviceClass held by DeviceManager[].
    //! \note Runs in the context of the DeviceManager thread.
	DeviceClass::NotifyStruct RemoveUsbDevice(LPCTSTR path);

	// All this is private since Singleton holder is the only one who can create, copy or assign
    // a DeviceManager class.
    DeviceManager();
    ~DeviceManager();
	DeviceManager(const DeviceManager&);
	DeviceManager& operator=(const DeviceManager&);
    
    virtual BOOL InitInstance();
    virtual int ExitInstance();

	std::map<uint32_t, DeviceClass*> _devClasses;
    std::map<HANDLE, Observer*> _callbacks;

	//! \brief Used by ~DeviceManager() to tell if DeviceManager::ExitInstance() was called.
    //! \see DeviceManager::Open()
    //! \see DeviceManager::Close()
	bool _bStopped;
	//! \brief Used by Open() to syncronize DeviceManager thread start.
    //! \see DeviceManager::Open()
    HANDLE _hStartEvent;

	// Message Support
	HDEVNOTIFY _hUsbDev;
	HDEVNOTIFY _hMxUsbDev;
	HDEVNOTIFY _hUsbHub;

	// WMDM Support
	/*DWORD _dwWMDMNotificationCookie;
//	CComPtr<IComponentAuthenticate> _pICompAuth;
	IWMDeviceManager3* _pWMDevMgr3;
    HRESULT RegisterWMDMNotification();
	HRESULT UnregisterWMDMNotification();
	HRESULT InitMTPDevMgr();*/
	/*class CWMDMNotification : public IWMDMNotification
	{
    public:
		// IUnknown interface
		STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();
		// IWMDMNotification interface
        STDMETHODIMP WMDMMessage(DWORD  dwMessageType, LPCWSTR  pwszCanonicalName);
        CWMDMNotification() : _cRef(1) { };
		~CWMDMNotification() { };
	private:
        ULONG _cRef;
	} _IWMDMCallbackObject;*/

	// AutoPlay support
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
		CQueryCancelAutoplay() 
			: _cRef(1)
			, _defaultDriveList(_T("DEFGHIJKLMNOPQRSTUVWXYZ"))
			, _driveList(_T("DEFGHIJKLMNOPQRSTUVWXYZ"))
			, _ROT(0)
			, _isRegistered(0)
		{};
		~CQueryCancelAutoplay() { };
		HRESULT SetCancelAutoPlay(bool rejectAutoPlay, LPCTSTR driveList);
	private:
		HRESULT Unregister();
		ULONG _cRef;
		CStdString _driveList;
		DWORD _ROT;
		bool _isRegistered;
		const CStdString _defaultDriveList;
	} _ICancelAutoPlayCallbackObject;

	DECLARE_MESSAGE_MAP()
};

//! \brief Use gDeviceManager::Instance() to access the single global DeviceManager class
typedef Loki::SingletonHolder<DeviceManager, Loki::CreateUsingNew, Loki::DefaultLifetime, Loki::ClassLevelLockable> gDeviceManager;
