#pragma once
#include "Exception.h"
#include "CircularBuffer.h"
#include "FastSpinlock.h"

#define BUFSIZE	65536

class ClientSession ;
class SessionManager;

enum IOType
{
	IO_NONE,
	IO_SEND,
	IO_RECV,
	IO_RECV_ZERO,
	IO_ACCEPT,
	IO_DISCONNECT
} ;

enum DisconnectReason
{
	DR_NONE,
	DR_ACTIVE,
	DR_ONCONNECT_ERROR,
	DR_IO_REQUEST_ERROR,
	DR_COMPLETION_ERROR,
};

struct OverlappedIOContext
{
	OverlappedIOContext(ClientSession* owner, IOType ioType);

	OVERLAPPED		m_Overlapped ;
	ClientSession*	m_SessionObject ;
	IOType			m_IoType ;
	WSABUF			m_WsaBuf;
	
} ;

struct OverlappedSendContext : public OverlappedIOContext
{
	OverlappedSendContext(ClientSession* owner) : OverlappedIOContext(owner, IO_SEND)
	{
	}
};

struct OverlappedRecvContext : public OverlappedIOContext
{
	OverlappedRecvContext(ClientSession* owner) : OverlappedIOContext(owner, IO_RECV)
	{
	}
};

struct OverlappedPreRecvContext : public OverlappedIOContext
{
	OverlappedPreRecvContext(ClientSession* owner) : OverlappedIOContext(owner, IO_RECV_ZERO)
	{
	}
};

struct OverlappedDisconnectContext : public OverlappedIOContext
{
	OverlappedDisconnectContext(ClientSession* owner, DisconnectReason dr) 
	: OverlappedIOContext(owner, IO_DISCONNECT), mDisconnectReason(dr)
	{}

	DisconnectReason mDisconnectReason;
};

struct OverlappedAcceptContext : public OverlappedIOContext
{
	OverlappedAcceptContext(ClientSession* owner) : OverlappedIOContext(owner, IO_ACCEPT)
	{}
};


void DeleteIoContext(OverlappedIOContext* context) ;


class ClientSession
{
public:
	ClientSession();
	~ClientSession() {}

	void	SessionReset();

	bool	IsConnected() const { return !!m_Connected; }

	bool	PostAccept();
	void	AcceptCompletion();

	bool	PreRecv() ; ///< zero byte recv

	bool	PostRecv();
	void	RecvCompletion(DWORD transferred);

	bool	PostSend();
	void	SendCompletion(DWORD transferred);
	
	void	DisconnectRequest(DisconnectReason dr);
	void	DisconnectCompletion(DisconnectReason dr);
	
	void	AddRef();
	void	ReleaseRef();

	void	SetSocket(SOCKET sock) { m_Socket = sock; }
	SOCKET	GetSocket() const { return m_Socket;  }

private:
	
	SOCKET			m_Socket ;

	SOCKADDR_IN		m_ClientAddr ;
		
	FastSpinlock	m_BufferLock;

	CircularBuffer	m_Buffer;

	// 이 두 녀석은 여러 스레드에서 접근해서 변경하므로 InterlockedExchange의 타겟이므로 volatile
	volatile long	m_RefCount;
	volatile long	m_Connected;

	friend class SessionManager;
} ;



