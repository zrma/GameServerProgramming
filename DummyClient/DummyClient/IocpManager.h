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

	long long GetSendCount() { return mSendCount; }
	long long GetRecvCount() { return mRecvCount; }
	void IncreaseSendCount( long count );
	void IncreaseRecvCount( long count );

	long GetConnectCount() { return mConnectCount; }
	void IncreaseConnectCount()
	{
		InterlockedIncrement( &mConnectCount );
	}
	
	HANDLE GetComletionPort()	{ return mCompletionPort; }
	int	GetIoThreadCount()		{ return mIoThreadCount;  }
	void DecreaseThreadCount();
	
	static char mAcceptBuf[64];
	static LPFN_CONNECTEX mFnConnectEx;
	static LPFN_DISCONNECTEX mFnDisconnectEx;
	static LPFN_ACCEPTEX mFnAcceptEx;
	
	static BOOL ConnectEx( SOCKET hSocket, const struct sockaddr *name, int nameLen,
						   PVOID lpSendBuffer, DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped );
	static BOOL DisconnectEx( SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved );
	static BOOL AcceptEx( SOCKET sListenSocket, SOCKET sAcceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength,
				   DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped );
private:

	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam);

	static bool PreReceiveCompletion(ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred);
	static bool ReceiveCompletion(ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred);
	static bool SendCompletion(ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred);

private:

	HANDLE	mCompletionPort;
	volatile long	mIoThreadCount;

	volatile long long	mSendCount;
	volatile long long	mRecvCount;

	volatile long	mConnectCount;

	SOCKET	mListenSocket;

	FastSpinlock	mLock;
};

extern IocpManager* GIocpManager;