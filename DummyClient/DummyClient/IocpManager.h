#pragma once
#include "FastSpinlock.h"

class ClientSession;

struct OverlappedSendContext;
struct OverlappedPreRecvContext;
struct OverlappedRecvContext;
struct OverlappedDisconnectContext;
struct OverlappedAcceptContext;

class IocpManager
{
public:
	IocpManager();
	~IocpManager();

	bool Initialize();
	void Finalize();

	bool StartIoThreads();
	void StartAccept();
	void StartConnect();

	HANDLE GetComletionPort()	{ return mCompletionPort; }
	
	static char mConnectBuf[64];
	static LPFN_CONNECTEX mFnConnectEx;
	static LPFN_DISCONNECTEX mFnDisconnectEx;
	
	static BOOL ConnectEx( SOCKET hSocket, const struct sockaddr *name, int nameLen,
						   PVOID lpSendBuffer, DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped );
	static BOOL DisconnectEx( SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved );
private:

	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam);

	static bool PreReceiveCompletion(ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred);
	static bool ReceiveCompletion(ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred);
	static bool SendCompletion(ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred);

private:

	HANDLE	mCompletionPort;
	volatile long	mIoThreadCount;
		
	SOCKET	mListenSocket;

	HANDLE*	mThreadHandle = nullptr;

	FastSpinlock	mLock;
};

extern IocpManager* GIocpManager;