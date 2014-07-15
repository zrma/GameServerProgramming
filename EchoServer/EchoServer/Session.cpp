#include "stdafx.h"
#include "Session.h"


bool Session::Recv()
{	
	DWORD	recvBytes = 0;
	DWORD	flags = 0;

	WSABUF	buf;
	buf.len = (ULONG)m_RecvBuffer.GetFreeSpaceSize();
	buf.buf = (char*)m_RecvBuffer.GetBuffer();

	if ( SOCKET_ERROR == WSARecv( m_Socket, &buf, 1, &recvBytes, &flags, NULL, NULL ) )
	{
		if ( WSAEWOULDBLOCK != ( m_ErrorCode = WSAGetLastError() ) )
		{
			m_IsConnected = false;
			return false;
		}
	}

	if ( recvBytes > 0 )
	{
		m_RecvBuffer.Commit( recvBytes );
	}

	return true;
}

bool Session::Send()
{
	size_t recvBytes = m_RecvBuffer.GetStoredSize();
	size_t sendSpaces = m_SendBuffer.GetFreeSpaceSize();

	if ( recvBytes > 0 )
	{
		if ( recvBytes < sendSpaces )
		{
			char* buf = new char[recvBytes];

			m_RecvBuffer.Read( buf, recvBytes );
			m_SendBuffer.Write( buf, recvBytes );

			delete[] buf;
		}
		else if ( sendSpaces > 0 )
		{
			char* buf = new char[sendSpaces];

			m_RecvBuffer.Read( buf, sendSpaces );
			m_SendBuffer.Write( buf, sendSpaces );

			delete[] buf;
		}
		else
		{
			printf_s( "Buffer is full!! \n" );
		}
	}

	DWORD	sendBytes = 0;
	DWORD	flags = 0;

	WSABUF	buf;
	buf.len = (ULONG)m_SendBuffer.GetContiguiousBytes();
	buf.buf = (char*)m_SendBuffer.GetBufferStart();

	if ( SOCKET_ERROR == WSASend( m_Socket, &buf, 1, &sendBytes, flags, NULL, NULL ) )
	{
		if ( WSAEWOULDBLOCK != ( m_ErrorCode = WSAGetLastError() ) )
		{
			m_IsConnected = false;
			return false;
		}
	}

	m_SendBuffer.Remove( sendBytes );
	
	return true;
}
