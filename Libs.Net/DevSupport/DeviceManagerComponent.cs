using System;
using System.ComponentModel;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace DevSupport.DeviceManager
{
    public delegate void DeviceChangedEventHandler(object sender, DeviceChangedEventArgs e);
    public delegate void AutoPlayEventHandler(object sender, AutoPlayEventArgs e);

    public partial class DeviceManagerComponent : Component
    {
        public event DeviceChangedEventHandler DeviceChanged;
        public event AutoPlayEventHandler CancelledAutoPlay;

        private static DeviceChangeWindow devChangeWnd = new DeviceChangeWindow();
        
        public DeviceManagerComponent()
        {
            InitializeComponent();

            this.backgroundWorker1.RunWorkerAsync(this);
        }

        public DeviceManagerComponent(IContainer container)
        {
            container.Add(this);

            InitializeComponent();
            this.backgroundWorker1.RunWorkerAsync(this);
        }

        private delegate void WorkerEventHandler(
           int numberToCheck,
           AsyncOperation asyncOp,
           SendOrPostCallback completionMethodDelegate);

        private WorkerEventHandler workerDelegate;
        
        private void backgroundWorker1_DoWork(object sender, DoWorkEventArgs e)
        {
            Thread.CurrentThread.Name = "DeviceManagerComponent Thread"; 
            Trace.WriteLine(String.Format("DeviceManagerComponent.ThreadProc(), {0}({1})", Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));

            try
            {
/*                devChangeWnd = new MsgWindow();
                // Makes WndProc start getting messages
                CreateParams createParams = new CreateParams();
                createParams.ClassName = "STATIC";
                CreateHandle(createParams);

                // register for usb devices notifications
                Win32.DEV_BROADCAST_DEVICEINTERFACE_NONAME broadcastInterface = new Win32.DEV_BROADCAST_DEVICEINTERFACE_NONAME();
                broadcastInterface.dbcc_devicetype = Win32.DBT_DEVTYP_DEVICEINTERFACE;
                broadcastInterface.dbcc_classguid = Win32.GUID_DEVINTERFACE_USB_DEVICE;
                IntPtr buffer = Marshal.AllocHGlobal(new IntPtr(broadcastInterface.dbcc_size));
                Marshal.StructureToPtr(broadcastInterface, buffer, false);
                _UsbDevNotifyHandle = Win32.RegisterDeviceNotification(devChangeWnd.Handle, buffer, Win32.DEVICE_NOTIFY_WINDOW_HANDLE);

                // register for usb hub notifications
                broadcastInterface.dbcc_classguid = Win32.GUID_DEVINTERFACE_USB_HUB;
                Marshal.StructureToPtr(broadcastInterface, buffer, true);
                _UsbHubNotifyHandle = Win32.RegisterDeviceNotification(devChangeWnd.Handle, buffer, Win32.DEVICE_NOTIFY_WINDOW_HANDLE);

                Marshal.FreeHGlobal(buffer);

                // Add an event handler that we can use to address Device Chnage messages in our own thread.
                devChangeWnd.DeviceChanged += new InternalDeviceChangeHandler(OnDeviceChangedEvent);
*/
                // The main loop that runs until _CloseRequested == true
                while (this.backgroundWorker1.CancellationPending == false)
                {
                    Application.DoEvents();
                    Thread.Sleep(100);
                }
            }
            catch (Exception exp)
            {
                Trace.WriteLine(exp.Message);
                throw exp;
            }
//            finally
//            {
//                Dispose();
//            }
            Trace.WriteLine(String.Format("Leaving DeviceManagerComponent.ThreadProc(), {0}({1})", Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));

        }
        
        private void OnDeviceChangedEvent(DeviceChangeEvent devEvent, String devDetails)
        {
            DeviceChangedEventArgs dcArgs = new DeviceChangedEventArgs(0, 0, devEvent, null, NotifyFilters.All, null, false, devDetails);

            // Notify anyone who cares.
            if (DeviceChanged != null)
            {
                DeviceChanged.Invoke(this, dcArgs);
            }

            Trace.WriteLine(String.Format("DeviceManager.Instance.OnDeviceChangedEvent(), {0}, {1}, {2}({3})", devEvent, devDetails, Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));
        }
    }

}
