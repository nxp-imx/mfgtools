using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Windows.Forms;

using DevSupport;
using DevSupport.Api;
using DevSupport.DeviceManager;

namespace BitInit
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine(Header);

            if (!ProcessCommandLine(args))
            {
                Console.WriteLine(Usage);
                return;
            }

            if (!File.Exists(Firmware))
            {
                Console.WriteLine(String.Format("Can not find firmware file {0}.", Firmware));
                return;
            }

            // See  if the device is present.
            CurrentDevice = (HidDevice)DeviceManager.Instance.FindDevice(typeof(HidDeviceClass), 0x066f, null);
            if (CurrentDevice != null)
            {
                CurrentDeviceId = CurrentDevice.DeviceInstanceId;
                RunState = RunStateType.Ready;
            }

            // If there is no device and we are not in one of the 'wait' RunModes, then exit.
            if ((RunState != RunStateType.Ready) && (RunMode == RunModeType.OneShotNoWait))
            {
                Console.WriteLine("Nothing to do. Exiting.");
                return;
            }

            //
            // We must be in a 'wait' mode, so register for DeviceArrival and update UI while waiting.
            //
            // Register for callback when a device arrives.
            DeviceManager.Instance.DeviceChanged += new DeviceManager.DeviceChangedEventHandler(DeviceManager_DeviceChanged);

            // Set a timer to update the UI while we wait.
            TimerCallback uiTimerDelegate = new TimerCallback(OnUITimer);
            UITimer = new System.Threading.Timer(uiTimerDelegate, null, 100, 100);

            // If this is a one time load, set a timer to let us know when Timeout expires.
            if ( RunMode == RunModeType.OneShot )
            {
                TimerCallback timeoutDelegate = new TimerCallback(OnTimeout);
                TimeoutTimer = new System.Threading.Timer(timeoutDelegate, null, Timeout, TimeSpan.FromMilliseconds(-1));
            }

            // Our wait loop. Check for QUIT or READY. Pump messages for DeviceManager.
            // The rest of the work is done in the OnUITimer, OnTimeout, and OnDeviceChange delegates.
            while (RunState != RunStateType.Done)
            {
                // Check for QUIT
                if (Console.KeyAvailable)
                {
                    if (Console.ReadKey(true).Key == ConsoleKey.Q)
                    {
                        RunState = RunStateType.Done;
                        UpdateStatus();
                        break;
                    }
                }

                if (RunState == RunStateType.Ready)
                {
                    Console.CursorLeft = 0;
                    Console.Write("Initializing {0} : ", CurrentDeviceId);
                    Int32 retCode = CurrentDevice.InitializeOtpRegs(Firmware);
                    if (retCode != 0)
                    {
                        Console.WriteLine(String.Format("FAIL Code: 0x{0:X8}({0})", retCode));
                        Console.WriteLine(CurrentDevice.ErrorString);
                    }
                    else
                    {
                        Console.WriteLine("PASS");
                    }

                    if (RunMode == RunModeType.Continuous)
                        RunState = RunStateType.WaitingForDisconnect;
                    else
                        RunState = RunStateType.Done;

                    UpdateStatus();
                }

                // Pump messages : 
                // This is necessary so that the DeviceManager gets his WM_DEVICECHANGE messages in a console app.
                Application.DoEvents();
            }

        }

        static public void OnUITimer(Object stateInfo)
        {
            // Change UI
            UpdateStatus();
        }

        static public void OnTimeout(Object stateInfo)
        {
            // Timeout expired so don't wait any more.
            Console.CursorLeft = 0;
            Console.WriteLine("Timeout expired. Exiting.                         ");
            RunState = RunStateType.Done;
            UpdateStatus();
        }

        static void DeviceManager_DeviceChanged(DeviceChangedEventArgs e)
        {
            if ( e.Event == DeviceChangeEvent.DeviceArrival )
            {
                // If we don't have a current device, see if the one arrived will work for us.
                if (String.IsNullOrEmpty(CurrentDeviceId))
                {
                    CurrentDevice = (HidDevice)DeviceManager.Instance.FindDevice(typeof(HidDeviceClass), 0x066f, null);

                    if (CurrentDevice != null)
                    {
                        CurrentDeviceId = CurrentDevice.DeviceInstanceId;
                        RunState = RunStateType.Ready;
                        UpdateStatus();
                    }
                }
            }
            else if (e.Event == DeviceChangeEvent.DeviceRemoval)
            {
                // Check our current DeviceId against the device that just left.
                if (!String.IsNullOrEmpty(CurrentDeviceId))
                {
                    // If it was our device, reset our states.
                    if ( String.Compare(e.DeviceId, CurrentDeviceId, true) == 0 )
                    {
                        CurrentDevice = null;
                        CurrentDeviceId = String.Empty;
                        RunState = RunStateType.WaitingForConnect;
                        UpdateStatus();
                    }
                }
            }
        }

        static private Char[] WaitSymbols = new Char[] {'|', '/', '-', '\\'};
        static private int StatePosition = 0;

        static void UpdateStatus()
        {
            lock(Lock)
            {
                switch (RunState)
                {
                    case RunStateType.WaitingForConnect:
                        StatePosition = 0;
                        Console.CursorLeft = 0;
                        Console.Write("Please connect device.    " + WaitSymbols[StatePosition++ % WaitSymbols.Length]);
                        RunState = RunStateType.WaitingForConnectPlus;
                        break;
                    case RunStateType.WaitingForConnectPlus:
                        Console.Write("\b{0}", WaitSymbols[StatePosition++ % WaitSymbols.Length]);
                        break;
                    case RunStateType.WaitingForDisconnect:
                        StatePosition = 0;
                        Console.CursorLeft = 0;
                        Console.Write("Please disconnect device. " + WaitSymbols[StatePosition++ % WaitSymbols.Length]);
                        RunState = RunStateType.WaitingForDisconnectPlus;
                        break;
                    case RunStateType.WaitingForDisconnectPlus:
                        Console.Write("\b{0}", WaitSymbols[StatePosition++ % WaitSymbols.Length]);
                        break;
                    case RunStateType.Done:
                        Console.CursorLeft = 0;
                        Console.WriteLine("                                              ");
                        break;
                    case RunStateType.Ready:
                        break;
                    default:
                        break;
                }
            }
        }

        static bool ProcessCommandLine(string[] args)
        {
            Int32 timeout = 0;

            foreach (String arg in args)
            {
                // /loop
                if ( arg.ToLower() == "/loop" )
                {
                    RunMode = RunModeType.Continuous;
                    continue;
                }
                // <timeout>
                else if ( Int32.TryParse(arg, out timeout) )
                {
                    Timeout = TimeSpan.FromSeconds(Int32.Parse(arg));

                    if (RunMode != RunModeType.Continuous)
                    {
                        if (Timeout == TimeSpan.Zero)
                            RunMode = RunModeType.OneShotNoWait;
                        else
                            RunMode = RunModeType.OneShot;
                    }
                    continue;
                }
                // /fw=<filename>
                else if ( arg.ToLower().Contains("/fw=") )
                {
                    String[] parts = arg.Split('=');
                    if (parts.Length == 2)
                    {
                        Firmware = parts[1];
                        if (!File.Exists(Firmware))
                        {
                            Console.WriteLine(String.Format("Can not find firmware file {0}.", Firmware));
                            return false;
                        }
                    }
                    else
                    {
                        Console.WriteLine("Invalid <filename> for this option.");
                        return false;
                    }
                }
                // /log[=<filename>]
                else if ( arg.ToLower().Contains("/log") )
                {
                    LogFile = "bitinit_log.txt";

                    String[] parts = arg.Split('=');
                    if (parts.Length == 2)
                    {
                        LogFile = parts[1];
                    }
                }
                // /? /h
                else if ( (arg == "/?") || (arg.ToLower() == "/h") )
                {
                    return false;
                }
                else
                {
                    Console.WriteLine(String.Format("Unrecognized command line option: {0}.", arg));
                    return false;
                }
            }
            return true;
        }

        public enum RunModeType { OneShot, OneShotNoWait, Continuous };
        public enum RunStateType { WaitingForDisconnect, WaitingForDisconnectPlus, WaitingForConnect, WaitingForConnectPlus, Ready, Done };
        
        public static String Header
        {
            get
            {
                String header = "\n" +
                  String.Format("BitInit.exe (Version {0})\n", Assembly.GetExecutingAssembly().GetName().Version);

                return header;
            }
        }

        public static String Usage
        {
            get
            {
                String usage = "\n" +
                               "USAGE: otpinit.exe [<timeout> | /loop] [/fw=<filename>] [/log[=<filename>]] [/?]\n\n" +
                               " where:\n\n" +
                               " <timeout>         : Specifies number of seconds to wait for a device before\n" +
                               "                      exiting the program. The default is 10 seconds. This\n" +
                               "                      option will be ignored if /loop is specified.\n" +
                               "                      Specify 0 to not wait for the device if it is not present.\n\n" +
                               " /loop             : The application will continually program otp registers on\n" +
                               "                      devices as they arrive. Press 'Q' to quit.\n\n" +
                               " /fw=<filename>    : Specifies the firmware image to load. OtpInit.sb will be\n" +
                               "                      loaded if this option is not specified.\n\n" +
//                               " /log[=<filename>] : Log the results to a file. If <filename> is not specified,\n" +
//                               "                      results will be logged to bitinit_log.txt\n\n" +
                               " /?                : Prints this usage screen.\n";

                return usage;
            }
        }

        private static HidDevice CurrentDevice = null;
        private static String CurrentDeviceId = String.Empty;
        private static String Firmware = "otpinit.sb";
        private static String LogFile = null;
        private static RunModeType RunMode = RunModeType.OneShot;
        private static RunStateType RunState = RunStateType.WaitingForConnect;
        private static TimeSpan Timeout = TimeSpan.FromSeconds(10);
        private static System.Threading.Timer UITimer = null;
        private static System.Threading.Timer TimeoutTimer = null;
        private static Object Lock = new Object();

    } // class Program

}
