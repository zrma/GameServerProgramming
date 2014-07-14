#pragma once
#include "CircularBuffer.h"

#define BUFFER_SIZE	(1024 * 10)

class Session
{
public:
	Session( SOCKET socket )
		: m_Socket( socket ), m_SendBuffer( BUFFER_SIZE ), m_RecvBuffer( BUFFER_SIZE ) {}
	~Session() {}

	bool			Recv();
	bool			Send();

	bool			HasDregs() const { return m_SendBuffer.GetStoredSize() > 0; }

private:
	SOCKET			m_Socket = NULL;
	
	CircularBuffer	m_SendBuffer;
	CircularBuffer	m_RecvBuffer;
};