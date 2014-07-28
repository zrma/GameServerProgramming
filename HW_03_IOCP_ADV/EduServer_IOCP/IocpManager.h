#pragma once

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


	HANDLE GetComletionPort() { return m_CompletionPort; }
	int	GetIoThreadCount() { return m_IoThreadCount; }

	SOCKET* GetListenSocket() { return &m_ListenSocket; }

	static BOOL DisconnectEx( SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD dwReserved );
	static BOOL AcceptEx( SOCKET sListenSocket, SOCKET sAcceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength,
				   DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped );

	// http://pacs.tistory.com/entry/C-%ED%81%B4%EB%9E%98%EC%8A%A4%EC%99%80-static
	static LPFN_ACCEPTEX		m_FnAcceptEx;
	static LPFN_DISCONNECTEX	m_FnDisconnectEx;

	char m_AcceptBuffer[1024];

private:
	static unsigned int WINAPI IoWorkerThread( LPVOID lpParam );

	static bool PreReceiveCompletion( ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred );
	static bool ReceiveCompletion( ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred );
	static bool SendCompletion( ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred );

	HANDLE	m_CompletionPort;
	int		m_IoThreadCount;

	SOCKET	m_ListenSocket;
};

extern __declspec(thread) int l_IoThreadId;
extern IocpManager* g_IocpManager;