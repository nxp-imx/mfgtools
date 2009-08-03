using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Diagnostics;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Threading;

using DevSupport.DeviceManager;
using DevSupport.UI;

namespace MfgTool
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private static DeviceManager MyDeviceManager;
        private Collection<PortPanel> PortPanels = new Collection<PortPanel>();

        public MainWindow()
        {
            Thread.CurrentThread.Name = "MainWindow thread";
            Trace.TraceInformation("MainWindow() - {0}({1})", Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode());

            InitializeComponent();

            PortPanels.Add(this.PanelA);
            PortPanels.Add(this.PanelB);
            PortPanels.Add(this.PanelC);
            PortPanels.Add(this.PanelD);

            // test code to convert system brushes to hex number
            SolidBrush brush = (SolidBrush)System.Drawing.SystemBrushes.InactiveBorder;
            string hex = string.Format("#{0}", brush.ToString().Substring(3)); 

        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            MyDeviceManager = DeviceManager.Instance;
            MyDeviceManager.DeviceChanged += new DeviceManager.DeviceChangedEventHandler(MyDeviceManager_DeviceChanged);
            MyDeviceManager.CancelAutoPlay += new CancelAutoPlayEventHandler(MyDeviceManager_CancelAutoPlay);
        }

        void MyDeviceManager_CancelAutoPlay(object args)
        {
            CancelAutoPlayEventArgs capeArgs = args as CancelAutoPlayEventArgs;
            String logStr = String.Format("Rejected \"{0}\" AutoPlay event for device: {1}( {2} )\r", capeArgs.Type, capeArgs.Label, capeArgs.Path);
            Trace.WriteLine(String.Format("*** MainWindow.OnCancelAutoPlayNotify(): {0}, {1}({2})", logStr, Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));
//            textBox.AppendText(logStr);
        }

        void MyDeviceManager_DeviceChanged(object eventArgs)
        {
            DeviceChangedEventArgs devChange = (DeviceChangedEventArgs)eventArgs;
            Trace.WriteLine(String.Format("*** MainWindow.DeviceManager_DeviceChanged(): {0}: {1}, {2}({3})", devChange.Event, devChange.DeviceId, Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));
            String logStr = devChange.Event + ": " + devChange.DeviceId + "\r"; ;
//            textBox.AppendText(logStr);
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            MyDeviceManager.DeviceChanged -= MyDeviceManager_DeviceChanged;
            MyDeviceManager.Dispose();
        }

        private void OnFileExit(object sender, RoutedEventArgs e)
        {
            this.Close();
        }

        private void OnOptionsConfiguration(object sender, RoutedEventArgs e)
        {
            // Instantiate the dialog box
            ConfigureUsbDlg dlg = new ConfigureUsbDlg(this, this.PortPanels);

            // Open the dialog box modally 
            dlg.ShowDialog();
        }

    }
}
