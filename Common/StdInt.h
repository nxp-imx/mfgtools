/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#ifndef STDINT_H
#define STDINT_H

namespace stdint
{
	typedef unsigned __int8    uint8_t;
	typedef unsigned __int16   uint16_t;
	typedef unsigned __int32   uint32_t;
	typedef unsigned __int64   uint64_t;

	typedef signed __int8      int8_t;
	typedef signed __int16     int16_t;
	typedef signed __int32     int32_t;
	typedef signed __int64     int64_t;

} // namespace stdint

using namespace stdint;

#endif /* STDINT_H */
