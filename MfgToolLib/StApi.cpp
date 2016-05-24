/*
 * Copyright 2009-2013, 2016 Freescale Semiconductor, Inc.
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

// StApi.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "StHidApi.h"



//////////////////////////////////////////////////////////////////////
//
// StApi class implementation
//
//////////////////////////////////////////////////////////////////////
StApi::~StApi()
{
	if ( _responseDataPtr )
	{
		free(_responseDataPtr);
		_responseDataPtr = NULL;
	}

    if ( _sendDataPtr )
    {
        free(_sendDataPtr);
        _sendDataPtr = NULL;
    }
}

// modifies member variables _sendDataPtr and _xferLength
int StApi::SetCommandData(const UCHAR * const pData, const size_t size)
{
//    if ( size == 0 || pData == NULL )
//        return ERROR_INVALID_PARAMETER;

    if ( _sendDataPtr )
    {
        free(_sendDataPtr);
        _sendDataPtr = NULL;
    }

//    size_t arraySize = max(size,_xferLength);

    if ( size && pData )
    {
        _sendDataPtr = (UCHAR*)malloc(size);

	    if ( _sendDataPtr == NULL )
        {
            _xferLength = 0;
		    return 8L;//ERROR_NOT_ENOUGH_MEMORY;
        }

	    // set all the allocated memory to 0xFF
//	    memset(_sendDataPtr, 0xFF, arraySize);

        memcpy(_sendDataPtr, pData, size);
    }

    _xferLength = (UINT)size;

    PrepareCommand();

    return ERROR_SUCCESS;
}

//////////////////////////////////////////////////////////////////////
//
// StApiFactory class implementation
//
//////////////////////////////////////////////////////////////////////
StApiFactory::StApiFactory()
{
	RegisterApi(_T("HidInquiry"), HidInquiry::Create);
	RegisterApi(_T("HidDownloadFw"), HidDownloadFw::Create);
	RegisterApi(_T("HidBltcRequestSense"), HidBltcRequestSense::Create);
	RegisterApi(_T("HidDeviceReset"), HidDeviceReset::Create);
	RegisterApi(_T("HidDevicePowerDown"), HidDevicePowerDown::Create);
	RegisterApi(_T("HidTestUnitReady"), HidTestUnitReady::Create);
	RegisterApi(_T("HidPitcRequestSense"), HidPitcRequestSense::Create);
	RegisterApi(_T("HidPitcInquiry"), HidPitcInquiry::Create);
	RegisterApi(_T("HidPitcRead"), HidPitcRead::Create);
	RegisterApi(_T("HidPitcWrite"), HidPitcWrite::Create);
/*	RegisterApi("HidInquiry", HidInquiry::Create);
	RegisterApi("HidInquiry", HidInquiry::Create);
	RegisterApi("HidInquiry", HidInquiry::Create);
	RegisterApi("HidInquiry", HidInquiry::Create);
	RegisterApi("HidInquiry", HidInquiry::Create);
	RegisterApi("HidInquiry", HidInquiry::Create);
*/
};

StApi* StApiFactory::CreateApi(CString name, CString paramStr)
{
	CallbackMap::const_iterator i = _callbacks.find(name);
	if ( i == _callbacks.end() )
	{
		// not found
		std::cerr << "*** ASSERTION FAILED: Line " << __LINE__ << " of file " << __TFILE__ << std::endl;
		//ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
		throw std::runtime_error("Unknown API name.");
	}
	// Invoke the creation function
	return (i->second)(paramStr);
};

bool StApiFactory::RegisterApi(CString name, CreateApiCallback createFn)
{
	return _callbacks.insert(CallbackMap::value_type(name, createFn)).second;
};

bool StApiFactory::UnregisterApi(CString name)
{
	return _callbacks.erase(name) == 1;
};

// The one and only StApiFactory object
StApiFactory& gStApiFactory()
{
	static StApiFactory factory;
	return factory;
};
