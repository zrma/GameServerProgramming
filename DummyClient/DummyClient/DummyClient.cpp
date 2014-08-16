// DummyClient.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "ThreadLocal.h"
#include "Exception.h"
#include "IocpManager.h"
#include "MemoryPool.h"
#include "SessionManager.h"

int MAX_CONNECTION = 100;
int LISTEN_PORT = 9001;

int _tmain(int argc, _TCHAR* argv[])
{
	LThreadType = THREAD_MAIN;

	/// for dump on crash
	SetUnhandledExceptionFilter( ExceptionFilter );

	/// Global Managers
	GMemoryPool = new MemoryPool;
	GSessionManager = new SessionManager;
	GIocpManager = new IocpManager;

	if ( false == GIocpManager->Initialize() )
		return -1;

	if ( false == GIocpManager->StartIoThreads() )
		return -1;

// 	printf_s( "Host      : \n" );
// 	printf_s( "Port      : \n" );
// 	printf_s( "Thread    : \n" );
// 	printf_s( "Sessions  : \n" );
// 	printf_s( "Session's buffer size : \n" );
// 	printf_s( "Time (seconds): \n" );

	GIocpManager->StartConnect();
	GIocpManager->Finalize();

	printf_s( "Total sessions connected : %ld \n", GIocpManager->GetConnectCount() );
	printf_s( "Total bytes written      : %lld \n", GIocpManager->GetSendCount() );
	printf_s( "Total bytes read         : %lld \n", GIocpManager->GetRecvCount() );

	delete GIocpManager;
	delete GSessionManager;
	delete GMemoryPool;

	getchar();

	return 0;
}
