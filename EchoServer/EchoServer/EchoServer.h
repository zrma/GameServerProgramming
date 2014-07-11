#pragma once

#define WM_SOCKET	WM_USER + 1
#define PORT		9001

HWND				MakeHiddenWindow( void );
LRESULT CALLBACK	WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

bool				InitNetwork();
bool				HandleMessage( WPARAM wParam, LPARAM lParam );

extern SOCKET		g_ListenSocket;