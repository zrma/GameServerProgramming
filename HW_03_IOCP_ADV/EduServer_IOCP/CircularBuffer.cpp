#include "stdafx.h"
#include "CircularBuffer.h"
#include <assert.h>


void CircularBuffer::Remove( size_t len )
{
	size_t cnt = len;

	/// Read와 마찬가지로 A가 있다면 A영역에서 먼저 삭제
	if ( m_ARegionSize > 0 )
	{
		size_t aRemove = ( cnt > m_ARegionSize ) ? m_ARegionSize : cnt;
		m_ARegionSize -= aRemove;
		m_ARegionPointer += aRemove;
		cnt -= aRemove;
	}

	// 제거할 용량이 더 남은경우 B에서 제거 
	if ( cnt > 0 && m_BRegionSize > 0 )
	{
		size_t bRemove = ( cnt > m_BRegionSize ) ? m_BRegionSize : cnt;
		m_BRegionSize -= bRemove;
		m_BRegionPointer += bRemove;
		cnt -= bRemove;
	}

	/// A영역이 비워지면 B를 A로 스위치 
	if ( m_ARegionSize == 0 )
	{
		if ( m_BRegionSize > 0 )
		{
			/// 앞으로 당겨 붙이기
			if ( m_BRegionPointer != m_Buffer )
				memmove( m_Buffer, m_BRegionPointer, m_BRegionSize );

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