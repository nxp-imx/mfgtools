/*
 * Copyright 2009-2013, 2016 Freescale Semiconductor, Inc.
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

// PropertyT.h
//
//////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __PARAMETER_XXX__
#define __PARAMETER_XXX__
#include "stdafx.h"
#include <map>
#include <vector>

typedef std::vector<CString> StdStringArray;

namespace property
{
	class Parameter
	{
	public:
		typedef std::map<CString, Parameter*> ParamMap;
		Parameter(LPCTSTR desc = NULL)
			: Desc(desc)
		{};
		virtual const CString ToString() const = 0;
		virtual int Parse(CString str) = 0;
		virtual StdStringArray GetValueStrings() = 0;
		CString Desc;
		ParamMap map;
		bool IsContainer() { return map.size() > 0; };
	};

	template <typename T>
	class ParameterT : public Parameter
	{
	public:
		ParameterT(T value = (T)0, T valDefault = (T)0, LPCTSTR desc = NULL)
			: Parameter(desc)
            , Value(value)
            , Default(valDefault)
		{};

        const CString ToString() const
		{
			CString str, fmt;
			fmt.Format(_T("0x%%0%dX"), 2*sizeof(T));

			if ( ValueList.empty() )
			{
				str.Format(fmt, Value);
			}
			else
			{
                typename std::map<T, CString>::const_iterator key;
                key = ValueList.find(Value);
                if ( key == ValueList.end() )
                    str = _T("Not found.");
                else
                    str = key->second;
			}

			return str;
		}

        int Parse(CString str)
		{
			int ret = FALSE;

			if ( ValueList.empty() )
			{
				_stscanf_s(str.GetBuffer(), _T("%i"), &Value);
			}
			else
			{
				typename std::map<T, CString>::iterator pair;
				for ( pair = ValueList.begin(); pair != ValueList.end(); ++pair )
				{
					if ( str.Compare((*pair).second) == 0 )
					{
						Value = (*pair).first;
						ret = TRUE;
						break;
					}
				}
			}

			return ret;
		}

		StdStringArray GetValueStrings()
		{
			StdStringArray strArray;
			typename std::map<T, CString>::iterator pair;
			for ( pair = ValueList.begin(); pair != ValueList.end(); ++pair )
			{
				strArray.push_back((*pair).second);
			}
			return strArray;
		}

		T Value;
		T Default;
		typename std::map<T, CString> ValueList;
	};

	int ParseParameterString(LPCTSTR stringToParse, Parameter::ParamMap& paramMap);
} // namespace property

using namespace property;
#endif
