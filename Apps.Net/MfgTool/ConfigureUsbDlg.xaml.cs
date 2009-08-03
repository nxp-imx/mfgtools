using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

using DevSupport.USB;
using DevSupport.UI;

namespace MfgTool
{
    /// <summary>
    /// Interaction logic for ConfigureUsbDlg.xaml
    /// </summary>
    public partial class ConfigureUsbDlg : Window
    {
        readonly UsbTreeViewModel _UsbTree;

        public ConfigureUsbDlg(Window parent, Collection<PortPanel> portPanels)
        {
            InitializeComponent();

            this.Owner = parent;

            // Create UI-friendly wrappers around the 
            // raw data objects (i.e. the view-model).
            _UsbTree = new UsbTreeViewModel(portPanels);

            // Let the UI bind to the view-model.
            base.DataContext = _UsbTree;
        }

    }
}
