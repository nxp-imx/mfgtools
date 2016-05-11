/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
