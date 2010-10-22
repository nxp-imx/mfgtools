/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
//
// mm_callback.h
//
#pragma once


typedef HRESULT (CALLBACK FAR * LPFN_MOUSEMOVE_CALLBACK)(PVOID pCallerClass, UINT);

typedef struct _MOUSEMOVECALLBACK {
	UINT	id;
	PVOID	pCallerClass;
	LPFN_MOUSEMOVE_CALLBACK pfnCallback;
} MOUSEMOVECALLBACK, *PMOUSEMOVECALLBACK;
