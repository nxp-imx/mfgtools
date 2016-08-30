/*
 * Copyright 2012-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

#include <vector>

#define UM_PORT_DLG_MSG			WM_USER+100
typedef enum PortDlgMsgEvent
{
	OP_CMD_START,
	OP_CMD_STOP
};
#define UM_PORT_DEVICE_CHANGE	WM_USER+102
#define UM_PORT_UPDATE_INFO		WM_USER+103
#define UM_OPBTN_DLG_MSG		WM_USER+101

#define UM_MODIFY_LINE			WM_USER+105

#define DEFAULT_PORT_NUMBERS	1


extern int g_totalOps;
extern int g_successOps;
extern int g_failedOps;

extern int g_PortMgrDlgNums;

void OutputInformation(CString strMsg);
void OutputToConsole(CString strMsg);
void ExiFslConsole();
void DetachFslConsole();
void StartForConsole();

void WhereXY(int *x, int *y);
void GoToXY(int x, int y);
extern BOOL g_bConsoleApp;

#define PROFILE_FOLDER_NAME_TAG		_T("-c")
#define BOARD_NAME_TAG				_T("-b")
#define LIST_NAME_TAG				_T("-l")
#define PORT_NUMBERS_TAG			_T("-p")

typedef enum _tagMsgType
{
	DEVICE1_UPDATE_PERCENT = 0,
	DEVICE2_UPDATE_PERCENT = 1,
	DEVICE3_UPDATE_PERCENT = 2,
	DEVICE4_UPDATE_PERCENT = 3,
	DEVICE1_DESCRIPTION,
	DEVICE2_DESCRIPTION,
	DEVICE3_DESCRIPTION,
	DEVICE4_DESCRIPTION,
	SUCCESS_AND_FAIL_COUNT,
	LAST_CURSOR_POSITION
} MSG_TYPE;

typedef struct _tagMsgCursorPos
{
	MSG_TYPE type;
	int x;
	int y;
	int length;
} MSG_CURSOR_POSITION;

extern std::vector<MSG_CURSOR_POSITION*> g_VolatileMsgPosArray;
void ModifySpecifiedLine(MSG_CURSOR_POSITION *pMsgPos, CString strMsgNew);

