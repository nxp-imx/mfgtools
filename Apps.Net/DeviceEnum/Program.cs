using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Windows.Forms;
using Microsoft.Win32;

namespace DeviceEnum
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [MTAThread]
        static void Main()
        {
            AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(CurrentDomain_AssemblyResolve); 
            
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainWindow());
        }

        static System.Reflection.Assembly CurrentDomain_AssemblyResolve(object sender, ResolveEventArgs args)
        {
            string[] asmName = args.Name.Split(',');
            string sharedPath = Registry.GetValue(@"HKEY_LOCAL_MACHINE\Software\Microsoft\.NETFramework\AssemblyFolders\Freescale DevSupport Reference Assemblies",
                                                String.Empty, String.Empty).ToString();
            if (sharedPath == String.Empty) throw (new Exception("Path to shared libraries not found."));
            string asmPath = Path.Combine(sharedPath, asmName[0] + ".dll");
            if (!File.Exists(asmPath)) throw (new Exception("Assembly " + asmName[0] + " not found."));
            return Assembly.LoadFile(asmPath, Assembly.GetExecutingAssembly().Evidence);
        }
    }
}

