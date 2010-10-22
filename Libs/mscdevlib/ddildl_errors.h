/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
////////////////////////////////////////////////////////////////////////////////
// Copyright(C) SigmaTel, Inc. 2003
//
// Filename: ddildl_errors.h
// Description: 
////////////////////////////////////////////////////////////////////////////////

#ifndef _DDILDL_ERRORS_H
#define _DDILDL_ERRORS_H

#define LDRIVE_ERROR_INVALID_DRIVE_NUMBER       0x000001
#define LDRIVE_ERROR_NOT_INITIALIZED            0x000002
#define LDRIVE_ERROR_HARDWARE_FAILURE           0x000003

#define LDRIVE_ERROR_INVALID_INFO_TYPE          0x001000
#define LDRIVE_ERROR_SECTOR_OUT_OF_BOUNDS       0x001001
#define LDRIVE_ERROR_WRITE_FAILURE              0x001002
#define LDRIVE_ERROR_WRITE_PROTECTED            0x001003


#define LMEDIA_ERROR_HARDWARE_FAILURE           0x002000
#define LMEDIA_ERROR_INVALID_MEDIA_NUMBER       0x002001
#define LMEDIA_ERROR_MEDIA_NOT_INITIALIZED      0x002002
#define LMEDIA_ERROR_MEDIA_NOT_DISCOVERED       0x002003
#define LMEDIA_ERROR_INVALID_MEDIA_INFO_TYPE    0x002004
#define LMEDIA_ERROR_ALLOCATION_TO_LARGE        0x002005
#define LMEDIA_ERROR_MEDIA_NOT_ERASED           0x002006

#endif // #ifndef _DDILDL_ERRORS_H
