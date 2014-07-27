#include "stdafx.h"
#include "Exception.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"

bool ClientSession::OnConnect( SOCKADDR_IN* addr )
{
	//TODO: 이 영역 lock으로 보호 할 것 -> 구현
	FastSpinlockGuard	dummyLocker( m_Lock );

	CRASH_ASSERT( l_ThreadType == THREAD_MAIN_ACCEPT );

	/// make socket non-blocking
	u_long arg = 1;
	ioctlsocket( m_Socket, FIONBIO, &arg );

	/// turn off nagle
	int opt = 1;
	setsockopt( m_Socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof( int ) );

	opt = 0;
	if ( SOCKET_ERROR == setsockopt( m_Socket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof( int ) ) )
	{
		printf_s( "[DEBUG] SO_RCVBUF change error: %d\n", GetLastError() );
		return false;
	}

	// HANDLE handle = 0; //TODO: 여기에서 CreateIoCompletionPort((HANDLE)mSocket, ...);사용하여 연결할 것 -> 구현
	HANDLE handle = CreateIoCompletionPort( (HANDLE)m_Socket, g_IocpManager->GetComletionPort(), ( ULONG_PTR )this, 0 );

	if ( handle != g_IocpManager->GetComletionPort() )
	{
		printf_s( "[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError() );
		return false;
	}

	memcpy( &m_ClientAddr, addr, sizeof( SOCKADDR_IN ) );
	m_Connected = true;

	printf_s( "[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa( m_ClientAddr.sin_addr ), ntohs( m_ClientAddr.sin_port ) );

	g_SessionManager->IncreaseConnectionCount();

	return PostRecv();
}

void ClientSession::Disconnect( DisconnectReason dr )
{
	//TODO: 이 영역 lock으로 보호할 것
	FastSpinlockGuard	dummyLocker( m_Lock );

	if ( !IsConnected() )
	{
		return;
	}

	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	if ( SOCKET_ERROR == setsockopt( m_Socket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof( LINGER ) ) )
	{
		printf_s( "[DEBUG] setsockopt linger option error: %d\n", GetLastError() );
	}

	printf_s( "[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa( m_ClientAddr.sin_addr ), ntohs( m_ClientAddr.sin_port ) );

	g_SessionManager->DecreaseConnectionCount();

	closesocket( m_Socket );

	m_Connected = false;
}

bool ClientSession::PostRecv() const
{
	if ( !IsConnected() )
	{
		return false;
	}

	// OverlappedIOContext의 구조
	//
	// OVERLAPPED				m_Overlapped;
	// const ClientSession*		m_SessionObject;
	// IOType					m_IoType;
	// WSABUF					m_WsaBuf;
	// char						m_Buffer[BUFSIZE];

	// m_SessionObject, m_IoType은 생성자에서 초기화
	OverlappedIOContext* recvContext = new OverlappedIOContext( this, IO_RECV );

	//TODO: WSARecv 사용하여 구현할 것 -> 구현
	DWORD recvBytes = 0;
	DWORD flags = 0;
	recvContext->m_WsaBuf.len = BUFSIZE;
	recvContext->m_WsaBuf.buf = recvContext->m_Buffer;

	if ( SOCKET_ERROR == WSARecv( m_Socket, &recvContext->m_WsaBuf, 1, &recvBytes, &flags, &(recvContext->m_Overlapped), NULL ) )
	{
		if ( WSA_IO_PENDING != WSAGetLastError() )
		{
			delete recvContext;
			return false;
		}
	}

	return true;
}

bool ClientSession::PostSend(const char* buf, int len) const
{
	if ( !IsConnected() )
	{
		return false;
	}

	OverlappedIOContext* sendContext = new OverlappedIOContext(this, IO_SEND);

	/// copy for echoing back..
	memcpy_s(sendContext->m_Buffer, BUFSIZE, buf, len);

	//TODO: WSASend 사용하여 구현할 것 -> 구현

	DWORD sendBytes = 0;
	DWORD flags = 0;
	sendContext->m_WsaBuf.len = len;
	sendContext->m_WsaBuf.buf = sendContext->m_Buffer;

	if ( SOCKET_ERROR == WSASend( m_Socket, &sendContext->m_WsaBuf, 1, &sendBytes, flags, &( sendContext->m_Overlapped ), NULL ) )
	{
		if ( WSA_IO_PENDING != WSAGetLastError() )
		{
			delete sendContext;
			return false;
		}
	}

	return true;
}