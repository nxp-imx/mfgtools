================================================================================
    StFwLoader Project Overview
===============================================================================

History

version 1.8
	Added Drag and Drop support. Drag a file onto the main window and it will be added to the file 
	list-box and selected as the current file to download.
	
version 1.7
	Changed the drop-down list of the combo box to show the longest text in the list. 
	This makes it easier for a user to know what is being selecting.
	
	Fixed the initial "Always on top" operation. Window setting did not reflect control setting.
	Fixed the initial "Reject AutoPlay" operation. Application setting did not reflect control setting.
	
version 1.6
	Added Win9x support - requires stunicow.dll to be in the same directory as stfwloader.exe.
	The unicode dll can be found here: ..\..\Libs\StUnicoWS\StUnicoWS.dll 

	Added command-line operation shown below:

		Usage: StFwLoader.exe [/f filename] [/w seconds] [/s] [/h]
		where:
				/f filename	- Specify the firmware file to download. Looks in the application
				              directory if the path is not specified.
				/w seconds  - Specifies the length of time to wait for a recovery-mode device.
				              The default is 30 seconds if not specified.
				/s          - Silent mode. Suppress message boxes used to display errors.
				/h          - Display this help screen.
		
		returns: 
				success: ERRORLEVEL == 0
				error:   ERRORLEVEL != 0

version 1.5
	GUI reports errors if they occur during DownLoad().
	
version 1.4
	Displays decimal version numbers for both old-style firmware files and 3600-style firmware files.
	Thanks for the BCD2D routine from C. Reed.
 
version 1.3
	Supports new version number (header) scheme and displays the "STMP" or "RSRC" file tag.
