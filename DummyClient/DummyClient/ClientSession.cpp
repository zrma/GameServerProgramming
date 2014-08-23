#include "stdafx.h"
#include "Exception.h"
#include "ThreadLocal.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"
#include "FastSpinlock.h"
#include "DummyClient.h"

OverlappedIOContext::OverlappedIOContext(ClientSession* owner, IOType ioType) 
: mSessionObject(owner), mIoType(ioType)
{
	memset(&mOverlapped, 0, sizeof(OVERLAPPED));
	memset(&mWsaBuf, 0, sizeof(WSABUF));
	mSessionObject->AddRef();
}

ClientSession::ClientSession() : mBuffer(BUFFER_SIZE), mConnected(0), mRefCount(0)
, mBufferLock(LO_LUGGAGE_CLASS)
{
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));
	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	struct sockaddr_in addr;
	ZeroMemory( &addr, sizeof( addr ) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = 0;
	int result = bind( mSocket, (SOCKADDR*)&addr, sizeof( addr ) );

	if ( result != 0 )
	{
		printf( "Bind failed: %d\n", WSAGetLastError() );
	}
}


void ClientSession::SessionReset()
{
	mConnected = 0;
	mRefCount = 0;
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));

	mBuffer.BufferReset();

	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER)))
	{
		printf_s("[DEBUG] setsockopt linger option error: %d\n", GetLastError());
	}
	closesocket(mSocket);

	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}


bool ClientSession::PostConnect()
{
	CRASH_ASSERT( LThreadType == THREAD_MAIN );

	HANDLE handle = CreateIoCompletionPort( (HANDLE)mSocket, GIocpManager->GetComletionPort(), ( ULONG_PTR )this, 0 );
	if ( handle != GIocpManager->GetComletionPort() )
	{
		printf_s( "[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError() );
		return false;
	}

	OverlappedConnectContext* connectContext = new OverlappedConnectContext( this );
	DWORD bytes = 0;
	connectContext->mWsaBuf.len = 0;
	connectContext->mWsaBuf.buf = nullptr;
	
	if ( SERVER_PORT <= 4000 || SERVER_PORT > 10000 )
	{
		SERVER_PORT = 9001;
	}

	if ( HOST_NAME.length() == 0 )
	{
		HOST_NAME = "127.0.0.1";
	}

	sockaddr_in addr;
	ZeroMemory( &addr, sizeof( addr ) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr( HOST_NAME.c_str() );
	addr.sin_port = htons( SERVER_PORT );

	if ( FALSE == IocpManager::ConnectEx( mSocket, (SOCKADDR*)&addr, sizeof( addr ), 
		GIocpManager->mConnectBuf, 0, &bytes, (LPOVERLAPPED)connectContext ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( connectContext );
			printf_s( "ConnectEx Error : %d\n", GetLastError() );

			return false;
		}
	}
	
	return true;
}

void ClientSession::ConnectCompletion()
{
	CRASH_ASSERT( LThreadType == THREAD_IO_WORKER );
	if ( 1 == InterlockedExchange( &mConnected, 1 ) )
	{
		/// already exists?
		CRASH_ASSERT( false );
		return;
	}

	bool resultOk = true;

	do
	{
		if ( SOCKET_ERROR == setsockopt( mSocket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0 ) )
		{
			printf_s( "[DEBUG] SO_UPDATE_CONNECT_CONTEXT error: %d\n", GetLastError() );
			resultOk = false;
			break;
		}

		int opt = 1;
		if ( NO_DELAY )
		{
			int opt = 1;
			if ( SOCKET_ERROR == setsockopt( mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof( int ) ) )
			{
				printf_s( "[DEBUG] TCP_NODELAY error: %d\n", GetLastError() );
				resultOk = false;
				break;
			}
		}

		opt = 0;
		if ( SOCKET_ERROR == setsockopt( mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof( int ) ) )
		{
			printf_s( "[DEBUG] SO_RCVBUF change error: %d\n", GetLastError() );
			resultOk = false;
			break;
		}

		int addrlen = sizeof( SOCKADDR_IN );
		if ( SOCKET_ERROR == getpeername( mSocket, (SOCKADDR*)&mClientAddr, &addrlen ) )
		{
			printf_s( "[DEBUG] getpeername error: %d\n", GetLastError() );
			resultOk = false;
			break;
		}

	} while ( false );
	
	if ( !resultOk )
	{
		DisconnectRequest( DR_ONCONNECT_ERROR );
		return;
	}

	FastSpinlockGuard criticalSection( mBufferLock );

	if ( BUFFER_SIZE <= 0 || BUFFER_SIZE > BUFSIZE )
	{
		BUFFER_SIZE = 4096;
	}

	char* temp = new char[BUFFER_SIZE];
	
	ZeroMemory( temp, sizeof( char ) * BUFFER_SIZE );
	for ( int i = 0; i < BUFFER_SIZE - 1; ++i )
	{
		temp[i] = 'a' + ( mSocket % 26 );
	}

	temp[BUFFER_SIZE - 1] = '\0';

	char* bufferStart = mBuffer.GetBuffer();
	memcpy( bufferStart, temp, BUFFER_SIZE );

	mBuffer.Commit( BUFFER_SIZE );
		
	CRASH_ASSERT( 0 != mBuffer.GetContiguiousBytes() );
		
	OverlappedSendContext* sendContext = new OverlappedSendContext( this );

	DWORD sendbytes = 0;
	DWORD flags = 0;
	sendContext->mWsaBuf.len = (ULONG)mBuffer.GetContiguiousBytes();
	sendContext->mWsaBuf.buf = mBuffer.GetBufferStart();

	delete[] temp;

	/// start async send
	if ( SOCKET_ERROR == WSASend( mSocket, &sendContext->mWsaBuf, 1, &sendbytes, flags, (LPWSAOVERLAPPED)sendContext, NULL ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( sendContext );
			printf_s( "ClientSession::PostSend Error : %d\n", GetLastError() );
		}
	}

	++mUseCount;
}

void ClientSession::DisconnectRequest(DisconnectReason dr)
{
	/// 이미 끊겼거나 끊기는 중이거나
	if (0 == InterlockedExchange(&mConnected, 0))
		return ;
	
	OverlappedDisconnectContext* context = new OverlappedDisconnectContext(this, dr);

	if (FALSE == IocpManager::DisconnectEx(mSocket, (LPWSAOVERLAPPED)context, TF_REUSE_SOCKET, 0))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(context);
			printf_s("ClientSession::DisconnectRequest Error : %d\n", GetLastError());
		}
	}
}

void ClientSession::DisconnectCompletion(DisconnectReason dr)
{
	printf_s("[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	/// release refcount when added at issuing a session
	ReleaseRef();
}


bool ClientSession::PreRecv()
{
	if (!IsConnected())
		return false;

	OverlappedPreRecvContext* recvContext = new OverlappedPreRecvContext(this);

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->mWsaBuf.len = 0;
	recvContext->mWsaBuf.buf = nullptr;

	/// start async recv
	if (SOCKET_ERROR == WSARecv(mSocket, &recvContext->mWsaBuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(recvContext);
			printf_s("ClientSession::PreRecv Error : %d\n", GetLastError());
			return false;
		}
	}

	return true;
}

bool ClientSession::PostRecv()
{
	if (!IsConnected())
		return false;

	FastSpinlockGuard criticalSection(mBufferLock);

	if (0 == mBuffer.GetFreeSpaceSize())
		return false;

	OverlappedRecvContext* recvContext = new OverlappedRecvContext(this);

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->mWsaBuf.len = (ULONG)mBuffer.GetFreeSpaceSize();
	recvContext->mWsaBuf.buf = mBuffer.GetBuffer();
	

	/// start real recv
	if (SOCKET_ERROR == WSARecv(mSocket, &recvContext->mWsaBuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(recvContext);
			printf_s("ClientSession::PostRecv Error : %d\n", GetLastError());
			return false;
		}
			
	}

	return true;
}

void ClientSession::RecvCompletion(DWORD transferred)
{
	FastSpinlockGuard criticalSection(mBufferLock);

	if ( *( mBuffer.GetBuffer() ) != ( 'a' + ( mSocket % 26 ) ) )
	{
		printf_s( "다른 데이터가 왔다! %c / %c \n", *( mBuffer.GetBuffer() ), 'a' + mSocket % 26 );
	}

	mBuffer.Commit(transferred);
	mRecvBytes += transferred;

	mBuffer.GetBuffer();
}

bool ClientSession::PostSend()
{
	if (!IsConnected())
		return false;

	FastSpinlockGuard criticalSection(mBufferLock);

	if ( 0 == mBuffer.GetContiguiousBytes() )
		return true;

	OverlappedSendContext* sendContext = new OverlappedSendContext(this);

	DWORD sendbytes = 0;
	DWORD flags = 0;
	sendContext->mWsaBuf.len = (ULONG) mBuffer.GetContiguiousBytes(); 
	sendContext->mWsaBuf.buf = mBuffer.GetBufferStart();

	/// start async send
	if (SOCKET_ERROR == WSASend(mSocket, &sendContext->mWsaBuf, 1, &sendbytes, flags, (LPWSAOVERLAPPED)sendContext, NULL))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(sendContext);
			printf_s("ClientSession::PostSend Error : %d\n", GetLastError());

			return false;
		}
	}

	return true;
}

void ClientSession::SendCompletion(DWORD transferred)
{
	FastSpinlockGuard criticalSection(mBufferLock);

	mBuffer.Remove(transferred);
	mSendBytes += transferred;
}

void ClientSession::AddRef()
{
	CRASH_ASSERT(InterlockedIncrement(&mRefCount) > 0);
}

void ClientSession::ReleaseRef()
{
	long ret = InterlockedDecrement(&mRefCount);
	CRASH_ASSERT(ret >= 0);
	
	if (ret == 0)
	{
		GSessionManager->ReturnClientSession(this);
	}
}

void DeleteIoContext(OverlappedIOContext* context)
{
	if (nullptr == context)
		return;

	context->mSessionObject->ReleaseRef();

	/// ObjectPool's operate delete dispatch
	switch (context->mIoType)
	{
	case IO_SEND:
		delete static_cast<OverlappedSendContext*>(context);
		break;

	case IO_RECV_ZERO:
		delete static_cast<OverlappedPreRecvContext*>(context);
		break;

	case IO_RECV:
		delete static_cast<OverlappedRecvContext*>(context);
		break;

	case IO_DISCONNECT:
		delete static_cast<OverlappedDisconnectContext*>(context);
		break;

	case IO_CONNECT:
		delete static_cast<OverlappedConnectContext*>(context);
		break;

	default:
		CRASH_ASSERT(false);
	}

	
}

