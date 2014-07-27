#include "stdafx.h"
#include "IocpManager.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "Exception.h"

#define GQCS_TIMEOUT	20

__declspec(thread) int l_IoThreadId = 0;
IocpManager* g_IocpManager = nullptr;

IocpManager::IocpManager(): m_CompletionPort( NULL ), m_IoThreadCount( 2 ), m_ListenSocket( NULL )
{
}

IocpManager::~IocpManager()
{
}

bool IocpManager::Initialize()
{
	// TODO: mIoThreadCount = ...;GetSystemInfo사용해서 set num of I/O threads -> 구현
	// 참고 : http://msdn.microsoft.com/en-us/library/ms724953(v=vs.85).aspx
	// 참고 : http://msdn.microsoft.com/en-us/library/ms724381(v=vs.85).aspx
	// 참고 : http://msdn.microsoft.com/en-us/library/ms724958(v=vs.85).aspx

	SYSTEM_INFO sysInfo;
	ZeroMemory( &sysInfo, sizeof( SYSTEM_INFO ) );
	GetSystemInfo( &sysInfo );

	m_IoThreadCount = (int)(sysInfo.dwNumberOfProcessors) * 2;

	/// winsock initializing
	WSADATA wsa;
	if ( WSAStartup( MAKEWORD( 2, 2 ), &wsa ) != 0 )
	{
		return false;
	}

	/// Create I/O Completion Port
	// TODO: mCompletionPort = CreateIoCompletionPort(...) -> 구현
	
	m_CompletionPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
	if ( !m_CompletionPort )
	{
		printf_s( "Create IOCP Failed with Error Code %d \n", GetLastError() );
		return false;
	}
		
	// 참고 : http://k1rha.tistory.com/entry/IOCP-Server-Client-C-%EC%98%88%EC%A0%9C%EC%BD%94%EB%93%9C-winsock2-IOCP-Server-Client-using-winsock2-via-C
	// 참고 : http://copynull.tistory.com/30

	/// create TCP socket
	// TODO: mListenSocket = ... -> 구현
	if ( INVALID_SOCKET == ( m_ListenSocket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED ) ) )
	{
		printf_s( "Socket() Failed with Error Code %d \n", WSAGetLastError() );
		return false;
	}

	int opt = 1;
	setsockopt(m_ListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	//TODO:  bind
	//if (SOCKET_ERROR == bind(mListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr))) -> 구현
	//	return false;
	SOCKADDR_IN	addr;
	ZeroMemory( &addr, sizeof( SOCKADDR_IN ) );

	addr.sin_family = AF_INET;
	addr.sin_port = htons( 9001 );
	addr.sin_addr.s_addr = htons( INADDR_ANY );

	int result = 0;

	if ( SOCKET_ERROR == ( result = bind( m_ListenSocket, (SOCKADDR*)&addr, sizeof( addr ) ) ) )
	{
		printf_s( "Bind() Failed with Error Code %d \n", WSAGetLastError() );
		return false;
	}

	return true;
}


bool IocpManager::StartIoThreads()
{
	/// I/O Thread
	for (int i = 0; i < m_IoThreadCount; ++i)
	{
		DWORD dwThreadId;
		//TODO: HANDLE hThread = (HANDLE)_beginthreadex...); -> 구현
		HANDLE hThread = (HANDLE)_beginthreadex( NULL, 0, IoWorkerThread, (LPVOID)i, 0, (unsigned int*)&dwThreadId );

		if ( !hThread )
		{
			printf_s( "Thread Creation Failed with Error Code %d \n", GetLastError() );
			return false;
		}

		CloseHandle( hThread );
	}

	return true;
}


bool IocpManager::StartAcceptLoop()
{
	/// listen
	if ( SOCKET_ERROR == listen( m_ListenSocket, SOMAXCONN ) )
	{
		return false;
	}

	/// accept loop
	while (true)
	{
		SOCKET acceptedSock = accept(m_ListenSocket, NULL, NULL);

		if (acceptedSock == INVALID_SOCKET)
		{
			printf_s("accept: invalid socket\n");
			continue;
		}

		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(acceptedSock, (SOCKADDR*)&clientaddr, &addrlen);

		/// 소켓 정보 구조체 할당과 초기화
		ClientSession* client = g_SessionManager->CreateClientSession(acceptedSock);

		/// 클라 접속 처리
		if (false == client->OnConnect(&clientaddr))
		{
			client->Disconnect(DR_ONCONNECT_ERROR);
			g_SessionManager->DeleteClientSession(client);
		}
	}

	return true;
}

void IocpManager::Finalize()
{
	CloseHandle(m_CompletionPort);

	/// winsock finalizing
	WSACleanup();
}


unsigned int WINAPI IocpManager::IoWorkerThread(LPVOID lpParam)
{
	l_ThreadType = THREAD_IO_WORKER; ///< 이건 스레드 로컬 변수다. 전역이 아니라.  -> 확인
	l_IoThreadId = reinterpret_cast<int>(lpParam); ///< 스레드 로컬 변수인데 g_ 태그 붙이는거는 적절하지 못하다.  -> 확인

	HANDLE hComletionPort = g_IocpManager->GetComletionPort();

	while (true)
	{
		DWORD dwTransferred = 0;
		OverlappedIOContext* context = nullptr;
		ClientSession* asCompletionKey = nullptr;

		// int ret = 0; ///<여기에는 GetQueuedCompletionStatus(hComletionPort, ..., GQCS_TIMEOUT)를 수행한 결과값을 대입 -> 구현
		int ret = GetQueuedCompletionStatus( hComletionPort, &dwTransferred, (PULONG_PTR)&asCompletionKey, (LPOVERLAPPED*)&context, GQCS_TIMEOUT );

		/// check time out first 
		if ( ret == 0 && GetLastError() == WAIT_TIMEOUT )
		{
			// 시간 초과요
			continue;
		}

		// 에러가 났거나, 타임오버가 아닌데 받았다고 해놓고, 받은 데이터가 없음
		if (ret == 0 || dwTransferred == 0)
		{
			/// connection closing
			asCompletionKey->Disconnect(DR_RECV_ZERO); ///< dr옵션 그냥 가져다 썼구만.. ㅋㅋㅋ
			g_SessionManager->DeleteClientSession(asCompletionKey);
			continue;
		}

		//TODO
		// if (nullptr == context) 인 경우 처리 -> 구현
		//{
		//}

		// 참고 : http://stackoverflow.com/questions/5830699/getqueuedcompletionstatus-delayed
		// 참고 : http://msdn.microsoft.com/en-us/library/windows/desktop/aa364986(v=vs.85).aspx
		// If *lpOverlapped is NULL, the function did not dequeue a completion packet from the completion port.
		// In this case, the function does not store information in the variables pointed to by the 
		// lpNumberOfBytes and lpCompletionKey parameters, and their values are indeterminate.
		if ( !context )
		{
			// 한 바퀴 더 돌아라
			// continue; ///< 끊거나 서버 종료 해야지... context == null 인 상황중에 정상인 경우는 앞에서 다 처리했다.  -> 확인

			// 자료구조가 깨졌다!
			CRASH_ASSERT( false );
			
			return -1;
		}

		bool completionOk = true;
		switch (context->m_IoType)
		{
		case IO_SEND:
			completionOk = SendCompletion(asCompletionKey, context, dwTransferred);
			break;

		case IO_RECV:
			completionOk = ReceiveCompletion(asCompletionKey, context, dwTransferred);
			break;

		default:
			printf_s("Unknown I/O Type: %d\n", context->m_IoType);
			break;
		}

		if ( !completionOk )
		{
			/// connection closing
			asCompletionKey->Disconnect(DR_COMPLETION_ERROR);
			g_SessionManager->DeleteClientSession(asCompletionKey);
		}

	}

	return 0;
}

bool IocpManager::ReceiveCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{
	/// echo back 처리 client->PostSend()사용. -> 구현
	if ( !client )
	{
		delete context;
		return false;
	}
	
	// client->PostSend( context->m_Buffer, dwTransferred ); ///< postsend가 false를 리턴하면?  -> 확인
	if ( !client->PostSend( context->m_Buffer, dwTransferred ) )
	{
		delete context;
		return false;
	}

	delete context;
	return client->PostRecv();
}

bool IocpManager::SendCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{
	/// 전송 다 되었는지 확인하는 것 처리..
	//if (context->mWsaBuf.len != dwTransferred) {...}
	if ( context->m_WsaBuf.len != dwTransferred )
	{
		printf_s( "Buffer != Sent Error - Buf : %d, Send %d \n", context->m_WsaBuf.len, dwTransferred );

		delete context;
		return false;
	}
	
	delete context;
	return true;
}
