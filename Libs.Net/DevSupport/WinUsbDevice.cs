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
using Microsoft.Win32.SafeHandles;

using DevSupport.DeviceManager.BulkOnlyTransportProtocol;

namespace DevSupport.DeviceManager
{
    /// <summary>
    /// A WinUSB device.
    /// </summary>
    sealed public partial class WinUsbDevice : Device//, IComparable
    {
        internal WinUsbDevice(/*DeviceClass deviceClass,*/ IntPtr deviceInstance, string path/*, int index*/)
            : base(/*deviceClass,*/ deviceInstance, path/*, index*/)
        {
            if (GetDeviceHandle(path))
            {
                InitializeDevice();
            }
        }
        
        internal struct devInfo
        {
            internal SafeFileHandle deviceHandle;
            internal Int32 winUsbHandle;
            internal Byte bulkInPipe;
            internal Byte bulkOutPipe;
            internal Byte interruptInPipe;
            internal Byte interruptOutPipe;
            internal Byte deviceSpeed;
        }
        internal devInfo myDevInfo = new devInfo();

        ///  <summary>
        ///  Closes the device handle obtained with CreateFile and frees resources.
        ///  </summary>
        ///  
        internal void CloseDeviceHandle()
        {
            try
            {
                WinUsb_Free(myDevInfo.winUsbHandle);

                if (!(myDevInfo.deviceHandle == null))
                {
                    if (!(myDevInfo.deviceHandle.IsInvalid))
                    {
                        myDevInfo.deviceHandle.Close();
                    }
                }
            }
            catch (Exception ex)
            {
                throw;
            }
        }

        ///  <summary>
        ///  Initiates a Control Read transfer. Data stage is device to host.
        ///  </summary>
        /// 
        ///  <param name="dataStage"> The received data. </param>
        ///  
        ///  <returns>
        ///  True on success, False on failure.
        ///  </returns>
        internal Boolean Do_Control_Read_Transfer(ref Byte[] dataStage)
        {
            UInt32 bytesReturned = 0;
            WINUSB_SETUP_PACKET setupPacket;
            Boolean success;

            try
            {
                //  Vendor-specific request to an interface with device-to-host Data stage.

                setupPacket.RequestType = 0XC1;

                //  The request number that identifies the specific request.

                setupPacket.Request = 2;

                //  Command-specific value to send to the device.

                setupPacket.Index = 0;

                //  Number of bytes in the request's Data stage.

                setupPacket.Length = System.Convert.ToUInt16(dataStage.Length);

                //  Command-specific value to send to the device.

                setupPacket.Value = 0;

                // ***
                //  winusb function 

                //  summary
                //  Initiates a control transfer.

                //  paramaters
                //  Device handle returned by WinUsb_Initialize.
                //  WINUSB_SETUP_PACKET structure 
                //  Buffer to hold the returned Data-stage data.
                //  Number of data bytes to read in the Data stage.
                //  Number of bytes read in the Data stage.
                //  Null pointer for non-overlapped.

                //  returns
                //  True on success.
                //  ***            

                success = WinUsb_ControlTransfer(myDevInfo.winUsbHandle, setupPacket, ref dataStage[0], System.Convert.ToUInt16(dataStage.Length), ref bytesReturned, IntPtr.Zero);
                return success;

            }
            catch (Exception ex)
            {
                throw;
            }
        }

        ///  <summary>
        ///  Initiates a Control Write transfer. Data stage is host to device.
        ///  </summary>
        ///  
        ///  <param name="dataStage"> The data to send. </param>
        ///  
        ///  <returns>
        ///  True on success, False on failure.
        ///  </returns>
        internal Boolean Do_Control_Write_Transfer(Byte[] dataStage)
        {
            UInt32 bytesReturned = 0;
            ushort index = System.Convert.ToUInt16(0);
            WINUSB_SETUP_PACKET setupPacket;
            Boolean success;
            ushort value = System.Convert.ToUInt16(0);

            try
            {
                //  Vendor-specific request to an interface with host-to-device Data stage.

                setupPacket.RequestType = 0X41;

                //  The request number that identifies the specific request.

                setupPacket.Request = 1;

                //  Command-specific value to send to the device.

                setupPacket.Index = index;

                //  Number of bytes in the request's Data stage.

                setupPacket.Length = System.Convert.ToUInt16(dataStage.Length);

                //  Command-specific value to send to the device.

                setupPacket.Value = value;

                // ***
                //  winusb function 

                //  summary
                //  Initiates a control transfer.

                //  parameters
                //  Device handle returned by WinUsb_Initialize.
                //  WINUSB_SETUP_PACKET structure 
                //  Buffer containing the Data-stage data.
                //  Number of data bytes to send in the Data stage.
                //  Number of bytes sent in the Data stage.
                //  Null pointer for non-overlapped.

                //  Returns
                //  True on success.
                //  ***

                success = WinUsb_ControlTransfer
                    (myDevInfo.winUsbHandle,
                    setupPacket,
                    ref dataStage[0],
                    System.Convert.ToUInt16(dataStage.Length),
                    ref bytesReturned,
                    IntPtr.Zero);
                return success;
            }
            catch (Exception ex)
            {
                throw;
            }
        }

        ///  <summary>
        ///  Initiates a Control Write transfer. Data stage is host to device.
        ///  </summary>
        ///  
        ///  <param name="dataStage"> The data to send. </param>
        ///  
        ///  <returns>
        ///  True on success, False on failure.
        ///  </returns>
        internal Boolean Do_BulkOnly_Mass_Storage_Reset()
        {
            Byte dataStage = 0;
            UInt32 bytesReturned = 0;
            WINUSB_SETUP_PACKET setupPacket;
            Boolean success;

            try
            {
                //  Class-specific request to an interface with host-to-device Data stage.

                setupPacket.RequestType = 0x21;

                //  The request number that identifies the specific request.

                setupPacket.Request = 0xFF;

                //  Command-specific value to send to the device.

                setupPacket.Value = 0;

                //  The interface number.

                setupPacket.Index = 0;

                //  Number of bytes in the request's Data stage.

                setupPacket.Length = 0;


                // ***
                //  winusb function 

                //  summary
                //  Initiates a control transfer.

                //  parameters
                //  Device handle returned by WinUsb_Initialize.
                //  WINUSB_SETUP_PACKET structure 
                //  Buffer containing the Data-stage data.
                //  Number of data bytes to send in the Data stage.
                //  Number of bytes sent in the Data stage.
                //  Null pointer for non-overlapped.

                //  Returns
                //  True on success.
                //  ***

                success = WinUsb_ControlTransfer
                    (myDevInfo.winUsbHandle,
                    setupPacket,
                    ref dataStage,
                    System.Convert.ToUInt16(0),
                    ref bytesReturned,
                    IntPtr.Zero);
                return success;
            }
            catch (Exception ex)
            {
                throw;
            }
        }

        ///  <summary>
        ///  Initiates a Control Write transfer. Data stage is host to device.
        ///  </summary>
        ///  
        ///  <param name="dataStage"> The data to send. </param>
        ///  
        ///  <returns>
        ///  True on success, False on failure.
        ///  </returns>
        internal Boolean Do_Clear_Feature(Byte endPoint)
        {
            Byte dataStage = 0;
            UInt32 bytesReturned = 0;
            WINUSB_SETUP_PACKET setupPacket;
            Boolean success;

            try
            {
                //  Standard request to an endpoint with host-to-device direction.

                setupPacket.RequestType = 0x02;

                //  The request number that identifies the specific request.

                setupPacket.Request = CLEAR_FEATURE;

                //  Command-specific value to send to the device.

                setupPacket.Value = ENDPOINT_HALT;

                //  The endpoint number.

                setupPacket.Index = endPoint;

                //  Number of bytes in the request's Data stage.

                setupPacket.Length = 0;


                // ***
                //  winusb function 

                //  summary
                //  Initiates a control transfer.

                //  parameters
                //  Device handle returned by WinUsb_Initialize.
                //  WINUSB_SETUP_PACKET structure 
                //  Buffer containing the Data-stage data.
                //  Number of data bytes to send in the Data stage.
                //  Number of bytes sent in the Data stage.
                //  Null pointer for non-overlapped.

                //  Returns
                //  True on success.
                //  ***

                success = WinUsb_ControlTransfer
                    (myDevInfo.winUsbHandle,
                    setupPacket,
                    ref dataStage,
                    System.Convert.ToUInt16(0),
                    ref bytesReturned,
                    IntPtr.Zero);
                return success;
            }
            catch (Exception ex)
            {
                throw;
            }
        }

        ///  <summary>
        ///  Requests a handle with CreateFile.
        ///  </summary>
        ///  
        ///  <param name="devicePathName"> Returned by SetupDiGetDeviceInterfaceDetail 
        ///  in an SP_DEVICE_INTERFACE_DETAIL_DATA structure. </param>
        ///  
        ///  <returns>
        ///  The handle.
        ///  </returns>
        internal Boolean GetDeviceHandle(String devicePathName)
        {
            Win32.SECURITY_ATTRIBUTES security = new Win32.SECURITY_ATTRIBUTES();
            security.nLength = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES));
            security.bInheritHandle = true;
//            security.lpSecurityDescriptor = IntPtr.Zero;
//            security.bInheritHandle = true;
//            security.nLength = Marshal.SizeOf(security);

            // ***
            // API function

            //  summary
            //  Retrieves a handle to a device.

            //  parameters 
            //  Device path name returned by SetupDiGetDeviceInterfaceDetail
            //  Type of access requested (read/write).
            //  FILE_SHARE attributes to allow other processes to access the device while this handle is open.
            //  Security structure. Using Null for this may cause problems under Windows XP.
            //  Creation disposition value. Use OPEN_EXISTING for devices.
            //  Flags and attributes for files. The winsub driver requires FILE_FLAG_OVERLAPPED.
            //  Handle to a template file. Not used.

            //  Returns
            //  A handle or INVALID_HANDLE_VALUE.
            // ***

            myDevInfo.deviceHandle = Win32.CreateFile
                (devicePathName,
                (Win32.GENERIC_WRITE | Win32.GENERIC_READ),
                Win32.FILE_SHARE_READ | Win32.FILE_SHARE_WRITE,
                ref security,
                Win32.OPEN_EXISTING,
                Win32.FILE_ATTRIBUTE_NORMAL | Win32.FILE_FLAG_OVERLAPPED,
                IntPtr.Zero);

            if (!(myDevInfo.deviceHandle.IsInvalid))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        ///  <summary>
        ///  Initializes a device interface and obtains information about it.
        ///  Calls these winusb API functions:
        ///    WinUsb_Initialize
        ///    WinUsb_QueryInterfaceSettings
        ///    WinUsb_QueryPipe
        ///  </summary>
        ///  
        ///  <param name="deviceHandle"> A handle obtained in a call to winusb_initialize. </param>
        ///  
        ///  <returns>
        ///  True on success, False on failure.
        ///  </returns>
        internal Boolean InitializeDevice()
        {
            USB_INTERFACE_DESCRIPTOR ifaceDescriptor;
            WINUSB_PIPE_INFORMATION pipeInfo;
            UInt32 pipeTimeout = System.Convert.ToUInt32(2000);
            Boolean success;

            try
            {
                ifaceDescriptor.bLength = 0;
                ifaceDescriptor.bDescriptorType = 0;
                ifaceDescriptor.bInterfaceNumber = 0;
                ifaceDescriptor.bAlternateSetting = 0;
                ifaceDescriptor.bNumEndpoints = 0;
                ifaceDescriptor.bInterfaceClass = 0;
                ifaceDescriptor.bInterfaceSubClass = 0;
                ifaceDescriptor.bInterfaceProtocol = 0;
                ifaceDescriptor.iInterface = 0;

                pipeInfo.PipeType = 0;
                pipeInfo.PipeId = 0;
                pipeInfo.MaximumPacketSize = 0;
                pipeInfo.Interval = 0;

                // ***
                //  winusb function 

                //  summary
                //  get a handle for communications with a winusb device        '

                //  parameters
                //  Handle returned by CreateFile.
                //  Device handle to be returned.

                //  returns
                //  True on success.
                //  ***

                success = WinUsb_Initialize
                    (myDevInfo.deviceHandle,
                    ref myDevInfo.winUsbHandle);

                if (success)
                {
                    //myDevInfo.winUsbHandle = myDevInfo.winUSBHandle;                   

                    // ***
                    //  winusb function 

                    //  summary
                    //  Get a structure with information about the device interface.

                    //  parameters
                    //  handle returned by WinUsb_Initialize
                    //  alternate interface setting number
                    //  USB_INTERFACE_DESCRIPTOR structure to be returned.

                    //  returns
                    //  True on success.

                    success = WinUsb_QueryInterfaceSettings
                        (myDevInfo.winUsbHandle,
                        0,
                        ref ifaceDescriptor);

                    if (success)
                    {
                        //  Get the transfer type, endpoint number, and direction for the interface's
                        //  bulk and interrupt endpoints. Set pipe policies.

                        // ***
                        //  winusb function 

                        //  summary
                        //  returns information about a USB pipe (endpoint address)

                        //  parameters
                        //  Handle returned by WinUsb_Initialize
                        //  Alternate interface setting number
                        //  Number of an endpoint address associated with the interface. 
                        //  (The values count up from zero and are NOT the same as the endpoint address
                        //  in the endpoint descriptor.)
                        //  WINUSB_PIPE_INFORMATION structure to be returned

                        //  returns
                        //  True on success   
                        // ***

                        for (Int32 i = 0; i <= ifaceDescriptor.bNumEndpoints - 1; i++)
                        {
                            WinUsb_QueryPipe
                                (myDevInfo.winUsbHandle,
                                0,
                                System.Convert.ToByte(i),
                                ref pipeInfo);

                            if (((pipeInfo.PipeType ==
                                USBD_PIPE_TYPE.UsbdPipeTypeBulk) &
                                UsbEndpointDirectionIn(pipeInfo.PipeId)))
                            {

                                myDevInfo.bulkInPipe = pipeInfo.PipeId;

                                SetPipePolicy
                                    (System.Convert.ToByte(myDevInfo.bulkInPipe),
                                    ((UInt32)(POLICY_TYPE.IGNORE_SHORT_PACKETS)),
                                    false);

                                SetPipePolicy
                                    (System.Convert.ToByte(myDevInfo.bulkInPipe),
                                    ((UInt32)(POLICY_TYPE.PIPE_TRANSFER_TIMEOUT)),
                                    pipeTimeout);

                            }
                            else if (((pipeInfo.PipeType ==
                                USBD_PIPE_TYPE.UsbdPipeTypeBulk) &
                                UsbEndpointDirectionOut(pipeInfo.PipeId)))
                            {

                                myDevInfo.bulkOutPipe = pipeInfo.PipeId;

                                SetPipePolicy
                                    (System.Convert.ToByte(myDevInfo.bulkOutPipe),
                                    ((UInt32)(POLICY_TYPE.IGNORE_SHORT_PACKETS)),
                                    false);

                                SetPipePolicy
                                    (System.Convert.ToByte(myDevInfo.bulkOutPipe),
                                    ((UInt32)(POLICY_TYPE.PIPE_TRANSFER_TIMEOUT)),
                                    pipeTimeout);

                            }
                            else if ((pipeInfo.PipeType ==
                                USBD_PIPE_TYPE.UsbdPipeTypeInterrupt) &
                                UsbEndpointDirectionIn(pipeInfo.PipeId))
                            {

                                myDevInfo.interruptInPipe = pipeInfo.PipeId;

                                SetPipePolicy
                                    (System.Convert.ToByte(myDevInfo.interruptInPipe),
                                    ((UInt32)(POLICY_TYPE.IGNORE_SHORT_PACKETS)),
                                    false);

                                SetPipePolicy
                                    (System.Convert.ToByte(myDevInfo.interruptInPipe),
                                    ((UInt32)(POLICY_TYPE.PIPE_TRANSFER_TIMEOUT)),
                                    pipeTimeout);

                            }
                            else if ((pipeInfo.PipeType ==
                                USBD_PIPE_TYPE.UsbdPipeTypeInterrupt) &
                                UsbEndpointDirectionOut(pipeInfo.PipeId))
                            {

                                myDevInfo.interruptOutPipe = pipeInfo.PipeId;

                                SetPipePolicy
                                    (System.Convert.ToByte(myDevInfo.interruptOutPipe),
                                    ((UInt32)(POLICY_TYPE.IGNORE_SHORT_PACKETS)),
                                    false);

                                SetPipePolicy
                                    (System.Convert.ToByte(myDevInfo.interruptOutPipe),
                                    ((UInt32)(POLICY_TYPE.PIPE_TRANSFER_TIMEOUT)),
                                    pipeTimeout);
                            }
                        }
                    }
                    else
                    {
                        success = false;
                    }
                }
                return success;
            }
            catch (Exception ex)
            {
                throw;
            }
        }

        ///  <summary>
        ///  Gets a value that corresponds to a USB_DEVICE_SPEED. 
        ///  </summary>
        internal Boolean QueryDeviceSpeed()
        {
            UInt32 length = 1;
            Byte speed = 0;
            Boolean success;

            // ***
            //  winusb function 

            //  summary
            //  Get the device speed. 
            //  (Normally not required but can be nice to know.)

            //  parameters
            //  Handle returned by WinUsb_Initialize
            //  Requested information type.
            //  Number of bytes to read.
            //  Information to be returned.

            //  returns
            //  True on success.
            // ***           

            success = WinUsb_QueryDeviceInformation
                (myDevInfo.winUsbHandle,
                DEVICE_SPEED,
                ref length,
                ref speed);

            if (success)
            {
                myDevInfo.deviceSpeed = speed;
            }

            return success;
        }

        ///  <summary>
        ///  Attempts to read data from a bulk IN endpoint.
        ///  </summary>
        ///  
        ///  <param name="InterfaceHandle"> Device interface handle. </param>
        ///  <param name="PipeID"> Endpoint address. </param>
        ///  <param name="bytesToRead"> Number of bytes to read. </param>
        ///  <param name="Buffer"> Buffer for storing the bytes read. </param>
        ///  <param name="bytesRead"> Number of bytes read. </param>
        ///  <param name="success"> Success or failure status. </param>
        ///  
        internal void ReadViaBulkTransfer(Byte pipeID, UInt32 bytesToRead, ref Byte[] buffer, ref UInt32 bytesRead, ref Boolean success)
        {
            try
            {
                // ***
                //  winusb function 

                //  summary
                //  Attempts to read data from a device interface.

                //  parameters
                //  Device handle returned by WinUsb_Initialize.
                //  Endpoint address.
                //  Buffer to store the data.
                //  Maximum number of bytes to return.
                //  Number of bytes read.
                //  Null pointer for non-overlapped.

                //  Returns
                //  True on success.
                // ***

                success = WinUsb_ReadPipe
                    (myDevInfo.winUsbHandle,
                    pipeID,
                    ref buffer[0],
                    bytesToRead,
                    ref bytesRead,
                    IntPtr.Zero);

                if (!(success))
                {
                    CloseDeviceHandle();
                }
            }
            catch (Exception ex)
            {
                throw;
            }
        }

        ///  <summary>
        ///  Attempts to read data from an interrupt IN endpoint. 
        ///  </summary>
        ///  
        ///  <param name="InterfaceHandle"> Device interface handle. </param>
        ///  <param name="PipeID"> Endpoint address. </param>
        ///  <param name="bytesToRead"> Number of bytes to read. </param>
        ///  <param name="Buffer"> Buffer for storing the bytes read. </param>
        ///  <param name="bytesRead"> Number of bytes read. </param>
        ///  <param name="success"> Success or failure status. </param>
        ///  
        internal void ReadViaInterruptTransfer
            (Byte pipeID,
            UInt32 bytesToRead,
            ref Byte[] buffer,
            ref UInt32 bytesRead,
            ref Boolean success)
        {
            try
            {
                // ***
                //  winusb function 

                //  summary
                //  Attempts to read data from a device interface.

                //  parameters
                //  Device handle returned by WinUsb_Initialize.
                //  Endpoint address.
                //  Buffer to store the data.
                //  Maximum number of bytes to return.
                //  Number of bytes read.
                //  Null pointer for non-overlapped.

                //  Returns
                //  True on success.
                // ***

                success = WinUsb_ReadPipe
                    (myDevInfo.winUsbHandle,
                    pipeID,
                    ref buffer[0],
                    bytesToRead,
                    ref bytesRead,
                    IntPtr.Zero);

                if (!(success))
                {
                    CloseDeviceHandle();
                }
            }
            catch (Exception ex)
            {
                throw;
            }
        }

        ///  <summary>
        ///  Attempts to send data via a bulk OUT endpoint.
        ///  </summary>
        ///  
        ///  <param name="buffer"> Buffer containing the bytes to write. </param>
        ///  <param name="bytesToWrite"> Number of bytes to write. </param>
        ///  
        ///  <returns>
        ///  True on success, False on failure.
        ///  </returns>
        internal Int32 SendViaBulkTransfer(ref Byte[] buffer, UInt32 bytesToWrite)
        {
            UInt32 bytesWritten = 0;
            Int32 retValue = Win32.ERROR_SUCCESS;

            try
            {
                // ***
                //  winusb function 

                //  summary
                //  Attempts to write data to a device interface.

                //  parameters
                //  Device handle returned by WinUsb_Initialize.
                //  Endpoint address.
                //  Buffer with data to write.
                //  Number of bytes to write.
                //  Number of bytes written.
                //  IntPtr.Zero for non-overlapped I/O.

                //  Returns
                //  True on success.
                //  ***

                bool success = WinUsb_WritePipe
                    (myDevInfo.winUsbHandle,
                    System.Convert.ToByte(myDevInfo.bulkOutPipe),
                    ref buffer[0],
                    bytesToWrite,
                    ref bytesWritten,
                    IntPtr.Zero);

                if (!(success))
                {
                    retValue = Marshal.GetLastWin32Error();
                    String errMsg = new Win32Exception(retValue).Message;
                    CloseDeviceHandle();
                }
                return retValue;
            }
            catch (Exception ex)
            {
                throw;
            }
        }

        ///  <summary>
        ///  Attempts to send data via an interrupt OUT endpoint.
        ///  </summary>
        ///  
        ///  <param name="buffer"> Buffer containing the bytes to write. </param>
        ///  <param name="bytesToWrite"> Number of bytes to write. </param>
        ///  
        ///  <returns>
        ///  True on success, False on failure.
        ///  </returns>
        internal Boolean SendViaInterruptTransfer(ref Byte[] buffer, UInt32 bytesToWrite)
        {
            UInt32 bytesWritten = 0;
            Boolean success;

            try
            {
                // ***
                //  winusb function 

                //  summary
                //  Attempts to write data to a device interface.

                //  parameters
                //  Device handle returned by WinUsb_Initialize.
                //  Endpoint address.
                //  Buffer with data to write.
                //  Number of bytes to write.
                //  Number of bytes written.
                //  IntPtr.Zero for non-overlapped I/O.

                //  Returns
                //  True on success.
                //  ***

                success = WinUsb_WritePipe
                    (myDevInfo.winUsbHandle,
                    System.Convert.ToByte(myDevInfo.interruptOutPipe),
                    ref buffer[0],
                    bytesToWrite,
                    ref bytesWritten,
                    IntPtr.Zero);

                if (!(success))
                {
                    CloseDeviceHandle();
                }

                return success;
            }
            catch (Exception ex)
            {
                throw;
            }
        }

        ///  <summary>
        ///  Sets pipe policy.
        ///  Used when the value parameter is a Byte (all except PIPE_TRANSFER_TIMEOUT).
        ///  </summary>
        ///  
        ///  <param name="pipeId"> Pipe to set a policy for. </param>
        ///  <param name="policyType"> POLICY_TYPE member. </param>
        ///  <param name="value"> Policy value. </param>
        ///  
        ///  <returns>
        ///  True on success, False on failure.
        ///  </returns>
        ///  
        private Boolean SetPipePolicy(Byte pipeId, UInt32 policyType, Boolean value)
        {
            Byte byteValue;
            Boolean success;

            try
            {
                // ***
                //  winusb function 

                //  summary
                //  sets a pipe policy 

                //  parameters
                //  handle returned by WinUsb_Initialize
                //  identifies the pipe
                //  POLICY_TYPE member.
                //  length of value in bytes
                //  value to set for the policy.

                //  returns
                //  True on success 
                // ***

                byteValue = System.Convert.ToByte(value);
                success = WinUsb_SetPipePolicy
                    (myDevInfo.winUsbHandle,
                    pipeId,
                    policyType,
                    System.Convert.ToUInt32(1),
                    ref byteValue);

                return success;
            }
            catch (Exception ex)
            {
                throw;
            }
        }

        ///  <summary>
        ///  Sets pipe policy.
        ///  Used when the value parameter is a UInt32 (PIPE_TRANSFER_TIMEOUT only).
        ///  </summary>
        ///  
        ///  <param name="pipeId"> Pipe to set a policy for. </param>
        ///  <param name="policyType"> POLICY_TYPE member. </param>
        ///  <param name="value"> Policy value. </param>
        ///  
        ///  <returns>
        ///  True on success, False on failure.
        ///  </returns>
        ///  
        private Boolean SetPipePolicy(Byte pipeId, UInt32 policyType, UInt32 value)
        {
            Boolean success;

            try
            {
                // ***
                //  winusb function 

                //  summary
                //  sets a pipe policy 

                //  parameters
                //  handle returned by WinUsb_Initialize
                //  identifies the pipe
                //  POLICY_TYPE member.
                //  length of value in bytes
                //  value to set for the policy.

                //  returns
                //  True on success 
                // ***

                success = WinUsb_SetPipePolicy1
                    (myDevInfo.winUsbHandle,
                    pipeId,
                    policyType,
                    System.Convert.ToUInt32(4),
                    ref value);

                return success;
            }
            catch (Exception ex)
            {
                throw;
            }
        }

        ///  <summary>
        ///  Is the endpoint's direction IN (device to host)?
        ///  </summary>
        ///  
        ///  <param name="addr"> The endpoint address. </param>
        ///  <returns>
        ///  True if IN (device to host), False if OUT (host to device)
        ///  </returns> 
        private Boolean UsbEndpointDirectionIn(Int32 addr)
        {
            Boolean usbEndpointDirectionInReturn;

            try
            {
                if (((addr & 0X80) == 0X80))
                {
                    usbEndpointDirectionInReturn = true;
                }
                else
                {
                    usbEndpointDirectionInReturn = false;
                }

            }
            catch (Exception ex)
            {
                throw;
            }
            return usbEndpointDirectionInReturn;
        }

        ///  <summary>
        ///  Is the endpoint's direction OUT (host to device)?
        ///  </summary>
        ///  
        ///  <param name="addr"> The endpoint address. </param>
        ///  
        ///  <returns>
        ///  True if OUT (host to device, False if IN (device to host)
        ///  </returns>
        private Boolean UsbEndpointDirectionOut(Int32 addr)
        {
            Boolean usbEndpointDirectionOutReturn;

            try
            {
                if (((addr & 0X80) == 0))
                {
                    usbEndpointDirectionOutReturn = true;
                }
                else
                {
                    usbEndpointDirectionOutReturn = false;
                }
            }
            catch (Exception ex)
            {
                throw;
            }
            return usbEndpointDirectionOutReturn;
        }

//===================================================================================================================
//===================================================================================================================
        public Int32 DownloadFile(Byte[] data)
        {
            Int32 retValue = Win32.ERROR_SUCCESS;

            // tell the UI we are beginning a command.
            SendCommandProgressArgs cmdProgress =
                new SendCommandProgressArgs("DownloadFile", Api.Api.CommandDirection.WriteWithData, (uint)data.Length);
            DoSendProgress(cmdProgress);

            // Variables for iteration
            Byte[] buffer = new Byte[64];
            
            //
            // Write drive in chunks
            //
            UInt32 byteIndex, numBytesToWrite = 0;
            for (byteIndex = 0; byteIndex < data.Length; byteIndex += numBytesToWrite)
            {
                // Init the buffer to 0xFF
                for (int i = 0; i < buffer.Length; ++i)
                    buffer[i] = 0xFF;

                // Get some data
                numBytesToWrite = (UInt32)Math.Min(buffer.Length, data.Length - byteIndex);
                Array.Copy(data, byteIndex, buffer, 0, numBytesToWrite);

                // Write the data to the device
                retValue = SendViaBulkTransfer(ref buffer, (uint)buffer.Length);
                if (retValue != Win32.ERROR_SUCCESS)
                {
                    ErrorString = new Win32Exception(retValue).Message;

                    Debug.WriteLine(String.Format("!ERROR: WinUsbDevice.DownloadFile() - {0}({1})", ErrorString, retValue));
                    goto DownloadFileExit;
                }
                // Update the UI
                cmdProgress.Position += (Int32)numBytesToWrite;
                DoSendProgress(cmdProgress);
            }

        DownloadFileExit:

            // tell the UI we are done
            cmdProgress.Position += 100;
            cmdProgress.Error = retValue;
            cmdProgress.InProgress = false;
            DoSendProgress(cmdProgress);

            return retValue;
        }

        private CSW.CommandStatus _Status;
        private static UInt32 _CdbTag;

        override public Int32 SendCommand(Api.Api api)
        {
//            Do_BulkOnly_Mass_Storage_Reset();
//            Do_Clear_Feature(myDevInfo.bulkInPipe);
//            Do_Clear_Feature(myDevInfo.bulkOutPipe);

            UInt32 bytesRead = 0;
            Boolean success = false;

            // reset the error string
            ErrorString = String.Empty;
            this._Status = CSW.CommandStatus.Passed;

            Api.ScsiApi scsiApi = api as Api.ScsiApi;
            if (scsiApi == null)
            {
                ErrorString = String.Format(" Error: Can not send \"{0}\" api type to \"{1}\" device.", api.ImageKey, this.ToString());
                return Win32.ERROR_INVALID_PARAMETER;
            }

            Debug.WriteLine(String.Format("+WinUsbDevice.SendCommand({0}) devInst:{1}", scsiApi.ToString(), this.DeviceInstance));

            scsiApi.CbwTag = ++WinUsbDevice._CdbTag;

            // tell the UI we are beginning a command.
            SendCommandProgressArgs cmdProgress = new SendCommandProgressArgs(scsiApi.ToString(), scsiApi.Direction, Convert.ToUInt32(scsiApi.TransferSize));
            cmdProgress.Status = String.Format("SendCommand({0}).Open()", scsiApi.ToString());
            DoSendProgress(cmdProgress);
/*
            // Open the device
            Win32.SECURITY_ATTRIBUTES secAttribs = new Win32.SECURITY_ATTRIBUTES();
            secAttribs.nLength = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES)); 
            SafeFileHandle hHidDevice = Win32.CreateFile(Path, Win32.GENERIC_READ | Win32.GENERIC_WRITE,
                Win32.FILE_SHARE_READ | Win32.FILE_SHARE_WRITE, ref secAttribs, Win32.OPEN_EXISTING, Win32.FILE_FLAG_NO_BUFFERING, IntPtr.Zero); //Win32.FILE_FLAG_OVERLAPPED

            if ( hHidDevice.IsInvalid )
            {
                cmdProgress.Error = Marshal.GetLastWin32Error();
                string errorMsg = new Win32Exception(cmdProgress.Error).Message;
                ErrorString = String.Format(" ERROR: {0} ({1})", errorMsg, cmdProgress.Error);
                cmdProgress.InProgress = false;
                DoSendProgress(cmdProgress);

                Trace.WriteLine(String.Format("-HidDevice.SendCommand({0}) {1}", hidApi.ToString(), cmdProgress.Status));
                return cmdProgress.Error;
            }
            Debug.WriteLine(String.Format("->HidDevice.SendCommand  OPEN:{0}", this.Path));

            FileStream fileStream = new FileStream(hHidDevice, FileAccess.ReadWrite, 8192);

            if (!ProcessWriteCommand(fileStream, hidApi, ref cmdProgress)) //CBW
            {
    //                fileStream.Dispose();

                ErrorString = String.Format(" ERROR: {0}.", cmdProgress.Status);
                cmdProgress.InProgress = false;
                DoSendProgress(cmdProgress);

                Trace.WriteLine(String.Format("-HidDevice.SendCommand({0}) {1}", hidApi.ToString(), cmdProgress.Status));
                return cmdProgress.Error;
            }
*/
            //
            // Send the CBW
            //
            Byte[] buffer = new CBW(scsiApi).ToByteArray();
            Int32 retValue = SendViaBulkTransfer(ref buffer, (uint)buffer.Length);
            if (retValue != Win32.ERROR_SUCCESS)
            {
                ErrorString = new Win32Exception(retValue).Message;

                Debug.WriteLine(String.Format("!ERROR: WinUsbDevice.SendCommand().SendCBW - {0}({1})", ErrorString, retValue));
                goto SendCommandExit;
            }
            // Update the UI
//            cmdProgress.Position += (Int32)numBytesToWrite;
//            DoSendProgress(cmdProgress);

            // Read/Write Data
            if (scsiApi.Direction != Api.Api.CommandDirection.NoData)
            {
                if (scsiApi.Direction != DevSupport.Api.Api.CommandDirection.ReadWithData)
                {
/*                    if (!ProcessWriteData(fileStream, hidApi, ref cmdProgress))
                    {
    //                        fileStream.Dispose();

                        ErrorString = String.Format(" ERROR: {0}.", cmdProgress.Status);
                        cmdProgress.InProgress = false;
                        DoSendProgress(cmdProgress);

                        Trace.WriteLine(String.Format("-HidDevice.SendCommand({0}) {1}", hidApi.ToString(), cmdProgress.Status));
                        return cmdProgress.Error;
                    }
*/                    
                    bytesRead = 0;
                    success = false;
                    buffer = new Byte[scsiApi.TransferSize];
                    ReadViaBulkTransfer(myDevInfo.bulkInPipe, (UInt32)buffer.Length, ref buffer, ref bytesRead, ref success);
                    if (!success)
                    {
                        Debug.WriteLine(String.Format("!ERROR: WinUsbDevice.SendCommand().ReadData - read {0} of {1} bytes.", bytesRead, buffer.Length));
                        goto SendCommandExit;
                    }

                    scsiApi.ProcessResponse(buffer, 0, bytesRead);
                }
/*                else
                {
                    if (!ProcessReadData(fileStream, ref hidApi, ref cmdProgress))
                    {
    //                        fileStream.Dispose();

                        ErrorString = String.Format(" ERROR: {0}.", cmdProgress.Status);
                        cmdProgress.InProgress = false;
                        DoSendProgress(cmdProgress);

                        Trace.WriteLine(String.Format("-HidDevice.SendCommand({0}) {1}", hidApi.ToString(), cmdProgress.Status));
                        return cmdProgress.Error;
                    }
                }
*/            }

            //
            // Get CSW
            //
            bytesRead = 0;
            success = false;
            buffer = new Byte[Marshal.SizeOf(typeof(CSW))];
            ReadViaBulkTransfer(myDevInfo.bulkInPipe, (UInt32)buffer.Length, ref buffer, ref bytesRead, ref success);
            if (!success)
            {
                Debug.WriteLine(String.Format("!ERROR: WinUsbDevice.SendCommand().GetStatus - read {0} of {1} bytes.", bytesRead, buffer.Length));
                goto SendCommandExit;
            }

            CSW csw = new CSW(buffer);
            _Status = csw.Status;

        SendCommandExit:

            Debug.WriteLine(String.Format("-WinUsbDevice.SendCommand({0}) devInst:{1}", scsiApi.ToString(), this.DeviceInstance));
            cmdProgress.InProgress = false;
            cmdProgress.Error = (Int32)this._Status;
            DoSendProgress(cmdProgress);

            return Win32.ERROR_SUCCESS;
        }



        [Browsable(false)]
        public override String ErrorString
        {
            get 
            {
                String errStr = String.IsNullOrEmpty(base.ErrorString) ? String.Empty : base.ErrorString + "\r\n";

                switch ( _Status )
                {
                    case CSW.CommandStatus.Passed:
    //                        errStr += String.Format(" HID Status: PASSED(0x{0:X2})\r\n", (Byte)_Status);
	                    break;
                    case CSW.CommandStatus.Failed:
                        errStr += String.Format(" HID Status: FAILED(0x{0:X2})\r\n", (Byte)_Status);
	                    break;
                    case CSW.CommandStatus.PhaseError:
                        errStr += String.Format(" HID Status: PHASE_ERROR(0x{0:X2})\r\n", (Byte)_Status);
	                    break;
                    default:
                        errStr += String.Format(" HID Status: UNKNOWN(0x{0:X2})\r\n", (Byte)_Status);
                        break;
                }
                return errStr;
            }
        }
//===================================================================================================================
    }
}
/*
RecoveryDevice::RecoveryDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
: Device(deviceClass, devInst, path)
{
    m_ErrorStatus=ERROR_SUCCESS;
}

RecoveryDevice::~RecoveryDevice(void)
{
}

BOOL RecoveryDevice::Open()
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

BOOL RecoveryDevice::Close()
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

void CALLBACK RecoveryDevice::IoCompletion(DWORD dwErrorCode,           // completion code
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


uint32_t RecoveryDevice::Download(const StFwComponent& fwComponent, Device::UI_Callback callbackFn)
{
	int32_t error = ERROR_SUCCESS;

    // Open the device
	if(!Open())
    {
        Close();
        ATLTRACE2(_T("  ERROR:(%d)\r\n"), m_ErrorStatus);
        return m_ErrorStatus;
    }

	// For Notifying the UI
	NotifyStruct nsInfo(_T("RecoveryDevice::Download()"));
    nsInfo.direction = Device::NotifyStruct::dataDir_ToDevice;

	uint8_t buffer[RecoveryDevice::PipeSize];
	size_t dataSize = fwComponent.size();
	uint32_t byteIndex, numBytesToWrite = 0;
	for ( byteIndex = 0; byteIndex < dataSize; byteIndex += numBytesToWrite )
	{
		// Init the buffer to 0xFF
		memset(buffer, 0xff, sizeof(buffer));

		// Get some data
		numBytesToWrite = min(RecoveryDevice::PipeSize, dataSize - byteIndex);
		memcpy_s(buffer, sizeof(buffer), &fwComponent.GetData()[byteIndex], numBytesToWrite);

        // Write the data to the device
		error = Write(buffer, numBytesToWrite);
		if( error != ERROR_SUCCESS )
			break;

		// Update the UI
		nsInfo.position += numBytesToWrite;
		callbackFn(nsInfo);
	}
   
	return error;
}

uint32_t RecoveryDevice::Write(uint8_t* pBuf, size_t Size)
{
	DWORD status;

	// Preparation
	m_FileOverlapped.Offset		= 0;
    m_FileOverlapped.OffsetHigh	= 0;
	m_FileOverlapped.hEvent		= (PVOID)(ULONGLONG)Size;

	ResetEvent(m_SyncEvent);

	// Write to the device
	if( !WriteFileEx( m_RecoveryHandle, pBuf, (uint32_t)Size, &m_FileOverlapped, 
		RecoveryDevice::IoCompletion ) )
	{
        m_ErrorStatus=GetLastError();
		return m_ErrorStatus;
	}

	// wait for completion
	if( (status = WaitForSingleObjectEx( m_SyncEvent, RecoveryDevice::DeviceTimeout, TRUE )) == WAIT_TIMEOUT )
	{
		CancelIo( m_RecoveryHandle );
        m_ErrorStatus=ERROR_SEM_TIMEOUT;
		return m_ErrorStatus;
	}

	if( m_FileOverlapped.Offset == 0 )
        m_ErrorStatus=ERROR_WRITE_FAULT;
    else
        m_ErrorStatus=ERROR_SUCCESS;
    
    return m_ErrorStatus;
}


// not used at this time 
CStdString RecoveryDevice::GetErrorStr()
{
	CStdString msg;

	return msg;
}
*/
