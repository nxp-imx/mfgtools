
http://blogs.msdn.com/dimeby8/archive/2006/12/06/where-are-the-wpd-property-keys-in-c.aspx


Since defining each of these property keys by hand may cause severe RSI, here's a little 
script that will generate the definitions for you given PortableDevice.h. In addition to 
generating the property keys, it will also generate the GUID definitions 
(such as for WPD_FUNCTIONAL_CATEGORY_STORAGE, WPD_OBJECT_FORMAT_WMA, etc.)

Copy the script from the text-box above, paste it into Notepad and save it as gencsinc.js. 
Copy PortableDevice.h to the same folder as gencsinc.js and then run gencsinc.js using "cscript gencsinc.js". 
This will generate a PortableDeviceConstants.cs file which you may add to your C# project.

Once the generated file is added to your project, add "using PortableDeviceConstants;" 
to your target C# source. To reference property keys, you can simply use 
PortableDevicePKeys.propertyname (e.g. PortableDevicePKeys.WPD_OBJECT_ID) and to 
reference GUIDs, you can simply use PortableDeviceGuids.guidname (e.g. PortableDeviceGuids.WPD_OBJECT_FORMAT_WMA).

