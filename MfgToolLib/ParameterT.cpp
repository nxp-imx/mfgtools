/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// ParameterT.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ParameterT.h"

int property::ParseParameterString(LPCTSTR stringToParse, Parameter::ParamMap& paramMap)
{
	int numParamsParsed = 0;

	// Parameter=Type:Hid,Vid:0x066F,Pid:0x1234,TimeOut:10
	CString theString(stringToParse);

	// Holder for Key:Value pair, Key part, Value part
	CString pairStr, keyStr, valueStr;

	// multiple Parameters are sepatated by commas
	while ( theString.GetLength() )
	{
		// get the key:value pair string
		int commaPos;
		if ( (commaPos = theString.Find(_T(','))) != -1 )
		{
			// get the string part up to the comma (key:value pair)
			pairStr = theString.Left(commaPos);
			// trime theString for the next round
			theString = theString.Right(theString.GetLength()-(commaPos+1));
		}
		else
		{
			// put the whole string in the pairStr
			pairStr = theString;
			// done
//temp			theString.erase();
		}

		// get the Key and Value strings
		int colonPos;
		if ( (colonPos = pairStr.Find(_T(':'))) != -1 )
		{
			// get the string part up to and including the colon (key:value pair)
			// leave the colon on the Key string to help remind us its a key
			keyStr = pairStr.Left(colonPos+1);
			// the Value is after the colon
			valueStr = pairStr.Right(pairStr.GetLength()-(colonPos+1));
		}
		else
		{
			;// Warning: no Key for this Key:Value pair
		}
		
		// Parse the Value into the ParameterT
		if( !keyStr.IsEmpty() )
		{
			numParamsParsed += paramMap[keyStr]->Parse(valueStr);
		}
	}
	
	return numParamsParsed;
};
/*
template <typename T>
const CString property::ParameterT<T>::ToString() const
{
	CString str, fmt;
	fmt.Format(_T("0x%%0%dX"), 2*sizeof(T));

	if ( ValueList.empty() )
	{
		str.Format(fmt, Value);
	}
	else
	{
        std::map<T, CString>::const_iterator key;
        key = ValueList.find(Value);
        if ( key == ValueList.end() )
            str = _T("Not found.");
        else
            str = key->second;
	}

	return str;
}

template <>
const CString property::ParameterT<__time64_t>::ToString() const
{
    CString dateStr; 
    struct tm modTime;
    if ( Value != 0 )
    {
        _gmtime64_s(&modTime, &Value);
        _tcsftime( dateStr.GetBufferSetLength(_MAX_PATH), _MAX_PATH, _T("%c"), &modTime);
        dateStr.ReleaseBuffer();
    }
    else
    {
        dateStr = _T("N/A");
    }

    return dateStr;
}
*/
