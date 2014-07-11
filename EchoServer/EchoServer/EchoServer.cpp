// EchoServer.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "EchoServer.h"

#pragma comment(lib,"ws2_32.lib")

HWND			g_hWnd = nullptr;
SOCKET			g_ListenSocket = NULL;
const WCHAR		WINDOW_NAME[] = L"EchoServer";

int _tmain(int argc, _TCHAR* argv[])
{
	g_hWnd = MakeHiddenWindow();

	if ( !g_hWnd )
	{
		printf_s( "Press Any Key \n" );
		getchar();

		return 0;
	}

	if ( !InitNetwork() )
	{
		printf_s( "Press Any Key \n" );
		getchar();

		return 0;
	}

	printf_s( "Server Start \n" );

	MSG	msg = { 0, };
	DWORD result = 0;

	while ( result = GetMessage( &msg, NULL, 0, 0 ) )
	{
		if ( -1 == result )
		{
			printf_s( "GetMessage() Failed with Error Code %d \n", GetLastError() );

			return 1;
		}

		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	return 0;
}

HWND MakeHiddenWindow( void )
{
	WNDCLASS	wndclass;
	HWND		hWnd = nullptr;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = (WNDPROC)WindowProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = NULL;
	wndclass.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	wndclass.hCursor = LoadCursor( NULL, IDC_ARROW );
	wndclass.hbrBackground = (HBRUSH)GetStockObject( WHITE_BRUSH );
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = (LPCWSTR)WINDOW_NAME;

	if ( 0 == RegisterClass( &wndclass ))
	{
		printf_s( "RegisterClass() Failed with Error Code %d \n", GetLastError() );
		return nullptr;
	}

	if ( nullptr == ( hWnd = CreateWindow( (LPCWSTR)WINDOW_NAME, WINDOW_NAME, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL ) ) )
	{
		printf_s( "CreateWindow() Failed with Error Code %d\n", GetLastError() );
		return nullptr;
	}

	return hWnd;
}

LRESULT CALLBACK WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( uMsg )
	{
		case WM_SOCKET:
		{
			HandleMessage( wParam, lParam );
		}
			return 0;
		
		case WM_DESTROY:
		{
			PostQuitMessage( 0 );
		}
			break;
	}
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

bool InitNetwork()
{
	SOCKADDR_IN addr;
	WSADATA		wsaData;

	if ( WSAStartup( ( 2, 2 ), &wsaData ) != 0 )
	{
		printf_s( "WSAStartup() Failed with Error Code %d \n", WSAGetLastError() );
		return false;
	}

	if ( INVALID_SOCKET == ( g_ListenSocket = socket( PF_INET, SOCK_STREAM, 0 ) ) )
	{
		printf_s( "Socket() Failed with Error Code %d \n", WSAGetLastError() );
		return false;
	}

	int opt = 1;
	setsockopt( g_ListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof( int ) );

	if ( 0 != WSAAsyncSelect( g_ListenSocket, g_hWnd, WM_SOCKET, FD_ACCEPT | FD_CLOSE ) )
	{
		printf_s( "Listen Socket WSAAsyncSelect() Failed with Error Code %d \n", WSAGetLastError() );
		return false;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl( INADDR_ANY );
	addr.sin_port = htons( PORT );

	if ( SOCKET_ERROR == bind( g_ListenSocket, (PSOCKADDR)&addr, sizeof( addr ) ) )
	{
		printf_s( "Bind() Failed with Error Code %d \n", WSAGetLastError() );
		return false;
	}

	if ( listen( g_ListenSocket, SOMAXCONN ) )
	{
		printf_s( "Listen() Failed with Error Code %d\n", WSAGetLastError() );
		return false;
	}

	return true;
}

bool HandleMessage( WPARAM wParam, LPARAM lParam )
{
	if ( WSAGETSELECTERROR( lParam ) )
	{
		SOCKADDR_IN addr;
		int len = sizeof( SOCKADDR_IN );

		if ( -1 == getsockname( wParam, (sockaddr *)&addr, &len ) )
		{
			printf_s( "Get Socket Name Failed with Error Code %d \n", WSAGetLastError() );
		}
		else
		{
			printf_s( "[Debug] Connection Closed IP:[%s] Port:[%d] with Error Code %d \n",
					  inet_ntoa( addr.sin_addr ), ntohs( addr.sin_port ), WSAGETSELECTERROR( lParam ) );
		}

		shutdown( wParam, SD_BOTH );
		closesocket( wParam );

		return false;
	}
	else
	{
		char inBuf[4096] = { 0, };
		int recvLen = 0;

		switch ( WSAGETSELECTEVENT( lParam ) )
		{
			case FD_ACCEPT:
			{
				SOCKET s;
				SOCKADDR_IN addr;
				int addrlen = sizeof( addr );

				if ( INVALID_SOCKET == ( s = accept( wParam, (SOCKADDR*)&addr, &addrlen ) ) )
				{
					printf_s( "Accept() Failed with Error %d\n", WSAGetLastError() );
					break;
				}
				else
				{
					printf_s( "[Debug] Connected IP:[%s] Port:[%d] \n", inet_ntoa( addr.sin_addr ), ntohs(addr.sin_port) );
					
					WSAAsyncSelect( s, g_hWnd, WM_SOCKET, FD_READ | FD_WRITE | FD_CLOSE );
				}
			}
				break;

			case FD_READ:
			{
				ZeroMemory( inBuf, sizeof( inBuf ) );
				recvLen = recv( wParam, inBuf, 4096, 0 );

				if ( SOCKET_ERROR == recvLen )
				{
					break;
				}
			}

			case FD_WRITE:
			{
				int sent = send( wParam, inBuf, recvLen, 0 );
			}
				break;

			case FD_CLOSE:
			{
				SOCKADDR_IN addr;
				int len = sizeof( SOCKADDR_IN );

				if ( -1 == getsockname( wParam, (sockaddr *)&addr, &len ) )
				{
					printf_s( "Get Socket Name Failed with Error Code %d \n", WSAGetLastError() );
				}
				else
				{
					printf_s( "[Debug] Connection Closed IP:[%s] Port:[%d] \n", inet_ntoa( addr.sin_addr ), ntohs( addr.sin_port ) );
				}

				shutdown( wParam, SD_BOTH );
				closesocket( wParam );
			}
				break;
		}
	}

	return true;
}
