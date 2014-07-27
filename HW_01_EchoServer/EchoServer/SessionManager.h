#pragma once

class Session;

class SessionManager
{
public:
	SessionManager();
	~SessionManager();

	Session*	CreateSession( SOCKET sock );
	void		DestroySession( SOCKET sock );

	Session*	FindSession( SOCKET sock );
	size_t		GetConnectionCount() { return m_SessionList.size(); }

private:
	typedef std::map<SOCKET, Session*> SessionList;
	SessionList		m_SessionList;
};

extern SessionManager* g_SessionManager;