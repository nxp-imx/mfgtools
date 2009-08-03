#include "stdafx.h"
#include "WindowsVersionInfo.h"

// The one and only WindowsVersionInfo object
WindowsVersionInfo& gWinVersionInfo()
{
	static WindowsVersionInfo wvi;
	return wvi;
};