#pragma once
#include "CircularBuffer.h"

#define BUFFER_SIZE	(1024 * 10)

class Session
{
public:
	Session( SOCKET socket )
		: m_Socket( socket ), m_SendBuffer( BUFFER_SIZE ), m_RecvBuffer( BUFFER_SIZE )
	{	m_IsConnected = true; }
	~Session() {}

	bool			Recv();
	bool			Send();

	bool			IsConnected() const { return m_IsConnected; }
	int				GerErrorCode() const { return m_ErrorCode; }

	bool			HasDregs() const { return m_SendBuffer.GetStoredSize() > 0; }

private:
	bool			m_IsConnected = false;
	int				m_ErrorCode = 0;

	SOCKET			m_Socket = NULL;
	
	CircularBuffer	m_SendBuffer;
	CircularBuffer	m_RecvBuffer;
};