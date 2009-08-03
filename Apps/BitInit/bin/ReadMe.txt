========================================================================
    CONSOLE APPLICATION : BitInit Project Overview
========================================================================

1. Looks for a HID device with PID:066f VID:3700 for 10 seconds.
2. Loads OtpInit.sb from the same directory as the BitInit.exe file into OCRAM.
3. Issues the InitOtpRegs command to burn the OTP registers to the values used to construct OtpInit.sb.