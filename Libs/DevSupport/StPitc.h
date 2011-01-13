/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
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
	StPitc(Device * const pDevice);
	StPitc(Device * const device, LPCTSTR fileName, StFwComponent::LoadFlag loadFlag = StFwComponent::LoadFlag_FileFirst, const uint16_t langId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    virtual ~StPitc(void);

public:
    StFwComponent& GetFwComponent() { return _fwComponent; };
    uint32_t DownloadPitc(Device::UI_Callback callbackFn);
    bool IsPitcLoaded();
    uint32_t SendPitcCommand(StApi& api);
	CStdString GetResponseStr() { return _strResponse; };
//    const uint32_t Id;
//    const CStdString FileName;
//	const uint16_t LangId;
private:
	Device * const  _pDevice;
    StFwComponent _fwComponent;
    CStdString _strResponse;
};
