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
 * @brief Synchronized Object Pool Template
 * @author Zhenyu Wu
 * @date Aug 02, 2013: Initial implementation
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef SyncObjPool_H
#define SyncObjPool_H

#include "BaseLib/Misc.h"
#include "BaseLib/Allocator.h"
#include "BaseLib/ManagedRef.h"
#include "SyncObjs.h"
#include "SyncQueue.h"

/**
 * @ingroup Threading
 * @brief Synchronized object pool template
 *
 * Defines a life-cycle managed pool of object resource, upports allocation provisioning and buffer reduction
 * @note Since the underlying implementation uses TSyncQueue, it inherits similar functionality limitations
 **/
template <class T, class TAllocator = SimpleAllocator<T>>
class TSyncObjPool {
public:
	class TSyncPoolObj {
		friend TSyncObjPool;
	protected:
		TSyncObjPool *const Parent;
		TSyncPoolObj(TSyncObjPool *xParent) : Parent(xParent) {}
		TSyncPoolObj(TSyncObjPool&&) = delete;
		TSyncPoolObj(TSyncObjPool const&) = delete;
		virtual ~TSyncPoolObj(void) {}
	public:
		inline T& operator*(void) const
		{ return *&*this; }
		inline T* operator->(void) const
		{ return &*this; }
		inline operator T&() const
		{ return **this; }
		inline bool operator==(TSyncPoolObj const &xSPO) const
		{ return (T&)*this == (T&)xSPO; }

		TSyncPoolObj& operator=(TSyncPoolObj &&) = delete;
		TSyncPoolObj& operator=(TSyncPoolObj const &) = delete;

		TSyncPoolObj* operator~(void)
		{ return this; }
		TSyncPoolObj const* operator~(void) const
		{ return this; }

		virtual T* operator&(void) const
		{ FAIL(_T("Abstract function")); }
		virtual void Replace(T *xObj)
		{ FAIL(_T("Abstract function")); }

		//! Release the resource back to the pool
		//! After this call, all references of this wrapper or the contained object are invalid!
		virtual void Release(void)
		{ Parent->Release(this); }
	};
protected:
	TLockableCS AquisitionLock;
	TEvent ObjReturn;
	TAllocator* ObjAlloc = nullptr;

	typedef TSyncQueue<TSyncPoolObj*> TSyncQueuePool;
	TSyncQueuePool Pool;

	typedef typename TSyncQueuePool::size_type size_type;
	size_type const AllocBlock;
	size_type const Sentinal;
	size_type const Limit;

	TInterlockedSyncOrdinal<size_type> AllocCnt;
	TSyncBool GrowLimit;

	void __CheckGrow(void);
	void __CheckShrink(size_type PoolSize);
	bool __ObjectReturnTryLock(void);

	virtual void Release(TSyncPoolObj* Obj);
public:
	TString const Name;

	TSyncObjPool(TString const &xName, UINT32 xLimit = INFINITE, UINT32 xAllocBlock = 256);
	virtual ~TSyncObjPool(void);

	/**
	 * Acquire an object via the TSyncPoolObj reference with timeout
	 * @note Resource releasing via the Release() function of the TSyncPoolObj reference
	 **/
	virtual TSyncPoolObj* Acquire(DWORD Timeout = INFINITE, TWaitable *xWaitEvent = nullptr);

	/**
	 * Return the instantaneous capacity (all allocated = released + acuqired) of the pool
	 **/
	inline size_type Capacity(void)
	{ return ~AllocCnt; }

	/**
	 * Wait for all objects return to the pool and lock the acquistion operation
	 * @note Only intended for short operations (otherwise may hang Acquire indefinitely)
	 **/
	bool ObjectReturnLock(DWORD Timeout = INFINITE, TWaitable *xWaitEvent = nullptr);

	/**
	 * Release acquistion operation lock
	 **/
	void ObjectReturnUnlock(void);

	/**
	 * Switch to a new instance of pool object allocator
	 **/
	bool SetAllocator(TAllocator *xObjAlloc, DWORD Timeout = INFINITE, TWaitable *xWaitEvent = nullptr);
};

#define SOPLogTag _T("Sync.ObjPool '%s'")
#define SOPLogHeader _T("{") SOPLogTag _T("} ")

//! @ingroup Threading
//! Synchronized object pool specific exception
class TSyncObjPoolException : public Exception {
public:
	TString const SyncObjPoolName;

	template <class T, class TAllocator, typename... Params>
	TSyncObjPoolException(TSyncObjPool<T, TAllocator> const &xSyncObjPool, LPCTSTR &&xSource, LPCTSTR ReasonFmt, Params&&... xParams) :
		SyncObjPoolName(xSyncObjPool.Name), Exception(std::move(xSource), ReasonFmt, xParams...) {}

	LPCTSTR Why(void) const override;
};

//! @ingroup Threading
//! Raise an exception within a synchronized object pool with formatted string message
#define SOPFAIL(...)															\
{																				\
	SOURCEMARK;																	\
	throw new TSyncObjPoolException(*this, std::move(__SrcMark), __VA_ARGS__);	\
}

//! Perform logging within a synchronized queue
#define SOPLOG(s, ...) LOG(SOPLogHeader s, Name.c_str(), __VA_ARGS__)
#define SOPLOGV(s, ...) LOGV(SOPLogHeader s, Name.c_str(), __VA_ARGS__)
#define SOPLOGVV(s, ...) LOGVV(SOPLogHeader s, Name.c_str(), __VA_ARGS__)

template<class T, class TAllocator>
class SyncPoolObj_Impl : public TSyncObjPool<T, TAllocator>::TSyncPoolObj {
protected:
	T* PoolObj;

public:
	SyncPoolObj_Impl(TSyncObjPool<T, TAllocator> *xParent) :
		TSyncObjPool<T, TAllocator>::TSyncPoolObj(xParent), PoolObj(TAllocator::Create()) {}
	~SyncPoolObj_Impl(void)
	{ TAllocator::Destroy(PoolObj); }

	inline T* operator&(void) const override
	{ return PoolObj; }
	inline void Replace(T *xObj) override
	{ TAllocator::Destroy((T*)InterlockedExchangePointer((PVOID*)&PoolObj, xObj)); }

	inline static SyncPoolObj_Impl* Create(TSyncObjPool<T, TAllocator> *xParent)
	{ return new SyncPoolObj_Impl(xParent); }
};

template<class T, class TAllocator>
TSyncObjPool<T, TAllocator>::TSyncObjPool(TString const &xName, UINT32 xLimit, UINT32 xAllocBlock) :
Name(xName), AllocBlock(xAllocBlock), Sentinal(xAllocBlock / 4), Limit(xLimit), AllocCnt(0), GrowLimit(false), Pool(xName + _T(".Pool")) {
	if (Sentinal == 0)
		SOPFAIL(_T("Allocation block size too small (%d)"), AllocBlock);
	if (Limit < Sentinal)
		SOPFAIL(_T("Allocation limit (%d) < Allocation sentinal (%d)"), Limit, Sentinal);
	if (AllocBlock < Sentinal)
		SOPFAIL(_T("Allocation block size (%d) < Allocation sentinal (%d)"), AllocBlock, Sentinal);
	if ((Limit != INFINITE) && (Limit % AllocBlock != 0)) {
		SOPLOG(_T("WARNING: Allocation limit (%d) is not a multiple of Allocation block size (%d)"), Limit, AllocBlock);
	}
	__CheckGrow();
}

template<class T, class TAllocator>
TSyncObjPool<T, TAllocator>::~TSyncObjPool(void) {
	SOPLOGVV(_T("Destruction in progress..."));

	Pool.__SyncLock();
	TSyncQueuePool::size_type PoolSize = Pool.Length();

	DEBUGV({
		if (PoolSize != ~AllocCnt)
		SOPLOGV(_T("WARNING: Total %d objects allocated, %d accounted for (%d delta)"), ~AllocCnt, PoolSize, ~AllocCnt - PoolSize);
	});

	while (PoolSize--) {
		TSyncPoolObj* Entry;
		if (!Pool.Dequeue(Entry, 0)) {
			SOPLOG(_T("WARNING: Administrative object pool dequeue timeout (unexpected)"));
			continue;
		}
		delete Entry;
	}

	if (ObjAlloc) delete ObjAlloc;
}

template<class T, class TAllocator>
void TSyncObjPool<T, TAllocator>::__CheckGrow(void) {
	// Quick return if already grown to the limit
	if (GrowLimit.CompareAndSwap(false, false))
		return;

	TSyncQueuePool::size_type PoolSize = Pool.Length();
	if (PoolSize <= Sentinal) {
		Synchronized(Pool, {
			// Check if already grown to the limit
			if ((Limit != INFINITE) && (~AllocCnt >= Limit)) {
				// Moderate frequency event
				//SOPLOGVV(_T("WARNING: Incremental allocation reached the limit of %d"), Limit);
				GrowLimit.Exchange(true);
				return;
			}
			// Get the authoritative length
			PoolSize = Pool.Length();
			if (PoolSize <= Sentinal) {
				TSyncQueuePool::size_type AllocSize = AllocBlock;
				// High frequency event
				//SOPLOGVV(_T("Incremental allocation of %d entries from %d"), AllocSize, ~AllocCnt);
				while (AllocSize-- > 0) {
					Pool.Enqueue(SyncPoolObj_Impl<T, TAllocator>::Create(this));
					++AllocCnt;
				}
			}
		});
	}
}

template<class T, class TAllocator>
void TSyncObjPool<T, TAllocator>::__CheckShrink(typename TSyncObjPool<T, TAllocator>::size_type PoolSize) {
	if (PoolSize >= AllocBlock + Sentinal) {
		Synchronized(Pool, {
			// Get the authoritative length
			PoolSize = Pool.Length();
			if (PoolSize >= AllocBlock + Sentinal) {
				TSyncQueuePool::size_type DeallocSize = Sentinal;
				// High frequency event
				//SOPLOGVV(_T("Incremental deallocation of %d entries from %d"), DeallocSize, ~AllocCnt);
				int iAllocCnt = 0;
				GrowLimit.Exchange(false);
				while (DeallocSize-- > 0) {
					TSyncPoolObj* Entry;
					if (!Pool.Dequeue(Entry, 0)) {
						SOPLOG(_T("WARNING: Administrative object pool dequeue timeout (unexpected)"));
						continue;
					}
					delete Entry;
					--AllocCnt;
				}
				Pool.AdjustSize();
			}
		});
	}
}

template<class T, class TAllocator>
void TSyncObjPool<T, TAllocator>::Release(TSyncPoolObj* Obj) {
	TSyncQueuePool::size_type PoolSize = Pool.Enqueue(Obj);
	if (PoolSize == ~AllocCnt)
		ObjReturn.Set();
	__CheckShrink(PoolSize);
}

template<class T, class TAllocator>
typename TSyncObjPool<T, TAllocator>::TSyncPoolObj* TSyncObjPool<T, TAllocator>::Acquire(DWORD Timeout, TWaitable *xWaitEvent) {
	Synchronized(AquisitionLock, {
		__CheckGrow();
		TSyncPoolObj* Entry = nullptr;
		if (!Pool.Dequeue(Entry, Timeout, xWaitEvent)) {
			//SOPLOGVV(_T("WARNING: Timeout dequeuing from object pool"));
		}
		return Entry;
	});
}

template<class T, class TAllocator>
bool TSyncObjPool<T, TAllocator>::__ObjectReturnTryLock(void) {
	Synchronized(AquisitionLock, {
		Synchronized(Pool, {
			if (Pool.Length() == ~AllocCnt) {
				AquisitionLock.__SyncLock();
				return true;
			}
			ObjReturn.Reset();
			return false;
		});
	});
}

template<class T, class TAllocator>
bool TSyncObjPool<T, TAllocator>::ObjectReturnLock(DWORD Timeout, TWaitable *xWaitEvent) {
	Flatten_FILETIME EnterTime;
	if (Timeout != INFINITE)
		GetSystemTimeAsFileTime(&EnterTime.FileTime);

	while (true) {
		if (__ObjectReturnTryLock())
			return true;

		DWORD Delta;
		if (Timeout != INFINITE) {
			Flatten_FILETIME CurTime;
			GetSystemTimeAsFileTime(&CurTime.FileTime);
			Delta = (DWORD)((CurTime.U64 - EnterTime.U64) / MSTime_o100ns);
			if (Delta > Timeout) return false;
		} else
			Delta = 0;
		if (xWaitEvent == nullptr) {
			if (ObjReturn.WaitFor(Timeout - Delta) != WaitResult::Signaled)
				return false;
		} else {
			if (WaitMultiple({ObjReturn, *xWaitEvent}, false, Timeout - Delta) != WaitResult::Signaled_0)
				return false;
		}
	}
}

template<class T, class TAllocator>
void TSyncObjPool<T, TAllocator>::ObjectReturnUnlock(void) {
	AquisitionLock.__SyncUnlock();
}

template<class T, class TAllocator>
bool TSyncObjPool<T, TAllocator>::SetAllocator(TAllocator *xObjAlloc, DWORD Timeout, TWaitable *xWaitEvent) {
	if (ObjectReturnLock(Timeout, xWaitEvent)) {
		// Remove all queue entries (so all objects are deallocated using the existing allocator)
		TSyncQueuePool::size_type PoolSize = Pool.Length();
		while (PoolSize--) {
			TSyncPoolObj* Entry;
			if (!Pool.Dequeue(Entry, 0)) {
				SOPLOG(_T("WARNING: Administrative object pool dequeue timeout (unexpected)"));
				continue;
			}
			delete Entry;
		}

		// Switch to the new allocator
		if (ObjAlloc) delete ObjAlloc;
		ObjAlloc = xObjAlloc;
		// Populate the queue back
		__CheckGrow();
		// Release the aquisition lock
		ObjectReturnUnlock();

		return true;
	}
	return false;
}

#undef SOPFAIL
#undef SOPLOG
#undef SOPLOGV
#undef SOPLOGVV

#endif //SyncObjPool_H