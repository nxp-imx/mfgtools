/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StApiFactory.h
//
//////////////////////////////////////////////////////////////////////

#if !defined(STAPIFACTORY_H__INCLUDED)
#define STAPIFACTORY_H__INCLUDED

//#include "Common/StdString.h"
#include "StDdiApi.h"
#include "StHidApi.h"

namespace api
{

	class StApiFactory
	{
	public:
		typedef StApi* (*CreateApiCallback)(CStdString paramStr);
	private:
		typedef std::map<CStdString, CreateApiCallback> CallbackMap;
	
	public:
        StApiFactory()
		{
			RegisterApi(L"HidInquiry", HidInquiry::Create);
			RegisterApi(L"HidDownloadFw", HidDownloadFw::Create);
			RegisterApi(L"HidBltcRequestSense", HidBltcRequestSense::Create);
			RegisterApi(L"HidDeviceReset", HidDeviceReset::Create);
			RegisterApi(L"HidDevicePowerDown", HidDevicePowerDown::Create);
			RegisterApi(L"HidTestUnitReady", HidTestUnitReady::Create);
			RegisterApi(L"HidPitcRequestSense", HidPitcRequestSense::Create);
/*			RegisterApi(L"HidInquiry", HidInquiry::Create);
			RegisterApi(L"HidInquiry", HidInquiry::Create);
			RegisterApi(L"HidInquiry", HidInquiry::Create);
			RegisterApi(L"HidInquiry", HidInquiry::Create);
			RegisterApi(L"HidInquiry", HidInquiry::Create);
			RegisterApi(L"HidInquiry", HidInquiry::Create);
			RegisterApi(L"HidInquiry", HidInquiry::Create);
			RegisterApi(L"HidInquiry", HidInquiry::Create);
			RegisterApi(L"HidInquiry", HidInquiry::Create);
			RegisterApi(L"HidInquiry", HidInquiry::Create);
*/
		};

		bool RegisterApi(CStdString name, CreateApiCallback createFn)
		{
			return _callbacks.insert(CallbackMap::value_type(name, createFn)).second;
		};
		
		bool UnregisterApi(CStdString name)
		{
			return _callbacks.erase(name) == 1;
		};
		
		StApi* CreateApi(CStdString name, CStdString paramStr="")
		{
			CallbackMap::const_iterator i = _callbacks.find(name);
			if ( i == _callbacks.end() )
			{
				// not found
				throw std::runtime_error("Unknown API name.");
			}
			// Invoke the creation function
			return (i->second)(paramStr);
		};
	
	private:
		CallbackMap _callbacks;
	};

} // namespace api

// The one and only StApiFactory object
StApiFactory& gStApiFactory()
{
	static StApiFactory factory;
	return factory;
};
using namespace api;

#endif // !defined(STAPIFACTORY_H__INCLUDED)
