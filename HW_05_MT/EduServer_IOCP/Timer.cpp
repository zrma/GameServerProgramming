﻿#include "stdafx.h"
#include "ThreadLocal.h"
#include "Exception.h"
#include "SyncExecutable.h"
#include "Timer.h"



Timer::Timer()
{
	LTickCount = GetTickCount64();
}


void Timer::PushTimerJob(SyncExecutablePtr owner, const TimerTask& task, uint32_t after)
{
	CRASH_ASSERT(LThreadType == THREAD_IO_WORKER);

	int64_t dueTimeTick = after + LTickCount;

	//TODO: mTimerJobQueue에 TimerJobElement를 push..  -> 구현
	mTimerJobQueue.push( TimerJobElement( owner, task, dueTimeTick ) );
}


void Timer::DoTimerJob()
{
	/// thread tick update
	LTickCount = GetTickCount64();

	while ( !mTimerJobQueue.empty() )
	{
		const TimerJobElement& timerJobElem = mTimerJobQueue.top();

		if ( LTickCount < timerJobElem.mExecutionTick )
			break;

		timerJobElem.mOwner->EnterLock();

		timerJobElem.mTask();

		const TimerJobElement& timerJobElem2 = mTimerJobQueue.top();
		
		timerJobElem2.mOwner->LeaveLock();

		mTimerJobQueue.pop();
	}

}

