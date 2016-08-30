/*
 * Copyright 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
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
