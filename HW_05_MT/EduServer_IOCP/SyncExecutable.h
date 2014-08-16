#pragma once

#include "TypeTraits.h"
#include "FastSpinlock.h"
#include "Timer.h"


class SyncExecutable : public std::enable_shared_from_this<SyncExecutable>
{
public:
	SyncExecutable() : mLock(LO_BUSINESS_CLASS)
	{}
	virtual ~SyncExecutable() {}

	template <class R, class T, class... Args>
	R DoSync(R (T::*memfunc)(Args...), Args... args)
	{
		static_assert(true == std::is_convertible<T, SyncExecutable>::value, "T should be derived from SyncExecutable");

		//TODO: mLock으로 보호한 상태에서, memfunc를 실행하고 결과값 R을 리턴  -> 구현
		FastSpinlockGuard guard( mLock );

		// http://stackoverflow.com/questions/17131768/how-to-directly-bind-member-function-to-stdfunction-in-visual-studio-11
		// http://en.cppreference.com/w/cpp/utility/functional/bind
		// bind to a member function 참조!
		return std::bind( memfunc, static_cast<T*>(this), std::forward<Args>( args )... )();
	}
	
	void EnterLock() { mLock.EnterWriteLock(); }
	void LeaveLock() { mLock.LeaveWriteLock(); }
	
 	template <class T>
	std::shared_ptr<T> GetSharedFromThis()
 	{
		static_assert(true == std::is_convertible<T, SyncExecutable>::value, "T should be derived from SyncExecutable");
 		
		//TODO: this 포인터를 std::shared_ptr<T>형태로 반환.  -> 구현
		//(HINT: 이 클래스는 std::enable_shared_from_this에서 상속받았다. 그리고 static_pointer_cast 사용)

		// http://ertai119.tistory.com/9  <-  참고 // 패 to the 망!
		// http://yielding.egloos.com/viewer/3866145  <-  참고
		// return std::shared_ptr<T>((Player*)this); ///< 이렇게 하면 안될걸???

		return std::static_pointer_cast<T>( shared_from_this() );
 	}

private:

	FastSpinlock mLock;
};


template <class T, class F, class... Args>
void DoSyncAfter(uint32_t after, T instance, F memfunc, Args&&... args)
{
	static_assert(true == is_shared_ptr<T>::value, "T should be shared_ptr");
	static_assert(true == std::is_convertible<T, std::shared_ptr<SyncExecutable>>::value, "T should be shared_ptr SyncExecutable");

	//TODO: instance의 memfunc를 bind로 묶어서 LTimer->PushTimerJob() 수행
	
	LTimer->PushTimerJob( instance, std::bind( memfunc, instance, std::forward<Args>( args )... ), after );
}