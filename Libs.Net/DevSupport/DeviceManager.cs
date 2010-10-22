/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// UsbEject version 1.0 March 2006
// written by Simon Mourier <email: simon [underscore] mourier [at] hotmail [dot] com>

using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Threading;
using Microsoft.Win32;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Reflection;
using System.Resources;
using System.Text;

using Utils;

using System.IO;

namespace DevSupport.DeviceManager
{
    /// <summary>
    /// Changes that might occur to a file or directory.
    /// </summary>
//    public enum WatcherChangeTypes
//    {
//        All,
//        ByPort,
//        ByDevice,
//        ByDeviceType
//    }

    /// <summary>
    /// Specifies changes to watch for regarding a device.
    /// </summary>
    [FlagsAttribute]
    public enum NotifyFilters
    {
        None        = 0x00,
        Port        = 0x01,
        Device      = 0x02,
        DeviceType  = 0x04,
        Any         = Port | Device | DeviceType
    }

    /// <summary>
    /// A Singleton class for managing devices.
    /// </summary>
    public sealed class DeviceManager : Win32.IQueryCancelAutoPlay, Win32.IHWEventHandler, IDisposable
    {
        private static SynchronizationContext _SynchronizationContext;
        private static object _TheLock;
        private static DeviceChangeWindow _DevChangeWnd;
        private static Collection<DeviceClass> _DeviceClassMgrs;
//        private static MSWMDMLib.IWMDeviceManager _WMDMDevMgr = new MSWMDMLib.IWMDeviceManager();

        /// <summary>
        /// Container to hold subcribers to the DeviceChanged event and/or callers of the
        /// RegisterForDeviceChangeNotification method. These subscribers will be notified on their
        /// respective thread the stipulated device change occurs.
        /// </summary>
        private static Collection<DeviceChangeWatcherArgs> _DeviceChangeCallbackList;

        /// <summary>
        /// Manual reset event handle that gets signaled when a DeviceChanged event occurs. This allows a thread 
        /// to block while waiting for a DeviceChanged event to occur. You should call the Reset() method on this 
        /// property before using it as a Waitxxx() argument.
        /// </summary>
        public static EventWaitHandle EventWaitHandle
        {
            get { return _EventWaitHandle; }
            private set { _EventWaitHandle = value; }
        }
        private static EventWaitHandle _EventWaitHandle;
        
        #region Construction
        
        /// <summary>
        /// Gets the single static DeviceManager instance.
        /// </summary>
        public static DeviceManager Instance
        {
            get { return Utils.Singleton<DeviceManager>.Instance; }
        }

        /// <summary>
        /// Private constructor to prevent casual instantiation of this class.
        /// </summary>
        private DeviceManager()
        {
            Trace.WriteLine(String.Format("*** DeviceManager.Instance.DeviceManager(), {0}({1})", Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));
        }

        /// <summary>
        /// Initialize the single static DeviceManager instance.
        /// </summary>
        static DeviceManager()
        {
            try
            {
            _TheLock = new Object();
            EventWaitHandle = new EventWaitHandle(true, EventResetMode.ManualReset);
            CancelAutoPlayDriveList = "DEFGHIJKLMNOPQRSTUVWXYZ";
            _CLSID_QueryCancelAutoPlay = new Guid("66A32FE6-229D-427B-A608-D273F40C034C");
            _CLSID_HWEventHandler = new Guid("C474001A-E005-4a8d-8EE5-D45224ED4DF3");
            _APPID_ComHWEventApp = new Guid("B2FE8EA6-5008-40b0-A30F-0A51511C9FB0");

            _SynchronizationContext = WindowsFormsSynchronizationContext.Current;
            _DevChangeWnd = new DeviceChangeWindow();
            _DevChangeWnd.DeviceChangedMsg += new DeviceChangeWindow.DeviceChangedMsgHandler(DevChangeWnd_DeviceChangedMsg);
            _DeviceChangeCallbackList = new Collection<DeviceChangeWatcherArgs>();

            _DeviceClassMgrs = new Collection<DeviceClass>();
            _DeviceClassMgrs.Add(RecoveryDeviceClass.Instance);
            _DeviceClassMgrs.Add(WinUsbDeviceClass.Instance);
            _DeviceClassMgrs.Add(HidDeviceClass.Instance);
            _DeviceClassMgrs.Add(MxRomDeviceClass.Instance);
            if (WpdDeviceClass.Instance.IsSupported)
                _DeviceClassMgrs.Add(WpdDeviceClass.Instance);
//            _DeviceClassMgrs.Add(ScsiDeviceClass.Instance);
//            _DeviceClassMgrs.Add(DiskDeviceClass.Instance);
            _DeviceClassMgrs.Add(VolumeDeviceClass.Instance);

//            _DeviceClassMgrs[DeviceClassType.UsbHubClass] = UsbHubClass.Instance;
//            _DeviceClassMgrs[DeviceClassType.UsbControllerClass] = UsbControllerClass.Instance;

/*            try
            {
                MSWMDMLib.MSWMDMLib mswmdm = new MSWMDMLib.MSWMDMLib();

//                MSWMDMLib.MSWMDMLib lib = new MSWMDMLib.MSWMDMLib();
                MSWMDMLib.MediaDevMgrClass wmdmDevMgr = new MSWMDMLib.MediaDevMgrClass();
                MSWMDMLib.IComponentAuthenticate pAuth = wmdmDevMgr as MSWMDMLib.IComponentAuthenticate;
                UInt32[] pProtocols = new UInt32[5];
                UInt32 pcProtocols;
                pAuth.SACGetProtocols(pProtocols, out pcProtocols);

                MSWMDMLib.IWMDeviceManager3 devMgr3 = wmdmDevMgr as MSWMDMLib.IWMDeviceManager3;

                devMgr3.Reinitialize();

//                MSWMDMLib.CSecureChannelClient sac = new MSWMDMLib.CSecureChannelClient();
//                sac.Authenticate(2);

                MSWMDMLib.IWMDMEnumDevice devEnumerator = devMgr3.EnumDevices2();
                devEnumerator.Reset();

                while (true)
                {
                    MSWMDMLib.IWMDMDevice device;// = new MSWMDMLib.WMDMDeviceClass();
                    uint fetched = devEnumerator.Next(1, out device);

                    if (fetched == 1)
                    {
                        String path = String.Empty;
                        MSWMDMLib.IWMDMDevice3 deviceI3 = device as MSWMDMLib.IWMDMDevice3;
                        deviceI3.GetCanonicalName(path, 256);
                        break;
                    }
                }
*/            
            }
            catch (COMException ce)
            {
                String msg = ce.Message;
                Trace.WriteLine(String.Format("DeviceManager.Instance.DeviceManager(), {0}", msg));
            }
            catch (Exception e)
            {
                String msg = e.Message;
                Trace.WriteLine(String.Format("DeviceManager.Instance.DeviceManager(), {0}", msg));
            }

        }

        #endregion

        #region static ImageList Implementation

        private static Win32.SP_CLASSIMAGELIST_DATA _ClassImageList;

        /// <summary>
        /// Returns an HIMAGELIST from setupapi.dll
        /// </summary>
        internal static Win32.SP_CLASSIMAGELIST_DATA ClassImageList
        {
            get
            {
                lock (_TheLock)
                {
                    if (_ClassImageList == null)
                    {
                        _ClassImageList = new Win32.SP_CLASSIMAGELIST_DATA();

                        if (!Win32.SetupDiGetClassImageList(_ClassImageList))
                        {
                            int error = Marshal.GetLastWin32Error();
                            throw new Win32Exception(error);
                        }
                    }
                }
                return _ClassImageList;
            }
        }

        /// <summary>
        /// Attaches the DeviceManager ImageList to the specified control.
        /// </summary>
        /// <param name="ctrl"></param>
        public static void AttachImageList(Control ctrl)
        {
            uint msg = 0;

            // Get the system "device icons" from setupapi.dll
            //
            // Attach the system Image List to the TreeView Image List
            //
            //1- A pointer to the tree view image list handle.
            //2- The value "4361" (which is the Id of the "SetImageList" message.)
            //3- The value "0" (message parameter, it's the default one since we're not doing anything particular.)
            //4- Handle to the system image list.
            //With this done, you should have the image list set up to your TreeView.
            if (ctrl is TreeView)
            {
                msg = Win32.TVM_SETIMAGELIST;
            }
            else if (ctrl is ToolBar || ctrl is ToolStrip)
            {
                msg = Win32.TB_SETIMAGELIST;
            }
            else if (ctrl is ListView)
            {
                msg = Win32.LVM_SETIMAGELIST;
            }
            else if (ctrl is ComboBox)
            {
                msg = Win32.CBEM_SETIMAGELIST;
            }
            else if (ctrl is TabControl)
            {
                msg = Win32.TCM_SETIMAGELIST;
            }
            else
            {
                // Control not implemented
                throw new ArgumentException(String.Format("Control {0} is not implemented.", ctrl));
            }

            lock (_TheLock)
            {
                HandleRef hCtrl = new HandleRef(ctrl, ctrl.Handle);
                IntPtr retVal = Win32.SendMessage(hCtrl, msg, new IntPtr(0), ClassImageList.ImageList);
            }
        }

        public static ImageList ImageList
        {
            get 
            {
                if ( _ImageList == null || _ImageList.Images.Count == 0 )
                {
//                    _ImageList = ImageListConverter.FromHandle(ClassImageList.ImageList);
                    _ImageList = new ImageList();

                    int imageCount = Win32.ImageList_GetImageCount(ClassImageList.ImageList);
                    for (int i = 0; i < imageCount; ++i)
                    {
                        IntPtr hIcon = Win32.ImageList_GetIcon(ClassImageList.ImageList, i, Win32.ILD_NORMAL);
                        System.Drawing.Icon icon = System.Drawing.Icon.FromHandle(hIcon);
//                        FileStream fs = new FileStream(String.Format("{0}{1}.ico", icon.ToString(), i), FileMode.Create);
//                        icon.Save(fs);
//                        fs.Close();
 
                        _ImageList.Images.Add(icon);
                        Win32.DestroyIcon(hIcon);
                    }
                }
                return _ImageList;
            }
        }
        private static ImageList _ImageList;

        #endregion

        #region CancelAutoPlay Implementation

        /// <summary>
        /// This event is fired if an AutoPlay event was canceled. This would allow a UI to update itself in its
        /// own thread if the client application wants to notify the user.
        /// </summary>
        public event CancelAutoPlayEventHandler CancelAutoPlay
        {
            add
            {
                lock (_TheLock)
                {
                    RegisterCancelAutoPlay(value);
                }
            }

            remove
            {
                lock (_TheLock)
                {
                    UnRegisterCancelAutoPlay(value);
                }
            }
        }
        private static CancelAutoPlayEventHandler _CancelAutoPlay;

        /// <summary>
        /// The cookie registered for CancelAutoPlay functionality. 0 means DeviceManager is 
        /// not registered for AutoPlay cancellation.
        /// </summary>
        private static int _CancelAutoPlayCookie;
        private static Guid _CLSID_QueryCancelAutoPlay;
        private static int _HWEventHandlerCookie;
        private static Guid _CLSID_HWEventHandler;
        private static Guid _APPID_ComHWEventApp;


        private bool RegisterCancelAutoPlay(CancelAutoPlayEventHandler capDelegate)
        {
            // Must be Windows XP or later
            if (Environment.OSVersion.Version < new Version(5, 1))
                return false; // false means failed

            if (CreateAutoPlayHandlerRegistryEntries())
            {
                System.Runtime.InteropServices.ComTypes.IRunningObjectTable objTbl;
                if (Win32.GetRunningObjectTable(0, out objTbl) == Win32.S_OK)
                {
                    ///
                    /// Volume-based AutoPlay Cancelation
                    ///

                    if (_CancelAutoPlayCookie == 0)
                    {
                        // Create the volume-based moniker that we'll put in the ROT
                        System.Runtime.InteropServices.ComTypes.IMoniker vbMoniker;
                        if (Win32.CreateClassMoniker(ref _CLSID_QueryCancelAutoPlay, out vbMoniker) == Win32.S_OK)
                        {
                            _CancelAutoPlayCookie = objTbl.Register(Win32.ROTFLAGS_REGISTRATIONKEEPSALIVE, this, vbMoniker);
                        }
                    }
                    ///
                    /// Non-Volume-based AutoPlay Cancelation
                    ///

                    if (_HWEventHandlerCookie == 0)
                    {
                        // Create the non-volume-based moniker that we'll put in the ROT
                        System.Runtime.InteropServices.ComTypes.IMoniker nvbMoniker;
                        if (Win32.CreateHardwareEventMoniker(ref _CLSID_HWEventHandler, "MTPMediaPlayerArrival", out nvbMoniker) == Win32.S_OK)
                        {
                            _HWEventHandlerCookie = objTbl.Register(Win32.ROTFLAGS_REGISTRATIONKEEPSALIVE | Win32.ROTFLAGS_ALLOWANYCLIENT, this, nvbMoniker);
                        }

                        if (_HWEventHandlerCookie != 0)
                        {
                            // Add our do-nothing handler to the list of MTPMediaPlayerArrival handlers.
                            // Note this will have to be removed when we unregister for AutoPlay cancelation so the
                            // do-nothing handler does not show up as a choice in the AutoPlay Dialog.
                            Registry.SetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\EventHandlers\MTPMediaPlayerArrival", "SgtlMtpAutoPlayHandler", String.Empty);
                        }
                    }
                } // if (Win32.GetRunningObjectTable(0, out objTbl) == Win32.S_OK) 

            }

            if (_CancelAutoPlayCookie != 0 && _HWEventHandlerCookie != 0)
            {
                _CancelAutoPlay += capDelegate;
                return true;
            }
            else
                return false; // false means failed
        }

        private void UnRegisterCancelAutoPlay(CancelAutoPlayEventHandler capDelegate)
        {
            System.Runtime.InteropServices.ComTypes.IRunningObjectTable objTbl;
            if (Win32.GetRunningObjectTable(0, out objTbl) == Win32.S_OK)
            {
                if (_CancelAutoPlayCookie != 0)
                {
                    objTbl.Revoke(_CancelAutoPlayCookie);
                    _CancelAutoPlayCookie = 0;
                }

                if (_HWEventHandlerCookie != 0)
                {
                    objTbl.Revoke(_HWEventHandlerCookie);
                    _HWEventHandlerCookie = 0;

                    // Remove our do-nothing handler to the list of MTPMediaPlayerArrival handlers so it
                    // does not show up as a choice in the AutoPlay Dialog.
                    using (RegistryKey hKey = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\EventHandlers\MTPMediaPlayerArrival", true))
                    {
                        try
                        {
                            hKey.DeleteValue("SgtlMtpAutoPlayHandler");
                        }
                        catch (Exception e)
                        {
                            Trace.WriteLine(e.Message);
                        }
                    }
                }

                _CancelAutoPlay -= capDelegate;
            }
        }

        /// <summary>
        /// Create the registry entries necessary for the IQueryCanelAutoPlay and IHWEventHandler
        /// interfaces to work properly.
        /// </summary>
        /// <returns>Success(true), Failed(false).</returns>
        public bool CreateAutoPlayHandlerRegistryEntries()
        {
            bool success = true;

            try
            {
                ///
                /// Volume-based AutoPlay Cancelation
                ///
                Registry.SetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\CancelAutoplay\CLSID", _CLSID_QueryCancelAutoPlay.ToString(), String.Empty);

                ///
                /// Non-Volume-based AutoPlay Cancelation
                ///
                // ProgID/CLSID mapping
                Registry.SetValue(@"HKEY_CLASSES_ROOT\CLSID\" + _CLSID_HWEventHandler.ToString("B").ToUpper(), "AppID", _APPID_ComHWEventApp.ToString("B").ToUpper());
                Registry.SetValue(@"HKEY_CLASSES_ROOT\SGTL.MtpAutoPlayHandler\CLSID\", null, _CLSID_HWEventHandler.ToString("B").ToUpper());
                // Required to be invoked from a service process.
                Registry.SetValue(@"HKEY_CLASSES_ROOT\AppID\" + _APPID_ComHWEventApp.ToString("B").ToUpper(), "RunAs", "Interactive User");
                // Required to be able to register in the Running Object Table with the ROTFLAGS_ALLOWANYCLIENT flag
                // Get the name of the current application to associate with the registered AppID
                String appName = Process.GetCurrentProcess().Modules[0].ModuleName;
                Registry.SetValue(@"HKEY_CLASSES_ROOT\AppID\" + appName, "AppID", _APPID_ComHWEventApp.ToString("B").ToUpper());
                // Add a do nothing Handler entry
                Registry.SetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\Handlers\SgtlMtpAutoPlayHandler", "ProgID", "SGTL.MtpAutoPlayHandler");

            }
            catch (System.Security.SecurityException e)
            {
                Trace.WriteLine(e.Message);
                success = false;
            }

            return success;
        }

        /// <summary>
        /// List of drives associated with the CancelAutoPlay functionality
        /// </summary>
        public static String CancelAutoPlayDriveList
        {
            get
            {
                lock (_TheLock)
                {
                    return _CancelAutoPlayDriveList;
                }
            }
            set
            {
                lock (_TheLock)
                {
                    _CancelAutoPlayDriveList = value;
                }
            }
        }
        private static String _CancelAutoPlayDriveList;

        #region Win32.IHWEventHandler Implemenation

        int Win32.IHWEventHandler.Initialize(String pszParams)
        {
            return Win32.S_OK;
        }

        int Win32.IHWEventHandler.HandleEventWithContent(
            String pszDeviceID, String pszAltDeviceID, String pszEventType,
            String pszContentTypeHandler, IntPtr pdataobject)
        {
            return Win32.E_NOTIMPL;
        }

        // Called on an arbitrary thread.
        int Win32.IHWEventHandler.HandleEvent(String pszDeviceID, String pszAltDeviceID, String pszEventType)
        {
            // Notify anyone who cares.
            lock (_TheLock)
            {
                if (_CancelAutoPlay != null)
                {
//                    String msg = String.Format("Rejected \"{0}\" AutoPlay event for device: {1}( {2} )", pszEventType, pszAltDeviceID, pszDeviceID);
                    CancelAutoPlayEventArgs args = new CancelAutoPlayEventArgs(pszDeviceID, pszAltDeviceID, pszEventType, 0);

                    // Post the message to the UI Thread.
                    _SynchronizationContext.Post(new SendOrPostCallback(_CancelAutoPlay), args);
                }
            }

            Trace.WriteLine(String.Format("*** DeviceManager.Instance.IHWEventHandler.HandleEvent(): Handled \"{0}\" AutoPlay event for device: {1}( {2} ), thread({3})", pszEventType, pszAltDeviceID, pszDeviceID, Thread.CurrentThread.GetHashCode()));
            return Win32.S_OK;
        }

        #endregion // Win32.IHWEventHandler Implemenation

        #region Win32.IQueryCancelAutoPlay Implementation

        // Called on an arbitrary thread.
        int Win32.IQueryCancelAutoPlay.AllowAutoPlay(String pszPath, UInt32 dwContentType, String pszLabel, UInt32 dwSerialNumber)
        {
            int hr = Win32.S_OK;

            // Is it the drive we want to cancel Autoplay for?
            if (pszPath.IndexOfAny(CancelAutoPlayDriveList.ToCharArray()) != -1)
            {
                // Notify anyone who cares.
                lock (_TheLock)
                {
                    if (_CancelAutoPlay != null)
                    {
//                        String msg = String.Format("Rejected AutoPlay for drive {0}", pszPath);
                        CancelAutoPlayEventArgs args = new CancelAutoPlayEventArgs(pszPath, pszLabel, "VolumeArrival", dwSerialNumber);

                        // Post the message to the UI Thread.
                        _SynchronizationContext.Post(new SendOrPostCallback(_CancelAutoPlay), args);
                    }
                }

                // Setting the return value to S_FALSE cancels the AutoPlay mechanism.
                hr = Win32.S_FALSE;
                Trace.WriteLine(String.Format("*** DeviceManager.Instance.IQueryCancelAutoPlay.AllowAutoPlay(): Rejected AutoPlay Drive: {0}, thread({1})", pszPath, Thread.CurrentThread.GetHashCode()));
            }

            return hr;
        }

        #endregion // Win32.IQueryCancelAutoPlay Implementation

        #endregion // CancelAutoPlay Implementation

        #region IDisposable Implementation

        /// <summary>
        /// Used to determine if Dispose() has already been called.
        /// </summary>
        private bool _Disposed = false;

        ~DeviceManager()
        {
            // Call our helper method. 
            // Specifying "false" signifies that the GC triggered the clean up.
            Dispose(false);
        }

        public void Dispose()
        {
            // Call our helper method. 
            // Specifying "true" signifies that the object user triggered the clean up.
            Dispose(true);

            // Now suppress finalization.
            GC.SuppressFinalize(this);
        }
        
        private void Dispose(bool disposing)
        {
            //
            // Be sure we have not already been disposed!
            //
            if (!_Disposed)
            {
                //
                // If disposing equal true, dispose all managed resources.
                //
                if (disposing)
                {
                    //
                    // Dispose managed resources.
                    //
                    _DevChangeWnd.DeviceChangedMsg -= DevChangeWnd_DeviceChangedMsg;
                    _DevChangeWnd.Dispose();
                }
                //
                // Clean up unmanaged resources here.
                //
                if (_ClassImageList != null)
                {
                    if (_ClassImageList.ImageList != IntPtr.Zero)
                    {
                        bool ret = Win32.SetupDiDestroyClassImageList(_ClassImageList);
                        _ClassImageList = null;
                    }
                }
                UnRegisterCancelAutoPlay(null);

            }
            _Disposed = true;

            Trace.WriteLine(String.Format("*** DeviceManager.Instance.Dispose({0}), {1}({2})", disposing, Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));
        }

        #endregion

        /// <summary>
        /// Callback function signature for the DeviceChanged event.
        /// </summary>
        /// <param name="e">Description of the Device Changed event.</param>
        public delegate void DeviceChangedEventHandler(DeviceChangedEventArgs e);

        /// <summary>
        /// This event is fired if a DeviceChanged event occurs in the system. This would allow a UI to 
        /// watch the coming and goings of all USB devices/hubs/volumes or just specific USB devices 
        /// and update itself in its own thread if the client application wants to notify the user.
        /// 
        /// Note: Watchers subscribing directly to the DeviceChange event will be called for all device 
        /// changes monitored by the DeviceManager. If you want to filter the device change messages sent
        /// by the DeviceManager, you must call RegisterForDeviceChangeNotification with the desired
        /// filter info and delegate.
        /// </summary>
        public event DeviceChangedEventHandler DeviceChanged
        {
            add 
            { 
                RegisterForDeviceChangeNotification(new DeviceChangeWatcherArgs(value));
            }
            
            remove
            {
                UnRegisterForDeviceChangeNotification(new DeviceChangeWatcherArgs(value));
            }
        }

        /// <summary>
        /// Allows users of DeviceManager to stipulate which device change notifications will get sent. Users that
        /// subscribe directly to the DeviceChanged event will get all notifications identified by NotifyFilters.Any.
        /// Call UnRegisterForDeviceChangeNotification() to stop receiving DeviceChanged event notifications.
        /// </summary>
        /// <param name="watcher"></param>
        public void RegisterForDeviceChangeNotification(DeviceChangeWatcherArgs watcher)
        {
            if (watcher == null)
            {
                throw new ArgumentNullException("watcher");
            }

            lock (_TheLock)
            {
                if (!_DeviceChangeCallbackList.Contains(watcher))
                {
                    _DeviceChangeCallbackList.Add(watcher);
                }
            }
        }
        
        /// <summary>
        /// Call to stop receiving DeviceChanged event notifications.
        /// </summary>
        /// <param name="watcher"></param>
        public void UnRegisterForDeviceChangeNotification(DeviceChangeWatcherArgs watcher)
        {
            if (watcher == null)
            {
                throw new ArgumentNullException("watcher");
            }

            lock (_TheLock)
            {
                _DeviceChangeCallbackList.Remove(watcher);
            }
        }

        /// <summary>
        /// Handler for DeviceChangeWindow.DeviceChangedMsg event which gets signaled in response to 
        /// WM_DEVICECHANGE Windows event. This handler is called asynchronously on a ThreadPool thread
        /// since it is called with BeginInvoke() from the DeviceChangeWindow.DeviceChangedMsg eventn handler.
        /// 
        /// The work done here is to Add/Remove the changing device to/from the DeviceManager's static lists of devices,
        /// and then notify the "watcher(s)" (on their appropriate thread(s)) what type of change occured.
        /// 
        /// Note: Watchers subscribing directly to the DeviceChange event will be called for all device 
        /// changes monitored by the DeviceManager. If you want to filter the device change messages sent
        /// by the DeviceManager, you must call RegisterForDeviceChangeNotification with the desired
        /// filter info and delegate.
        /// </summary>
        /// <param name="devEvent"></param>
        /// <param name="devDetails"></param>
        private static void DevChangeWnd_DeviceChangedMsg(DeviceChangeEvent devEvent, String devDetails)
        {
            Thread.CurrentThread.Name = Thread.CurrentThread.IsThreadPoolThread ? "Some ThreadPool Thread" : "WhoKnows";
            Trace.WriteLine(String.Format("*** DeviceManager.DevChangeWnd_DeviceChangedMsg(): {0} {1}, {2}({3})", devEvent, devDetails, String.IsNullOrEmpty(Thread.CurrentThread.Name) ? "thread" : Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));
            DeviceChangedEventArgs[] eventArgs = null;
            ///
            /// Add/Remove the device from our static list of devices.
            ///
            switch (devEvent)
            {
                case DeviceChangeEvent.VolumeArrival:
                    eventArgs = VolumeDeviceClass.Instance.AddUsbDevice(devDetails, devEvent);
                    break;
                case DeviceChangeEvent.DeviceArrival:
                    eventArgs = Instance.AddUsbDevice(devDetails, devEvent);
                    break;
                case DeviceChangeEvent.HubArrival:
                    eventArgs = UsbHubClass.Instance.AddUsbDevice(devDetails, devEvent);
                    break;
                case DeviceChangeEvent.VolumeRemoval:
                    eventArgs = VolumeDeviceClass.Instance.RemoveUsbDevice(devDetails, devEvent);
                    break;
                case DeviceChangeEvent.DeviceRemoval:
                    eventArgs = Instance.RemoveUsbDevice(devDetails, devEvent);
                    break;
                case DeviceChangeEvent.HubRemoval:
                    eventArgs = UsbHubClass.Instance.RemoveUsbDevice(devDetails, devEvent);
                    break;
                default:
                    break;
            }

//            if (eventArgs == null)
//            {
//                eventArgs = new DeviceChangedEventArgs[1];
//                eventArgs[0] = new DeviceChangedEventArgs(devEvent, devDetails);
//            }

	        /// Signal anyone waiting on a device change to go see what changed. See FindDevice() for example.
            EventWaitHandle.Set();

            if (eventArgs != null)
            {
                ///
                /// Notify all interested parties.
                ///
                bool doNotify = false;
                foreach (DeviceChangeWatcherArgs watcher in _DeviceChangeCallbackList)
                {
                    foreach (DeviceChangedEventArgs eventArg in eventArgs)
                    {
                        switch (watcher.NotifyFilter)
                        {
                            case NotifyFilters.Any:
                                doNotify = true;
                                break;
                            case NotifyFilters.Device:
                                if (eventArg.DeviceId == watcher.DeviceId)
                                    doNotify = true;
                                break;
                            case NotifyFilters.DeviceType:
                                if (eventArg.DeviceType == watcher.DeviceType)
                                    doNotify = true;
                                break;
                            case NotifyFilters.Port:
                                if (eventArg.Port == watcher.Port && eventArg.Hub == watcher.Hub)
                                    doNotify = true;
                                break;
                            case NotifyFilters.None:
                            default:
                                break;
                        }

                        if (doNotify)
                        {
                            object[] args = new object[1] { eventArg };

                            if (watcher.Delegate.Target is ISynchronizeInvoke)
                                ((ISynchronizeInvoke)watcher.Delegate.Target).BeginInvoke(watcher.Delegate, args);
                            else
                                watcher.Delegate(eventArg);
                        }

                    } // foreach eventArg

                } // foreach watcher

            } // if ( eventArgs != null )
        }

        DeviceChangedEventArgs[] AddUsbDevice(String path, DeviceChangeEvent devEvent)
        {
            DeviceChangedEventArgs[] eventArgs = null;

	        foreach ( DeviceClass deviceClass in _DeviceClassMgrs )
	        {
	            // don't look for USB arrival in MSC class, wait for Volume arrival.
	            if ( !(deviceClass is VolumeDeviceClass) )
	            {
                    eventArgs = deviceClass.AddUsbDevice(path, devEvent);
                    if (eventArgs != null)
			            break;
                }
	        }

            return eventArgs;
        }

        DeviceChangedEventArgs[] RemoveUsbDevice(String path, DeviceChangeEvent devEvent)
        {
            DeviceChangedEventArgs[] eventArgs = null;

            foreach (DeviceClass deviceClass in _DeviceClassMgrs)
            {
		        // don't look for USB removal in MSC class, should have already happened by Volume removal.
		        if ( !(deviceClass is VolumeDeviceClass) )
		        {
                    eventArgs = deviceClass.RemoveUsbDevice(path, devEvent);
                    if (eventArgs != null)
                        break;
                }
            }

            return eventArgs;
        }

        public void Refresh()
        {
            foreach (DeviceClass deviceClass in _DeviceClassMgrs)
                deviceClass.Refresh();
        }

/*
        public Device FindDevice(Type devClass, UInt16 vid, UInt16? pid, uint timeout)
        {
            Debug.WriteLine(String.Format("+DeviceManager.FindDevice({0}, {1:X4}, {2:X4}, {3})\n", devClass==null ? "null" : devClass.Name, vid, pid==null ? "null" : pid.ToString(), timeout));

            Device device = FindDevice(devClass, vid, pid);

            if (device != null)
                return device;

            //
            // We didn't find a device on our first pass
            //
            // Just return right away if there was no wait period specified
            if ( timeout == 0 )
            {
                Debug.WriteLine("-DeviceManager::FindDevice() : No device, not waiting.\n");
                return null;
            }

            //
            // Since a timeout was specified, we need to hang out and see if a device arrives before the 
            // timeout expires.
            //
 	        // Reset the WaitHandle so we will know if the DeviceManager got a device arrival.
            // EventWaitHandle.Reset();

            while( device == null )
            {
//                bool index = EventWaitHandle.WaitOne();//timeout * 1000, false);
                IntPtr[] handles = new IntPtr[1] { EventWaitHandle.SafeWaitHandle.DangerousGetHandle() }; 
                int ret;
                if ((ret = Win32.MsgWaitForMultipleObjectsEx(1, handles, timeout * 1000, 0, 0)) == 0)
                {
                    // Timeout
                    break;
                }
                else
                {
                    // Device Change Event
                    device = FindDevice(devClass, vid, pid);
                    continue;
                }
            }
            
            Debug.WriteLine(String.Format("-DeviceManager::FindDevice() : {0}.\n", device == null ? "No device" : "Found device"));
            return device;
        }
*/
        public Device FindDevice(Type devClass, UInt16? vid, UInt16? pid)
        {
            // Look in specified device class
            if (devClass != null)
            {
                DeviceClass deviceClass = null;

                if (devClass.IsAssignableFrom(typeof(HidDeviceClass)))
                    deviceClass = HidDeviceClass.Instance;
                else if (devClass.IsAssignableFrom(typeof(WpdDeviceClass)) )
                    deviceClass = WpdDeviceClass.Instance;
                else if (devClass.IsAssignableFrom(typeof(VolumeDeviceClass)) )
                    deviceClass = VolumeDeviceClass.Instance;
                else if (devClass.IsAssignableFrom(typeof(RecoveryDeviceClass)))
                    deviceClass = RecoveryDeviceClass.Instance;
                else if (devClass.IsAssignableFrom(typeof(WinUsbDeviceClass)))
                    deviceClass = WinUsbDeviceClass.Instance;
                else if (devClass.IsAssignableFrom(typeof(MxRomDeviceClass)))
                    deviceClass = MxRomDeviceClass.Instance;

                if (deviceClass == null)
                {
                    Trace.WriteLine(String.Format("!ERROR: DeviceManager::FindDevice() can not find {0}\n", devClass));
                    return null;
                }
                else
                {
                    foreach (Device dev in deviceClass.Devices)
                    {
                        if (dev.Match(vid, pid))
                            return dev;
                    }
                }
            }
            // Look in all device classes
            else
            {
                foreach (DeviceClass dev_class in _DeviceClassMgrs)
                {
                    foreach (Device dev in dev_class.Devices)
                    {
                        if (dev.Match(vid, pid))
                            return dev;
                    }

                }
            }
            // didn't ever find it
            return null;
        }

        public static Device FindChildDevice(String driverName)
        {
//            Trace.WriteLine(String.Format("+UsbPort.FindChildDevice( \"{0}\" )", driverName));
            if (String.IsNullOrEmpty(driverName))
            {
                return null;
            }

            UsbDeviceClass usbDevMgr = UsbDeviceClass.Instance;
            usbDevMgr.Refresh();

            Device tempDev = usbDevMgr.FindDeviceByDriver(driverName);
//            Trace.WriteLine(String.Format(" tempDev.Class: {0}, tempDev.UsbDevice.Driver: {1}", tempDev.Class, tempDev.UsbDevice.Driver));

            while (tempDev.Child != null)
                tempDev = tempDev.Child;

//            Trace.WriteLine(String.Format("UsbPort.FindChildDevice() - DevInst:{0}, Path:{1}", tempDev.DeviceInfoData.devInst, tempDev.Path));

            Device dev;
            if (Win32.GUID_DEVCLASS_STMP3XXX_USB_BULK_DEVICE == tempDev.ClassGuid)
            {
                RecoveryDeviceClass devMgr = RecoveryDeviceClass.Instance;
                dev = devMgr.FindDeviceByDriver(tempDev.Driver);// CreateDevice(tempDev.DeviceInfoData, tempDev.Path);
//                Trace.WriteLine(String.Format(" RecoveryDeviceClass - DevInst:{0}, Path:{1}", dev.DeviceInstance, dev.Path));
            }
            else if (Win32.GUID_DEVINTERFACE_WINUSB_BULK_DEVICE == tempDev.ClassGuid)
            {
                WinUsbDeviceClass devMgr = WinUsbDeviceClass.Instance;
                dev = devMgr.FindDeviceByDriver(tempDev.Driver);// CreateDevice(tempDev.DeviceInfoData, tempDev.Path);
//                Trace.WriteLine(String.Format(" WinUsbDevice - DevInst:{0}, Path:{1}", dev.DeviceInstance, dev.Path));
            }
            else if (Win32.GUID_DEVCLASS_HIDCLASS == tempDev.ClassGuid)
            {
                HidDeviceClass devMgr = HidDeviceClass.Instance;
                dev = devMgr.FindDeviceByDriver(tempDev.Driver);// CreateDevice(tempDev.DeviceInfoData, tempDev.Path);
//                Trace.WriteLine(String.Format(" HidDevice - DevInst:{0}, Path:{1}", dev.DeviceInstance, dev.Path));
            }
            else if (Win32.GUID_DEVCLASS_VOLUME == tempDev.ClassGuid)
            {
                VolumeDeviceClass devMgr = VolumeDeviceClass.Instance;
                dev = devMgr.FindDeviceByDriver(tempDev.Driver);// CreateDevice(tempDev.DeviceInfoData, tempDev.Path);
                Trace.WriteLine(String.Format(" Volume - DevInst:{0}, Path:{1}", dev.DeviceInstance, dev.Path));
            }
            else if (Win32.GUID_DEVCLASS_WPD == tempDev.ClassGuid)
            {
                WpdDeviceClass devMgr = WpdDeviceClass.Instance;
                dev = devMgr.FindDeviceByDriver(tempDev.Driver);// CreateDevice(tempDev.DeviceInfoData, tempDev.Path);
//                Trace.WriteLine(String.Format(" WpdDevice - DevInst:{0}, Path:{1}", dev.DeviceInstance, dev.Path));
            }
            else
            {
                dev = tempDev;
//                Trace.WriteLine(String.Format(" Device - DevInst:{0}, Path:{1}", dev.DeviceInstance, dev.Path));
            }

            return dev;
        }

        /// <summary>
        /// Finds the Device Instance Handle by traversing the DevNode tree in the registry.
        /// </summary>
        /// <param name="driverToFind">the driver key name of the device to find.</param>
        /// <returns>IntPtr  DeviceInstanceHandle</returns>
        public static IntPtr FindDeviceInstance(String driverToFind)
        {
            IntPtr devInst;
            IntPtr devInstNext;
            Win32.CONFIGRET cr;
            bool walkDone = false;
            int len;

            // Get Root DevNode
            //
            cr = Win32.CM_Locate_DevNode(out devInst, null, 0);

            if (cr != Win32.CONFIGRET.CR_SUCCESS)
            {
                return IntPtr.Zero;
            }

            // Do a depth first search for the DevNode with a matching
            // DriverName value
            //
            while (!walkDone)
            {
                // Get the DriverName value
                //
                StringBuilder buf = new StringBuilder(1024);
                len = buf.Capacity;
                cr = Win32.CM_Get_DevNode_Registry_Property(
                    devInst,
                    Win32.CM_DRP_DRIVER,
                    IntPtr.Zero,
                    buf,
                    ref len,
                    0);

                // If the DriverName value matches, return the DeviceInstance
                //
                if (cr == Win32.CONFIGRET.CR_SUCCESS && String.Compare(buf.ToString(), driverToFind, true) == 0)
                {
                    // SUCCESS
                    return devInst;
                }

                // This DevNode didn't match, go down a level to the first child.
                //
                cr = Win32.CM_Get_Child( out devInstNext, devInst, 0);

                if (cr == Win32.CONFIGRET.CR_SUCCESS)
                {
                    devInst = devInstNext;
                    continue;
                }

                // Can't go down any further, go across to the next sibling.  If
                // there are no more siblings, go back up until there is a sibling.
                // If we can't go up any further, we're back at the root and we're
                // done.
                //
                for (; ; )
                {
                    cr = Win32.CM_Get_Sibling( out devInstNext, devInst, 0);

                    if (cr == Win32.CONFIGRET.CR_SUCCESS)
                    {
                        devInst = devInstNext;
                        break;
                    }

                    cr = Win32.CM_Get_Parent( out devInstNext, devInst, 0);

                    if (cr == Win32.CONFIGRET.CR_SUCCESS)
                    {
                        devInst = devInstNext;
                    }
                    else
                    {
                        walkDone = true;
                        break;
                    }
                }
            
            } // while ( ! walkdone )

            return IntPtr.Zero;
        }


    } // public class DeviceManager : IDisposable

    public class DeviceChangeWatcherArgs : IEquatable<DeviceChangeWatcherArgs>
    {
        public DeviceChangeWatcherArgs(DeviceManager.DeviceChangedEventHandler method)
            : this(0, 0, null, String.Empty, NotifyFilters.Any, method)
        { }

        public DeviceChangeWatcherArgs(int hub, int port, DeviceManager.DeviceChangedEventHandler method)
            : this(hub, port, null, String.Empty, NotifyFilters.Port, method)
        { }

        public DeviceChangeWatcherArgs(String deviceId, DeviceManager.DeviceChangedEventHandler method)
            : this(0, 0, null, String.Empty, NotifyFilters.Device, method)
        { }

        public DeviceChangeWatcherArgs(Type deviceClass, DeviceManager.DeviceChangedEventHandler method)
            : this(0, 0, deviceClass, String.Empty, NotifyFilters.DeviceType, method)
        { }

        public DeviceChangeWatcherArgs(
            int hub,
            int port,
            Type deviceClass,
            String deviceId,
            NotifyFilters notifyFilter,
            DeviceManager.DeviceChangedEventHandler method)
        {
            this._Hub = hub;
            this._Port = port;
            this._DeviceType = deviceClass;
            this._DeviceId = deviceId;
            this._NotifyFilter = notifyFilter;
            this._Delegate = method;
            this._WatcherId = Guid.NewGuid();
        }

        public int Port
        {
            get { return _Port; }
        }
        private int _Port;

        public int Hub
        {
            get { return _Hub; }
        }
        private int _Hub;

        public Type DeviceType
        {
            get { return _DeviceType; }
        }
        private Type _DeviceType;

        public String DeviceId
        {
            get { return _DeviceId; }
        }
        private String _DeviceId;

        public NotifyFilters NotifyFilter
        {
            get { return _NotifyFilter; }
        }
        private NotifyFilters _NotifyFilter;

        public Guid WatcherId
        {
            get { return _WatcherId; }
        }
        private Guid _WatcherId;

        public DeviceManager.DeviceChangedEventHandler Delegate
        {
            get { return _Delegate; }
        }
        private DeviceManager.DeviceChangedEventHandler _Delegate;

        #region IEquatable<DeviceChangeWatcherArgs> Members

        public bool Equals(DeviceChangeWatcherArgs other)
        {
            return Equals(this.Delegate, other.Delegate);
        }

        #endregion
    }

    public class DeviceChangedEventArgs : EventArgs
    {
        public DeviceChangedEventArgs(
            DeviceChangeEvent eventType,
            String deviceId,
            Type deviceType,
            int hub,
            int port)
        {
            this._Event = eventType;
            this._DeviceId = deviceId;
            this._DeviceType = deviceType;
            this._Hub = hub;
            this._Port = port;
        }

        public DeviceChangeEvent Event
        {
            get { return _Event; }
        }
        private DeviceChangeEvent _Event;

        public String DeviceId
        {
            get { return _DeviceId; }
        }
        private String _DeviceId;

        public int Port
        {
            get { return _Port; }
        }
        private int _Port;

        public int Hub
        {
            get { return _Hub; }
        }
        private int _Hub;

        public Type DeviceType
        {
            get { return _DeviceType; }
        }
        private Type _DeviceType;
    }

    public delegate void CancelAutoPlayEventHandler(Object message);
    public class CancelAutoPlayEventArgs : EventArgs
    {
        public CancelAutoPlayEventArgs(
            String path,
            String label,
            String type,
            UInt32 serialNo)
        {
            this._Path = path;
            this._Label = label;
            this._Type = type;
            this._SerialNo = serialNo;
        }

        public String Path
        {
            get { return _Path; }
        }
        private String _Path;

        public String Label
        {
            get { return _Label; }
        }
        private String _Label;

        public String Type
        {
            get { return _Type; }
        }
        private String _Type;

        public UInt32 SerialNo
        {
            get { return _SerialNo; }
        }
        private UInt32 _SerialNo;
    }

} // namespace DevSupport.DeviceManager
