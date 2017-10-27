#ifndef __LIBFDT_ENV__
#define __LIBFDT_ENV__

#include <stdint.h> 

typedef __int16 fdt16_t;
typedef __int32 fdt32_t;
typedef __int64 fdt64_t;

inline uint64_t EndianSwap64(uint64_t x) {
	return  (((x & 0x00000000000000ffLL) << 56) |
			((x & 0x000000000000ff00LL) << 40) |
			((x & 0x0000000000ff0000LL) << 24) |
			((x & 0x00000000ff000000LL) << 8) |
			((x & 0x000000ff00000000LL) >> 8) |
			((x & 0x0000ff0000000000LL) >> 24) |
			((x & 0x00ff000000000000LL) >> 40) |
			((x & 0xff00000000000000LL) >> 56));
}

inline uint32_t EndianSwap(uint32_t x)
{
	return (x >> 24) |
		((x << 8) & 0x00FF0000) |
		((x >> 8) & 0x0000FF00) |
		(x << 24);
}

#define fdt32_to_cpu(x)         EndianSwap(x)
#define cpu_to_fdt32(x)         EndianSwap(x)

#define fdt64_to_cpu(x)         EndianSwap64(x)
#define cpu_to_fdt64(x)         EndianSwap64(x)

#ifndef _AFXDLL
#ifdef _DEBUG
#pragma comment(lib, "uafxcwd.lib")
#else
#pragma comment(lib, "uafxcw.lib")
#endif
#endif 

#endif