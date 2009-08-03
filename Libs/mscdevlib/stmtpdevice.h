#pragma once

#include <sac.h>
#include <SCClient.h>
#include "StVersionInfo.h"

class CStMtpDevice
{
    HRESULT                m_hrInit;
//    IWMDMNotification     *m_pICallbackObject;
//    DWORD                  m_dwNotificationCookie;

public:

    CSecureChannelClient    *m_pSAC;
    IWMDeviceManager        *m_pWMDevMgr;
    IWMDMDevice3            *m_pWmdmDevice;


    CStMtpDevice(wchar_t *szManufacturer, USHORT VendorId, USHORT ProductId);
    ~CStMtpDevice(void);
	BOOL IsValid() { return m_pWmdmDevice != NULL; }; 

    ST_ERROR ResetMtpDeviceForRecovery(void);
	BOOL OldResetMtpDeviceForRecovery(void);
    ST_ERROR ResetMtpDeviceToMscUpdaterMode(void);
	ST_ERROR MtpGetUpdaterNandVersion(CStVersionInfo& updaterVersion);
	ST_ERROR MtpSetResetToUpdaterFlag(void);
};
