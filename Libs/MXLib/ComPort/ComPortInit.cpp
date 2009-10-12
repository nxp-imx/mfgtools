#include "..\stdafx.h"
#include <stdio.h>
#include <string.h>
//#include "../../MxUsb/Platform/MXDefine.h"

static HANDLE  hComm;

BOOL init_com_port(char* Com, int Config, HANDLE &hComPort, int iBaudRate)
{
	char portName[10];
	BOOL initStatus = FALSE;		// comm port init status

	strcpy_s(portName, 10, "COM");
	strcat_s(portName, 10, Com);
	strcat_s(portName, 10, ":");

	//Open the com port
	hComm = CreateFile((LPCTSTR)portName,
						GENERIC_READ | GENERIC_WRITE,
						0,
						0,
						OPEN_EXISTING,
						FILE_FLAG_OVERLAPPED,
						0);
	hComPort = hComm;
	
	if(hComm != INVALID_HANDLE_VALUE)
	{
		initStatus = TRUE;
	}
		
	/*
		Setting Serial Communication parameters
		baud:			115200 initial
		Parity:			No parity
		Stop bits:		1
		Data bits:		8
		FlowControl:	No FlowControl
	*/
	
	DCB dcb ={0};
	
	dcb.DCBlength =sizeof(dcb);
	GetCommState (hComm, &dcb);
	
	dcb.BaudRate		= iBaudRate;	//Current Baud Rate
	dcb.fBinary			= TRUE;		    //Binary mode; no EOF check
	dcb.fParity			= FALSE;	    //Enable parity checking
	dcb.fOutxCtsFlow	= FALSE ;	    //No CTS output flow control
	dcb.fOutxDsrFlow	= FALSE;	    //No DSR output flow control
	dcb.fDtrControl		= DTR_CONTROL_ENABLE;

	dcb.fDsrSensitivity = FALSE;		//DSR sensitivity		
	dcb.fTXContinueOnXoff= TRUE;		//XOFF continues Tx
	dcb.fOutX			= FALSE;		//No XON/XOFF out flow control 
	dcb.fInX			= FALSE;		//No XON/XOFF in flow control
	dcb.fErrorChar		= FALSE;		//Disable error replacement
	dcb.fNull			= FALSE;		//Disable null stripping
	dcb.fRtsControl		= RTS_CONTROL_ENABLE;
	dcb.fAbortOnError	= TRUE;			//Do not abort reads/writes on error
	dcb.ByteSize		= 8;			//Number of bits/bytes,4-8

	//TO1_0
	if (Config == 0)
	{
		dcb.Parity			= ODDPARITY;	//No parity bit
		dcb.StopBits		= TWOSTOPBITS;	// 2 stop bits
	}

	// TO2_0
	if (Config == 1)
	{
		dcb.Parity			= NOPARITY;		//No parity bit
		dcb.StopBits		= ONESTOPBIT;	//1 stop bit
	}

	if (initStatus)
	{
		initStatus = FALSE;
		if (SetCommState(hComm, &dcb))
		{
			//Setting the timeouts for com
			COMMTIMEOUTS T_OUTS;
			FillMemory(&T_OUTS, sizeof(T_OUTS),0);
			T_OUTS.ReadIntervalTimeout=MAXDWORD;
			T_OUTS.ReadTotalTimeoutConstant=3*1000;
			T_OUTS.ReadTotalTimeoutMultiplier=5;
			T_OUTS.WriteTotalTimeoutConstant=10;
			T_OUTS.WriteTotalTimeoutMultiplier=1000;
			if (SetCommTimeouts(hComm,&T_OUTS))
			{
				initStatus = TRUE;	
			}
		}
	}
	return initStatus;
}

/*  Function: WriteBuffer
 *	Inputs: lpbuf - storage
 *			dwToWrite - length of buf storage
 *	Outputs: bool - true if successful read. otherwise false.
 */

BOOL WriteBuffer(PUCHAR lpBuf, DWORD dwToWrite){
	OVERLAPPED osWrite = {0};
	DWORD dwWritten;
	DWORD dwRes;
	bool fRes;
	DWORD temp4;
	osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Clear any outstanding comm port errors
	DWORD errorStatus;
	ClearCommError(hComm, &errorStatus,NULL);

	if(!WriteFile(hComm, lpBuf, dwToWrite, &dwWritten, &osWrite)){
		temp4 = GetLastError ();
		if(GetLastError() != ERROR_IO_PENDING){
			TRACE("Write error! error code is %d\n",GetLastError());
			// unsuccessful write operation
			fRes = FALSE;
		}
		else{	// Write is pending
			dwRes = WaitForSingleObject(osWrite.hEvent, INFINITE);
			switch(dwRes){
				case WAIT_OBJECT_0:
					if(!GetOverlappedResult(hComm, &osWrite, &dwWritten, true)){
						TRACE("Write error! GetOverlappedResult failed\n");
						fRes = false;
					}
					else
						fRes = true;
					break;
				default:
					fRes = false;
					break;
			}
		}
	}
	else{	// instantly written
		fRes = TRUE;
	}

	CloseHandle(osWrite.hEvent);
	return fRes;
}

/*  Function: ReadBuffer
 *	Inputs: buf - storage
 *			dwToRead - length of buf storage
 *	Outputs: bool - true if successful read. otherwise false.
 */

BOOL ReadBuffer(PUCHAR buf, DWORD dwToRead, int to){
	DWORD dwRead;
	OVERLAPPED osReader = {0};
	int timeout = 0;

	osReader.hEvent = CreateEvent(NULL, true, false, NULL);

	if(osReader.hEvent == NULL){
		//printf("Error creating overlapped event abort\n");
		return false;
	}
	
	while (dwToRead > 0) {

		if(!ReadFile(hComm, buf, dwToRead, &dwRead, &osReader)){
			if(GetLastError() != ERROR_IO_PENDING){
				TRACE("Readfile reported an Error in comm\n");
				return false;
			}
			else {
				TRACE("Read from UART pending, timeout:%d\n", timeout);
			}
		}
		else {	// read instantly
			dwToRead -= dwRead;
			TRACE("Read from UART:%d, left:%d\n", dwRead, dwToRead);
			buf += dwRead;
			continue;
		}

		dwRead = 0;

		if(!GetOverlappedResult(hComm, &osReader, &dwRead, true)){
			TRACE("Getoverlappedresult reported Error in comm port\n");
			return false;
		}
		else{
			if (dwRead != 0) {
				
				TRACE("Getoverlappedresult in comm port\n");
				dwToRead -= dwRead;
				TRACE("Read from UART:%d, left:%d\n", dwRead, dwToRead);
				buf += dwRead;
			} 
			else{
				if (++timeout > to)
					return false;
			}
			//return true;
		}
		
	}
	CloseHandle(osReader.hEvent);
	return true;
}

BOOL close_com_port(void)
{
	return (CloseHandle(hComm));
}