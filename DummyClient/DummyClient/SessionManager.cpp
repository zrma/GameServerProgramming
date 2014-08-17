#include "stdafx.h"
#include "FastSpinlock.h"
#include "MemoryPool.h"
#include "ThreadLocal.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "IocpManager.h"
#include "DummyClient.h"


SessionManager* GSessionManager = nullptr;

SessionManager::SessionManager() : mCurrentIssueCount(0), mCurrentReturnCount(0), mLock(LO_FIRST_CLASS)
{
}

SessionManager::~SessionManager()
{
	for (auto it : mFreeSessionList)
	{
		xdelete(it);
	}
}

void SessionManager::PrepareSessions()
{
	CRASH_ASSERT(LThreadType == THREAD_MAIN);

	if ( MAX_CONNECTION <= 0 || MAX_CONNECTION > 1000 )
	{
		MAX_CONNECTION = 1000;
	}

	for (int i = 0; i < MAX_CONNECTION; ++i)
	{
		ClientSession* client = xnew<ClientSession>();
			
		mFreeSessionList.push_back(client);
	}
}


void SessionManager::ReturnClientSession(ClientSession* client)
{
	FastSpinlockGuard guard(mLock);

	CRASH_ASSERT(client->mConnected == 0 && client->mRefCount == 0);

	client->SessionReset();

	mFreeSessionList.push_back(client);

	++mCurrentReturnCount;
}

bool SessionManager::ConnectSessions()
{
	FastSpinlockGuard guard( mLock );

	while ( mCurrentIssueCount - mCurrentReturnCount < MAX_CONNECTION )
	{
		ClientSession* newClient = mFreeSessionList.back();
		mFreeSessionList.pop_back();

		++mCurrentIssueCount;

		newClient->AddRef(); ///< refcount +1 for issuing 

		if ( false == newClient->PostConnect() )
			return false;
	}
	
	return true;
}