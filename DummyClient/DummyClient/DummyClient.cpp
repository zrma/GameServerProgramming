// DummyClient.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "ThreadLocal.h"
#include "Exception.h"
#include "IocpManager.h"
#include "MemoryPool.h"
#include "SessionManager.h"
#include "ClientSession.h"

std::string HOST_NAME = "127.0.0.1";
int MAX_CONNECTION = 100;
int SERVER_PORT = 9001;
int THREAD_COUNT = 4;
int BUFFER_SIZE = 4096;
bool NO_DELAY = true;
int TIME = 10;

int optionCheck( int argc, _TCHAR* argv[] );

int _tmain(int argc, _TCHAR* argv[])
{
	LThreadType = THREAD_MAIN;

	if ( optionCheck( argc, argv ) < 2 )
	{
		printf_s( "Performance test client  \n" );
		printf_s( "Allowed options :        \n" );
		printf_s( "  --host arg             set the server address          \n" );
		printf_s( "  --port arg             set the server port             \n" );
		printf_s( "  --threads arg (=4)     set thread count                \n" );
		printf_s( "  --session arg (=100)   set the maximum connections     \n" );
		printf_s( "  --buffer arg (=4096)   set session buffer size (Bytes) \n" );
		printf_s( "  --no_delay arg (=true) set TCP_NODELAY option          \n" );
		printf_s( "  --time arg (=10)       set test duration (seconds)     \n" );

		return -1;
	}
			
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
		
	printf_s( "Host      : %s \n", HOST_NAME.c_str() );
	printf_s( "Port      : %d \n", SERVER_PORT );
	printf_s( "Thread    : %d \n", THREAD_COUNT );
	printf_s( "Sessions  : %d \n", MAX_CONNECTION );
	printf_s( "Session's buffer size : %d \n", BUFFER_SIZE );
	printf_s( "TCP_NODELAY   : %s \n", NO_DELAY ? "true" : "false" );
	printf_s( "Time (seconds): %d \n", TIME );

	GIocpManager->StartConnect();
	GIocpManager->Finalize();

	printf_s( "Total sessions connected : %ld \n", GSessionManager->GetAllSessionConnectCount() );
	printf_s( "Total bytes written      : %lld \n", GSessionManager->GetAllSessionSendBytes() );
	printf_s( "Total bytes read         : %lld \n", GSessionManager->GetAllSessionRecvBytes() );

	delete GIocpManager;
	delete GSessionManager;
	delete GMemoryPool;
	
	return 0;
}

int optionCheck( int argc, _TCHAR* argv[] )
{
	int optionCount = 0;

	for ( int i = 1; i + 1 < argc; ++i )
	{
		USES_CONVERSION;

		if ( !strcmp( W2A( argv[i] ), "--host" ) )
		{
			HOST_NAME = W2A( argv[i + 1] );

			++optionCount;
			++i;
			continue;
		}

		if ( !strcmp( W2A( argv[i] ), "--port" ) )
		{
			SERVER_PORT = atoi( W2A( argv[i + 1] ) );
			
			if ( SERVER_PORT <= 4000 || SERVER_PORT > 10000 )
			{
				SERVER_PORT = 9001;
			}

			++optionCount;
			++i;
			continue;
		}

		if ( !strcmp( W2A( argv[i] ), "--threads" ) )
		{
			THREAD_COUNT = atoi( W2A( argv[i + 1] ) );

			SYSTEM_INFO si;
			GetSystemInfo( &si );

			if ( THREAD_COUNT <= 0 || THREAD_COUNT > MAX_IO_THREAD )
			{
				THREAD_COUNT = 4;
			}

			++i;
			continue;
		}

		if ( !strcmp( W2A( argv[i] ), "--sessions" ) )
		{
			MAX_CONNECTION = atoi( W2A( argv[i + 1] ) );

			if ( MAX_CONNECTION <= 0 || MAX_CONNECTION > 1000 )
			{
				MAX_CONNECTION = 1000;
			}

			++i;
			continue;
		}

		if ( !strcmp( W2A( argv[i] ), "--buffer" ) )
		{
			BUFFER_SIZE = atoi( W2A( argv[i + 1] ) );

			if ( BUFFER_SIZE <= 0 || BUFFER_SIZE > BUFSIZE )
			{
				BUFFER_SIZE = 4096;
			}

			++i;
			continue;
		}

		if ( !strcmp( W2A( argv[i] ), "--no_delay" ) )
		{
			std::string delayOption;
			delayOption.append( W2A( argv[i + 1] ) );

			std::transform( delayOption.begin(), delayOption.end(), delayOption.begin(), tolower );
			
			if ( delayOption == "false" )
			{
				NO_DELAY = false;
			}

			++i;
			continue;
		}

		if ( !strcmp( W2A( argv[i] ), "--time" ) )
		{
			TIME = atoi( W2A( argv[i + 1] ) );

			if ( TIME <= 0 || TIME > 100000 )
			{
				TIME = 60;
			}

			++i;
			continue;
		}
	}
	
	return optionCount;
}
