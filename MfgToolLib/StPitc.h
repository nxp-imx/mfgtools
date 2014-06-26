/*
 * Copyright (C) 2009-2014, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
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
