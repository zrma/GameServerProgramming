#pragma once

class FastSpinlock
{
public:
	FastSpinlock();
	~FastSpinlock();

	void EnterLock();
	void LeaveLock();
	
private:
	FastSpinlock(const FastSpinlock& rhs);
	FastSpinlock& operator=(const FastSpinlock& rhs);

	volatile long mLockFlag;
};

// Dummy Lock Class
//
// 스택을 사용해서 생성자-소멸자 에 들어있는 EnterLock-LeaveLock 를 호출
// 일종의 더미 렌더러 같은 역할
class FastSpinlockGuard
{
public:
	FastSpinlockGuard(FastSpinlock& lock) : m_Lock(lock)
	{
		m_Lock.EnterLock();
	}

	~FastSpinlockGuard()
	{
		m_Lock.LeaveLock();
	}

private:
	FastSpinlock& m_Lock;
};

template <class TargetClass>
class ClassTypeLock
{
public:
	struct LockGuard
	{
		LockGuard()
		{
			TargetClass::m_Lock.EnterLock();
		}

		~LockGuard()
		{
			TargetClass::m_Lock.LeaveLock();
		}

	};

private:
	static FastSpinlock m_Lock;
	
	//friend struct LockGuard;
};

template <class TargetClass>
FastSpinlock ClassTypeLock<TargetClass>::m_Lock;