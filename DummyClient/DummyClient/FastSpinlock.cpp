#include "stdafx.h"
#include "Exception.h"
#include "FastSpinlock.h"
#include "ThreadLocal.h"

FastSpinlock::FastSpinlock(const int lockOrder) : mLockFlag(0), mLockOrder(lockOrder)
{
}


FastSpinlock::~FastSpinlock()
{
}


void FastSpinlock::EnterWriteLock()
{
	while (true)
	{
		/// 다른놈이 writelock 풀어줄때까지 기다린다.
		while ( mLockFlag & LF_WRITE_MASK )
		{
			// http://msdn.microsoft.com/en-us/library/windows/desktop/ms687419(v=vs.85).aspx
			// http://blog.naver.com/dkdaf/90157791966
			YieldProcessor();
		}

		// 비트 플래그 연산으로 쓰기 걸기. 이미 걸려 있다면 쓰기 마스크 결과 쓰기 플래그가 나오지는 않는다.
		// 즉 단 하나의 쓰기 연산만 걸릴 수 있어야(이전에 쓰기 연산이 걸려 있지 않았어야...) 진입 가능
		if ((InterlockedAdd(&mLockFlag, LF_WRITE_FLAG) & LF_WRITE_MASK) == LF_WRITE_FLAG)
		{
			/// 다른놈이 readlock 풀어줄때까지 기다린다.
			// 읽기 마스크로 검출이 되면 기다린다.
			while ( mLockFlag & LF_READ_MASK )
			{
				YieldProcessor();
			}

			// 아무 문제 없으면 락 걸린 상태로 리턴(임계 영역 진입 성공!)
			return;
		}

		// 쓰기 걸기 했던 것 하나 걷어내고 다시 와일루프 ㄱㄱ
		InterlockedAdd(&mLockFlag, -LF_WRITE_FLAG);
	}

}

void FastSpinlock::LeaveWriteLock()
{
	InterlockedAdd(&mLockFlag, -LF_WRITE_FLAG);
}

void FastSpinlock::EnterReadLock()
{
	while (true)
	{
		/// 다른놈이 writelock 풀어줄때까지 기다린다.
		while ( mLockFlag & LF_WRITE_MASK )
		{
			YieldProcessor();
		}

		//TODO: Readlock 진입 구현 (mLockFlag를 어떻게 처리하면 되는지?)  -> 구현
		// if ( readlock을 얻으면 )
			//return;
		// else
			// mLockFlag 원복
				
		if ( ( InterlockedAdd( &mLockFlag, 1 ) & LF_WRITE_FLAG ) != LF_WRITE_FLAG )
		{
			return;
		}
		else
		{
			InterlockedAdd( &mLockFlag, -1 );
		}
	}
}

void FastSpinlock::LeaveReadLock()
{
	//TODO: mLockFlag 처리  -> 구현
	CRASH_ASSERT( ( ( InterlockedAdd( &mLockFlag, -1 ) & LF_READ_MASK ) >= 0 ) );
}