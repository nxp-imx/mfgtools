/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Reflection;

using PortableDeviceApiLib;
using PortableDeviceTypesLib;
using DevSupport.WPD;
using DevSupport.Api;

namespace DevSupport.DeviceManager
{
    /// <summary>
    /// A Windows Portable Device (WPD).
    /// </summary>
    public class WpdDevice : Device, ILiveUpdater, IResetToRecovery
    {
//        public static PortableDeviceApiLib._tagpropertykey WPD_MTP_CUSTOM_COMMAND ;

        // Create our client information collection
        private PortableDeviceApiLib.IPortableDeviceValues _ClientInfo = (PortableDeviceApiLib.IPortableDeviceValues)new PortableDeviceValuesClass();
        // Create a new IPortableDevice instance
        private PortableDeviceClass _PortableDevice = new PortableDeviceClass();
        private uint _ResponseCode;

        internal WpdDevice(/*DeviceClass deviceClass,*/ IntPtr deviceInstance, string path/*, int index*/)
            : base(/*deviceClass,*/ deviceInstance, path/*, index*/)
        {
            // Create our client information collection
            _ClientInfo.SetStringValue(ref PortableDevicePKeys.WPD_CLIENT_NAME, "DevSupport");
            _ClientInfo.SetSignedIntegerValue(ref PortableDevicePKeys.WPD_CLIENT_MAJOR_VERSION, Assembly.GetExecutingAssembly().GetName().Version.Major);
            _ClientInfo.SetSignedIntegerValue(ref PortableDevicePKeys.WPD_CLIENT_MINOR_VERSION, Assembly.GetExecutingAssembly().GetName().Version.Minor);
            _ClientInfo.SetSignedIntegerValue(ref PortableDevicePKeys.WPD_CLIENT_REVISION, Assembly.GetExecutingAssembly().GetName().Version.Revision);
            //  Some device drivers need to impersonate the caller in order to function correctly.  Since our application does not
            //  need to restrict its identity, specify SECURITY_IMPERSONATION so that we work with all devices.
            _ClientInfo.SetUnsignedIntegerValue(ref PortableDevicePKeys.WPD_CLIENT_SECURITY_QUALITY_OF_SERVICE, Win32.SECURITY_IMPERSONATION);
      }

        public override int SendCommand(DevSupport.Api.Api api)
        {
            // reset the error string
            ErrorString = String.Empty;
            _ResponseCode = MtpExtensions.MTP_RESPONSE_OK;

            // check the api is for our type of device.
            Api.WpdApi wpdApi = api as Api.WpdApi;
            if (wpdApi == null)
            {
                ErrorString = String.Format(" Error: Can not send \"{0}\" api type to \"{1}\" device.", api.ImageKey, this.ToString());
                return Win32.ERROR_INVALID_PARAMETER;
            }

            // tell the UI we are beginning a command.
            SendCommandProgressArgs cmdProgress = new SendCommandProgressArgs(api.ToString(), api.Direction, Convert.ToUInt32(api.TransferSize));
            DoSendProgress(cmdProgress);

            // open the device
            _PortableDevice.Open(Path, _ClientInfo);

            // pointer to return values
            PortableDeviceApiLib.IPortableDeviceValues retValues = (PortableDeviceApiLib.IPortableDeviceValues)new PortableDeviceValuesClass();
            
            // send the command
            _PortableDevice.SendCommand(0, wpdApi.CommandValues, out retValues);

            if (wpdApi.Direction != DevSupport.Api.Api.CommandDirection.NoData)
            {
                if (wpdApi.Direction != DevSupport.Api.Api.CommandDirection.ReadWithData)
                {
                    ProcessReadData();
                }
                else
                {
                    ProcessWriteData();
                }

                ProcessEndData();
            }

            // close the device
            _PortableDevice.Close();

            // Check if the driver succeeded in sending the command by interrogating WPD_PROPERTY_COMMON_HRESULT
            int cmdError;
            retValues.GetErrorValue(ref PortableDevicePKeys.WPD_PROPERTY_COMMON_HRESULT, out cmdError);

            if (cmdError == Win32.S_OK)
            {
                // If the command was executed successfully, we check the MTP response code to see if the
                // device could handle the command. Note that there is a distinction between the command
                // being successfully sent to the device and the command being handled successfully by the device
                retValues.GetUnsignedIntegerValue(ref MtpExtensions.WPD_PROPERTY_MTP_EXT_RESPONSE_CODE, out _ResponseCode);
                cmdError = (_ResponseCode == MtpExtensions.MTP_RESPONSE_OK) ? Win32.S_OK : Win32.E_FAIL;
            }

            // If the command was executed successfully, the MTP response parameters are returned in 
            // the WPD_PROPERTY_MTP_EXT_RESPONSE_PARAMS property which is a PropVariantCollection
            PortableDeviceApiLib.IPortableDevicePropVariantCollection responseParams =
                (PortableDeviceApiLib.IPortableDevicePropVariantCollection)new PortableDevicePropVariantCollectionClass();
            if (cmdError == Win32.S_OK)
            {
                retValues.GetIPortableDevicePropVariantCollectionValue(ref MtpExtensions.WPD_PROPERTY_MTP_EXT_RESPONSE_PARAMS,
                                                                                out responseParams);

                if ( wpdApi.Direction != DevSupport.Api.Api.CommandDirection.WriteWithData )
                {
                    // let the api parse the return data
                    cmdError = wpdApi.ProcessResponse(responseParams);
                    if (cmdError != Win32.ERROR_SUCCESS)
                    {
                        ErrorString = String.Format(" ERROR: The \"{0}\" api was unable to process the response.", wpdApi.ToString());
                    }
                }
            }
            
	        // tell the UI we are done
            cmdProgress.Error = cmdError;
            cmdProgress.InProgress = false;
            DoSendProgress(cmdProgress);

            return cmdProgress.Error;
        }

        private void ProcessReadData()
        {
            //TODO: implement this.
            throw new NotImplementedException();
        }

        private void ProcessWriteData()
        {
            //TODO: implement this.
            throw new NotImplementedException();
        }

        private void ProcessEndData()
        {
            //TODO: implement this.
            throw new NotImplementedException();
        }

        public override String ErrorString
        {
            get
            {
                String errStr = String.IsNullOrEmpty(base.ErrorString) ? String.Empty : base.ErrorString + "\r\n";

                if (_ResponseCode != WPD.MtpExtensions.MTP_RESPONSE_OK)
                {
                    errStr = String.Format(" MTP Response: ERROR(0x{0:X4})\r\n", _ResponseCode);
                }

                return errStr;
            }
        }

        private bool CompareObjectName(PortableDeviceApiLib.IPortableDeviceProperties properties, String objectId, String objectName)
        {
            // Specify the properties we are interested in
            PortableDeviceApiLib.IPortableDeviceKeyCollection spPropertyKeys =
                (PortableDeviceApiLib.IPortableDeviceKeyCollection)new PortableDeviceTypesLib.PortableDeviceKeyCollectionClass();
            spPropertyKeys.Add(ref PortableDevicePKeys.WPD_OBJECT_NAME);

            // Holder for the property values
            PortableDeviceApiLib.IPortableDeviceValues spPropertyValues;
            // get the property values
            properties.GetValues(objectId, spPropertyKeys, out spPropertyValues);
            // Get the WPD_OBJECT_NAME property
            String propertyValue_Name;
            spPropertyValues.GetStringValue(ref PortableDevicePKeys.WPD_OBJECT_NAME, out propertyValue_Name);

            if (String.Compare(propertyValue_Name, objectName, true) == 0)
            {
                // found object by name
                return true;
            }
            else
            {
                return false;
            }
        }

        public String ObjectIdFromName(String objectName)
        {
            // open the device
            _PortableDevice.Open(Path, _ClientInfo);

            //
            // Get content interface required to enumerate
            //
            PortableDeviceApiLib.IPortableDeviceContent pContent;
            _PortableDevice.Content(out pContent);

            String retId =  RecurseObjectIdFromName(ref pContent, "DEVICE", objectName);

            // close the device
            _PortableDevice.Close();

            return retId;
        }

        private String RecurseObjectIdFromName(ref PortableDeviceApiLib.IPortableDeviceContent pContent, String parentId, String objectName)
        {
            String returnId = String.Empty;

            // Get the IPortableDeviceProperties interface of supplied object
            PortableDeviceApiLib.IPortableDeviceProperties spProperties;
            pContent.Properties(out spProperties);

            if (CompareObjectName(spProperties, parentId, objectName))
            {
                returnId = parentId;
            }
            else
            {
                //
                // Enumerate children (if any)
                //
                PortableDeviceApiLib.IEnumPortableDeviceObjectIDs pEnum;
                pContent.EnumObjects(0, parentId, null, out pEnum);

                uint cFetched = 0;
                do
                {
                    string objectID;
                    pEnum.Next(1, out objectID, ref cFetched);

                    if (cFetched > 0)
                    {
                        //
                        // Recurse into children
                        //
                        returnId = RecurseObjectIdFromName(ref pContent, objectID, objectName);
                    }
                } while (cFetched > 0 && String.IsNullOrEmpty(returnId));
            }

            return returnId;
        }

        #region ILiveUpdater Members

        public Byte[] GetDeviceDataFromFile(String fileName)
        {
            Byte[] returnBytes = null;

            ///
            /// Get the Obect ID for filename
            /// 
            String objectId = ObjectIdFromName(fileName);
            if (String.IsNullOrEmpty(objectId))
            {
                ErrorString = String.Format(" Error: Problem getting object ID for \"{0}\".", fileName);
                return returnBytes;
            }

            // open the device
            _PortableDevice.Open(Path, _ClientInfo);

            try
            {
                // Get content interface for the device
                PortableDeviceApiLib.IPortableDeviceContent pContent;
                _PortableDevice.Content(out pContent);

                // Get the interface required to get properties from the enumerated root object
                PortableDeviceApiLib.IPortableDeviceProperties spProperties;
                pContent.Properties(out spProperties);

                // Get ALL the properties
                PortableDeviceApiLib.IPortableDeviceValues spPropertyValues;
                spProperties.GetValues(objectId, null, out spPropertyValues);

                // Get the size of the object
                UInt64 propertyValue_Size = 0;
                try
                {
                    spPropertyValues.GetUnsignedLargeIntegerValue(ref PortableDevicePKeys.WPD_OBJECT_SIZE, out propertyValue_Size);
                }
                catch (Exception e)
                {
                    // WPD_OBJECT_SIZE may not be supported some objects
                    propertyValue_Size = 0;
                    Debug.WriteLine(e.Message);
                }

                // Get the interface used to read from content data of an existing object resource.
                PortableDeviceApiLib.IPortableDeviceResources resourcesIface;
                pContent.Transfer(out resourcesIface);

                // Gets an IStream interface with which to read or write the content data in an object on a device
                uint optimalBufferSize = 0;
                PortableDeviceApiLib.IStream readStream;
                resourcesIface.GetStream(objectId, ref PortableDevicePKeys.WPD_RESOURCE_DEFAULT, (uint)WPD.Utils.StgmConstants.STGM_READ, ref optimalBufferSize, out readStream);

                //convert to a useful stream object
                System.Runtime.InteropServices.ComTypes.IStream sourceStream =
                    (System.Runtime.InteropServices.ComTypes.IStream)readStream;

                // read the bytes
                returnBytes = new Byte[propertyValue_Size];
                UInt64 totalBytesRead = 0;
                Byte[] streamBuffer = new byte[optimalBufferSize];

                GCHandle hBytesRead = GCHandle.Alloc(new ulong(), GCHandleType.Pinned);
                while (totalBytesRead < propertyValue_Size)
                {
                    sourceStream.Read(streamBuffer, (int)optimalBufferSize, hBytesRead.AddrOfPinnedObject());
                    Array.Copy(streamBuffer, 0, returnBytes, (long)totalBytesRead, Convert.ToInt64((UInt64)hBytesRead.Target));

                    totalBytesRead += (ulong)hBytesRead.Target;

                }
                hBytesRead.Free();
            }
            finally
            {
                // close the device
                _PortableDevice.Close();
            }

            return returnBytes;
        }

        public int CopyUpdateFileToMedia(string fileName)
        {
            // reset the error string
            Int32 retValue = Win32.ERROR_SUCCESS;
            ErrorString = String.Empty;
            _ResponseCode = MtpExtensions.MTP_RESPONSE_OK;

            ///
            /// Get the firmware file data
            ///
            FileInfo fileInfo = new FileInfo(fileName);
            if ( !fileInfo.Exists )
            {
                ErrorString = String.Format(" Error: \"{0}\" does not exist.", fileName);
                return Win32.ERROR_FILE_NOT_FOUND;
            }

            Byte[] fileData = new Byte[fileInfo.Length];
            try
            {
                fileData = File.ReadAllBytes(fileInfo.FullName);
            }
            catch (Exception e)
            {
                ErrorString = String.Format(" Error: Problem reading {0}. {1}.", fileName, e.Message);
                return Win32.ERROR_OPEN_FAILED;
            }

            ///
            /// Delete the file if it exists on the device
            ///
            DeleteFile(fileName);

            ///
            /// Get the Internal Storage Obect ID
            /// 
            String storageObjectId = ObjectIdFromName("Internal Storage");
            if (String.IsNullOrEmpty(storageObjectId))
            {
                ErrorString = String.Format(" Error: Problem getting object ID for \"Internal Storage\".");
                return Win32.ERROR_FILE_NOT_FOUND;
            }

            ///
            /// Tell the UI we are beginning a command.
            ///
            SendCommandProgressArgs cmdProgress = new SendCommandProgressArgs("CopyUpdateFileToMedia", Api.Api.CommandDirection.WriteWithData, (UInt32)fileInfo.Length);
            DoSendProgress(cmdProgress);

            ///
            /// Open the device
            ///
            try
            {
                _PortableDevice.Open(Path, _ClientInfo);
            }
            catch (Exception e)
            {
                ErrorString = String.Format(" Error: Problem opening device. {0}.", e.Message);
                retValue = Win32.ERROR_OPEN_FAILED;
                goto CopyUpdateFileToMedia_Exit;
            }

            ///
            /// Create the object properties structure and attach the basic properties for the firmware object.
            ///
            PortableDeviceApiLib.IPortableDeviceValues pValues = (PortableDeviceApiLib.IPortableDeviceValues)new PortableDeviceValuesClass();

            pValues.SetStringValue(ref PortableDevicePKeys.WPD_OBJECT_PARENT_ID, storageObjectId);
            pValues.SetUnsignedLargeIntegerValue(ref PortableDevicePKeys.WPD_OBJECT_SIZE, (ulong)fileInfo.Length);
            pValues.SetStringValue(ref PortableDevicePKeys.WPD_OBJECT_ORIGINAL_FILE_NAME, fileInfo.Name);
            pValues.SetStringValue(ref PortableDevicePKeys.WPD_OBJECT_NAME, fileInfo.Name);
            
            ///
            /// Transfer the content to the device
            ///

            // Get content interface for the device
            PortableDeviceApiLib.IPortableDeviceContent pContent;
            _PortableDevice.Content(out pContent);
            if (pContent == null)
            {
                ErrorString = " Error: Problem getting IPortableDeviceContent interface.";
                retValue = Win32.ERROR_GEN_FAILURE;
                goto CopyUpdateFileToMedia_Exit;
            }

            uint optimalBufferSize = 0;
            PortableDeviceApiLib.IStream writeStream;
            String cookie = "ThisIsAUniqueString";
            pContent.CreateObjectWithPropertiesAndData(pValues, out writeStream, ref optimalBufferSize, ref cookie);
            
            //convert to a useful stream object
            System.Runtime.InteropServices.ComTypes.IStream destinationStream =
                (System.Runtime.InteropServices.ComTypes.IStream)writeStream;

            // write the bytes
            UInt32 totalBytesWritten = 0;
            Byte[] streamBuffer = new byte[optimalBufferSize];

            GCHandle hBytesWritten = GCHandle.Alloc(new UInt32(), GCHandleType.Pinned);
            while (totalBytesWritten < fileInfo.Length)
            {
                Int32 numBytesToWrite = Math.Min(streamBuffer.Length, fileData.Length - (Int32)totalBytesWritten);
                Array.Copy(fileData, totalBytesWritten, streamBuffer, 0, numBytesToWrite);
                try
                {
                    destinationStream.Write(streamBuffer, numBytesToWrite, hBytesWritten.AddrOfPinnedObject());
                    totalBytesWritten += (UInt32)hBytesWritten.Target;

                    // Update the UI
                    cmdProgress.Position = Convert.ToInt32(totalBytesWritten);
                    DoSendProgress(cmdProgress);
                }
                catch (Exception e)
                {
                    ErrorString = e.Message;
                    retValue = Win32.ERROR_GEN_FAILURE;
                    goto CopyUpdateFileToMedia_Exit;
                }
            }
            hBytesWritten.Free();

            if (totalBytesWritten != fileInfo.Length)
            {
                ErrorString = String.Format(" Error: Only wrote {0} of {1} bytes to device.", totalBytesWritten, fileInfo.Length);
                retValue = Win32.ERROR_BAD_LENGTH;
                goto CopyUpdateFileToMedia_Exit;
            }

            // Success so Commit changes
            destinationStream.Commit(0);

CopyUpdateFileToMedia_Exit:
            
            // Close the device
            _PortableDevice.Close();

            // Tell the UI we are done.
            cmdProgress.Error = retValue;
            cmdProgress.InProgress = false;
            DoSendProgress(cmdProgress);

            return Win32.ERROR_SUCCESS;
        }

        #endregion

        public Int32 DeleteFile(String fileName)
        {
            Int32 retValue = Win32.ERROR_SUCCESS;

            String objectId = ObjectIdFromName(fileName);
            if ( String.IsNullOrEmpty(objectId) )
                return Win32.ERROR_FILE_NOT_FOUND;

            // open the device
            _PortableDevice.Open(Path, _ClientInfo);

            // Get content interface for the device
            PortableDeviceApiLib.IPortableDeviceContent pContent;
            _PortableDevice.Content(out pContent);

            // Make the collection of objectIds parameter
            PortableDeviceApiLib.IPortableDevicePropVariantCollection objectIds = 
                (PortableDeviceApiLib.IPortableDevicePropVariantCollection)new PortableDevicePropVariantCollectionClass();
            PortableDeviceApiLib.tag_inner_PROPVARIANT pvObjectId = WPD.Utils.ToPropVarient(objectId);
            objectIds.Add(ref pvObjectId);
            // Make the results parameter
            PortableDeviceApiLib.IPortableDevicePropVariantCollection results = 
                (PortableDeviceApiLib.IPortableDevicePropVariantCollection)new PortableDevicePropVariantCollectionClass();
            
            ///
            /// Delete the object
            ///
            pContent.Delete(0/*PORTABLE_DEVICE_DELETE_NO_RECURSION*/, objectIds, ref results);

            // Get the result  out of the result Variant collection
            PortableDeviceApiLib.tag_inner_PROPVARIANT propvarValue = new PortableDeviceApiLib.tag_inner_PROPVARIANT();
            results.GetAt(0, ref propvarValue);
            GCHandle hVariant = GCHandle.Alloc(propvarValue, GCHandleType.Pinned);
            object obj = Marshal.GetObjectForNativeVariant(hVariant.AddrOfPinnedObject());
            retValue = Convert.ToInt32(obj);
            hVariant.Free();

            // close the device
            _PortableDevice.Close();

            return retValue;
        }

        #region IResetToRecovery Members

        public Int32 ResetToRecovery()
        {
            WpdApi.ResetToRecovery api = new WpdApi.ResetToRecovery();
            return SendCommand(api);
        }

        #endregion
    }
}
/*
/*            
            // Get capabilities interface for the device
            PortableDeviceApiLib.IPortableDeviceCapabilities pCapabilities;
            _PortableDevice.Capabilities(out pCapabilities);
            if (pCapabilities == null)
            {
                ErrorString = " Error: Problem getting IPortableDeviceCapabilities interface.";
                retValue = Win32.ERROR_GEN_FAILURE;
                goto CopyUpdateFileToMedia_Exit;
            }

            // Get the WPD_FUNCTIONAL_CATEGORY_STORAGE objects
            PortableDeviceApiLib.IPortableDevicePropVariantCollection storageObjects =
                 (PortableDeviceApiLib.IPortableDevicePropVariantCollection)new PortableDevicePropVariantCollectionClass();
            pCapabilities.GetFunctionalObjects(ref WPD.PortableDeviceGuids.WPD_FUNCTIONAL_CATEGORY_STORAGE, out storageObjects);
            if (storageObjects == null)
            {
                ErrorString = " Error: Problem getting WPD_FUNCTIONAL_CATEGORY_STORAGE objects.";
                retValue = Win32.ERROR_GEN_FAILURE;
                goto CopyUpdateFileToMedia_Exit;
            }

            // The first storage object should be the Internal Storage
            UInt32 storageObjectCount = 0;
            storageObjects.GetCount(ref storageObjectCount);
            if (storageObjectCount < 1)
            {
                ErrorString = " Error: No WPD_FUNCTIONAL_CATEGORY_STORAGE objects.";
                retValue = Win32.ERROR_OPEN_FAILED;
                goto CopyUpdateFileToMedia_Exit;
            }

            // GetFunctionalObjects() returned the ids of the StorageObject as PropVarient Strings
            // so create a PropVarient to retrieve the StorageObjectIds as Strings.
            PortableDeviceApiLib.tag_inner_PROPVARIANT propvarValue = WPD.Utils.ToPropVarient(String.Empty);
*/
            // Get the StorageObjectId and convert it to a string.
//            storageObjects.GetAt(0/*Internal Storage*/, ref propvarValue);
//            String storageObjectId = WPD.Utils.PropVariantToString(propvarValue);

/*
MtpDevice::MtpDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
: Device(deviceClass, devInst, path)
, _pWmdmDevice3(NULL)
, _responseCode(0)
{

	HRESULT hr = S_OK;

	// we need to find out which device we are trying to create. We have a path from the DevNode: Device::_path. 
	// We can compare a partial string of our _path to the CanonicalName of each of the devices enumerated by the WMDM device manager. 
	//
	// skip the first 4 characters //?/ or \\.\
	//
	CStdString devPath = _path.get().GetBuffer() + 4;
	//
	// Take everything off after the last # and convert it to upper case
	devPath = devPath.Left(devPath.ReverseFind(_T('#'))).ToUpper();


	// Go through the WMDM devices and see if our devPath key string lives in any of the WMDM device CanonicalNames
	CComPtr<IWMDMEnumDevice> pMtpDeviceEnum = NULL;
	CComPtr<IWMDeviceManager3> pWmdmDevMgr = gDeviceManager::Instance().GetWmdmDeviceManager();
	if ( pWmdmDevMgr )
	{
		hr = pWmdmDevMgr->Reinitialize();

		hr = pWmdmDevMgr->EnumDevices2(&pMtpDeviceEnum);
	}
	if ( pMtpDeviceEnum )
	{
		hr = pMtpDeviceEnum->Reset();

		while( TRUE )
		{
			IWMDMDevice*  pWmdmDevice = NULL;
			IWMDMDevice3* pWmdmDevice3 = NULL;
			ULONG         ulFetched = 0;
			WCHAR         szCanotonicalName[512] = {0};

			hr = pMtpDeviceEnum->Next( 1, &pWmdmDevice, &ulFetched );
			if( hr != S_OK )
			{
				break;
			}
			if( ulFetched != 1 )
			{
				hr = E_UNEXPECTED;
				break;
			}

			// We need the third interface for vendor commands.
			hr = pWmdmDevice->QueryInterface (IID_IWMDMDevice3, (void**) &pWmdmDevice3);
			if( FAILED(hr) )
			{
				_pWmdmDevice3 = NULL;
				return;
			}

			hr = pWmdmDevice3->GetCanonicalName( szCanotonicalName, sizeof(szCanotonicalName)/sizeof(szCanotonicalName[0]) - 1 );
			if( FAILED(hr) )
			{
				wcscpy_s( szCanotonicalName, 512, L"" );
				return;
			}

			CStdString canonicalName = szCanotonicalName;
			canonicalName.ToUpper();
			if ( canonicalName.Find(devPath) != -1 )
			{
				// FOUND OUR DEVICE. Save the third interface as our device pointer.
				_pWmdmDevice3 = pWmdmDevice3;
				break;
			}
		}
	}
}

MtpDevice::~MtpDevice(void)
{
	if ( _pWmdmDevice3 )
	{
		_pWmdmDevice3->Release();
		_pWmdmDevice3 = NULL;
	}
}
*/

/*
CStdString MtpDevice::GetSerialNumberStr()
{
    CStdString serNoStr = _T("");

	WMDMID SerialNumberStruct;
	BYTE abMAC[] = {0, 0, 0, 0, 0, 0, 0, 0};

    if ( _pWmdmDevice3 )
	{
		HRESULT hr = _pWmdmDevice3->GetSerialNumber(&SerialNumberStruct, abMAC );
		if( SUCCEEDED(hr) )
		{
			serNoStr = (LPCWSTR)SerialNumberStruct.pID;
		}
	}

	return serNoStr;
}
*/
/*
uint32_t MtpDevice::ResetChip()
{
	api::MtpDeviceReset api;

	return SendCommand(api);
}

uint32_t MtpDevice::ResetToRecovery()
{
	api::MtpResetToRecovery api;

	return SendCommand(api);
}

uint32_t MtpDevice::OldResetToRecovery()
{
	uint32_t error;
	api::MtpEraseBootmanager apiEraseBootMgr;
	api::MtpDeviceReset apiReset;

	error = SendCommand(apiEraseBootMgr);

	if ( error == ERROR_SUCCESS )
	{
		error = SendCommand(apiReset);
	}

	return error;
}
*/
/*
BOOL MtpDevice::Open()
{
	m_RecoveryHandle = CreateFile(_path.get(), GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if( m_RecoveryHandle == INVALID_HANDLE_VALUE)        
    {
        m_ErrorStatus=GetLastError();
        return FALSE;
    }
 
    m_FileOverlapped.hEvent = 0;
	m_FileOverlapped.Offset = 0;
	m_FileOverlapped.OffsetHigh = 0;

    m_SyncEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	
    if( !m_SyncEvent )
	{
		m_ErrorStatus = GetLastError();
		return FALSE;
	}

    return TRUE;
}

BOOL MtpDevice::Close()
{
    if( m_SyncEvent )
	{
		CloseHandle(m_SyncEvent);
		m_SyncEvent = NULL;
	}

    if(m_RecoveryHandle)
        return CloseHandle(m_RecoveryHandle);
    else
        return 0; // error       
}

void CALLBACK MtpDevice::IoCompletion(DWORD dwErrorCode,           // completion code
								      DWORD dwNumberOfBytesTransfered,  // number of bytes transferred
									  LPOVERLAPPED lpOverlapped)        // pointer to structure with I/O information
{
    if( ((ULONG)(ULONGLONG)lpOverlapped->hEvent != dwNumberOfBytesTransfered) || dwErrorCode )
	{
		*(BOOL *)&lpOverlapped->Offset = 0;
	}
	else
	{
		*(BOOL *)&lpOverlapped->Offset = dwNumberOfBytesTransfered;
	}

    SetEvent(m_SyncEvent);
}


// not used at this time 
CStdString MtpDevice::GetErrorStr()
{
	CStdString msg;

	return msg;
}
*/
/*
uint32_t MtpDevice::SendCommand(StApi& api, uint8_t* additionalInfo)
{

//	// If it is not a MTP Api, return error.
//	if ( api.GetType() != API_TYPE_ST_MTP )
//		return ERROR_INVALID_PARAMETER;

    // tell the UI we are beginning a command.
    NotifyStruct nsInfo(api.GetName());
    nsInfo.direction = api.IsWriteCmd() ? Device::NotifyStruct::dataDir_ToDevice : Device::NotifyStruct::dataDir_FromDevice;
    Notify(nsInfo);

	// init parameter if it is used
	if (additionalInfo)
		*additionalInfo = ERROR_SUCCESS;

	// make sure the command itself is ready
	api.PrepareCommand();

    HRESULT hr;
    DWORD dwSize = SIZEOF_REQUIRED_COMMAND_DATA_OUT;
    
	MTP_COMMAND_DATA_IN MtpDataIn(*(_MTP_COMMAND_DATA_IN*)api.GetCdbPtr());
    MTP_COMMAND_DATA_OUT MtpDataOut = {0};

    hr = _pWmdmDevice3->DeviceIoControl ( IOCTL_MTP_CUSTOM_COMMAND,
                                (BYTE *)&MtpDataIn, (DWORD)SIZEOF_REQUIRED_COMMAND_DATA_IN,
                                (BYTE *)&MtpDataOut, (LPDWORD)&dwSize);

	nsInfo.error = hr;
	_responseCode = MtpDataOut.ResponseCode;
	
	if (SUCCEEDED(hr) && _responseCode == MTP_RESPONSE_OK)
	{
		nsInfo.error = ERROR_SUCCESS;
	}
//	else
    if (additionalInfo)
	{
		*additionalInfo = true;
	}

	if ( !api.IsWriteCmd() )
	{
		api.ProcessResponse((uint8_t*)&MtpDataOut, 0, sizeof(MtpDataOut));
	}
    
	// tell the UI we are done
    nsInfo.inProgress = false;
    Notify(nsInfo);

    return nsInfo.error;

}
//WMDM_E_NOTSUPPORTED
//E_NOTIMPL

CStdString MtpDevice::GetSendCommandErrorStr()
{
	CStdString msg;
	if ( _responseCode == MTP_RESPONSE_OK )
	{
		msg.Format(_T("MTP Response: OK(0x%04X)\r\n"), _responseCode);
	}
	else
	{
		msg.Format(_T("MTP Response: ERROR(0x%04X)\r\n"), _responseCode);
	}

	return msg;
}
*/
