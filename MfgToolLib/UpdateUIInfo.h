/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#include "MfgToolLib_Export.h"

typedef struct _ui_update_info
{
	DWORD OperationID;
	BOOL bUpdateDescription;
	TCHAR strDescription[MAX_CHAR_NUMBERS];
	BOOL bUpdateCommandsProgress;
	int CommandsProgressIndex;
	int CommandStatus;
	BOOL bUpdateProgressInCommand;
	int ProgressIndexInCommand;
	int ProgressRangeInCommand;
	BOOL bUpdatePhaseProgress;
	int CurrentPhaseIndex;
	int PhaseStatus;
	MX_DEVICE_STATE currentState;
} UI_UPDATE_INFORMATION;
