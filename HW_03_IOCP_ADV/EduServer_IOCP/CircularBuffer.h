#pragma once


class CircularBuffer
{
public:

	CircularBuffer( size_t capacity ): m_BRegionPointer( nullptr ), m_ARegionSize( 0 ), m_BRegionSize( 0 ), m_Capacity( capacity )
	{
		m_Buffer = new char[m_Capacity];
		m_BufferEnd = m_Buffer + m_Capacity;
		m_ARegionPointer = m_Buffer;
	}

	~CircularBuffer()
	{
		delete[] m_Buffer;
	}

	void BufferReset()
	{
		m_BRegionPointer = nullptr;
		m_ARegionSize = 0;
		m_BRegionSize = 0;

		memset( m_Buffer, 0, m_Capacity );

		m_BufferEnd = m_Buffer + m_Capacity;
		m_ARegionPointer = m_Buffer;
	}

	/// 버퍼의 첫부분 len만큼 날리기
	void Remove( size_t len );

	size_t GetFreeSpaceSize()
	{
		if ( m_BRegionPointer != nullptr )
			return GetBFreeSpace();
		else
		{
			/// A 버퍼보다 더 많이 존재하면, B 버퍼로 스위치
			if ( GetAFreeSpace() < GetSpaceBeforeA() )
			{
				AllocateB();
				return GetBFreeSpace();
			}
			else
				return GetAFreeSpace();
		}
	}

	size_t GetStoredSize() const
	{
		return m_ARegionSize + m_BRegionSize;
	}

	size_t GetContiguiousBytes() const
	{
		if ( m_ARegionSize > 0 )
			return m_ARegionSize;
		else
			return m_BRegionSize;
	}

	/// 쓰기가 가능한 위치 (버퍼의 끝부분) 반환
	char* GetBuffer() const
	{
		if ( m_BRegionPointer != nullptr )
			return m_BRegionPointer + m_BRegionSize;
		else
			return m_ARegionPointer + m_ARegionSize;
	}

	/// 커밋(aka. IncrementWritten)
	void Commit( size_t len )
	{
		if ( m_BRegionPointer != nullptr )
			m_BRegionSize += len;
		else
			m_ARegionSize += len;
	}

	/// 버퍼의 첫부분 리턴
	char* GetBufferStart() const
	{
		if ( m_ARegionSize > 0 )
			return m_ARegionPointer;
		else
			return m_BRegionPointer;
	}

private:
	void AllocateB()
	{
		m_BRegionPointer = m_Buffer;
	}

	size_t GetAFreeSpace() const
	{
		return ( m_BufferEnd - m_ARegionPointer - m_ARegionSize );
	}

	size_t GetSpaceBeforeA() const
	{
		return ( m_ARegionPointer - m_Buffer );
	}


	size_t GetBFreeSpace() const
	{
		if ( m_BRegionPointer == nullptr )
			return 0;

		return ( m_ARegionPointer - m_BRegionPointer - m_BRegionSize );
	}

private:

	char*	m_Buffer;
	char*	m_BufferEnd;

	char*	m_ARegionPointer;
	size_t	m_ARegionSize;

	char*	m_BRegionPointer;
	size_t	m_BRegionSize;

	size_t	m_Capacity;
};
