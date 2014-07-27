#include "stdafx.h"
#include "FastSpinlock.h"


FastSpinlock::FastSpinlock() : mLockFlag(0)
{
}


FastSpinlock::~FastSpinlock()
{
}


void FastSpinlock::EnterLock()
{
	// 빙글 빙글(Spin)
	for (int nloops = 0; ; nloops++)
	{
		// 참고 : http://blog.naver.com/pdpdds/120131228284
		// 참고 : http://msdn.microsoft.com/query/dev12.query?appId=Dev12IDEF1&l=KO-KR&k=k(winnt%2FInterlockedExchange);k(InterlockedExchange);k(DevLang-C%2B%2B);k(TargetOS-Windows)&rd=true
		// 원자적인 변수의 값 변경
		//
		// mLockFlag가 0이면 1로 바꾸고 바로 return (진입 가능)
		if ( InterlockedExchange( &mLockFlag, 1 ) == 0 )
		{
			return;
		}

		// mLockFlag가 0이 아니었으므로(락 걸린 상태) 한 턴 쉬고 재시도를 합시다

		// minimum timer resolution를 1로
		UINT uTimerRes = 1;
		timeBeginPeriod(uTimerRes); 

		// 최소 sleep(0)부터 최대 sleep(10)까지 쉬어봅시다
		Sleep((DWORD)min(10, nloops));
		
		// timer resolution을 원래대로
		timeEndPeriod(uTimerRes);

		// 참고 : http://chanik.egloos.com/viewer/3882115
		// 읽어볼 거리 : http://www.gpgstudy.com/forum/viewtopic.php?t=8609

		// 현재 스레드 한타임 쉬고 오라는 것인데
		// 해상도를 조절하지 않고 바로 sleep() 하면 대략 15ms 이상 쉴 수 있으므로
		// 해상도를 높여준 후 sleep()하고, 한 번 해봐서 안 되면 점점 쉬는 시간을 길게 해서 재시도
	}
}

void FastSpinlock::LeaveLock()
{
	// 락 풀 때 1 -> 0으로 바꾸기
	InterlockedExchange(&mLockFlag, 0);
}