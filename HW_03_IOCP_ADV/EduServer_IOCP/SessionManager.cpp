#include "stdafx.h"
#include "FastSpinlock.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "IocpManager.h"


SessionManager* g_SessionManager = nullptr;

SessionManager::~SessionManager()
{
	for (auto it : mFreeSessionList)
	{
		delete it;
	}
}

// 세션 풀 생성 - 소켓을 미리 파 놓는다
void SessionManager::PrepareSessions()
{
	CRASH_ASSERT( LThreadType == THREAD_MAIN );

	for ( int i = 0; i < MAX_CONNECTION; ++i )
	{
		ClientSession* client = new ClientSession();

		mFreeSessionList.push_back( client );
	}
}

void SessionManager::ReturnClientSession(ClientSession* client)
{
	FastSpinlockGuard guard(mLock);

	// 뭔가 접속이 안 끊어졌거나, Ref Count가 남아있는데 여기 들어오면 안 된다구욧!
	CRASH_ASSERT( client->m_Connected == 0 && client->m_RefCount == 0 );

	// 세션 정보 초기화하고 소켓 끊어버리고 새 것으로 ㅇㅅㅇ
	client->SessionReset();

	mFreeSessionList.push_back(client);

	++m_CurrentReturnCount;
}

// 얘는 트루를 리턴하는 한 0.1초마다 한 번씩 반복 호출 됨
bool SessionManager::AcceptSessions()
{
	FastSpinlockGuard guard(mLock);

	//////////////////////////////////////////////////////////////////////////
	// m_CurrentIssueCount = 들어온 놈
	// m_CurrentReturnCount = 나간 놈
	//
	// m_CurrentIssueCount - m_CurrentReturnCount = 현재 남아있는 놈
	//////////////////////////////////////////////////////////////////////////
	while (m_CurrentIssueCount - m_CurrentReturnCount < MAX_CONNECTION)
	{
		// TODO mFreeSessionList에서 ClientSession* 꺼내서 PostAccept() 해주기.. -> 구현
		// (위의 ReturnClientSession와 뭔가 반대로 하면 될 듯?)
		
		// AddRef()도 당연히 해줘야 하고...

		// 실패시 false
		// if (false == newClient->PostAccept())
		//		return false;

		ClientSession* newClient = mFreeSessionList.back();
		mFreeSessionList.pop_back();

		++m_CurrentIssueCount;

		// 전에 sm9 교수님이 설명해주셨는데 세션을 가비지 컬렉팅 하기 위해서
		// 현재 세션이 작업 중인지 아닌지 레퍼런스 카운팅을 해야 한다고 하셨음
		// 요청을 하면 +1하고 요청이 완료 되면 -1 하고...
		//
		// ReleaseRef()에서 Ref Count를 -1 하고 현재 카운트가 0인지 체크해서 접속을 종료한다.
		// 그러므로 처음 AcceptEx로 +1, DisconnectEx로 -1 해서 0 맞춰지면 종료
		newClient->AddRef();

		if ( false == newClient->PostAccept() )
		{
			return false;
		}
	}
	
	return true;
}