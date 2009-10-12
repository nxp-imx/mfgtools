#include "..\stdafx.h"
#include <stdio.h>
#include <string.h>
//#include "../../MxUsb/Platform/MXDefine.h"

static HANDLE  hComm;

BOOL init_com_port(char* Com, int Config, HANDLE &hComPort, int iBaudRate);

/*  Function: WriteBuffer
 *	Inputs: lpbuf - storage
 *			dwToWrite - length of buf storage
 *	Outputs: bool - true if successful read. otherwise false.
 */
BOOL WriteBuffer(PUCHAR lpBuf, DWORD dwToWrite);

/*  Function: ReadBuffer
 *	Inputs: buf - storage
 *			dwToRead - length of buf storage
 *	Outputs: bool - true if successful read. otherwise false.
 */
BOOL ReadBuffer(PUCHAR buf, DWORD dwToRead, int to);

BOOL close_com_port(void);