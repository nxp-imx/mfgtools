========================================================================
    CONSOLE APPLICATION : BitInit Overview
========================================================================

Version 2.1

Operation:

 1. Looks for a HID device with PID:066f VID:xxxx for 10 seconds.
 2. Loads OtpInit.sb from the same directory as the BitInit.exe file into OCRAM.
 3. Issues the InitOtpRegs command to burn the OTP registers to the values used to construct OtpInit.sb.


USAGE: otpinit.exe [<timeout> | /loop] [/fw=<filename>] [/log[=<filename>]] [/?]

 where:

 <timeout>         : Specifies number of seconds to wait for a device before
                      exiting the program. The default is 10 seconds. This
                      option will be ignored if /loop is specified.
                      Specify 0 to not wait for the device if it is not present.

 /loop             : The application will continually program otp registers on
                      devices as they arrive. Press 'Q' to quit.

 /fw=<filename>    : Specifies the firmware image to load. OtpInit.sb will be
                      loaded if this option is not specified.

 /?                : Prints this usage screen.


History

Version 2.1
	Added error checking after OTP initialization.

Version 2.0 
	Initial .Net release.