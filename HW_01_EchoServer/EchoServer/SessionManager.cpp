#include "stdafx.h"
#include "SessionManager.h"
#include "Session.h"

SessionManager* g_SessionManager = nullptr;

SessionManager::SessionManager()
{
}

SessionManager::~SessionManager()
{
}

Session* SessionManager::CreateSession( SOCKET sock )
{
	Session* session = new Session( sock );
	m_SessionList.insert( SessionList::value_type( sock, session) );

	return session;
}

void SessionManager::DestroySession( SOCKET sock )
{
	if ( m_SessionList.find( sock ) != m_SessionList.end() )
	{
		auto toBeDelete = m_SessionList[sock];
		delete ( toBeDelete );

		m_SessionList.erase( sock );
	}

	shutdown( sock, SD_BOTH );
	closesocket( sock );
}

Session* SessionManager::FindSession( SOCKET sock )
{
	if ( m_SessionList.find( sock ) != m_SessionList.end() )
	{
		return m_SessionList[sock];
	}

	return nullptr;
}
