#pragma once

class FastSpinlock
{
public:
	FastSpinlock();
	~FastSpinlock();

	void EnterLock();
	void LeaveLock();

private:
	FastSpinlock( const FastSpinlock& rhs );
	FastSpinlock& operator=( const FastSpinlock& rhs );

	volatile long m_LockFlag;
};

class FastSpinlockGuard
{
public:
	FastSpinlockGuard( FastSpinlock& lock ): m_Lock( lock )
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