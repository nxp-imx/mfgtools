
/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#define DEFAULT_LANGIDS			10

#ifdef G_STBINDER
/*** defined in stbinder.cpp
LANGID g_DefaultIDs[DEFAULT_LANGIDS] =
{
	0x007F,	// Invariant Language used as failsafe or language neutral
	0x0404,	// Chinese Traditional
	0x0804,	// Chinese Simplified
	0x0109,	// English
	0x000C,	// French
	0x0007, // German
	0x0011,	// Japanese
	0x0012,	// Korean
	0x0016,	// Portuguese
	0x000A	// Spanish
};
*******/
#else
extern LANGID *g_LangIds;
extern USHORT g_LangIdCount;
#endif
