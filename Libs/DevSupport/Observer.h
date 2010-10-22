/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StProgress.h: interface for the CStProgress class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STPROGRESS_H__0CC7F0E0_AF6A_4A9A_AF61_C55442D93668__INCLUDED_)
#define AFX_STPROGRESS_H__0CC7F0E0_AF6A_4A9A_AF61_C55442D93668__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Common/StdString.h"
#include "Common/StdInt.h"

//#include "Libs\Loki\Singleton.h"
//#include "Libs\Loki\Functor.h"
/*!
 * This is a mix-in class that proves as an abstract interface to objects that are
 * platform-specific. Cross-platform code can use this interface without worrying about
 * any platform issues.
 */
class Observer 
{
	//! \brief Needed by SingletonHolder to create class.
//    friend struct Loki::CreateUsingNew<Observer>;
public:
    //! \brief Callback function typedef. i.e. void OnUpdateUiProgress(const uint32_t step, LPCTSTR task)
//    typedef Loki::Functor<void, LOKI_TYPELIST_2(const uint32_t, LPCTSTR), Loki::ClassLevelLockable> UI_Callback;

    Observer();
	virtual ~Observer();

//	virtual void SetTotalTasks(uint32_t _total_tasks)=0;
	virtual void SetCurrentTask(uint32_t taskId, uint32_t taskRange)=0;
//    virtual void SetCurrentTask(TASK_TYPE _task, uint32_t _range, uint8_t _drive_index)=0;//{};

	virtual void UpdateProgress(uint32_t step=1)=0;
//	virtual void UpdateGrandProgress()=0;
//	virtual void StartSearch(){}


	bool InProgress();
	virtual void Begin();
	virtual void Relax();

//	TASK_TYPE GetCurrentTask() { return m_current_task; }
//	uint32_t GetTaskRange() { return m_task_range; }

protected:

//	uint32_t			m_total_tasks;
	uint32_t			_taskRange;
	uint32_t			_taskId;
	bool				_inProgress;
//	CStMap_Task_Time	m_map_task_time;

};

#endif // !defined(AFX_STPROGRESS_H__0CC7F0E0_AF6A_4A9A_AF61_C55442D93668__INCLUDED_)
