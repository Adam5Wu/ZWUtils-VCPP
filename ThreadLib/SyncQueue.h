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
 * @brief Synchronized Message Queue
 * @author Zhenyu Wu
 * @date Aug 01, 2013: Initial implementation
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef SyncQueue_H
#define SyncQueue_H

#include <deque>

#include "SyncObjs.h"
#include "SyncPrems.h"

//#define SIGNAL_MODERATION

/**
 * @ingroup Threading
 * @brief Synchronized queue
 *
 * Synchronized wrapper around a STL deque-like container for queue (FIFO) operations
 **/
template<class T, class Container = std::deque<T>>
class TSyncQueue : TLockable {
public:
	typedef typename Container::size_type size_type;
protected:
	TSyncObj<Container> Queue;
	TEvent WaitEvent;
#ifdef EMPTY_EVENT
	TEvent EmptyEvent;
#endif //EMPTY_EVENT
#ifdef SIGNAL_MODERATION
	TLockableCS WaitLock;
#endif //SIGNAL_MODERATION

	bool TryDequeue(T& entry);
public:
	TString const Name;

#ifdef EMPTY_EVENT
#ifdef SIGNAL_MODERATION
	TSyncQueue(TString const &xName) : Name(xName), WaitEvent(false), EmptyEvent(true) {}
#else
	TSyncQueue(TString const &xName) : Name(xName), WaitEvent(true), EmptyEvent(true) {}
#endif
#else
#ifdef SIGNAL_MODERATION
	TSyncQueue(TString const &xName) : Name(xName), WaitEvent(false) {}
#else
	TSyncQueue(TString const &xName) : Name(xName), WaitEvent(true) {}
#endif
#endif //EMPTY_EVENT

	~TSyncQueue() override;

	/**
	 * Put an object into the queue (no upper limit)
	 **/
	size_type Enqueue(T entry);
	/**
	 * Construct and put an object into the queue (no upper limit)
	 **/
	template<typename... Params>
	size_type Emplace_Enqueue(Params&&... xParams);

	/**
	 * Try get an object fromt the queue with given timeout
	 * @note: For multiple concurrent getters, fairness and timeout precision are NOT guaranteed!
	 **/
	bool Dequeue(T &entry, DWORD Timeout = INFINITE, TWaitable *xWaitEvent = nullptr);

	/**
	 * Return the instantaneous length of the queue
	 **/
	inline size_type Length(void);

	inline void AdjustSize(void);

#ifdef EMPTY_EVENT
	/**
	 * Try waiting for queue to become empty and hold a lock on the queue
	 * @note: For multiple concurrent waiters, fairness and timeout precision are NOT guaranteed!
	 **/
	bool EmptyLock(DWORD Timeout = INFINITE, TWaitable *xWaitEvent = nullptr);

	/**
	 * Release lock on the queue acquired by EmptyLock
	 * @note: You MUST use this function instead of SyncUnlock, or other threads on EmptyLock may sleep longer than needed!
	 **/
	void EmptyUnlock(void);
#endif //EMPTY_EVENT

	inline void __SyncLock(void) override
	{ Queue.__SyncLock(); }
	inline bool __SyncTryLock(void) override
	{ return Queue.__SyncTryLock(); }
	inline void __SyncUnlock(void) override
	{ Queue.__SyncUnlock(); }

	using TLockable::SyncLock;
	using TLockable::SyncTryLock;
};

#define SQLogTag _T("Sync.Q '%s'")
#define SQLogHeader _T("{") SQLogTag _T("} ")

//! @ingroup Threading
//! Synchronized queue specific exception
class TSyncQueueException : public Exception {
public:
	TString const SyncQueueName;

	template <class CSyncQueue, typename... Params>
	TSyncQueueException(const CSyncQueue& xSyncQueue, LPCTSTR &&xSource, LPCTSTR ReasonFmt, Params&&... xParams) :
		SyncQueueName(xSyncQueue.Name), Exception(xSource, xParams...) {}

	LPCTSTR Why(void) const override;
};

//! @ingroup Threading
//! Raise an exception within a synchronized queue with formatted string message
#define SQFAIL(...)																\
{																				\
	SOURCEMARK;																	\
	throw new TSyncQueueException(*this, std::move(__SrcMark), __VA_ARGS__);	\
}

//! Perform logging within a synchronized queue
#define SQLOG(s, ...) LOG(SQLogHeader s, Name.c_str(), __VA_ARGS__)
#define SQLOGV(s, ...) LOGV(SQLogHeader s, Name.c_str(), __VA_ARGS__)
#define SQLOGVV(s, ...) LOGVV(SQLogHeader s, Name.c_str(), __VA_ARGS__)

template<class T, class Container>
TSyncQueue<T, Container>::~TSyncQueue() {
	SQLOGV(_T("Destruction in progress..."));
	auto rQueue(Queue.Pickup());
	if (size_t QSize = rQueue->size()) {
		SQLOGV(_T("There are %d entries left over in queue"), (int)QSize);
	}
}

template<class T, class Container>
typename TSyncQueue<T, Container>::size_type TSyncQueue<T, Container>::Enqueue(T entry) {
	auto rQueue(Queue.Pickup());
	rQueue->push_back(entry);
#ifdef SIGNAL_MODERATION
	auto Lock = WaitLock.SyncTryLock();
	if (!Lock.isLocked)
		// There is a getter waiting / trying to wait
		WaitEvent.Set();
	return rQueue->size();
#else
	size_type Ret = rQueue->size();
	if (Ret == 1)
		// There may be a getter waiting
		WaitEvent.Set();
	return Ret;
#endif//SIGNAL_MODERATION
}

template<class T, class Container>
template<typename... Params>
typename TSyncQueue<T, Container>::size_type TSyncQueue<T, Container>::Emplace_Enqueue(Params&&... xParams) {
	auto rQueue(Queue.Pickup());
	rQueue->emplace_back(xParams...);
#ifdef SIGNAL_MODERATION
	auto Lock = WaitLock.SyncTryLock();
	if (!Lock.isLocked)
		// There is a getter waiting / trying to wait
		WaitEvent.Set();
	return rQueue->size();
#else
	size_type Ret = rQueue->size();
	if (Ret == 1)
		// There may be a getter waiting
		WaitEvent.Set();
	return Ret;
#endif//SIGNAL_MODERATION
}

template<class T, class Container>
bool TSyncQueue<T, Container>::TryDequeue(T& entry) {
	auto rQueue(Queue.Pickup());
	size_t QueueSize = rQueue->size();
	if (QueueSize > 0) {
		entry = rQueue->front();
		rQueue->pop_front();
#ifdef EMPTY_EVENT
		if (QueueSize == 1)
			// Signal empty condition
			EmptyEvent.Set();
#endif //EMPTY_EVENT
		return true;
	} else {
		// Queue is empty!
		WaitEvent.Reset();
	}
	return false;
}

template<class T, class Container>
bool TSyncQueue<T, Container>::Dequeue(T &entry, DWORD Timeout, TWaitable *xWaitEvent) {
	Flatten_FILETIME EnterTime;
	if (Timeout != INFINITE)
		GetSystemTimeAsFileTime(&EnterTime.FileTime);
	while (true) {
		if (TryDequeue(entry))
			return true;
#ifdef SIGNAL_MODERATION
		Synchronized(WaitLock, {
			size_type QueueSize = Length();
			if (QueueSize == 0) {
				DWORD Delta;
				if (Timeout != INFINITE) {
					Flatten_FILETIME CurTime;
					GetSystemTimeAsFileTime(&CurTime.FileTime);
					Delta = (DWORD)((CurTime.U64 - EnterTime.U64) / MSTime_o100ns);
					if (Delta > Timeout) return false;
				} else
					Delta = 0;
				if (xWaitEvent == nullptr) {
					if (WaitEvent.WaitFor(Timeout - Delta) != WaitResult::Signaled)
						return false;
				} else {
					if (WaitMultiple({WaitEvent, *xWaitEvent}, false, Timeout - Delta) != WaitResult::Signaled_0)
						return false;
				}
			}
		});
#else
		DWORD Delta;
		if (Timeout != INFINITE) {
			Flatten_FILETIME CurTime;
			GetSystemTimeAsFileTime(&CurTime.FileTime);
			Delta = (DWORD)((CurTime.U64 - EnterTime.U64) / MSTime_o100ns);
			if (Delta > Timeout) return false;
		} else
			Delta = 0;
		if (xWaitEvent == nullptr) {
			if (WaitEvent.WaitFor(Timeout - Delta) != WaitResult::Signaled)
				return false;
		} else {
			if (WaitMultiple({WaitEvent, *xWaitEvent}, false, Timeout - Delta) != WaitResult::Signaled_0)
				return false;
		}
#endif//SIGNAL_MODERATION
	}
}

template<class T, class Container>
typename TSyncQueue<T, Container>::size_type TSyncQueue<T, Container>::Length(void) {
	auto rQueue(Queue.Pickup());
	return rQueue->size();
}

template<class T, class Container>
void TSyncQueue<T, Container>::AdjustSize(void) {
	auto rQueue(Queue.Pickup());
	rQueue->shrink_to_fit();
}


#ifdef EMPTY_EVENT

template<class T, class Container>
bool TSyncQueue<T, Container>::EmptyLock(DWORD Timeout, TWaitable *xWaitEvent) {
	Flatten_FILETIME EnterTime;
	if (Timeout != INFINITE)
		GetSystemTimeAsFileTime(&EnterTime.FileTime);

	while (true) {
		// Synchronized Frame
		{
			auto rQueue(Queue.Pickup());
			if (rQueue->size() == 0) {
				Queue.__SyncLock();
				return true;
			}
		}

		DWORD Delta;
		if (Timeout != INFINITE) {
			Flatten_FILETIME CurTime;
			GetSystemTimeAsFileTime(&CurTime.FileTime);
			Delta = (DWORD)((CurTime.U64 - EnterTime.U64) / MSTime_o100ns);
			if (Delta > Timeout) return false;
		} else
			Delta = 0;
		if (xWaitEvent == nullptr) {
			if (EmptyEvent.WaitFor(Timeout - Delta) != WaitResult::Signaled)
				return false;
		} else {
			if (WaitMultiple({EmptyEvent, *xWaitEvent}, false, Timeout - Delta) != WaitResult::Signaled_0)
				return false;
		}
	}
}

template<class T, class Container>
void TSyncQueue<T, Container>::EmptyUnlock(void) {
	auto rQueue(Queue.Pickup());
	if (rQueue->size() != 0)
		EmptyEvent.Reset();
	Queue.__SyncUnlock();
}

#endif //EMPTY_EVENT

#undef SQFAIL
#undef SQLOG
#undef SQLOGV
#undef SQLOGVV

#endif//SyncQueue_H