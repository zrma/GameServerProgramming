#pragma once

class ClientSession;
struct OverlappedIOContext;

class IocpManager
{
public:
	IocpManager();
	~IocpManager();

	bool Initialize();
	void Finalize();

	bool StartIoThreads();
	bool StartAcceptLoop();

	HANDLE GetComletionPort() { return m_CompletionPort; }
	int	GetIoThreadCount() { return m_IoThreadCount; }

private:
	static unsigned int WINAPI IoWorkerThread( LPVOID lpParam );

	static bool ReceiveCompletion( const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred );
	static bool SendCompletion( const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred );

private:
	HANDLE	m_CompletionPort;
	int		m_IoThreadCount;

	SOCKET	m_ListenSocket;
};

extern __declspec(thread) int l_IoThreadId;
extern IocpManager* g_IocpManager;