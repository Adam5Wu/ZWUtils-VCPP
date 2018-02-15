/*
Copyright (c) 2005 - 2016, Zhenyu Wu; 2012 - 2016, NEC Labs America Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of ZWUtils-VCPP nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * @addtogroup Threading Threading Support Utilities
 * @file
 * @brief Synchronized Objects Template
 * @author Zhenyu Wu
 * @date Feb 28, 2005: Initial implementation
 * @date Oct 20, 2005: Minor functionality improvement
 * @date Jul 29, 2013: Port to Visual C++ 2012
 * @date Jan 29, 2014: Make the code C++ style (avoid performance hit of __try __finally)
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef SyncObjs_H
#define SyncObjs_H

#include "BaseLib/Exception.h"
#include "BaseLib/WinError.h"
#include "BaseLib/Misc.h"
#include "BaseLib/Allocator.h"
#include "BaseLib/ManagedRef.h"

/**
 * @ingroup Threading
 * @brief Lockable class
 *
 * Abstract class for suppoting lock operations
 **/
class TLockable {
public:
	virtual ~TLockable(void) {}

	class TLock {
		friend class TLockable;
	private:
		TLockable &Instance;
	protected:
		TLock(TLockable &xInstance, bool xLocked) :
			Instance(xInstance), isLocked(xLocked) {}
	public:
		bool const isLocked;

		TLock(TLock const&) = delete;
		TLock(TLock &&xLock) : Instance(xLock.Instance), isLocked(xLock.isLocked)
		{ *const_cast<bool*>(&xLock.isLocked) = false; }

		~TLock(void)
		{ if (isLocked) Instance.__SyncUnlock(); }
	};

	/**
	 * Acquire the lock, wait forever
	 **/
	virtual void __SyncLock(void)
	{ FAIL(_T("Abstract function")); }
	/**
	 * Try to acquire the lock in this instant, return false if failed
	 **/
	virtual bool __SyncTryLock(void)
	{ FAIL(_T("Abstract function")); }
	/**
	 * Release acquired lock
	 **/
	virtual void __SyncUnlock(void)
	{ FAIL(_T("Abstract function")); }

	inline TLock SyncLock(void) {
		__SyncLock();
		return TLock(*this, true);
	}

	inline TLock SyncTryLock(void) {
		return TLock(*this, __SyncTryLock());
	}
};
typedef TLockable* PLockable;

/**
 * @ingroup Threading
 * @brief Lockable synchronization premitive class
 *
 * Adapter class to convert synchronization premitives to lockables
 **/
template<class TSyncPerms>
class TLockableSyncPerms : public TLockable {
protected:
	//! We will be using this premitive to implement required operations
	TSyncPerms rSyncPerms;
};

#include "SyncPrems.h"

// !Lockable using semaphore
class TLockableSemaphore : public TLockableSyncPerms < TSemaphore > {
public:
	void __SyncLock(void) override {
		WaitResult Ret = rSyncPerms.WaitFor();
		if (Ret == WaitResult::Error)
			SYSFAIL(_T("Failed to lock semaphore"))
		else {
			if (Ret != WaitResult::Signaled)
				LOGSYSERR(_T("WARNING: Unable to lock semaphore"));
		}
	}
	bool __SyncTryLock(void) override {
		WaitResult Ret = rSyncPerms.WaitFor(0);
		if (Ret == WaitResult::Error)
			SYSFAIL(_T("Failed to lock semaphore"))
		else {
			if (Ret != WaitResult::Signaled)
				LOGSYSERR(_T("WARNING: Unable to lock semaphore"));
		}
		return Ret == WaitResult::Signaled;
	}
	inline void __SyncUnlock(void) override
	{ rSyncPerms.Signal(); }
};

// !Lockable using mutex
class TLockableMutex : public TLockableSyncPerms < TMutex > {
public:
	void __SyncLock(void) override {
		WaitResult Ret = rSyncPerms.Acquire();
		if (Ret == WaitResult::Error)
			SYSFAIL(_T("Failed to lock mutex"))
		else {
			if (Ret == WaitResult::Abandoned) {
				LOGV(_T("WARNING: Locked abandoned mutex"))
			} else if (Ret != WaitResult::Signaled)
			SYSFAIL(_T("WARNING: Unable to lock mutex"));
		}
	}
	bool __SyncTryLock(void) override {
		WaitResult Ret = rSyncPerms.TryAcquire();
		if (Ret == WaitResult::Error)
			SYSFAIL(_T("Failed to lock mutex"))
		else {
			if (Ret == WaitResult::Abandoned) {
				LOGV(_T("WARNING: Locked abandoned mutex"));
				return true;
			}
			return Ret == WaitResult::Signaled;
		}
	}
	inline void __SyncUnlock(void) override
	{ rSyncPerms.Release(); }
};

// !Lockable using critical section
class TLockableCS : public TLockableSyncPerms < TCriticalSection > {
public:
	inline void __SyncLock(void) override
	{ rSyncPerms.Enter(); }
	inline bool __SyncTryLock(void) override
	{ return rSyncPerms.TryEnter(); }
	inline void __SyncUnlock(void) override
	{ rSyncPerms.Leave(); }
};

/**
 * @ingroup Threading
 * @brief GenericWaitResult::apper to synchronize any object
 *
 * Wraps around an object with a lockable class to protect asynchronous accesses
 **/
template<class T, class TAllocator = SimpleAllocator<T>, class CLockable = TLockableCS>
class TSyncObj : private ManagedRef<T, TAllocator>, public TLockable {
protected:
	CLockable Lock;
public:
	class TDeSyncObj final {
		friend TSyncObj;
	protected:
		TSyncObj &Container;
		T* const Obj;
		TDeSyncObj(TSyncObj *xContainer) : Container(*xContainer), Obj(&xContainer->__Pickup()) {}
		TDeSyncObj(TSyncObj &xContainer) : Container(xContainer), Obj(xContainer.__TryPickup()) {}
	public:
		TDeSyncObj(TDeSyncObj const &) = delete;
		TDeSyncObj(TDeSyncObj &&xDeSyncObj) :
			Container(xDeSyncObj.Container), Obj(xDeSyncObj.Obj)
		{
			*const_cast<T**>(&xDeSyncObj.Obj) = nullptr;
		}

		~TDeSyncObj(void) { if (Obj) Container.__Drop(); }

		TDeSyncObj& operator=(TDeSyncObj const &) = delete;
		TDeSyncObj& operator=(TDeSyncObj &&xDeSyncObj) = delete;

		inline T* operator&(void) const
		{ return Obj; }
		inline T& operator*(void) const
		{ return *&(*this); }
		inline T* operator->(void) const
		{ return &(*this); }
		inline operator T&() const
		{ return *(*this); }
	};

public:
	/**
	 * Create a new instance of T and managed object with default parameter
	 **/
	TSyncObj(void) : ManagedRef(EMPLACE_CONSTRUCT) {}
	/**
	 * Create a new instance of T and managed object with given parameter
	 **/
	template<typename A, typename... Params,
		typename = std::enable_if<!std::is_same<HANDOFF_CONSTRUCT_T const&, A>::value>::type,
		typename = std::enable_if<!std::is_same<TSyncObj const&, A>::value>::type,
		typename = std::enable_if<!std::is_same<TSyncObj&&, A>::value>::type>
		TSyncObj(A &&xA, Params&&... xParams) : ManagedRef(EMPLACE_CONSTRUCT, xA, xParams...) {}
	/**
	 * Create a new instance of T and take onwership of the object
	 **/
	TSyncObj(HANDOFF_CONSTRUCT_T const&, T *Obj) : ManagedRef(Obj, HANDOFF_CONSTRUCT) {}
	/**
	 * Copy construction
	 **/
	TSyncObj(TSyncObj const &Src);
	/**
	 * Move construction
	 **/
	TSyncObj(TSyncObj &&Src);
	/**
	 * Use discard managed instance
	 **/
	virtual ~TSyncObj(void);
	/**
	 * Copy assignment
	 **/
	TSyncObj& operator=(TSyncObj const &Src);
	/**
	 * Move assignment
	 **/
	TSyncObj& operator=(TSyncObj &&Src);

	TSyncObj* operator&(void)
	{ return this; }

	/**
	 * Lock theWaitResult::apper, and return an reference of managed T instance
	 **/
	inline T& __Pickup(void);
	/**
	 * Try to lock theWaitResult::apper in the instant, and return a pointer to managed T instance, NULL if could not obtain lock
	 **/
	inline T* __TryPickup(void);
	/**
	 * Unlock theWaitResult::apper, render all references and pointers of the managed T invalid
	 **/
	inline void __Drop(void);

	/**
	 * Lock theWaitResult::apper, and return an reference of managed T instance
	 **/
	TDeSyncObj Pickup(void);
	/**
	 * Try to lock theWaitResult::apper in the instant, and return a pointer to managed T instance, NULL if could not obtain lock
	 **/
	TDeSyncObj TryPickup(void);

	/**
	 * Lock theWaitResult::apper, reassign the managed T instance as RefObj, and unlock theWaitResult::apper
	 * @note T must support assignment operator
	 **/
	T Assign(T const &RefObj);

	/**
	 * Lock theWaitResult::apper, if managed T instance equals CmpObj, swap the instance content of managed T and RefObj, and unlock theWaitResult::apper
	 * @return Whether T instance content swap took place
	 * @note If swap did not took place, RefObj will be reassigned as the current T instance
	 * @note T must support equality operator, copy consutrction, and assignment operator
	 **/
	bool CompareAndSwap(T const &CmpObj, T &RefObj);

	/**
	 * Lock theWaitResult::apper, assign a copy of the managed T instance to DstObj, and unlock theWaitResult::apper
	 * @note T must support assignment operator
	 **/
	void Snapshot(T &DstObj) const;

	inline void __SyncLock(void) override
	{ Lock.__SyncLock(); }
	inline bool __SyncTryLock(void) override
	{ return Lock.__SyncTryLock(); }
	inline void __SyncUnlock(void) override
	{ Lock.__SyncUnlock(); }

};

#include "ThreadLib/Threading.h"

template<class T, class TAllocator, class CLockable>
TSyncObj<T, TAllocator, CLockable>::TSyncObj(TSyncObj const &Src) {
	Synchronized_TC(Src, {
		ManagedRef::Assign(&Src, false);
	});
}

template<class T, class TAllocator, class CLockable>
TSyncObj<T, TAllocator, CLockable>::TSyncObj(TSyncObj &&Src) {
	Synchronized_TC(Src, {
		ManagedRef::operator=(std::move(Src));
	});
}

template<class T, class TAllocator, class CLockable>
TSyncObj<T, TAllocator, CLockable>::~TSyncObj(void) {
	Lock.__SyncLock();
}

template<class T, class TAllocator, class CLockable>
TSyncObj<T, TAllocator, CLockable>& TSyncObj<T, TAllocator, CLockable>::operator= (TSyncObj const &Src) {
	Synchronized(Src, {
		Synchronized(Lock, {
			ManagedRef::Assign(&Src, false);
		});
	});
	return *this;
}

template<class T, class TAllocator, class CLockable>
TSyncObj<T, TAllocator, CLockable>& TSyncObj<T, TAllocator, CLockable>::operator= (TSyncObj &&Src) {
	Synchronized(Src, {
		Synchronized(Lock, {
			ManagedRef::operator=(std::move(Src));
		});
	});
	return *this;
}

template<class T, class TAllocator, class CLockable>
T& TSyncObj<T, TAllocator, CLockable>::__Pickup(void) {
	Lock.__SyncLock();
	__try {
		return *this;
	} __except ([&] {
		Lock.__SyncUnlock();
		return EXCEPTION_CONTINUE_SEARCH;
		}()) {
		// Should not reach!
	}
}

template<class T, class TAllocator, class CLockable>
T* TSyncObj<T, TAllocator, CLockable>::__TryPickup(void) {
	if (Lock.__SyncTryLock()) {
		__try {
			return &((T&)*this);
		} __except ([&] {
			Lock.__SyncUnlock();
			return EXCEPTION_CONTINUE_SEARCH;
			}()) {
			// Should not reach!
		}
	}
	return nullptr;
}

template<class T, class TAllocator, class CLockable>
void TSyncObj<T, TAllocator, CLockable>::__Drop(void) {
	Lock.__SyncUnlock();
}

template<class T, class TAllocator, class CLockable>
typename TSyncObj<T, TAllocator, CLockable>::TDeSyncObj TSyncObj<T, TAllocator, CLockable>::Pickup(void) {
	return TDeSyncObj{this};
}

template<class T, class TAllocator, class CLockable>
typename TSyncObj<T, TAllocator, CLockable>::TDeSyncObj TSyncObj<T, TAllocator, CLockable>::TryPickup(void) {
	return TDeSyncObj{*this};
}

template<class T, class TAllocator, class CLockable>
T TSyncObj<T, TAllocator, CLockable>::Assign(const T &RefObj) {
	Synchronized(Lock, {
		return **this = RefObj;
	});
}

template<class T, class TAllocator, class CLockable>
bool TSyncObj<T, TAllocator, CLockable>::CompareAndSwap(const T& CmpObj, T& RefObj) {
	Synchronized(Lock, {
		if (**this == CmpObj) {
			T TmpObj(**this);
			**this = RefObj;
			RefObj = std::move(TmpObj);
			return true;
		} else {
			RefObj = **this;
			return false;
		}
	});
}

template<class T, class TAllocator, class CLockable>
void TSyncObj<T, TAllocator, CLockable>::Snapshot(T& DstObj) const {
	Synchronized((*const_cast<CLockable*>(&Lock)), {
		DstObj = **this;
	});
}

#endif SyncObjs_H
