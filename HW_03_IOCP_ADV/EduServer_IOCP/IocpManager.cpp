#include "stdafx.h"
#include "Exception.h"
#include "IocpManager.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"

// #define GQCS_TIMEOUT	INFINITE //20 -> 무한대로 대기 태우지 않도록...
#define GQCS_TIMEOUT	20

__declspec(thread) int l_IoThreadId = 0;
IocpManager* g_IocpManager = nullptr;

LPFN_ACCEPTEX IocpManager::m_FnAcceptEx = nullptr;
LPFN_DISCONNECTEX IocpManager::m_FnDisconnectEx = nullptr;

//TODO AcceptEx DisconnectEx 함수 사용할 수 있도록 구현. -> 구현
BOOL IocpManager::DisconnectEx(SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD dwReserved)
{
	// return ...
	// return 0;
		
	if ( IocpManager::m_FnDisconnectEx )
	{
		return IocpManager::m_FnDisconnectEx( hSocket, lpOverlapped, dwFlags, dwReserved );
	}

	return FALSE;
}

BOOL IocpManager::AcceptEx( SOCKET sListenSocket, SOCKET sAcceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength,
	DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped)
{
	// return ...
	// return 0;

	if ( IocpManager::m_FnAcceptEx )
	{
		return IocpManager::m_FnAcceptEx( sListenSocket, sAcceptSocket, lpOutputBuffer, dwReceiveDataLength,
										  dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived, lpOverlapped );
	}

	return FALSE;
}

IocpManager::IocpManager(): m_CompletionPort( NULL ), m_IoThreadCount( 2 ), m_ListenSocket( NULL )
{
	ZeroMemory( m_AcceptBuffer, sizeof( m_AcceptBuffer ) );
}


IocpManager::~IocpManager()
{
}

bool IocpManager::Initialize()
{
	/// set num of I/O threads
	SYSTEM_INFO si;
	GetSystemInfo( &si );
	m_IoThreadCount = si.dwNumberOfProcessors;

	/// winsock initializing
	WSADATA wsa;
	if ( WSAStartup( MAKEWORD( 2, 2 ), &wsa ) != 0 )
	{
		return false;
	}

	/// Create I/O Completion Port
	m_CompletionPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
	if ( m_CompletionPort == NULL )
	{
		return false;
	}

	/// create TCP socket
	m_ListenSocket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );
	if ( m_ListenSocket == INVALID_SOCKET )
	{
		return false;
	}

	HANDLE handle = CreateIoCompletionPort( (HANDLE)m_ListenSocket, m_CompletionPort, 0, 0 );
	if ( handle != m_CompletionPort )
	{
		printf_s( "[DEBUG] listen socket IOCP register error: %d\n", GetLastError() );
		return false;
	}

	int opt = 1;
	setsockopt( m_ListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof( int ) );

	/// bind
	SOCKADDR_IN serveraddr;
	ZeroMemory( &serveraddr, sizeof( serveraddr ) );
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons( LISTEN_PORT );
	serveraddr.sin_addr.s_addr = htonl( INADDR_ANY );

	if ( SOCKET_ERROR == bind( m_ListenSocket, (SOCKADDR*)&serveraddr, sizeof( serveraddr ) ) )
	{
		return false;
	}

	//TODO : WSAIoctl을 이용하여 AcceptEx, DisconnectEx 함수 사용가능하도록 하는 작업.. -> 구현
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes = 0;

	if ( SOCKET_ERROR == WSAIoctl( m_ListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx, sizeof( GUID ), &m_FnAcceptEx, sizeof( LPFN_ACCEPTEX ), &dwBytes, NULL, NULL ) )
	{
		printf_s( "[Debug] WSAIoctl - AcceptEx Error : %d \n", WSAGetLastError() );
		return false;
	}

	GUID guidDisconnectEx = WSAID_DISCONNECTEX;

	if ( SOCKET_ERROR == WSAIoctl( m_ListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidDisconnectEx, sizeof( GUID ), &m_FnDisconnectEx, sizeof( LPFN_DISCONNECTEX ), &dwBytes, NULL, NULL ) )
	{
		printf_s( "[Debug] WSAIoctl - DisconnectEx Error : %d \n", WSAGetLastError() );
		return false;
	}

	/// make session pool
	g_SessionManager->PrepareSessions();

	return true;
}

bool IocpManager::StartIoThreads()
{
	/// I/O Thread
	for ( int i = 0; i < m_IoThreadCount; ++i )
	{
		DWORD dwThreadId;
		HANDLE hThread = (HANDLE)_beginthreadex( NULL, 0, IoWorkerThread, (LPVOID)( i + 1 ), 0, (unsigned int*)&dwThreadId );
		if ( hThread == NULL )
		{
			return false;
		}
	}

	return true;
}

void IocpManager::StartAccept()
{
	/// listen
	if ( SOCKET_ERROR == listen( m_ListenSocket, SOMAXCONN ) )
	{
		printf_s( "[DEBUG] listen error\n" );
		return;
	}

	while ( g_SessionManager->AcceptSessions() )
	{
		Sleep( 100 );
	}
}

void IocpManager::Finalize()
{
	CloseHandle( m_CompletionPort );

	/// winsock finalizing
	WSACleanup();
}

unsigned int WINAPI IocpManager::IoWorkerThread(LPVOID lpParam)
{
	LThreadType = THREAD_IO_WORKER;

	l_IoThreadId = reinterpret_cast<int>(lpParam);
	HANDLE hComletionPort = g_IocpManager->GetComletionPort();

	while (true)
	{
		DWORD dwTransferred = 0;
		OverlappedIOContext* context = nullptr;
		ULONG_PTR completionKey = 0;

		int ret = GetQueuedCompletionStatus(hComletionPort, &dwTransferred, (PULONG_PTR)&completionKey, (LPOVERLAPPED*)&context, GQCS_TIMEOUT);

		ClientSession* theClient = context ? context->m_SessionObject : nullptr ;
		
		// GQCS 결과 뭔가 문제가 있다(FALSE 리턴이거나, 0바이트 받았음)
		if (ret == 0 || dwTransferred == 0)
		{
			int gle = GetLastError();

			//TODO: check time out first ... GQCS 타임 아웃의 경우는 어떻게? -> 구현
			// if ( WAIT_TIMEOUT == gle && ret == 0 )
			if ( WAIT_TIMEOUT == gle && nullptr == context ) // by sm9
			{
				// 시간 초과요! = 세이프!
				continue;
			}

			// 아래에서 context 참조할 건데 nullptr 아니라는 보장은?!
			CRASH_ASSERT( nullptr != context );
						
			// 뭔가 IO_RECV나 IO_SEND 였다면 DR_COMPLETION_ERROR 보내고 끊어버림
			if (context->m_IoType == IO_RECV || context->m_IoType == IO_SEND )
			{
				CRASH_ASSERT(nullptr != theClient);
			
				theClient->DisconnectRequest(DR_COMPLETION_ERROR);
				DeleteIoContext(context);

				continue;
			}
		}

		CRASH_ASSERT( nullptr != theClient );
	
		bool completionOk = false;
		switch (context->m_IoType)
		{
		case IO_DISCONNECT:
			theClient->DisconnectCompletion(static_cast<OverlappedDisconnectContext*>(context)->mDisconnectReason);
			completionOk = true;
			break;

		case IO_ACCEPT:
			theClient->AcceptCompletion();
			completionOk = true;
			break;

		case IO_RECV_ZERO:
			completionOk = PreReceiveCompletion(theClient, static_cast<OverlappedPreRecvContext*>(context), dwTransferred);
			break;

		case IO_SEND:
			completionOk = SendCompletion(theClient, static_cast<OverlappedSendContext*>(context), dwTransferred);
			break;

		case IO_RECV:
			completionOk = ReceiveCompletion(theClient, static_cast<OverlappedRecvContext*>(context), dwTransferred);
			break;

		default:
			printf_s("Unknown I/O Type: %d\n", context->m_IoType);
			CRASH_ASSERT(false);
			break;
		}

		if ( !completionOk )
		{
			/// connection closing
			theClient->DisconnectRequest(DR_IO_REQUEST_ERROR);
		}

		DeleteIoContext(context);
	}

	return 0;
}

bool IocpManager::PreReceiveCompletion(ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred)
{
	/// real receive...
	// return client->PreRecv(); -> 확인

	return client->PostRecv();
}

bool IocpManager::ReceiveCompletion(ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred)
{
	client->RecvCompletion(dwTransferred);

	/// echo back
	return client->PostSend();
}

bool IocpManager::SendCompletion(ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred)
{
	client->SendCompletion(dwTransferred);

	if (context->m_WsaBuf.len != dwTransferred)
	{
		printf_s("Partial SendCompletion requested [%d], sent [%d]\n", context->m_WsaBuf.len, dwTransferred) ;
		return false;
	}
	
	/// zero receive
	return client->PreRecv();
}
