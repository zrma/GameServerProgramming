#include "stdafx.h"
#include "Exception.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"


OverlappedIOContext::OverlappedIOContext( ClientSession* owner, IOType ioType )
	: m_SessionObject( owner ), m_IoType( ioType )
{
	memset( &m_Overlapped, 0, sizeof( OVERLAPPED ) );
	memset( &m_WsaBuf, 0, sizeof( WSABUF ) );
	m_SessionObject->AddRef();
}

ClientSession::ClientSession(): m_Buffer( BUFSIZE ), m_Connected( 0 ), m_RefCount( 0 )
{
	memset( &m_ClientAddr, 0, sizeof( SOCKADDR_IN ) );
	m_Socket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );
}


void ClientSession::SessionReset()
{
	m_Connected = 0;
	m_RefCount = 0;
	memset( &m_ClientAddr, 0, sizeof( SOCKADDR_IN ) );

	m_Buffer.BufferReset();

	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	if ( SOCKET_ERROR == setsockopt( m_Socket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof( LINGER ) ) )
	{
		printf_s( "[DEBUG] setsockopt linger option error: %d\n", GetLastError() );
	}

	// 끊어버리고
	closesocket( m_Socket );

	// 새것으로 교체 ㅇㅅㅇ
	m_Socket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );
}

bool ClientSession::PostAccept()
{
	CRASH_ASSERT( LThreadType == THREAD_MAIN );

	OverlappedAcceptContext* acceptContext = new OverlappedAcceptContext( this );

	//TODO : AccpetEx를 이용한 구현. -> 구현
	DWORD dwBytes = 0;
	DWORD flags = 0;
	acceptContext->m_WsaBuf.buf = m_Buffer.GetBuffer();
	acceptContext->m_WsaBuf.len = m_Buffer.GetFreeSpaceSize();
	
	if ( FALSE == IocpManager::AcceptEx( *( g_IocpManager->GetListenSocket() ), m_Socket, acceptContext->m_WsaBuf.buf, 0,
		sizeof( SOCKADDR_IN ) + 16, sizeof( SOCKADDR_IN ) + 16, &dwBytes, (LPOVERLAPPED)acceptContext ) )
	{
		if ( WSA_IO_PENDING != WSAGetLastError() )
		{
			DeleteIoContext( acceptContext );
			printf_s( "AcceptEx Failed with Error Code %d \n", GetLastError() );

			return false;
		}
	}

	return true;
}

void ClientSession::AcceptCompletion()
{
	CRASH_ASSERT( LThreadType == THREAD_IO_WORKER );

	// InterlockedExchange는 이전 값과 지금 값을 원자적으로(?) 교체해줌
	// 이전 값이 1이었다는건 이미 접속 중인 세션에 중복 접속시도
	if ( 1 == InterlockedExchange( &m_Connected, 1 ) )
	{
		/// already exists?
		CRASH_ASSERT( false );
		return;
	}

	bool resultOk = true;
	do
	{
		if ( SOCKET_ERROR == setsockopt( m_Socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)g_IocpManager->GetListenSocket(), sizeof( SOCKET ) ) )
		{
			printf_s( "[DEBUG] SO_UPDATE_ACCEPT_CONTEXT error: %d\n", GetLastError() );
			resultOk = false;
			break;
		}

		int opt = 1;
		if ( SOCKET_ERROR == setsockopt( m_Socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof( int ) ) )
		{
			printf_s( "[DEBUG] TCP_NODELAY error: %d\n", GetLastError() );
			resultOk = false;
			break;
		}

		opt = 0;
		if ( SOCKET_ERROR == setsockopt( m_Socket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof( int ) ) )
		{
			printf_s( "[DEBUG] SO_RCVBUF change error: %d\n", GetLastError() );
			resultOk = false;
			break;
		}

		int addrlen = sizeof( SOCKADDR_IN );
		if ( SOCKET_ERROR == getpeername( m_Socket, (SOCKADDR*)&m_ClientAddr, &addrlen ) )
		{
			printf_s( "[DEBUG] getpeername error: %d\n", GetLastError() );
			resultOk = false;
			break;
		}

		//TODO: CreateIoCompletionPort를 이용한 소켓 연결 -> 구현
		//HANDLE handle = CreateIoCompletionPort(...);
		HANDLE handle = CreateIoCompletionPort( (HANDLE)m_Socket, g_IocpManager->GetComletionPort(), ( ULONG_PTR )this, 0 );
		if ( handle != g_IocpManager->GetComletionPort() )
		{
			printf_s( "[DEBUG] CreateIoCompletionPort Error in Client AcceptCompletion : %d \n", GetLastError );
			resultOk = false;
			break;
		}

	} while ( false );

	if ( !resultOk )
	{
		DisconnectRequest( DR_ONCONNECT_ERROR );
		return;
	}

	printf_s( "[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa( m_ClientAddr.sin_addr ), ntohs( m_ClientAddr.sin_port ) );

	if ( false == PreRecv() )
	{
		printf_s( "[DEBUG] PreRecv error: %d\n", GetLastError() );
	}
}


void ClientSession::DisconnectRequest( DisconnectReason dr )
{
	/// 이미 끊겼거나 끊기는 중이거나
	if ( 0 == InterlockedExchange( &m_Connected, 0 ) )
	{
		return;
	}

	OverlappedDisconnectContext* context = new OverlappedDisconnectContext( this, dr );

	//TODO: DisconnectEx를 이용한 연결 끊기 요청 <- 구현
	if ( FALSE == IocpManager::DisconnectEx( m_Socket, (LPOVERLAPPED)context, TF_REUSE_SOCKET, 0 ) )
	{
		if ( WSA_IO_PENDING != WSAGetLastError() )
		{
			DeleteIoContext( context );
			printf_s( "[Debug] ClientSession DisconnectRequest Error : %d \n", GetLastError() );
		}
	}
}

void ClientSession::DisconnectCompletion( DisconnectReason dr )
{
	printf_s( "[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa( m_ClientAddr.sin_addr ), ntohs( m_ClientAddr.sin_port ) );

	/// release refcount when added at issuing a session
	ReleaseRef();
}


bool ClientSession::PreRecv()
{
	if ( !IsConnected() )
	{
		return false;
	}

	OverlappedPreRecvContext* recvContext = new OverlappedPreRecvContext( this );

	//TODO: zero-byte recv 구현 <- 구현
	DWORD recvBytes = 0;
	DWORD flags = 0;
	recvContext->m_WsaBuf.len = 0;
	recvContext->m_WsaBuf.buf = nullptr;

	if ( SOCKET_ERROR == WSARecv( m_Socket, &recvContext->m_WsaBuf, 1, &recvBytes, &flags,
		(LPWSAOVERLAPPED)recvContext, NULL ) )
	{
		if ( WSA_IO_PENDING != WSAGetLastError() )
		{
			DeleteIoContext( recvContext );
			printf_s( "[DEBUG] ClientSession::PreRecv Error : %d \n", GetLastError() );

			return false;
		}
	}

	return true;
}

bool ClientSession::PostRecv()
{
	if ( !IsConnected() )
	{
		return false;
	}

	FastSpinlockGuard criticalSection( m_BufferLock );

	if ( 0 == m_Buffer.GetFreeSpaceSize() )
	{
		return false;
	}

	OverlappedRecvContext* recvContext = new OverlappedRecvContext( this );

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->m_WsaBuf.len = (ULONG)m_Buffer.GetFreeSpaceSize();
	recvContext->m_WsaBuf.buf = m_Buffer.GetBuffer();

	/// start real recv
	if ( SOCKET_ERROR == WSARecv( m_Socket, &recvContext->m_WsaBuf, 1, &recvbytes, &flags,
		(LPWSAOVERLAPPED)recvContext, NULL ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( recvContext );
			printf_s( "[DEBUG] ClientSession::PostRecv Error : %d \n", GetLastError() );

			return false;
		}
	}

	return true;
}

void ClientSession::RecvCompletion(DWORD transferred)
{
	FastSpinlockGuard criticalSection(m_BufferLock);

	m_Buffer.Commit(transferred);
}

bool ClientSession::PostSend()
{
	if ( !IsConnected() )
	{
		return false;
	}

	FastSpinlockGuard criticalSection( m_BufferLock );

	if ( 0 == m_Buffer.GetContiguiousBytes() )
	{
		return true;
	}

	OverlappedSendContext* sendContext = new OverlappedSendContext( this );

	DWORD sendbytes = 0;
	DWORD flags = 0;
	sendContext->m_WsaBuf.len = (ULONG)m_Buffer.GetContiguiousBytes();
	sendContext->m_WsaBuf.buf = m_Buffer.GetBufferStart();

	/// start async send
	if ( SOCKET_ERROR == WSASend( m_Socket, &sendContext->m_WsaBuf, 1, &sendbytes, flags,
		(LPWSAOVERLAPPED)sendContext, NULL ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( sendContext );
			printf_s( "[DEBUG] ClientSession::PostSend Error : %d \n", GetLastError() );

			return false;
		}
	}

	return true;
}

void ClientSession::SendCompletion(DWORD transferred)
{
	FastSpinlockGuard criticalSection(m_BufferLock);

	m_Buffer.Remove(transferred);
}


void ClientSession::AddRef()
{
	CRASH_ASSERT(InterlockedIncrement(&m_RefCount) > 0);
}

void ClientSession::ReleaseRef()
{
	long ret = InterlockedDecrement(&m_RefCount);
	CRASH_ASSERT(ret >= 0);
	
	if (ret == 0)
	{
		g_SessionManager->ReturnClientSession(this);
	}
}


void DeleteIoContext( OverlappedIOContext* context )
{
	if ( nullptr == context )
	{
		return;
	}

	context->m_SessionObject->ReleaseRef();

	delete context;
}
