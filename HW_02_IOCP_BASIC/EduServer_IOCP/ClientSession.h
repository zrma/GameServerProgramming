#pragma once
#include "FastSpinlock.h"

#define BUFSIZE	4096

class ClientSession ;
class SessionManager;

enum IOType
{
	IO_NONE,
	IO_SEND,
	IO_RECV,
	IO_RECV_ZERO,
	IO_ACCEPT
};

enum DisconnectReason
{
	DR_NONE,
	DR_RECV_ZERO,
	DR_ACTIVE,
	DR_ONCONNECT_ERROR,
	DR_COMPLETION_ERROR,
};

struct OverlappedIOContext
{
	OverlappedIOContext( const ClientSession* owner, IOType ioType ): m_SessionObject( owner ), m_IoType( ioType )
	{
		memset( &m_Overlapped, 0, sizeof( OVERLAPPED ) );
		memset( &m_WsaBuf, 0, sizeof( WSABUF ) );
		memset( m_Buffer, 0, BUFSIZE );
	}

	OVERLAPPED				m_Overlapped;
	const ClientSession*	m_SessionObject;
	IOType			m_IoType;
	WSABUF			m_WsaBuf;
	char			m_Buffer[BUFSIZE];
};


class ClientSession
{
public:
	ClientSession( SOCKET sock )
		: m_Socket( sock ), m_Connected( false )
	{
		memset( &m_ClientAddr, 0, sizeof( SOCKADDR_IN ) );
	}

	~ClientSession() {}

	bool	OnConnect( SOCKADDR_IN* addr );
	bool	IsConnected() const { return m_Connected; }

	bool	PostRecv() const;
	bool	PostSend( const char* buf, int len ) const;
	void	Disconnect( DisconnectReason dr );


private:
	bool			m_Connected;
	SOCKET			m_Socket;

	SOCKADDR_IN		m_ClientAddr;

	//TODO: mLock; 선언할 것 -> 구현
	FastSpinlock	m_Lock;

	friend class SessionManager;
};




