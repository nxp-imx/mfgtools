/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StBase.cpp: implementation of the CStBase class.
//
//////////////////////////////////////////////////////////////////////
#include "StHeader.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStBase::CStBase(string _name)
{
	m_last_error		= STERR_NONE;
	m_system_last_error = 0;
	m_obj_name			= string(_name.c_str());
}

CStBase::CStBase(const CStBase& _base)
{
	*this = _base;
}

CStBase::~CStBase()
{	
	
} 

CStBase& CStBase::operator=(const CStBase& _base)
{
	if( this == &_base )
		return *this;

	m_last_error		= _base.GetLastError();
	m_system_last_error = _base.GetSystemLastError();
	m_obj_name			= string( _base.GetObjName().c_str() );

	return *this;
}

ST_ERROR CStBase::GetLastError() const
{
	return m_last_error;
}

long CStBase::GetSystemLastError() const
{
	return m_system_last_error;
}

string CStBase::GetObjName() const
{
	return m_obj_name;
}

CStBaseToResource::CStBaseToResource(void):CStBase( "CStResource" )
{
}

CStBaseToResource::~CStBaseToResource(void)
{
}

CStBaseToCmdLineProcessor::CStBaseToCmdLineProcessor(void):CStBase( "CStBaseToCmdLineProcessor" )
{
}

CStBaseToCmdLineProcessor::~CStBaseToCmdLineProcessor(void)
{
}

CStBaseToLogger::CStBaseToLogger(void):CStBase( "CStBaseToLogger" )
{
}

CStBaseToLogger::~CStBaseToLogger(void)
{
}
