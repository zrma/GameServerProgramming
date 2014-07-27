#include "stdafx.h"
#include "FastSpinlock.h"


FastSpinlock::FastSpinlock(): m_LockFlag( 0 )
{
}


FastSpinlock::~FastSpinlock()
{
}


void FastSpinlock::EnterLock()
{
	for ( int nloops = 0;; nloops++ )
	{
		if ( InterlockedExchange( &m_LockFlag, 1 ) == 0 )
		{
			return;
		}

		UINT uTimerRes = 1;
		timeBeginPeriod( uTimerRes );
		Sleep( (DWORD)min( 10, nloops ) );
		timeEndPeriod( uTimerRes );
	}
}

void FastSpinlock::LeaveLock()
{
	InterlockedExchange( &m_LockFlag, 0 );
}