#pragma once
#include <list>
#include <WinSock2.h>
#include "FastSpinlock.h"

class ClientSession;

class SessionManager
{
public:
	SessionManager() : m_CurrentIssueCount(0), m_CurrentReturnCount(0)
	{}
	
	~SessionManager();

	void PrepareSessions();
	bool AcceptSessions();

	void ReturnClientSession(ClientSession* client);

	

private:
	typedef std::list<ClientSession*> ClientList;
	ClientList	m_FreeSessionList;

	FastSpinlock m_Lock;

	uint64_t m_CurrentIssueCount;
	uint64_t m_CurrentReturnCount;
};

extern SessionManager* g_SessionManager;
