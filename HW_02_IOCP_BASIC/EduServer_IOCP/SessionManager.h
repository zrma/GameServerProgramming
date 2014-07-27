#pragma once
#include <map>
#include <WinSock2.h>
#include "FastSpinlock.h"

class ClientSession;

class SessionManager
{
public:
	SessionManager() : m_CurrentConnectionCount(0)	{}

	ClientSession* CreateClientSession(SOCKET sock);

	void DeleteClientSession(ClientSession* client);

	// 원자적으로 1 증가
	int IncreaseConnectionCount() { return InterlockedIncrement(&m_CurrentConnectionCount); }

	// 원자적으로 1 감소
	int DecreaseConnectionCount() { return InterlockedDecrement(&m_CurrentConnectionCount); }
	
private:
	typedef std::map<SOCKET, ClientSession*> ClientList;
	ClientList	m_ClientList;

	//TODO: mLock; 선언 -> 구현
	FastSpinlock	m_Lock;

	// 2013년 OS 윈도우 실습 때 공부했던 volatile 다시 한 번 되새김질! ///< 그냥 interlocked.... 쓰려면 대상이 volatbile이어야 한다는것만 기억해도 됨. -> 확인
	// 참고 : http://zapiro.tistory.com/25
	// 참고 : http://en.wikipedia.org/wiki/Volatile_variable
	volatile long	m_CurrentConnectionCount;
};

extern SessionManager* g_SessionManager;
