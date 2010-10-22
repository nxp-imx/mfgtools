//
// Name: genCSinc.js
// Copyright: Microsoft 2006
// Revision: 1.0
//
// This script can be used to generate a C# .cs file that contains
// the equivalent WPD property-keys and GUIDs as defined in
// portabledevice.h
// This script is provided as-is and Microsoft does not assume any
// liability. This script may be redistributed as long as the file
// contains these terms of use unmodified.
//
// Usage:
// Switch to the folder where portabledevice.h is present and then run
//     cscript //nologo genCSinc.js
// This will generate portabledeviceconstants.cs. This .cs file can
// then be included in a C# project. 
// To use the propertykeys and the GUIDs, add "using PortableDeviceConstants;"
// to the target file.
// Propertykeys can then be referenced as PortableDevicePKeys.desired_prop_name
//     e.g. PortableDevicePKeys.WPD_PROPERTY_COMMON_HRESULT
// GUIDs can be referenced as PortableDeviceGuids.desired_GUID_name
//     e.g. PortableDeviceGuids.WPD_OBJECT_FORMAT_ALL
//

//
// Use FSO to read/write input/output
//
var fso = new ActiveXObject("Scripting.FileSystemObject");

var f = null;
var fOut = null;
var sIn = "portabledevice.h";
var sOut = "portabledeviceconstants.cs";

//
// Check for input file
//
try
{
    f = fso.OpenTextFile(sIn);
}
catch(e)
{
    WScript.Echo("Expected portabledevice.h to be in the current folder!");
}

//
// Check for output file
//
try
{
    fOut = fso.OpenTextFile(sOut, 2, true);
}
catch(e)
{
    WScript.Echo("Cannot open " + sOut + " for writing!");
}

//
// Write out header
//
fOut.Write("\
using System;\r\n\
\r\n\
namespace PortableDeviceConstants\r\n\
{\r\n\
    class PortableDevicePKeys\r\n\
    {\r\n\
        static PortableDevicePKeys()\r\n\
        {\r\n\
");    

//
// RegEx declarations for PKEYs and GUIDs
//
//e.g. DEFINE_PROPERTYKEY( WPD_CLIENT_MINOR_VERSION , 0x204D9F0C, 0x2292, 0x4080, 0x9F, 0x42, 0x40, 0x66, 0x4E, 0x70, 0xF8, 0x59 , 4 ); 
var rePKEY = /\s*DEFINE_PROPERTYKEY\(\s*(\w+)\s*,\s*(.+)\s*,\s*(\d+)\s*\);/;
var arrPKEY = new Array();

//e.g. DEFINE_GUID(WPD_EVENT_DEVICE_RESET, 0x7755CF53, 0xC1ED, 0x44F3, 0xB5, 0xA2, 0x45, 0x1E, 0x2C, 0x37, 0x6B, 0x27 ); 
var reGUID = /\s*DEFINE_GUID\((\w+),\s(.+)\s\);/;
var arrGUID = new Array();

//
// Parse the file
//
while (!f.AtEndOfStream)
{
    var l = f.ReadLine();

    //
    // Check for PKEYs
    //
    if (l.match(rePKEY))
    {
        //
        // Write out initializations for the PKEYs
        //
        var sName = l.replace(rePKEY, "$1");
        var sGUID = l.replace(rePKEY, "$2");
        var sPID = l.replace(rePKEY, "$3");

        //WPD_CLIENT_NAME.fmtid = new Guid(0x204D9F0C, 0x2292, 0x4080, 0x9F, 0x42, 0x40, 0x66, 0x4E, 0x70, 0xF8, 0x59);
        //WPD_CLIENT_NAME.pid = 2; 
        
        fOut.Write("            " + sName + ".fmtid = new Guid( " + sGUID + ");\r\n");
        fOut.Write("            " + sName + ".pid = " + sPID + ";\r\n");
        fOut.Write("\r\n");

        //
        // Save the PKEY name for declaration at class level
        //
        arrPKEY.push(sName);
    }
    else if (l.match(reGUID))
    {
        //
        // Save the GUIDs since they go into a second class
        //
        var sName = l.replace(reGUID, "$1");
        var sGUID = l.replace(reGUID, "$2");

        arrGUID.push("public static Guid " + sName + " = new Guid( " + sGUID + " );");
    }
}

//
// Write out declarations for PKEYs
//
fOut.Write("        }\r\n\r\n");

for (var i = 0; i < arrPKEY.length; i++)
{
    fOut.Write("        " + "public static PortableDeviceApiLib._tagpropertykey " + arrPKEY[i] + ";\r\n");
}

fOut.Write("\
    } // class PortableDevicePKeys\r\n\
");

//
// Write out GUIDs
//
fOut.Write("\r\n\r\n");
fOut.Write("\
    class PortableDeviceGuids\r\n\
    {\r\n\
");

for (var i = 0; i < arrGUID.length; i++)
{
    fOut.Write("        " + arrGUID[i] + "\r\n");
}

//
// Write out footer
//
fOut.Write("\
    } // class PortableDeviceGuids\r\n\
} // namespace PortableDeviceConstants\r\n\
");

WScript.Echo("Done: " + sOut + " now contains C# WPD constants");
