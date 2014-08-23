#pragma once
//#include <WinSock2.h>
#include "XTL.h"
#include "FastSpinlock.h"

class ClientSession;

class SessionManager
{
public:
	SessionManager();
	
	~SessionManager();

	void PrepareSessions();
	bool ConnectSessions();
	void ReturnClientSession(ClientSession* client);
	
	long long	GetAllSessionRecvBytes();
	long long	GetAllSessionSendBytes();
	long		GetAllSessionConnectCount();

private:
	typedef xlist<ClientSession*>::type ClientList;
	ClientList	mFreeSessionList;
	ClientList	mTotalSessionList;

	FastSpinlock mLock;

	uint64_t mCurrentIssueCount;
	uint64_t mCurrentReturnCount;
};

extern SessionManager* GSessionManager;
