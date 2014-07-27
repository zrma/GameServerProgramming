// EduServer_IOCP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Exception.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "IocpManager.h"

__declspec(thread) int LThreadType = -1;

int _tmain(int argc, _TCHAR* argv[])
{
	LThreadType = THREAD_MAIN;

	/// for dump on crash
	SetUnhandledExceptionFilter(ExceptionFilter);

	/// Global Managers
	g_SessionManager = new SessionManager;
	g_IocpManager = new IocpManager;

	if ( false == g_IocpManager->Initialize() )
	{
		return -1;
	}

	if ( false == g_IocpManager->StartIoThreads() )
	{
		return -1;
	}
	
	printf_s("Start Server\n");
	g_IocpManager->StartAccept(); ///< block here...  <- 여기가 무한루프

	g_IocpManager->Finalize();
	printf_s("End Server\n");

	delete g_IocpManager;
	delete g_SessionManager;

	return 0;
}

