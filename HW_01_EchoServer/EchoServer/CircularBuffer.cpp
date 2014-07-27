#include "stdafx.h"
#include "CircularBuffer.h"
#include <assert.h>

bool CircularBuffer::Peek( OUT char* destbuf, size_t bytes ) const
{
	assert( m_Buffer != nullptr );

	if ( m_ARegionSize + m_BRegionSize < bytes )
	{
		return false;
	}

	size_t cnt = bytes;
	size_t aRead = 0;

	if ( m_ARegionSize > 0 )
	{
		aRead = ( cnt > m_ARegionSize ) ? m_ARegionSize : cnt;
		memcpy( destbuf, m_ARegionPointer, aRead );
		cnt -= aRead;
	}

	if ( cnt > 0 && m_BRegionSize > 0 )
	{
		assert( cnt <= m_BRegionSize );

		size_t bRead = cnt;

		memcpy( destbuf + aRead, m_BRegionPointer, bRead );
		cnt -= bRead;
	}

	assert( cnt == 0 );

	return true;

}

bool CircularBuffer::Read( OUT char* destbuf, size_t bytes )
{
	assert( m_Buffer != nullptr );

	if ( m_ARegionSize + m_BRegionSize < bytes )
	{
		return false;
	}

	size_t cnt = bytes;
	size_t aRead = 0;

	if ( m_ARegionSize > 0 )
	{
		aRead = ( cnt > m_ARegionSize ) ? m_ARegionSize : cnt;
		memcpy( destbuf, m_ARegionPointer, aRead );

		m_ARegionSize -= aRead;
		m_ARegionPointer += aRead;
		cnt -= aRead;
	}

	if ( cnt > 0 && m_BRegionSize > 0 )
	{
		assert( cnt <= m_BRegionSize );

		size_t bRead = cnt;

		memcpy( destbuf + aRead, m_BRegionPointer, bRead );
		m_BRegionSize -= bRead;
		m_BRegionPointer += bRead;

		cnt -= bRead;
	}

	assert( cnt == 0 );

	if ( m_ARegionSize == 0 )
	{
		if ( m_BRegionSize > 0 )
		{
			if ( m_BRegionPointer != m_Buffer )
			{
				memmove( m_Buffer, m_BRegionPointer, m_BRegionSize );
			}

			m_ARegionPointer = m_Buffer;
			m_ARegionSize = m_BRegionSize;

			m_BRegionPointer = nullptr;
			m_BRegionSize = 0;
		}
		else
		{
			m_BRegionPointer = nullptr;
			m_BRegionSize = 0;

			m_ARegionPointer = m_Buffer;
			m_ARegionSize = 0;
		}
	}

	return true;
}




bool CircularBuffer::Write( const char* data, size_t bytes )
{
	assert( m_Buffer != nullptr );

	if ( m_BRegionPointer != nullptr )
	{
		if ( GetBFreeSpace() < bytes )
		{
			return false;
		}

		memcpy( m_BRegionPointer + m_BRegionSize, data, bytes );
		m_BRegionSize += bytes;

		return true;
	}

	if ( GetAFreeSpace() < GetSpaceBeforeA() )
	{
		AllocateB();

		if ( GetBFreeSpace() < bytes )
		{
			return false;
		}

		memcpy( m_BRegionPointer + m_BRegionSize, data, bytes );
		m_BRegionSize += bytes;

		return true;
	}
	else
	{
		if ( GetAFreeSpace() < bytes )
		{
			return false;
		}

		memcpy( m_ARegionPointer + m_ARegionSize, data, bytes );
		m_ARegionSize += bytes;

		return true;
	}
}

void CircularBuffer::Remove( size_t len )
{
	size_t cnt = len;

	if ( m_ARegionSize > 0 )
	{
		size_t aRemove = ( cnt > m_ARegionSize ) ? m_ARegionSize : cnt;
		m_ARegionSize -= aRemove;
		m_ARegionPointer += aRemove;
		cnt -= aRemove;
	}

	if ( cnt > 0 && m_BRegionSize > 0 )
	{
		size_t bRemove = ( cnt > m_BRegionSize ) ? m_BRegionSize : cnt;
		m_BRegionSize -= bRemove;
		m_BRegionPointer += bRemove;
		cnt -= bRemove;
	}

	if ( m_ARegionSize == 0 )
	{
		if ( m_BRegionSize > 0 )
		{
			if ( m_BRegionPointer != m_Buffer )
			{
				memmove( m_Buffer, m_BRegionPointer, m_BRegionSize );
			}

			m_ARegionPointer = m_Buffer;
			m_ARegionSize = m_BRegionSize;
			m_BRegionPointer = nullptr;
			m_BRegionSize = 0;
		}
		else
		{
			m_BRegionPointer = nullptr;
			m_BRegionSize = 0;
			m_ARegionPointer = m_Buffer;
			m_ARegionSize = 0;
		}
	}
}