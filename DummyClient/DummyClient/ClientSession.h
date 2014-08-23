#pragma once
#include "ObjectPool.h"
#include "MemoryPool.h"
#include "CircularBuffer.h"

#define BUFSIZE	65536

class ClientSession ;
class SessionManager;

enum IOType
{
	IO_NONE,
	IO_SEND,
	IO_RECV,
	IO_RECV_ZERO,
	IO_CONNECT,
	IO_DISCONNECT
} ;

enum DisconnectReason
{
	DR_NONE,
	DR_ACTIVE,
	DR_ONCONNECT_ERROR,
	DR_IO_REQUEST_ERROR,
	DR_COMPLETION_ERROR,
};

struct OverlappedIOContext
{
	OverlappedIOContext(ClientSession* owner, IOType ioType);

	OVERLAPPED		mOverlapped ;
	ClientSession*	mSessionObject ;
	IOType			mIoType ;
	WSABUF			mWsaBuf;
	
} ;

struct OverlappedSendContext : public OverlappedIOContext, public ObjectPool<OverlappedSendContext>
{
	OverlappedSendContext(ClientSession* owner) : OverlappedIOContext(owner, IO_SEND)
	{
	}
};

struct OverlappedRecvContext : public OverlappedIOContext, public ObjectPool<OverlappedRecvContext>
{
	OverlappedRecvContext(ClientSession* owner) : OverlappedIOContext(owner, IO_RECV)
	{
	}
};

struct OverlappedPreRecvContext : public OverlappedIOContext, public ObjectPool<OverlappedPreRecvContext>
{
	OverlappedPreRecvContext(ClientSession* owner) : OverlappedIOContext(owner, IO_RECV_ZERO)
	{
	}
};

struct OverlappedConnectContext: public OverlappedIOContext, public ObjectPool < OverlappedConnectContext >
{
	OverlappedConnectContext( ClientSession* owner ): OverlappedIOContext( owner, IO_CONNECT )
	{
	}
};

struct OverlappedDisconnectContext : public OverlappedIOContext, public ObjectPool<OverlappedDisconnectContext>
{
	OverlappedDisconnectContext(ClientSession* owner, DisconnectReason dr) 
	: OverlappedIOContext(owner, IO_DISCONNECT), mDisconnectReason(dr)
	{}

	DisconnectReason mDisconnectReason;
};

void DeleteIoContext(OverlappedIOContext* context) ;


class ClientSession: public PooledAllocatable
{
public:
	ClientSession();
	~ClientSession() {}

	void	SessionReset();

	bool	IsConnected() const { return !!mConnected; }

	bool	PostConnect();

	void	ConnectCompletion();

	bool	PreRecv(); ///< zero byte recv

	bool	PostRecv();
	void	RecvCompletion( DWORD transferred );

	bool	PostSend();
	void	SendCompletion( DWORD transferred );

	void	DisconnectRequest( DisconnectReason dr );
	void	DisconnectCompletion( DisconnectReason dr );

	void	AddRef();
	void	ReleaseRef();

	void	SetSocket( SOCKET sock ) { mSocket = sock; }
	SOCKET	GetSocket() const { return mSocket; }

	long long	GetSendBytes() { return mSendBytes; }
	long long	GetRecyBytes() { return mRecvBytes; }
	long		GetConnectCount() { return mUseCount; }

private:
	SOCKET			mSocket;
	SOCKADDR_IN		mClientAddr;

	FastSpinlock	mBufferLock;

	CircularBuffer	mBuffer;

	volatile long	mRefCount;
	volatile long	mConnected;

	long long		mSendBytes = 0;
	long long		mRecvBytes = 0;
	long			mUseCount = 0;

	friend class SessionManager;
};