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
 * @addtogroup Threading Threading Supporting Utilities
 * @file
 * @brief Synchronization Premitive Classes
 * @author Zhenyu Wu
 * @date Oct 10, 2006: Initial implementation
 * @date Jul 26, 2013: Porting to Visual C++ 2012
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef SyncPrems_H
#define SyncPrems_H

#include <Windows.h>
#include <vector>

#include "BaseLib/Exception.h"

class TWaitable;
enum WaitResult {
	Signaled,
	TimedOut,
	Error,
	Abandoned,
	APC,
	Message,
	Signaled_0 = 100,
	Abandoned_0 = 100 + MAXIMUM_WAIT_OBJECTS,
};

WaitResult WaitSingle(const TWaitable &Waitable, DWORD Timeout = INFINITE, bool WaitAPC = false, bool WaitMsg = false);
WaitResult WaitMultiple(std::vector<std::reference_wrapper<TWaitable const>> const &Waitables, bool WaitAll, DWORD Timeout = INFINITE, bool WaitAPC = false, bool WaitMsg = false);

/**
 * @ingroup Threading
 * @brief Waitable base class
 *
 * Base class for all handle-based waitable objects
 **/
class TWaitable {
public:
	virtual ~TWaitable() {}
	/**
	 * Wait for a given amount of time or until signaled
	 **/
	virtual WaitResult WaitFor(DWORD Timeout = INFINITE)
	{ return WaitSingle(*this, Timeout); };

	/**
	 * Get the waitable handle for advanced operations
	 **/
	virtual HANDLE CreateWaitHandle(void) const
	{ FAIL(_T("Abstract function")); };
};

/**
 * @ingroup Threading
 * @brief Semaphore
 *
 * (Optionally Named) Semaphore
 **/
class TSemaphore : public TWaitable {
private:
	HANDLE rSemaphore;

public:
	TSemaphore(LONG Initial = 0, LONG Maximum = MAXINT, LPCTSTR Name = nullptr);
	~TSemaphore(void) override;

	/**
	 * Signal the semaphore with given count
	 * @return Previous semaphore count
	 **/
	LONG Signal(LONG Count = 1);

	HANDLE CreateWaitHandle(void) const override;
};

/**
 * @ingroup Threading
 * @brief Mutex
 *
 * (Optionally Named) Mutex
 **/
class TMutex : public TWaitable {
private:
	HANDLE rMutex;

public:
	TMutex(bool Acquired = false, LPCTSTR Name = nullptr);
	~TMutex(void) override;

	/**
	 * Acquire the lock, wait forever
	 **/
	WaitResult Acquire(void)
	{ return WaitFor(); }

	/**
	 * Try to acquire the lock within given time
	 **/
	WaitResult TryAcquire(DWORD Timeout = 0)
	{ return WaitFor(Timeout); }

	/**
	 * Release the lock
	 **/
	void Release(void);

	HANDLE CreateWaitHandle(void) const override;
};

/**
 * @ingroup Threading
 * @brief Event
 *
 * (Optionally Named) Event
 **/
class TEvent : public TWaitable {
private:
	HANDLE rEvent;

public:
	TEvent(bool ManualReset = false, bool Initial = false, LPCTSTR Name = nullptr);
	~TEvent(void) override;

	/**
	 * Set the event
	 **/
	void Set(void);

	/**
	 * Reset the event
	 **/
	void Reset(void);

	/**
	 * Pulse the event
	 **/
	void Pulse(void);

	HANDLE CreateWaitHandle(void) const override;
};

/**
 * @ingroup Threading
 * @brief Critical Section
 *
 * Lightweight synchronization for single thread
 * @note Not a waitable object (no handle)
 **/
class TCriticalSection {
private:
	RTL_CRITICAL_SECTION rCriticalSection;
	DEBUGVV(DWORD OwnerThreadID);

public:
	TCriticalSection(bool Acquired = false, DWORD SpinCount = 5120);
	virtual ~TCriticalSection(void);

	/**
	 * Enter the critical section
	 **/
	void Enter(void);

	/**
	 * Try enter the critical section
	 **/
	bool TryEnter(void);

	/**
	 * Leave the critical section
	 **/
	void Leave(void);
};

template<typename TOrdinal32>
class TInterlockedSyncOrdinal32 {
private:
	LONG volatile rOrdinal;

public:
	TInterlockedSyncOrdinal32(void) {}
	TInterlockedSyncOrdinal32(TOrdinal32 const &xOrdinal) : rOrdinal(xOrdinal) {}
	virtual ~TInterlockedSyncOrdinal32(void) {}

	inline TOrdinal32 Increment(void)
	{ return InterlockedIncrement(&rOrdinal); }
	inline TOrdinal32 Decrement(void)
	{ return InterlockedDecrement(&rOrdinal); }
	inline TOrdinal32 Add(TOrdinal32 const &Amount)
	{ return InterlockedAdd(&rOrdinal, (LONG)Amount); }
	inline TOrdinal32 Exchange(TOrdinal32 const &SwpVal)
	{ return InterlockedExchange(&rOrdinal, (LONG)SwpVal); }
	inline TOrdinal32 ExchangeAdd(TOrdinal32 const &Amount)
	{ return InterlockedExchangeAdd(&rOrdinal, (LONG)Amount); }
	inline TOrdinal32 CompareAndSwap(TOrdinal32 const &Cmp, TOrdinal32 const &SwpVal)
	{ return InterlockedCompareExchange(&rOrdinal, (LONG)SwpVal, (LONG)Cmp); }

	inline TOrdinal32 operator++(void)
	{ return Increment(); }
	inline TOrdinal32 operator++(int)
	{ return Increment() - 1; }
	inline TOrdinal32 operator--(void)
	{ return Decrement(); }
	inline TOrdinal32 operator--(int)
	{ return Decrement() + 1; }
	inline TOrdinal32 operator+=(TOrdinal32 const &Amount)
	{ return Add(Amount); }
	inline TOrdinal32 operator-=(TOrdinal32 const &Amount)
	{ return Add(-Amount); }
	inline TOrdinal32 operator=(TOrdinal32 const &Value)
	{ Exchange(Value); return Value; }
	inline TOrdinal32 operator~(void)
	{ return Add(0); }
};

template<typename TOrdinal64>
class TInterlockedSyncOrdinal64 {
private:
	LONGLONG volatile rOrdinal;

public:
	TInterlockedSyncOrdinal64(void) {}
	TInterlockedSyncOrdinal64(TOrdinal64 const &xOrdinal) : rOrdinal(xOrdinal) {}
	virtual ~TInterlockedSyncOrdinal64(void) {}

	inline TOrdinal64 Increment(void)
	{ return InterlockedIncrement64(&rOrdinal); }
	inline TOrdinal64 Decrement(void)
	{ return InterlockedDecrement64(&rOrdinal); }
	inline TOrdinal64 Add(TOrdinal64 const &Amount)
	{ return InterlockedAdd64(&rOrdinal, (LONGLONG)Amount); }
	inline TOrdinal64 Exchange(TOrdinal64 const &SwpVal)
	{ return InterlockedExchange64(&rOrdinal, (LONGLONG)SwpVal); }
	inline TOrdinal64 ExchangeAdd(TOrdinal64 const &Amount)
	{ return InterlockedExchangeAdd64(&rOrdinal, (LONGLONG)Amount); }
	inline TOrdinal64 CompareAndSwap(TOrdinal64 const &Cmp, TOrdinal64 const &SwpVal)
	{ return InterlockedCompareExchange64(&rOrdinal, (LONGLONG)SwpVal, (LONGLONG)Cmp); }

	inline TOrdinal64 operator++(void)
	{ return Increment(); }
	inline TOrdinal64 operator++(int)
	{ return Increment() - 1; }
	inline TOrdinal64 operator--(void)
	{ return Decrement(); }
	inline TOrdinal64 operator--(int)
	{ return Decrement() + 1; }
	inline TOrdinal64 operator+=(TOrdinal64 const &Amount)
	{ return Add(Amount); }
	inline TOrdinal64 operator-=(TOrdinal64 const &Amount)
	{ return Add(-Amount); }
	inline TOrdinal64 operator=(TOrdinal64 const &Value)
	{ Exchange(Value); return Value; }
	inline TOrdinal64 operator~(void)
	{ return Add(0); }
};

#ifdef _WIN64
#define TInterlockedSyncOrdinal TInterlockedSyncOrdinal64
#else
#define TInterlockedSyncOrdinal TInterlockedSyncOrdinal32
#endif //_WIN64

#endif //SyncPerms_H
