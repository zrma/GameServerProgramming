#include "stdafx.h"
#include "ClientSession.h"
#include "SessionManager.h"

SessionManager* g_SessionManager = nullptr;

ClientSession* SessionManager::CreateClientSession(SOCKET sock)
{
	ClientSession* client = new ClientSession(sock);

	//TODO: lock으로 보호할 것 -> 구현
	FastSpinlockGuard	dummyLocker( m_Lock );
	{
		m_ClientList.insert(ClientList::value_type(sock, client));
	}

	return client;
}


void SessionManager::DeleteClientSession(ClientSession* client)
{
	//TODO: lock으로 보호할 것 -> 구현
	FastSpinlockGuard	dummyLocker( m_Lock );
	{
		m_ClientList.erase(client->m_Socket);
	}
	
	delete client;
}