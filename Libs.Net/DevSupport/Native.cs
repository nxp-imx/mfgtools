// UsbEject version 1.0 March 2006
// written by Simon Mourier <email: simon [underscore] mourier [at] hotmail [dot] com>

using System;
//using System.Collections;
using System.ComponentModel;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Threading;
using System.Text;

using Microsoft.Win32.SafeHandles;

namespace DevSupport
{
    public sealed class Win32
    {
        // from shobjidl.h

        #region Interface IQueryCancelAutoPlay
        [ComImport,
        Guid("DDEFE873-6997-4e68-BE26-39B633ADBE12"),
        InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IQueryCancelAutoPlay
        {
            [PreserveSig]
            int AllowAutoPlay(
                String pszPath,          // [string][in] LPCWSTR
                UInt32 dwContentType,    // [in] DWORD
                String pszLabel,         // [string][in] LPCWSTR
                UInt32 dwSerialNumber);  // [in] DWORD
        }
        #endregion

        #region Interface IHWEventHandler
        [ComImport(),
        Guid("C1FB73D0-EC3A-4BA2-B512-8CDB9187B6D1"),
        InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IHWEventHandler
        {
            [PreserveSig()]
            int Initialize(
                String pszParams);      // [string][in] LPCWSTR

            [PreserveSig()]
            int HandleEvent(
                [In(), MarshalAs(UnmanagedType.LPWStr)]
                String pszDeviceID,        // [string][in] LPCWSTR
                [In(), MarshalAs(UnmanagedType.LPWStr)]
                String pszAltDeviceID,     // [string][in] LPCWSTR
                [In(), MarshalAs(UnmanagedType.LPWStr)]
                String pszEventType);      // [string][in] LPCWSTR

            [PreserveSig()]
            int HandleEventWithContent(
                String pszDeviceID,             // [string][in] LPCWSTR
                String pszAltDeviceID,          // [string][in] LPCWSTR
                String pszEventType,            // [string][in] LPCWSTR
                String pszContentTypeHandler,   // [string][in] LPCWSTR
                IntPtr pdataobject);            // [in] IDataObject
        }
        #endregion

        // from windef.h
        internal const int MAX_PATH = 260;

        // from dbt.h
        [StructLayout(LayoutKind.Sequential)]
        internal struct DEV_BROADCAST_HDR
        {
            internal uint dbch_size;
            internal uint dbch_devicetype;
            internal uint dbch_reserved;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
        internal class DEV_BROADCAST_DEVICEINTERFACE
        {
            internal uint dbcc_size = (uint)Marshal.SizeOf(typeof(DEV_BROADCAST_DEVICEINTERFACE));
            internal uint dbcc_devicetype;
            internal uint dbcc_reserved;
            internal Guid dbcc_classguid;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
            internal String dbcc_name = "";
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
        internal class DEV_BROADCAST_DEVICEINTERFACE_NONAME
        {
            internal uint dbcc_size = (uint)Marshal.SizeOf(typeof(DEV_BROADCAST_DEVICEINTERFACE_NONAME));
            internal uint dbcc_devicetype;
            internal uint dbcc_reserved;
            internal Guid dbcc_classguid;
            internal short dbcc_name;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct DEV_BROADCAST_VOLUME
        { 
            internal uint dbcv_size;
            internal uint dbcv_devicetype;
            internal uint dbcv_reserved;
            internal uint dbcv_unitmask;
            internal uint dbcv_flags;
        }

        internal const int DBT_DEVICEARRIVAL          = 0x8000;      // system detected a new device
        internal const int DBT_DEVICEREMOVECOMPLETE   = 0x8004;      // device is gone
        internal const int DBT_DEVTYP_DEVICEINTERFACE = 0x00000005;  // device interface class
        internal const int DBT_DEVTYP_VOLUME          = 0x00000002;  // logical volume


        // from hidpi.h
        internal const int HIDP_STATUS_SUCCESS = 0x00110000;
        internal const int HIDP_STATUS_INVALID_PREPARSED_DATA = unchecked((int)0xC0110001);
        
        [StructLayout(LayoutKind.Sequential)]
        public struct HIDP_CAPS
        {
            public UInt16 _Usage;
            public UInt16 _UsagePage;
            public UInt16 _InputReportByteLength;
            public UInt16 _OutputReportByteLength;
            public UInt16 _FeatureReportByteLength;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 17)]
            public UInt16[] _Reserved;

            public UInt16 _NumberLinkCollectionNodes;

            public UInt16 _NumberInputButtonCaps;
            public UInt16 _NumberInputValueCaps;
            public UInt16 _NumberInputDataIndices;

            public UInt16 _NumberOutputButtonCaps;
            public UInt16 _NumberOutputValueCaps;
            public UInt16 _NumberOutputDataIndices;

            public UInt16 _NumberFeatureButtonCaps;
            public UInt16 _NumberFeatureValueCaps;
            public UInt16 _NumberFeatureDataIndices;

            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 Usage { get { return _Usage; } }
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 UsagePage { get { return _UsagePage; } }
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 InputReportByteLength { get { return _InputReportByteLength; } }
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 OutputReportByteLength { get { return _OutputReportByteLength; } }
            public UInt16[] Reserved { get { return _Reserved; } }
            public UInt16 NumberLinkCollectionNodes { get { return _NumberLinkCollectionNodes; } }
            public UInt16 NumberInputButtonCaps { get { return _NumberInputButtonCaps; } }
            public UInt16 NumberInputValueCaps { get { return _NumberInputValueCaps; } }
            public UInt16 NumberInputDataIndices { get { return _NumberInputDataIndices; } }
            public UInt16 NumberOutputButtonCaps { get { return _NumberOutputButtonCaps; } }
            public UInt16 NumberOutputValueCaps { get { return _NumberOutputValueCaps; } }
            public UInt16 NumberOutputDataIndices { get { return _NumberOutputDataIndices; } }
            public UInt16 NumberFeatureButtonCaps { get { return _NumberFeatureButtonCaps; } }
            public UInt16 NumberFeatureValueCaps { get { return _NumberFeatureValueCaps; } }
            public UInt16 NumberFeatureDataIndices { get { return _NumberFeatureDataIndices; } }
        }

        /// <summary>
        /// Returns a list of capabilities of a given hid device as described by its preparsed data.
        /// </summary>
        /// <param name="PreparsedData">[in] The preparsed data returned from HIDCLASS.</param>
        /// <param name="Capabilities">[out] a HIDP_CAPS structure.</param>
        /// <returns>HIDP_STATUS_SUCCESS,  HIDP_STATUS_INVALID_PREPARSED_DATA</returns>
        [DllImport("hid.dll", CharSet = CharSet.Auto, SetLastError = false)]
        internal static extern Int32 HidP_GetCaps(IntPtr PreparsedData, ref HIDP_CAPS Capabilities);

        // from hidsdi.h
        /// <summary>
        /// Given a handle to a valid Hid Class Device Object, retrieve the preparsed data for the device.
        /// This routine will allocate the appropriately sized buffer to hold this preparsed data. It is up 
        /// to client to call HidP_FreePreparsedData to free the memory allocated to this structure when it 
        /// is no longer needed.
        /// </summary>
        /// <param name="HidDeviceObject">[in] A handle to a Hid Device that the client obtains using a call
        /// to CreateFile on a valid Hid device string name. The string name can be obtained using standard PnP calls.</param>
        /// <param name="PreparsedData">[out] An opaque data structure used by other functions in this library 
        /// to retrieve information about a given device.</param>
        /// <returns>TRUE if successful. FALSE otherwise  -- Use GetLastError() to get extended error information.</returns>
        [DllImport("hid.dll", CharSet = CharSet.Auto, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool HidD_GetPreparsedData(SafeFileHandle HidDeviceObject, out IntPtr PreparsedData);

        [DllImport("hid.dll", CharSet = CharSet.Auto, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool HidD_FreePreparsedData(IntPtr PreparsedData); 

        // from commctrl.h
        internal const int TVM_SETIMAGELIST  = (0x1100 + 9);      // TreeView
        internal const int HDM_SETIMAGELIST  = (0x1200 + 8);      // Header
        internal const int TB_SETIMAGELIST   = (WM_USER + 48);    // Toolbar
        internal const int LVM_SETIMAGELIST  = (0x1000 + 3);      // ListView
        internal const int CBEM_SETIMAGELIST = (WM_USER + 2);     // ComboBox
        internal const int TCM_SETIMAGELIST  = (0x1300 + 3);      // Tab control
        internal const int BCM_SETIMAGELIST  = (0x1600 + 0x0002); // Button control ( requires pButtonImageList instead of himl)

        internal const int ILD_NORMAL = 0;

        [DllImport("comctl32.dll", SetLastError = false)]
        internal static extern int ImageList_GetImageCount(IntPtr himl);

        [DllImport("comctl32.dll", SetLastError = false)]
        internal static extern IntPtr ImageList_GetIcon(IntPtr himl, int i, uint flags);

        // from winuser.h
        internal const int WM_USER                     = 0x0400;
        public const int WM_DEVICECHANGE             = 0x0219;
        internal const int DEVICE_NOTIFY_WINDOW_HANDLE = 0x00000000;
        public const Int32 WM_SYSCOMMAND = 0x112;
        public const Int32 MF_SEPARATOR = 0x800;
        public const Int32 MF_STRING = 0x0;
        public const Int32 IDM_ABOUT = 1000;
        public const uint WM_VSCROLL = 0x0115;
        public const uint SB_BOTTOM = 7;

        /// <summary>
        /// The SendMessage function sends the specified message to a window or windows. It calls the window procedure for the specified window and does not return until the window procedure has processed the message. 
        /// </summary>
        /// <param name="hWnd">[in] Handle to the window whose window procedure will receive the message. If this parameter is HWND_BROADCAST, the message is sent to all top-level windows in the system, including disabled or invisible unowned windows, overlapped windows, and pop-up windows; but the message is not sent to child windows.</param>
        /// <param name="msg">[in] Specifies the message to be sent.</param>
        /// <param name="wParam">[in] Specifies additional message-specific information.</param>
        /// <param name="lParam">[in] Specifies additional message-specific information.</param>
        /// <returns>The return value specifies the result of the message processing; it depends on the message sent.</returns>
        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = false)]
        public static extern IntPtr SendMessage(HandleRef hWnd, uint msg, IntPtr wParam, IntPtr lParam);

        [DllImport("user32.dll")]
        public static extern IntPtr GetSystemMenu(IntPtr hWnd, bool bRevert);

        [DllImport("user32.dll")]
        public static extern bool AppendMenu(IntPtr hMenu, Int32 wFlags, Int32
        wIDNewItem, string lpNewItem);

        /// <summary>
        /// Registers the device or type of device for which a window will receive notifications.
        /// </summary>
        [DllImport("user32.dll", SetLastError = true)]
        internal static extern IntPtr RegisterDeviceNotification(IntPtr hRecipient,
           IntPtr NotificationFilter, uint Flags);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool UnregisterDeviceNotification(IntPtr Handle);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool DestroyIcon(IntPtr hIcon);

        [DllImport("user32.dll")]
        internal static extern int MsgWaitForMultipleObjectsEx(uint nCount, IntPtr[] pHandles,
           uint dwMilliseconds, uint dwWakeMask, uint dwFlags);

        // from winbase.h
        internal const int INVALID_HANDLE_VALUE = -1;
        internal const int GENERIC_READ = unchecked((int)0x80000000);
        internal const int GENERIC_WRITE = unchecked((int)0x40000000);
        internal const int FILE_SHARE_READ = 0x00000001;
        internal const int FILE_SHARE_WRITE = 0x00000002;
        internal const int FILE_FLAG_OVERLAPPED = 0x40000000;
        internal const int FILE_FLAG_NO_BUFFERING = 0x20000000;
        internal const Int16 FILE_ATTRIBUTE_NORMAL = ((Int16)(0X80));

        internal const int OPEN_EXISTING = 3;
        public const int FSCTL_LOCK_VOLUME = 0x00090018;
        public const int FSCTL_UNLOCK_VOLUME = 0x0009001c;
        public const int FSCTL_DISMOUNT_VOLUME = 0x00090020;

        [StructLayout(LayoutKind.Sequential)]
        internal struct SECURITY_ATTRIBUTES
        {
            internal Int32 nLength;// = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES));
            internal IntPtr lpSecurityDescriptor;
            [MarshalAs(UnmanagedType.Bool)]
            internal bool bInheritHandle;
        }
        internal const uint SECURITY_IMPERSONATION = ((uint)SECURITY_IMPERSONATION_LEVEL.SecurityImpersonation) << 16;

        // fom winnt.h
        internal const int KEY_QUERY_VALUE = 0x0001;
        internal enum SECURITY_IMPERSONATION_LEVEL
        {
            SecurityAnonymous,
            SecurityIdentification,
            SecurityImpersonation,
            SecurityDelegation
        }

        [DllImport("kernel32", CharSet = CharSet.Auto, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool GetVolumeNameForVolumeMountPoint(
            string volumeName,
            StringBuilder uniqueVolumeName,
            int uniqueNameBufferCapacity);

        [DllImport("Kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        internal static extern SafeFileHandle CreateFile(
            string lpFileName,
            int dwDesiredAccess,
            int dwShareMode,
//            [MarshalAs(UnmanagedType.LPStruct)]
            ref SECURITY_ATTRIBUTES lpSecurityAttributes,
            int dwCreationDisposition,
            int dwFlagsAndAttributes,
            IntPtr hTemplateFile);

        [DllImport("Kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool WriteFile(SafeFileHandle hDevice, IntPtr lpBuffer, int nNumberOfBytesToWrite, out int lpNumberOfBytesWritten, IntPtr lpOverlapped);

        [DllImport("Kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool ReadFile(SafeFileHandle hDevice, IntPtr lpInBuffer, int nNumberOfBytesToRead, out int lpNumberOfBytesRead, IntPtr lpOverlapped);

        [DllImport("Kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool DeviceIoControl(SafeFileHandle hDevice, int dwIoControlCode, IntPtr lpInBuffer, int nInBufferSize, IntPtr lpOutBuffer, int nOutBufferSize, out int lpBytesReturned, ref NativeOverlapped lpOverlapped);

        [DllImport("Kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool DeviceIoControl(SafeFileHandle hDevice, int dwIoControlCode, IntPtr lpInBuffer, int nInBufferSize, IntPtr lpOutBuffer, int nOutBufferSize, out int lpBytesReturned, IntPtr lpOverlapped);

//        [DllImport("Kernel32.dll", SetLastError = true)]
//        [return: MarshalAs(UnmanagedType.Bool)]
//        internal static extern bool CloseHandle(SafeFileHandle hObject);

        // from winerror.h
        public const int ERROR_SUCCESS               = 0;
        internal const int ERROR_FILE_NOT_FOUND      = 2;
        internal const int ERROR_NOT_ENOUGH_MEMORY   = 8;
        internal const int ERROR_INVALID_DATA        = 13;
        internal const int ERROR_BAD_LENGTH          = 24;
        public const int ERROR_DISK_FULL           = 112;
        public const int ERROR_GEN_FAILURE           = 31;
        internal const int ERROR_INVALID_PARAMETER   = 87;
        internal const int ERROR_OPEN_FAILED         = 110;
        internal const int ERROR_INSUFFICIENT_BUFFER = 122;
        internal const int ERROR_LOCK_FAILED = 167;
        internal const int ERROR_NO_MORE_ITEMS       = 259;
        internal const int S_OK                     = 0x00000000;
        internal const int S_FALSE                  = 0x00000001;
        internal const int E_NOTIMPL                = unchecked((int)0x80000001);
        internal const int E_FAIL                   = unchecked((int)0x80004005);

        // from wtypes.h
        internal const int ROTFLAGS_REGISTRATIONKEEPSALIVE  = 0x1;
        internal const int ROTFLAGS_ALLOWANYCLIENT          = 0x2;
        internal const int CLSCTX_LOCAL_SERVER              = 0x04;
        internal const int REGCLS_MULTIPLEUSE               = 1;

        // from winioctl.h
        internal static readonly Guid GUID_DEVINTERFACE_VOLUME = new Guid("53f5630d-b6bf-11d0-94f2-00a0c91efb8b");
        internal static readonly Guid GUID_DEVINTERFACE_DISK   = new Guid("53f56307-b6bf-11d0-94f2-00a0c91efb8b");
        internal const int IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS = 0x00560000;
        internal const int FILE_DEVICE_CONTROLLER = 0x00000004;
        public const int IOCTL_DISK_UPDATE_PROPERTIES = 0x00070140;

        internal enum MethodType
        {
            Buffered = 0,
            InDirect,
            OutDirect,
            Neither
        }
        internal enum FileAccessType
        {
            Any = 0,
            Special = 0,
            Read,
            Write
        }

        // from devguid.h
        // DEFINE_GUID( GUID_DEVCLASS_VOLUME,      0x71a27cddL, 0x812a, 0x11d0, 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f );
        // DEFINE_GUID( GUID_DEVCLASS_DISKDRIVE,   0x4d36e967L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 );
        // DEFINE_GUID( GUID_DEVCLASS_COMPUTER,    0x4d36e966L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 );
        // DEFINE_GUID( GUID_DEVCLASS_WPD,         0xeec5ad98L, 0x8080, 0x425f, 0x92, 0x2a, 0xda, 0xbf, 0x3d, 0xe3, 0xf6, 0x9a );
        // DEFINE_GUID( GUID_DEVCLASS_HIDCLASS,    0x745a17a0L, 0x74d3, 0x11d0, 0xb6, 0xfe, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda );
        // DEFINE_GUID( GUID_DEVCLASS_USB,         0x36fc9e60L, 0xc465, 0x11cf, 0x80, 0x56, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
        // DEFINE_GUID( GUID_DEVCLASS_SCSIADAPTER, 0x4d36e97bL, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 );
        internal static readonly Guid GUID_DEVCLASS_VOLUME      = new Guid("71a27cdd-812a-11d0-bec7-08002be2092f");
        internal static readonly Guid GUID_DEVCLASS_DISKDRIVE   = new Guid("4d36e967-e325-11ce-bfc1-08002be10318");
        internal static readonly Guid GUID_DEVCLASS_COMPUTER    = new Guid("4d36e966-e325-11ce-bfc1-08002be10318");
        internal static readonly Guid GUID_DEVCLASS_WPD         = new Guid("eec5ad98-8080-425f-922a-dabf3de3f69a");
        internal static readonly Guid GUID_DEVCLASS_HIDCLASS    = new Guid("745a17a0-74d3-11d0-b6fe-00a0c90f57da");
        internal static readonly Guid GUID_DEVCLASS_USB         = new Guid("36fc9e60-c465-11cf-8056-444553540000");
        internal static readonly Guid GUID_DEVCLASS_SCSIADAPTER = new Guid("4d36e97b-e325-11ce-bfc1-08002be10318");

        // from usbiodef.h
        internal static readonly Guid GUID_DEVINTERFACE_USB_HUB             = new Guid("f18a0e88-c30c-11d0-8815-00a0c906bed8");
        internal static readonly Guid GUID_DEVINTERFACE_USB_DEVICE          = new Guid("a5dcbf10-6530-11d2-901f-00c04fb951ed");
        internal static readonly Guid GUID_DEVINTERFACE_USB_HOST_CONTROLLER = new Guid("3ABF6F2D-71C4-462a-8A92-1E6861E6AF27");

        // from hidclass.h
        //DEFINE_GUID( GUID_DEVINTERFACE_HID, 0x4D1E55B2L, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30);
        internal static readonly Guid GUID_DEVINTERFACE_HID = new Guid("4D1E55B2-F16F-11CF-88CB-001111000030");

        // Device Class GUID for the MTP Devices
        // {EEC5AD98-8080-425F-922A-DABF3DE3F69A}
        internal static readonly Guid GUID_DEVCLASS_MTP = new Guid("EEC5AD98-8080-425F-922A-DABF3DE3F69A");

        // Device Interface GUID for the USB Bulk Recovery Driver
        // Used by StMp3Rec.sys and StUpdaterApp.exe
        // {A441A6E1-EC62-46FB-9989-2CD78F1AAA34}
        // DEFINE_GUID(GUID_DEVINTERFACE_STMP3XXX_USB_BULK_DEVICE, 0xa441a6e1, 0xec62, 0x46fb, 0x99, 0x89, 0x2c, 0xd7, 0x8f, 0x1a, 0xaa, 0x34);
        internal static readonly Guid GUID_DEVINTERFACE_STMP3XXX_USB_BULK_DEVICE = new Guid("a441a6e1-ec62-46fb-9989-2cd78f1aaa34");

        // Device Class GUID for the USB Bulk Recovery Driver
        // Used in StMp3Rec.inf
        // {9FFF066D-3ED3-4567-9123-8B82CFE1CDD4}
        // DEFINE_GUID(GUID_DEVCLASS_STMP3XXX_USB_BULK_DEVICE, 0x9FFF066D, 0x3ED3, 0x4567, 0x91, 0x23, 0x8B, 0x82, 0xCF, 0xE1, 0xCD, 0xD4);
        internal static readonly Guid GUID_DEVCLASS_STMP3XXX_USB_BULK_DEVICE = new Guid("9FFF066D-3ED3-4567-9123-8B82CFE1CDD4");

        // Device Interface GUID for the WinUSB Bulk Driver
        // Used by WinUsb.sys
        // {5E53D0B8-DD42-4691-B542-57EF0DE22D6F}
        internal static readonly Guid GUID_DEVINTERFACE_WINUSB_BULK_DEVICE = new Guid("5e53d0b8-dd42-4691-b542-57ef0de22d6f");

        // Device Interface GUID for the MX ROM WDF USB Bulk Recovery Driver
        // Used by imxusb.inf, imxusb.sys
        // {00873FDF-61A8-11D1-AA5E-00C04FB1728B}
        // DEFINE_GUID(GUID_DEVINTERFACE_MX_ROM_WDF_USB_BULK_DEVICE, 0x00873FDF, 0x61A8, 0x11D1, 0xAA, 0x5E, 0x00, 0xC0, 0x4F, 0xB1, 0x72, 0x8B);
        internal static readonly Guid GUID_DEVINTERFACE_MX_ROM_WDF_USB_BULK_DEVICE = new Guid("00873FDF-61A8-11D1-AA5E-00C04FB1728B");

        [StructLayout(LayoutKind.Sequential)]
        internal struct DISK_EXTENT
        {
            internal int DiskNumber;
            internal long StartingOffset;
            internal long ExtentLength;
        }

        // from cfg.h
        internal enum PNP_VETO_TYPE
        {
            Ok,

            TypeUnknown,
            LegacyDevice,
            PendingClose,
            WindowsApp,
            WindowsService,
            OutstandingOpen,
            Device,
            Driver,
            IllegalDeviceRequest,
            InsufficientPower,
            NonDisableable,
            LegacyDriver,
        }

        // from cfgmgr32.h
        internal const int RegDisposition_OpenExisting = 0x00000001;   // open key only if exists
        internal const int CM_REGISTRY_HARDWARE = 0x00000000;

        /// <summary>
        /// Contains constants for determining devices capabilities.
        /// This enumeration has a FlagsAttribute attribute that allows a bitwise combination of its member values.
        /// </summary>
        [Flags]
        public enum DeviceCapabilities
        {
            Unknown = 0x00000000,
            // matches cfmgr32.h CM_DEVCAP_* definitions

            LockSupported = 0x00000001,
            EjectSupported = 0x00000002,
            Removable = 0x00000004,
            DockDevice = 0x00000008,
            UniqueId = 0x00000010,
            SilentInstall = 0x00000020,
            RawDeviceOk = 0x00000040,
            SurpriseRemovalOk = 0x00000080,
            HardwareDisabled = 0x00000100,
            NonDynamic = 0x00000200,
        }

        public enum CONFIGRET
        {
            CR_SUCCESS                  = 0x00000000,
            CR_DEFAULT                  = 0x00000001,
            CR_OUT_OF_MEMORY            = 0x00000002,
            CR_INVALID_POINTER          = 0x00000003,
            CR_INVALID_FLAG             = 0x00000004,
            CR_INVALID_DEVNODE          = 0x00000005,
            CR_INVALID_DEVINST          = CR_INVALID_DEVNODE,
            CR_INVALID_RES_DES          = 0x00000006,
            CR_INVALID_LOG_CONF         = 0x00000007,
            CR_INVALID_ARBITRATOR       = 0x00000008,
            CR_INVALID_NODELIST         = 0x00000009,
            CR_DEVNODE_HAS_REQS         = 0x0000000A,
            CR_DEVINST_HAS_REQS         = CR_DEVNODE_HAS_REQS,
            CR_INVALID_RESOURCEID       = 0x0000000B,
            CR_DLVXD_NOT_FOUND          = 0x0000000C,   // WIN 95 ONLY
            CR_NO_SUCH_DEVNODE          = 0x0000000D,
            CR_NO_SUCH_DEVINST          = CR_NO_SUCH_DEVNODE,
            CR_NO_MORE_LOG_CONF         = 0x0000000E,
            CR_NO_MORE_RES_DES          = 0x0000000F,
            CR_ALREADY_SUCH_DEVNODE     = 0x00000010,
            CR_ALREADY_SUCH_DEVINST     = CR_ALREADY_SUCH_DEVNODE,
            CR_INVALID_RANGE_LIST       = 0x00000011,
            CR_INVALID_RANGE            = 0x00000012,
            CR_FAILURE                  = 0x00000013,
            CR_NO_SUCH_LOGICAL_DEV      = 0x00000014,
            CR_CREATE_BLOCKED           = 0x00000015,
            CR_NOT_SYSTEM_VM            = 0x00000016,   // WIN 95 ONLY
            CR_REMOVE_VETOED            = 0x00000017,
            CR_APM_VETOED               = 0x00000018,
            CR_INVALID_LOAD_TYPE        = 0x00000019,
            CR_BUFFER_SMALL             = 0x0000001A,
            CR_NO_ARBITRATOR            = 0x0000001B,
            CR_NO_REGISTRY_HANDLE       = 0x0000001C,
            CR_REGISTRY_ERROR           = 0x0000001D,
            CR_INVALID_DEVICE_ID        = 0x0000001E,
            CR_INVALID_DATA             = 0x0000001F,
            CR_INVALID_API              = 0x00000020,
            CR_DEVLOADER_NOT_READY      = 0x00000021,
            CR_NEED_RESTART             = 0x00000022,
            CR_NO_MORE_HW_PROFILES      = 0x00000023,
            CR_DEVICE_NOT_THERE         = 0x00000024,
            CR_NO_SUCH_VALUE            = 0x00000025,
            CR_WRONG_TYPE               = 0x00000026,
            CR_INVALID_PRIORITY         = 0x00000027,
            CR_NOT_DISABLEABLE          = 0x00000028,
            CR_FREE_RESOURCES           = 0x00000029,
            CR_QUERY_VETOED             = 0x0000002A,
            CR_CANT_SHARE_IRQ           = 0x0000002B,
            CR_NO_DEPENDENT             = 0x0000002C,
            CR_SAME_RESOURCES           = 0x0000002D,
            CR_NO_SUCH_REGISTRY_KEY     = 0x0000002E,
            CR_INVALID_MACHINENAME      = 0x0000002F,   // NT ONLY
            CR_REMOTE_COMM_FAILURE      = 0x00000030,   // NT ONLY
            CR_MACHINE_UNAVAILABLE      = 0x00000031,   // NT ONLY
            CR_NO_CM_SERVICES           = 0x00000032,   // NT ONLY
            CR_ACCESS_DENIED            = 0x00000033,   // NT ONLY
            CR_CALL_NOT_IMPLEMENTED     = 0x00000034,
            CR_INVALID_PROPERTY         = 0x00000035,
            CR_DEVICE_INTERFACE_ACTIVE  = 0x00000036,
            CR_NO_SUCH_DEVICE_INTERFACE = 0x00000037,
            CR_INVALID_REFERENCE_STRING = 0x00000038,
            CR_INVALID_CONFLICT_LIST    = 0x00000039,
            CR_INVALID_INDEX            = 0x0000003A,
            CR_INVALID_STRUCTURE_SIZE   = 0x0000003B
        }

        [DllImport("cfgmgr32.dll")]
        internal static extern CONFIGRET CM_Open_DevNode_Key(
            IntPtr dnDevNode,
            int samDesired,
            int ulHardwareProfile,
            int Disposition,
            out IntPtr phkDevice,
            int ulFlags);

        internal const int CM_DRP_DRIVER = 0x0000000A; // Driver REG_SZ property (RW)

        [DllImport("cfgmgr32.dll")]
        internal static extern CONFIGRET CM_Get_DevNode_Registry_Property(
            IntPtr dnDevInst,
            int ulProperty,
            IntPtr pulRegDataType,
            StringBuilder Buffer,
            ref int pulLength,
            int ulFlags);

        [DllImport("setupapi.dll")]
        internal static extern CONFIGRET CM_Get_DevNode_Status(
            out int pulStatus,
            out int pulProblemNumber,
            IntPtr dnDevInst,
            int ulFlags);

        [DllImport("setupapi.dll")]
        internal static extern CONFIGRET CM_Get_Parent(
            out IntPtr pdnDevInst,
            IntPtr dnDevInst,
            int ulFlags);

        [DllImport("setupapi.dll")]
        internal static extern CONFIGRET CM_Get_Child(
            out IntPtr pdnDevInst,
            IntPtr dnDevInst,
            int ulFlags);
        
        [DllImport("setupapi.dll")]
        internal static extern CONFIGRET CM_Get_Sibling(
            out IntPtr pdnDevInst,
            IntPtr dnDevInst,
            int ulFlags);

        [DllImport("setupapi.dll")]
        internal static extern CONFIGRET CM_Get_Device_ID(
            IntPtr dnDevInst,
            StringBuilder buffer,
            int bufferLen,
            int ulFlags);

        [DllImport("setupapi.dll")]
        internal static extern CONFIGRET CM_Locate_DevNode(
            out IntPtr pdnDevInst,
            string pDeviceID, 
            int ulFlags);
        
        [DllImport("setupapi.dll")]
        internal static extern CONFIGRET CM_Request_Device_Eject(
            IntPtr dnDevInst,
            out PNP_VETO_TYPE pVetoType,
            StringBuilder pszVetoName,
            int ulNameLength,
            int ulFlags
            );

        [DllImport("setupapi.dll", EntryPoint = "CM_Request_Device_Eject")]
        internal static extern CONFIGRET CM_Request_Device_Eject_NoUi(
            IntPtr dnDevInst,
            IntPtr pVetoType,
            StringBuilder pszVetoName,
            int ulNameLength,
            int ulFlags
            );

        // from setupapi.h
        internal const int DIGCF_PRESENT         = 0x00000002;
        internal const int DIGCF_ALLCLASSES      = 0x00000004;
        internal const int DIGCF_DEVICEINTERFACE = 0x00000010;

        internal enum SetupDiRegistryProperty : int
        {
            SPDRP_DEVICEDESC = 0x00000000,
            SPDRP_CAPABILITIES = 0x0000000F,
            SPDRP_CLASS = 0x00000007,
            SPDRP_CLASSGUID = 0x00000008,
            SPDRP_DRIVER = 0x00000009,  // Driver (R/W)
            SPDRP_FRIENDLYNAME = 0x0000000C,
            SPDRP_ENUMERATOR_NAME = 0x00000016  // Enumerator Name (R)
        }

        [StructLayout(LayoutKind.Sequential)]
        public class SP_DEVINFO_DATA
        {
            public int cbSize = Marshal.SizeOf(typeof(SP_DEVINFO_DATA));
            public Guid classGuid = Guid.Empty; // temp
            public IntPtr devInst;
            public int reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 2)]
        internal class SP_DEVICE_INTERFACE_DETAIL_DATA
        {
            internal int cbSize = Marshal.SizeOf(typeof(SP_DEVICE_INTERFACE_DETAIL_DATA));
            internal short devicePath;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal class SP_DEVICE_INTERFACE_DATA
        {
            internal int cbSize = Marshal.SizeOf(typeof(SP_DEVICE_INTERFACE_DATA));
            internal Guid interfaceClassGuid = Guid.Empty; // temp
            internal int flags;
            internal int reserved;
        }

        /// <summary>An SP_CLASSIMAGELIST_DATA structure describes a class image list.</summary>
        /// <remarks>Defined in setupapi.h.</remarks>
        [StructLayout(LayoutKind.Sequential)]
        internal class SP_CLASSIMAGELIST_DATA
        {
            /// <summary>The size, in bytes, of the SP_CLASSIMAGE_DATA structure.</summary>
            internal int cbSize = Marshal.SizeOf(typeof(SP_CLASSIMAGELIST_DATA));
            /// <summary>A handle to the class image list.</summary>
            internal IntPtr ImageList;
            /// <summary>Reserved. For internal use only.</summary>
            internal int Reserved;
        }

//        [DllImport("setupapi.dll", CharSet = CharSet.Auto)]
//        internal static extern IntPtr SetupDiGetClassDevs(
//            ref Guid ClassGuid,
//            [MarshalAs(UnmanagedType.LPTStr)]
//            String Enumerator,
//            IntPtr hwndParent,
//            UInt32 Flags);

        [DllImport("setupapi.dll", CharSet = CharSet.Auto)]
        internal static extern IntPtr SetupDiGetClassDevs(           // 1st form using a ClassGUID
           ref Guid ClassGuid,
           int Enumerator,
           IntPtr hwndParent,
           int Flags
        );
        [DllImport("setupapi.dll", CharSet = CharSet.Auto)]     // 2nd form uses an Enumerator
        internal static extern IntPtr SetupDiGetClassDevs(
           int ClassGuid,
           string Enumerator,
           IntPtr hwndParent,
           int Flags
        );
        
        [DllImport("setupapi.dll", SetLastError = true, CharSet = CharSet.Auto)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool SetupDiEnumDeviceInterfaces(
            IntPtr deviceInfoSet,
            SP_DEVINFO_DATA deviceInfoData,
            ref Guid interfaceClassGuid,
            int memberIndex,
            SP_DEVICE_INTERFACE_DATA deviceInterfaceData);

        /// <summary>
        /// The SetupDiEnumDeviceInfo function retrieves a context structure for a device information element 
        /// of the specified device information set. Each call returns information about one device. 
        /// The function can be called repeatedly to get information about several devices.
        /// </summary>
        /// <param name="DeviceInfoSet">[in] Handle to the device information set containing the devices for which to return
        /// element information.</param>
        /// <param name="MemberIndex">[in] Zero-based index to the list of interfaces in the device information set.
        /// You should first call this function with the MemberIndex parameter set to zero to obtain the first interface. 
        /// Then, repeatedly increment MemberIndex and retrieve an interface until this function fails and GetLastError 
        /// returns ERROR_NO_MORE_ITEMS.</param>
        /// <param name="DeviceInfoData">[out] Pointer to an SP_DEVINFO_DATA structure that receives information about this element.
        /// You must set the cbSize member to sizeof( SP_DEVINFO_DATA) before calling this function.</param>
        /// <returns>If the function succeeds, the return value is nonzero.
        /// If the function fails, the return value is zero. To get extended error information, call GetLastError.</returns>
        [DllImport("setupapi.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool SetupDiEnumDeviceInfo(
            IntPtr DeviceInfoSet,
            int MemberIndex,
            SP_DEVINFO_DATA DeviceInfoData);
        
        [DllImport("setupapi.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool SetupDiOpenDeviceInfo(
            IntPtr deviceInfoSet,
            string deviceInstanceId,
            IntPtr hwndParent,
            int openFlags,
            SP_DEVINFO_DATA deviceInfoData
            );

        [DllImport("setupapi.dll", SetLastError = true, CharSet = CharSet.Auto)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool SetupDiGetDeviceInterfaceDetail(
            IntPtr deviceInfoSet,
            SP_DEVICE_INTERFACE_DATA deviceInterfaceData,
            IntPtr deviceInterfaceDetailData,
            int deviceInterfaceDetailDataSize,
            ref int requiredSize,
            SP_DEVINFO_DATA deviceInfoData);

        [DllImport("setupapi.dll", CharSet = CharSet.Auto, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool SetupDiGetDeviceRegistryProperty(
            IntPtr deviceInfoSet,
            SP_DEVINFO_DATA deviceInfoData,
            SetupDiRegistryProperty property,
            out int propertyRegDataType,
            IntPtr propertyBuffer,
            int propertyBufferSize,
            out int requiredSize);

        [DllImport("setupapi.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool SetupDiDestroyDeviceInfoList(
            IntPtr deviceInfoSet);

        /// <summary>
        /// The SetupDiGetClassImageList function builds an image list that contains bitmaps for every installed class and returns the list in a data structure.
        /// </summary>
        /// <param name="ClassImageListData">A pointer to an SP_CLASSIMAGELIST_DATA structure to receive information regarding the class image list, including a handle to the image list. The cbSize field of this structure must be initialized with the size of the structure, in bytes, before calling this function or it will fail.</param>
        /// <returns>The function returns TRUE if it is successful. Otherwise, it returns FALSE and the logged error can be retrieved by a call to GetLastError.</returns>
        /// <remarks>The image list built by this function should be destroyed by calling SetupDiDestroyClassImageList. Call SetupDiGetClassImageListEx to retrieve the image list for classes installed on a remote machine. Declared in setupapi.h</remarks>
        [SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
        [DllImport("setupapi.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool SetupDiGetClassImageList(
            SP_CLASSIMAGELIST_DATA ClassImageListData);

        /// <summary>
        /// The SetupDiDestroyClassImageList function destroys a class image list that was built by a call to SetupDiGetClassImageList or SetupDiGetClassImageListEx.
        /// </summary>
        /// <param name="ClassImageListData">A pointer to an SP_CLASSIMAGELIST_DATA structure that contains the class image list to destroy.</param>
        /// <returns>The function returns TRUE if it is successful. Otherwise, it returns FALSE and the logged error can be retrieved by a call to GetLastError.</returns>
        /// <remarks>Declared in setupapi.h.</remarks>
        [DllImport("setupapi.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool SetupDiDestroyClassImageList(
            SP_CLASSIMAGELIST_DATA ClassImageListData);

        /// <summary>
        /// The SetupDiGetClassImageIndex function retrieves the index within the class image list of a specified class.
        /// </summary>
        /// <param name="ClassImageListData">A pointer to an SP_CLASSIMAGELIST_DATA structure that describes a class image list that includes the image for the device setup class that is specified by the ClassGuid parameter.</param>
        /// <param name="ClassGuid">A pointer to the GUID of the device setup class for which to retrieve the index of the class image in the specified class image list.</param>
        /// <param name="ImageIndex">A pointer to an INT-typed variable that receives the index of the specified class image in the class image list.</param>
        /// <returns>The function returns TRUE if it is successful. Otherwise, it returns FALSE and the logged error can be retrieved by a call to GetLastError.</returns>
        /// <remarks>If the specified device setup class is not included in the specified class image list, SetupDiGetClassImageIndex returns the image index for the Unknown device setup class in the ImageIndex parameter. Declared in setupapi.h.</remarks>
        [DllImport("setupapi.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool SetupDiGetClassImageIndex(
		    SP_CLASSIMAGELIST_DATA  ClassImageListData,
            ref Guid ClassGuid,
            out int ImageIndex);

        /// <summary>
        /// The SetupDiGetClassDescription function retrieves the class description associated with the specified setup class GUID.
        /// </summary>
        /// <param name="ClassGuid">The GUID of the setup class whose description is to be retrieved.</param>
        /// <param name="ClassDescription">A pointer to a character buffer that receives the class description.</param>
        /// <param name="ClassDescriptionSize">The size, in characters, of the ClassDescription buffer.</param>
        /// <param name="RequiredSize">A pointer to variable of type DWORD that receives the size, in characters, that is required to store the class description (including a NULL terminator). RequiredSize is always less than LINE_LEN. This parameter is optional and can be NULL.</param>
        /// <returns>The function returns TRUE if it is successful. Otherwise, it returns FALSE and the logged error can be retrieved with a call to GetLastError.</returns>
        [DllImport("setupapi.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool SetupDiGetClassDescription(
            ref Guid ClassGuid,
            StringBuilder ClassDescription,
            int ClassDescriptionSize,
            out int RequiredSize);


        /// <summary>
        /// The SetupDiCreateDeviceInfoList function creates an empty device information set. This set can be associated with a class GUID.
        /// </summary>
        /// <param name="ClassGuid">[in] GUID of the setup class associated with this device information set. If this parameter is specified, only devices of this class can be included in this device information set. This parameter is optional.</param>
        /// <param name="hwndParent">[in] Handle to the top-level window in which to display a user interface. This parameter is optional.</param>
        /// <returns>If the function succeeds, the return value is a handle to an empty device information set.
        /// If the function fails, the return value is INVALID_HANDLE_VALUE. To get extended error information, call GetLastError.</returns>
        /// <remarks>To destroy the device information set when you have finished, call the SetupDiDestroyDeviceInfoList function.</remarks>
        [DllImport("setupapi.dll", SetLastError = true)]
        internal static extern IntPtr SetupDiCreateDeviceInfoList(
            IntPtr ClassGuid,    // This param is optional, and we only pass null here.
            IntPtr hwndParent);  // This param is optional, and we only pass null here.

        // from usbhcdi.h
        public enum USB_DEVICE_SPEED : byte
        {
            UsbLowSpeed = 1,
            UsbFullSpeed,
            UsbHighSpeed
        }

        // from usbioctl.h
        public enum USB_HUB_NODE
        {
            UsbHub,
            UsbMIParent
        }

        public enum USB_CONNECTION_STATUS
        {
            NoDeviceConnected,
            DeviceConnected,
            /* failure codes, these map to fail reasons */
            DeviceFailedEnumeration,
            DeviceGeneralFailure,
            DeviceCausedOvercurrent,
            DeviceNotEnoughPower,
            DeviceNotEnoughBandwidth,
            DeviceHubNestedTooDeeply,
            DeviceInLegacyHub
        }

        [StructLayout(LayoutKind.Sequential)]
        public class USB_NODE_INFORMATION
        {
            /// <summary>A USB_HUB_NODE enumerator that indicates whether the parent device is a hub or a non-hub composite device.</summary>
            public USB_HUB_NODE NodeType;
            public USB_HUB_INFORMATION HubInformation;      // Yeah, I'm assuming we'll just use the first form
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct USB_HUB_INFORMATION
        {
            public USB_HUB_DESCRIPTOR HubDescriptor;
            public byte HubIsBusPowered;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct USB_HUB_DESCRIPTOR
        {
            public byte bDescriptorLength;
            public byte bDescriptorType;
            public byte bNumberOfPorts;
            public short wHubCharacteristics;
            public byte bPowerOnToPowerGood;
            public byte bHubControlCurrent;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 64)]
            public byte[] bRemoveAndPowerMask;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct USB_DEVICE_DESCRIPTOR
        {
            public byte bLength;
            public byte bDescriptorType;
            public short bcdUSB;
            public byte bDeviceClass;
            public byte bDeviceSubClass;
            public byte bDeviceProtocol;
            public byte bMaxPacketSize0;
            public short idVendor;
            public short idProduct;
            public short bcdDevice;
            public byte iManufacturer;
            public byte iProduct;
            public byte iSerialNumber;
            public byte bNumConfigurations;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public class USB_NODE_CONNECTION_INFORMATION
        {
            public int ConnectionIndex;
            public USB_DEVICE_DESCRIPTOR DeviceDescriptor;
            public byte CurrentConfigurationValue;
            public byte LowSpeed;
            public byte DeviceIsHub;
            public short DeviceAddress;
            public int NumberOfOpenPipes;
            public USB_CONNECTION_STATUS ConnectionStatus;
            //public IntPtr PipeList;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public class USB_NODE_CONNECTION_INFORMATION_EX
        {
            public int ConnectionIndex
            {
                get { return _ConnectionIndex; }
                set { _ConnectionIndex = value; }
            }
            private int _ConnectionIndex;
            public USB_DEVICE_DESCRIPTOR DeviceDescriptor;
            public byte CurrentConfigurationValue;
            public USB_DEVICE_SPEED Speed;
            public byte DeviceIsHub;
            public short DeviceAddress;
            public int NumberOfOpenPipes;
            public USB_CONNECTION_STATUS ConnectionStatus;
            //public IntPtr PipeList;


            public static implicit operator USB_NODE_CONNECTION_INFORMATION_EX(USB_NODE_CONNECTION_INFORMATION info)
            {
                USB_NODE_CONNECTION_INFORMATION_EX infoEx = new USB_NODE_CONNECTION_INFORMATION_EX();
                infoEx.ConnectionIndex = info.ConnectionIndex;
                infoEx.DeviceDescriptor = info.DeviceDescriptor;
                infoEx.CurrentConfigurationValue = info.CurrentConfigurationValue;
                infoEx.Speed = Convert.ToBoolean(info.LowSpeed) ? USB_DEVICE_SPEED.UsbLowSpeed : USB_DEVICE_SPEED.UsbFullSpeed;
                infoEx.DeviceIsHub = info.DeviceIsHub;
                infoEx.DeviceAddress = info.DeviceAddress;
                infoEx.NumberOfOpenPipes = info.NumberOfOpenPipes;
                infoEx.ConnectionStatus = info.ConnectionStatus;

                return infoEx;
            }
}

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
        internal struct USB_NODE_CONNECTION_DRIVERKEY_NAME
        {
            internal int ConnectionIndex;
            internal int ActualLength;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
            internal string DriverKeyName;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
        internal struct USB_ROOT_HUB_NAME
        {
            internal int ActualLength;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
            internal string RootHubName;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
        internal struct USB_HCD_DRIVERKEY_NAME
        {
            internal int ActualLength;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
            internal string DriverKeyName;
        }

        internal const int IOCTL_USB_GET_NODE_INFORMATION = 0x220408;
        internal const int IOCTL_USB_GET_NODE_CONNECTION_INFORMATION    = 0x22040C;
        internal const int IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX = 0x220448;
        internal const int IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME = 0x220420;
        internal const int IOCTL_USB_GET_ROOT_HUB_NAME = 0x220408;
        internal const int IOCTL_GET_HCD_DRIVERKEY_NAME = 0x220424;

        [DllImport("advapi32.dll", CharSet = CharSet.Unicode, EntryPoint = "RegQueryValueExW", SetLastError = true)]
        internal static extern int RegQueryValueEx(
            IntPtr hKey,
            string lpValueName,
            IntPtr lpReserved,
            IntPtr lpType,
            StringBuilder lpData,
            ref int lpcbData);
    
        [DllImport("advapi32.dll", SetLastError = true)]
        internal static extern int RegCloseKey(
            IntPtr hKey);

        [DllImport("ole32.dll")]
        internal static extern int CreateClassMoniker(
            [In] ref Guid rclsid,
            out IMoniker ppmk);

        [DllImport("ole32.dll")]
        internal static extern int GetRunningObjectTable(
            uint reserved,
            out System.Runtime.InteropServices.ComTypes.IRunningObjectTable pprot);

        [DllImport("ole32.dll")]
        internal static extern int CoRegisterClassObject(
            [In] ref Guid rclsid,
            [MarshalAs(UnmanagedType.IUnknown)] object pUnk,
            uint dwClsContext,
            uint flags,
            out uint lpdwRegister);

        [DllImport("ole32.dll")]
        internal static extern int CoRevokeClassObject(
            uint dwRegister);

        [DllImport("shsvcs.dll")]
        internal static extern int CreateHardwareEventMoniker(
            ref Guid clsid,
            String pszEventHandler,
            out IMoniker ppmoniker);

        // from scsi.h
        //
        // SCSI bus status codes.
        public enum ScsiSenseStatus : byte
        {
            GOOD = 0x00,
            CHECK_CONDITION = 0x02,
            CONDITION_MET = 0x04,
            BUSY = 0x08,
            INTERMEDIATE = 0x10,
            INTERMEDIATE_COND_MET = 0x14,
            RESERVATION_CONFLICT = 0x18,
            COMMAND_TERMINATED = 0x22,
            QUEUE_FULL = 0x28
        }

        //
        // Sense Codes
        public enum ScsiSenseKey : byte
        {
            SCSI_SENSE_NO_SENSE        = 0x00,
            SCSI_SENSE_RECOVERED_ERROR = 0x01,
            SCSI_SENSE_NOT_READY       = 0x02,
            SCSI_SENSE_MEDIUM_ERROR    = 0x03,
            SCSI_SENSE_HARDWARE_ERROR  = 0x04,
            SCSI_SENSE_ILLEGAL_REQUEST = 0x05,
            SCSI_SENSE_UNIT_ATTENTION  = 0x06,
            SCSI_SENSE_DATA_PROTECT    = 0x07,
            SCSI_SENSE_BLANK_CHECK     = 0x08,
            SCSI_SENSE_UNIQUE          = 0x09,
            SCSI_SENSE_COPY_ABORTED    = 0x0A,
            SCSI_SENSE_ABORTED_COMMAND = 0x0B,
            SCSI_SENSE_EQUAL           = 0x0C,
            SCSI_SENSE_VOL_OVERFLOW    = 0x0D,
            SCSI_SENSE_MISCOMPARE      = 0x0E,
            SCSI_SENSE_RESERVED        = 0x0F
        }

        [TypeConverter(typeof(ExpandableObjectConverter))]
        [StructLayout(LayoutKind.Sequential, Size = 18)]
        public class SENSE_DATA
        {
            private Byte _ErrorCode;
            private bool _Valid;
            private Byte _SegmentNumber;
            private Win32.ScsiSenseKey _SenseKey;
            private bool _Reserved;
            private bool _IncorrectLength;
            private bool _EndOfMedia;
            private bool _FileMark;
            private Byte[] _Information = new Byte[4];
            private Byte _AdditionalSenseLength;
            private Byte[] _CommandSpecificInformation = new Byte[4];
            private Byte _AdditionalSenseCode;
            private Byte _AdditionalSenseCodeQualifier;
            private Byte _FieldReplaceableUnitCode;
            private Byte[] _SenseKeySpecific = new Byte[3];

            public void CopyFrom(Byte[] senseBytes)
            {
                ErrorCode = (Byte)(senseBytes[0] & 0x7F);
                Valid = (senseBytes[0] & 0x80) == 0x80;
                SegmentNumber = senseBytes[1];
                SenseKey = (Win32.ScsiSenseKey)(senseBytes[2] & 0x0F);
                Reserved = (senseBytes[2] & 0x10) == 0x10;
                IncorrectLength = (senseBytes[2] & 0x20) == 0x20;
                EndOfMedia = (senseBytes[2] & 0x40) == 0x40;
                FileMark = (senseBytes[2] & 0x80) == 0x80;
                Array.Copy(senseBytes, 3, Information, 0, 4);
                AdditionalSenseLength = senseBytes[7];
                Array.Copy(senseBytes, 8, CommandSpecificInformation, 0, 4);
                AdditionalSenseCode = senseBytes[12];
                AdditionalSenseCodeQualifier = senseBytes[13];
                FieldReplaceableUnitCode = senseBytes[14];
                Array.Copy(senseBytes, 15, SenseKeySpecific, 0, 3);
            }

            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte ErrorCode { get { return _ErrorCode; } set { _ErrorCode = value; } }
            public bool Valid { get { return _Valid; } set { _Valid = value; } }
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte SegmentNumber { get { return _SegmentNumber; } set { _SegmentNumber = value; } }
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public Win32.ScsiSenseKey SenseKey { get { return _SenseKey; } set { _SenseKey = value; } }
            public bool Reserved { get { return _Reserved; } set { _Reserved = value; } }
            public bool IncorrectLength { get { return _IncorrectLength; } set { _IncorrectLength = value; } }
            public bool EndOfMedia { get { return _EndOfMedia; } set { _EndOfMedia = value; } }
            public bool FileMark { get { return _FileMark; } set { _FileMark = value; } }
            public Byte[] Information { get { return _Information; } set { _Information = value; } }
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte AdditionalSenseLength { get { return _AdditionalSenseLength; } set { _AdditionalSenseLength = value; } }
            public Byte[] CommandSpecificInformation { get { return _CommandSpecificInformation; } set { _CommandSpecificInformation = value; } }
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte AdditionalSenseCode { get { return _AdditionalSenseCode; } set { _AdditionalSenseCode = value; } }
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte AdditionalSenseCodeQualifier { get { return _AdditionalSenseCodeQualifier; } set { _AdditionalSenseCodeQualifier = value; } }
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte FieldReplaceableUnitCode { get { return _FieldReplaceableUnitCode; } set { _FieldReplaceableUnitCode = value; } }
            public Byte[] SenseKeySpecific { get { return _SenseKeySpecific; } set { _SenseKeySpecific = value; } }

            public override string ToString()
            {
                String errString = String.Empty;
                Utils.DecimalConverterEx decCvtr = new Utils.DecimalConverterEx();
                Utils.EnumConverterEx enumCvtr = new Utils.EnumConverterEx(typeof(Win32.ScsiSenseKey));

                errString  = "  Valid: " + Valid.ToString() + "\r\n";
                errString += "  SenseKey: " + enumCvtr.ConvertToString(SenseKey) + "\r\n";
                errString += "  ErrorCode: " + decCvtr.ConvertToString(ErrorCode) + "\r\n";
                errString += "  AdditionalSenseCode: " + decCvtr.ConvertToString(AdditionalSenseCode) + "\r\n";
                errString += "  AdditionalSenseCodeQualifier: " + decCvtr.ConvertToString(AdditionalSenseCodeQualifier) + "\r\n";
                errString += String.Format("  Information[0-3]: {0:X2} {1:X2} {2:X2} {3:X2}\r\n", Information[0], Information[1], Information[2], Information[3]);
                errString += String.Format("  CommandSpecificInformation[0-3]: {0:X2} {1:X2} {2:X2} {3:X2}\r\n", CommandSpecificInformation[0], CommandSpecificInformation[1], CommandSpecificInformation[2], CommandSpecificInformation[3]);
                errString += String.Format("  SenseKeySpecific[0-2]: {0:X2} {1:X2} {2:X2}\r\n", SenseKeySpecific[0], SenseKeySpecific[1], SenseKeySpecific[2]);
                errString += "  SegmentNumber: " + decCvtr.ConvertToString(SegmentNumber) + "\r\n";
                errString += "  IncorrectLength: " + IncorrectLength.ToString() + "\r\n";
                errString += "  EndOfMedia: " + EndOfMedia.ToString() + "\r\n";
                errString += "  FileMark: " + FileMark.ToString() + "\r\n";
                errString += "  AdditionalSenseLength: " + decCvtr.ConvertToString(AdditionalSenseLength) + "\r\n";
                errString += "  FieldReplaceableUnitCode: " + decCvtr.ConvertToString(FieldReplaceableUnitCode);

                return errString;
            }
        }

        // from ntddscsi.h
        [StructLayout(LayoutKind.Sequential)]
        internal class SCSI_PASS_THROUGH
        {
            public UInt16 Length;
            public ScsiSenseStatus ScsiStatus;
            public Byte PathId;
            public Byte TargetId;
            public Byte Lun;
            public Byte CdbLength;
            public Byte SenseInfoLength;
            public Byte DataIn;
            public UInt32 DataTransferLength;
            public UInt32 TimeOutValue;
            public UInt32 DataBufferOffset;
            public UInt32 SenseInfoOffset;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
            public Byte[] Cdb = new Byte[16];
        }
        //
        // Define values for pass-through DataIn field.
        //
        internal const Byte SCSI_IOCTL_DATA_OUT          = 0;
        internal const Byte SCSI_IOCTL_DATA_IN           = 1;
        internal const Byte SCSI_IOCTL_DATA_UNSPECIFIED  = 2;

        internal const Int32 IOCTL_SCSI_BASE = FILE_DEVICE_CONTROLLER;

        private static Int32 CTL_CODE(int DeviceType, int Function, int Method, int Access)
        {
            return ( ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method));
        }

        internal const Int32 IOCTL_SCSI_PASS_THROUGH = IOCTL_SCSI_BASE << 16 | 0x0401 << 2 | (int)MethodType.Buffered | ((int)FileAccessType.Read | (int)FileAccessType.Write) << 14;
        internal const Int32 IOCTL_SCSI_MINIPORT = IOCTL_SCSI_BASE << 16 | 0x0402 << 2 | (int)MethodType.Buffered | ((int)FileAccessType.Read | (int)FileAccessType.Write) << 14;
        internal const Int32 IOCTL_SCSI_GET_INQUIRY_DATA = IOCTL_SCSI_BASE << 16 | 0x0403 << 2 | (int)MethodType.Buffered | (int)FileAccessType.Read << 14;
        internal const Int32 IOCTL_SCSI_GET_CAPABILITIES = IOCTL_SCSI_BASE << 16 | 0x0404 << 2 | (int)MethodType.Buffered | (int)FileAccessType.Read << 14;
        internal const Int32 IOCTL_SCSI_PASS_THROUGH_DIRECT = IOCTL_SCSI_BASE << 16 | 0x0405 << 2 | (int)MethodType.Buffered | ((int)FileAccessType.Read | (int)FileAccessType.Write) << 14;
        internal const Int32 IOCTL_SCSI_GET_ADDRESS = IOCTL_SCSI_BASE << 16 | 0x0406 << 2 | (int)MethodType.Buffered | (int)FileAccessType.Read << 14;
        internal const Int32 IOCTL_SCSI_RESCAN_BUS = IOCTL_SCSI_BASE << 16 | 0x0407 << 2 | (int)MethodType.Buffered | (int)FileAccessType.Read << 14;
        internal const Int32 IOCTL_SCSI_GET_DUMP_POINTERS = IOCTL_SCSI_BASE << 16 | 0x0408 << 2 | (int)MethodType.Buffered | (int)FileAccessType.Read << 14;
        internal const Int32 IOCTL_SCSI_FREE_DUMP_POINTERS = IOCTL_SCSI_BASE << 16 | 0x0409 << 2 | (int)MethodType.Buffered | (int)FileAccessType.Read << 14;
        internal const Int32 IOCTL_IDE_PASS_THROUGH = IOCTL_SCSI_BASE << 16 | 0x040a << 2 | (int)MethodType.Buffered | ((int)FileAccessType.Read | (int)FileAccessType.Write) << 14;
        /*

                [DllImport("mswmdm.dll")]
                internal static extern class CSecureChannelClient
                {

                    extern int SetCertificate(
                        UInt32 dwFlags,
                        [MarshalAs(UnmanagedType.LPArray, ArraySubType = Byte)]
                    ref Byte[] pbAppCert,
                        UInt32 dwCertLen,
                        [MarshalAs(UnmanagedType.LPArray, ArraySubType = Byte)]
                    ref Byte[] pbAppPVK,
                        UInt32 dwPVKLen);
                }
                HRESULT SetCertificate(DWORD dwFlags,
                                       BYTE* pbAppCert,
                                       DWORD dwCertLen,
                                       BYTE* pbAppPVK,
                                       DWORD dwPVKLen);
                void SetInterface(IComponentAuthenticate* pComponentAuth);
                HRESULT Authenticate(DWORD dwProtocolID);
        */
    }
}

