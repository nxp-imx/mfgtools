/*
 * Copyright 2009-2014, Freescale Semiconductor, Inc. All Rights Reserved.
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
#pragma once
#include "HidDevice.h"
#include "StHidApi.h"

class StPitc
{
//protected: clw: Was originally intended only for use as a base class
public:
    StPitc(HidDevice * const pHidDevice);
    StPitc(HidDevice * const HidDevice, UCHAR *pFileDataBuf, ULONGLONG dwFileSize);
    virtual ~StPitc(void);

public:
    //StFwComponent& GetFwComponent() { return _fwComponent; };
    UINT32 DownloadPitc();
    bool IsPitcLoaded();
    UINT32 SendPitcCommand(StApi& api);
    CString GetResponseStr() { return _strResponse; };
//    const UINT32 Id;
//    const CStdString FileName;
//    const uint16_t LangId;
private:
    HidDevice * const  _pHidDevice;
    UCHAR * _pFileDataBuf;
    ULONGLONG _dwFileSize;
    CString _strResponse;
};
