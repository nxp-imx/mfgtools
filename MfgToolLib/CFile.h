#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <cstdio>


class CFile
{


public:
	CFile();
	CFile(HANDLE File);
	CFile(LPCTSTR filename, UINT OpenFlags);



};


