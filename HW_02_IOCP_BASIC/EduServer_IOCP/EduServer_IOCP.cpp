// EduServer_IOCP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Exception.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "IocpManager.h"


__declspec(thread) int l_ThreadType = -1;

int _tmain(int argc, _TCHAR* argv[])
{
	l_ThreadType = THREAD_MAIN_ACCEPT;

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

	if ( false == g_IocpManager->StartAcceptLoop() )
	{
		return -1;
	}

	g_IocpManager->Finalize();

	printf_s("End Server\n");

	delete g_IocpManager;
	delete g_SessionManager;

	return 0;
}

