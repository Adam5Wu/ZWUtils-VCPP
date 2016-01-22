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

#define __MM_MAIN__
#include "BaseLib/MMSwitcher.h"

#include <Windows.h>

#include "BaseLib/Misc.h"

#include "BaseLib/DebugLog.h"
#include "BaseLib/Exception.h"
#include "BaseLib/WinError.h"

#include "ThreadLib/SyncObjs.h"
#include "ThreadLib/WorkerThread.h"
#include "ThreadLib/SyncQueue.h"
#include "ThreadLib/SyncObjPool.h"
#include "ThreadLib/ThreadThrottler.h"
#include "ThreadLib/StackWalker.h"

#include "Modeling/Identifier.h"

class TestExcept : public TRunnable {
protected:
	void* Run(TWorkerThread &WorkerThread, void* NoUse) override {
		LOG(_T("Yee Hah 1!"));
		FAIL(_T("Test!"));
		return nullptr;
	}
};

class TestCrash : public TRunnable {
protected:
	void* Run(TWorkerThread &WorkerThread, void* NoUse) override {
		LOG(_T("Yee Hah 2!"));
		int** p = 0;
		return *p;
	}
};

struct Integer {
	int value;
	Integer() {}
	Integer(int v) : value(v) {}

	operator int&() {
		return value;
	}
	int operator++(int) {
		return value++;
	}
	Integer& operator=(int i) {
		return value = i, *this;
	}
};
bool operator==(Integer const &A, Integer const &B)
{ return A.value == B.value; }
typedef TSyncObj<Integer> TSyncInteger;

class TestCount : public TRunnable {
protected:
	void* Run(TWorkerThread &WorkerThread, void* pSyncInt) override {
		int COUNT = 100000;
		TSyncInteger& Ctr = *static_cast<TSyncInteger*>(pSyncInt);
		for (int i = 0; i < COUNT; i++) {
			((Integer&)Ctr.Pickup())++;
		}
		LOG(_T("Count of %d done!"), COUNT);
		return nullptr;
	}
};

typedef TSyncQueue<int> TSyncIntQueue;

class TestQueuePut : public TRunnable {
protected:
	void* Run(TWorkerThread &WorkerThread, void* pSyncIntQueue) override {
		TSyncIntQueue& Q = *(TSyncIntQueue*)pSyncIntQueue;

		int COUNT = IsDebuggerPresent() ? 10000 : 1000000;
		Flatten_FILETIME StartTime;
		GetSystemTimeAsFileTime(&StartTime.FileTime);
		for (int i = 0; i < COUNT; i++)
			Q.Enqueue(i);
		Flatten_FILETIME EndTime;
		GetSystemTimeAsFileTime(&EndTime.FileTime);

		double TimeSpan = (double)(EndTime.U64 - StartTime.U64) / MSTime_o100ns / MSTime_aSecond;
		LOG(_T("Enqueue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
		return nullptr;
	}
};

class TestQueueGet : public TRunnable {
protected:
	void* Run(TWorkerThread &WorkerThread, void* pSyncIntQueue) override {
		TSyncIntQueue& Q = *(TSyncIntQueue*)pSyncIntQueue;
		int j;

		int COUNT = IsDebuggerPresent() ? 10000 : 1000000;
		Flatten_FILETIME StartTime;
		GetSystemTimeAsFileTime(&StartTime.FileTime);
		for (int i = 0; i < COUNT; i++) {
			Q.Dequeue(j);
			if (i != j)
				FAIL(_T("Expect %d, got %d"), i, j);
		}
		Flatten_FILETIME EndTime;
		GetSystemTimeAsFileTime(&EndTime.FileTime);

		double TimeSpan = (double)(EndTime.U64 - StartTime.U64) / MSTime_o100ns / MSTime_aSecond;
		LOG(_T("Dequeue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
		return nullptr;
	}
};

typedef TSyncObjPool<Integer> TSyncIntPool;

class TestSyncPool : public TRunnable {
private:
	TSyncQueue<TSyncIntPool::TSyncPoolObj*> ObjQueue;
	TSyncBool SenderReceiver;
protected:
	void* Run(TWorkerThread &WorkerThread, void* pSyncIntPool) override {
		TSyncIntPool& Pool = *(TSyncIntPool*)pSyncIntPool;

		int COUNT = IsDebuggerPresent() ? 10000 : 1000000;

		bool Sender = true;
		if (SenderReceiver.CompareAndSwap(false, Sender)) {
			// Sender Job
			Flatten_FILETIME StartTime;
			GetSystemTimeAsFileTime(&StartTime.FileTime);
			for (int i = 0; i < COUNT; i++) {
				TSyncIntPool::TSyncPoolObj *Entry = Pool.Acquire();
				ObjQueue.Enqueue(Entry);
			}
			Flatten_FILETIME EndTime;
			GetSystemTimeAsFileTime(&EndTime.FileTime);

			double TimeSpan = (double)(EndTime.U64 - StartTime.U64) / MSTime_o100ns / MSTime_aSecond;
			LOG(_T("Enqueue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
		} else {
			// Receiver Job
			// Sender Job
			Flatten_FILETIME StartTime;
			GetSystemTimeAsFileTime(&StartTime.FileTime);
			for (int i = 0; i < COUNT; i++) {
				TSyncIntPool::TSyncPoolObj *Entry;
				ObjQueue.Dequeue(Entry);
				Entry->Release();
			}
			Flatten_FILETIME EndTime;
			GetSystemTimeAsFileTime(&EndTime.FileTime);

			double TimeSpan = (double)(EndTime.U64 - StartTime.U64) / MSTime_o100ns / MSTime_aSecond;
			LOG(_T("Dequeue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
		}
		return nullptr;
	}
public:
	TestSyncPool() : SenderReceiver(false), ObjQueue(_T("TestSyncPool_Queue")) {}
};

class BusyRunnable : public TRunnable {
protected:
	TSyncInt32 WorkerCnt = 0;
	void* Run(TWorkerThread &WorkerThread, void* xFactor) override {
		int Factor = (int)xFactor;
		bool State = false;
		while (WorkerThread.CurrentState() == TWorkerThread::State::Running) {
			Flatten_FILETIME CurTime;
			GetSystemTimeAsFileTime(&CurTime.FileTime);
			if ((CurTime.U64 / MSTime_o100ns / MSTime_aSecond / Factor) & 1) {
				if (State) {
					State = false;
					LOG(_T("{BusyWorker %s} Idle (%d Workers)"), WorkerThread.Name.c_str(), --WorkerCnt);
				}
				Sleep(500);
			} else {
				if (!State) {
					State = true;
					LOG(_T("{BusyWorker %s} Busy (%d Workers)"), WorkerThread.Name.c_str(), ++WorkerCnt);
				}
			}
		}
		return nullptr;
	}
};

void TestException(void) {
	LOG(_T("*** Test Exception throwing and catching"));
	try {
		FAIL(_T("XD"));
	} catch (Exception *e) {
		e->Show();
		delete e;
	}
}

void TestErrCode(void) {
	LOGS(_T("*** Test Error code decoding"));
	TCHAR Msg[2048];
	DecodeError(Msg, 2048, 6);
	LOGS(_T("Error 6: %s"), Msg);

	DecodeError(Msg, 2048, 123);
	LOGS(_T("Error 123: %s"), Msg);

	DecodeError(Msg, 2048, 234);
	LOGS(_T("Error 234: %s"), Msg);

	HMODULE ModuleHandle = GetModuleHandle(_T("ntdll.dll"));
	DecodeError(ModuleHandle, Msg, 2048, STATUS_INVALID_HANDLE);
	LOGS(_T("{ntdll.dll} Error %0.8x: %s"), STATUS_INVALID_HANDLE, Msg);
}

void TestSyncObj(void) {
	LOG(_T("*** Test SyncObj (Non-threading correctness)"));
	TSyncInteger a;
	TSyncInteger b(100);

	LOG(_T("--- Increment by one"));
	LOG(_T("a = %d"), (Integer&)a.Pickup());
	((Integer)a.Pickup())++;
	LOG(_T("a = %d"), (Integer&)a.Pickup());

	LOG(_T("--- Assign and snapshot"));
	a.Assign(50);
	Integer c;
	a.Snapshot(c);
	LOG(_T("c = %d"), c);

	LOG(_T("--- Compare and Swap (Success)"));
	Integer d = 55;
	a.CompareAndSwap(50, d);
	LOG(_T("d = %d"), d);
	LOG(_T("a = %d"), (Integer&)a.Pickup());

	LOG(_T("--- Compare and Swap (Fail)"));
	d = 60;
	a.CompareAndSwap(50, d);
	LOG(_T("d = %d"), d);
	LOG(_T("a = %d"), (Integer&)a.Pickup());

	TSyncInteger _b(HANDOFF_CONSTRUCT, new Integer(123));
	LOG(_T("_b = %d"), (Integer&)_b.Pickup());

	LOG(_T("*** Test WorkerThread and SyncObj (Threading correctness)"));
	{
		TestExcept TestRun;
		TWorkerThread TestWT(_T("TestExcept"), TestRun, nullptr);
		TestWT.WaitFor();
	}

	{
		TestCrash TestRun;
		TWorkerThread TestWT(_T("TestCrash"), TestRun, nullptr);
		TestWT.WaitFor();
	}

	TSyncInteger e(0);
	LOG(_T("e = %d"), (Integer&)e.Pickup());
	LOG(_T("--- Start Counting..."));

	TestCount TestCtr;
	TWorkerThread TestWTCount1(_T("CounterThread1"), TestCtr, (PVOID)&e);
	TWorkerThread TestWTCount2(_T("CounterThread2"), TestCtr, (PVOID)&e);
	TWorkerThread TestWTCount3(_T("CounterThread3"), TestCtr, (PVOID)&e);
	TWorkerThread TestWTCount4(_T("CounterThread4"), TestCtr, (PVOID)&e);
	TWorkerThread TestWTCount5(_T("CounterThread5"), TestCtr, (PVOID)&e);
	WaitMultiple({TestWTCount1, TestWTCount2, TestWTCount3, TestWTCount4, TestWTCount5}, true);

	LOG(_T("--- Finished All Counting..."));
	LOG(_T("e = %d"), (Integer&)e.Pickup());
}

void TestSyncQueue(void) {
	LOG(_T("*** Test SyncQueue (Non-threading correctness)"));
	TSyncQueue<int> TestQueue(_T("TestQueue1"));
	TestQueue.Emplace_Enqueue(123);
	int f = 0;
	TestQueue.Dequeue(f);
	LOG(_T("f = %d"), f);
	LOG(_T("--- Wait 2 seconds and fail"));
	if (TestQueue.Dequeue(f, 2000))
		FAIL(_T("Should not reach"))
	else
	LOG(_T("Failed to dequeue (expected)"));

	LOG(_T("*** Test SyncQueue (Threading correctness)"));
	TSyncIntQueue Queue(_T("SyncIntQueue"));
	TestQueuePut TestQPut;
	TWorkerThread TestWTQPut(_T("QueuePutThread"), TestQPut, &Queue);
	TestQueueGet TestQGet;
	TWorkerThread TestWTQGet(_T("QueueGetThread"), TestQGet, &Queue);
	TestWTQGet.WaitFor();
	LOG(_T("--- Finished All Queue Operation..."));
	if (IsDebuggerPresent())
		LOG(_T("--- NOTE: Since you have debugger attached, the performance is NOT accurate"));
}

void TestSyncObjPool(void) {
	LOG(_T("*** Test SyncObjPool (Non-threading correctness)"));
	TSyncIntPool Pool(_T("SyncIntPool"), 16, 8);
	LOG(_T("--- Acquire 5 entries (expect grow)"));
	TSyncIntPool::TSyncPoolObj* PoolObj1 = Pool.Acquire();
	TSyncIntPool::TSyncPoolObj* PoolObj2 = Pool.Acquire();
	TSyncIntPool::TSyncPoolObj* PoolObj3 = Pool.Acquire();
	TSyncIntPool::TSyncPoolObj* PoolObj4 = Pool.Acquire();
	TSyncIntPool::TSyncPoolObj* PoolObj5 = Pool.Acquire();
	LOG(_T("--- Acquire 8 more entries (expect reach grow limit)"));
	TSyncIntPool::TSyncPoolObj* PoolObj6 = Pool.Acquire();
	TSyncIntPool::TSyncPoolObj* PoolObj7 = Pool.Acquire();
	TSyncIntPool::TSyncPoolObj* PoolObj8 = Pool.Acquire();
	TSyncIntPool::TSyncPoolObj* PoolObj9 = Pool.Acquire();
	TSyncIntPool::TSyncPoolObj* PoolObjA = Pool.Acquire();
	TSyncIntPool::TSyncPoolObj* PoolObjB = Pool.Acquire();
	TSyncIntPool::TSyncPoolObj* PoolObjC = Pool.Acquire();
	TSyncIntPool::TSyncPoolObj* PoolObjD = Pool.Acquire();
	LOG(_T("--- Acquire 4 more entries (expect last operation timeout in 2sec)"));
	TSyncIntPool::TSyncPoolObj* PoolObjE = Pool.Acquire(2000);
	TSyncIntPool::TSyncPoolObj* PoolObjF = Pool.Acquire(2000);
	TSyncIntPool::TSyncPoolObj* PoolObjX = Pool.Acquire(2000);
	TSyncIntPool::TSyncPoolObj* PoolObjY = Pool.Acquire(2000);
	LOG(_T("--- Release 12 entries (expect shrink)"));
	PoolObj1->Release();
	PoolObj2->Release();
	PoolObj3->Release();
	PoolObj4->Release();
	PoolObj5->Release();
	PoolObj6->Release();
	PoolObj7->Release();
	PoolObj8->Release();
	PoolObj9->Release();
	PoolObjA->Release();
	PoolObjB->Release();
	PoolObjC->Release();
	LOG(_T("--- Release 4 entries (expect shrink again)"));
	PoolObjD->Release();
	PoolObjE->Release();
	PoolObjF->Release();
	PoolObjX->Release();
	LOG(_T("--- Acquire 5 entries (expect grow)"));
	PoolObj1 = Pool.Acquire();
	PoolObj2 = Pool.Acquire();
	PoolObj3 = Pool.Acquire();
	PoolObj4 = Pool.Acquire();
	PoolObj5 = Pool.Acquire();
	LOG(_T("--- Destroying pool (expect warnings: 5 object unaccounted for)"));
#ifdef _DEBUG
	LOGV(_T("*** Note that since 5 pool object were intentionally lost, 10 blocks of memory leak is expected at program exit."));
#endif

	LOG(_T("*** Test SyncObjPool (Threading correctness)"));
	TSyncIntPool ThreadPool(_T("SyncIntThreadPool"), 256, 64);
	TestSyncPool TestSOP;
	TWorkerThread TestSOPSend(_T("PoolSenderThread"), TestSOP, &ThreadPool);
	TWorkerThread TestSOPRecv(_T("PoolReceiverThread"), TestSOP, &ThreadPool);
	TestSOPRecv.WaitFor();
	LOG(_T("--- Finished All Pool Operation..."));
	LOG(_T("--- Final pool capacity: %d"), ThreadPool.Capacity());
	if (IsDebuggerPresent())
		LOG(_T("--- NOTE: Since you have debugger attached, the performance is NOT accurate"));
}

void TestIdentifiers(void) {
	LOG(_T("*** Test Identifiers"));
	LOG(_T("RootIdent: %s"), RootIdent().toString().c_str());
	if (RootIdent != RootIdent)
		FAIL(_T("RootIdent not equal to itself"));

	INameIdent& A = GetNameIdent(_T("A"));
	LOG(_T("NameIdent A: %s"), A.toString().c_str());
	if (A == RootIdent())
		FAIL(_T("RootIdent equal to other"));

	INameIdent& B = GetNameIdent(_T("B"));
	LOG(_T("NameIdent B: %s"), B.toString().c_str());
	if (&A == &B)
		FAIL(_T("One NameIdent found for A and B"));
	if (A == B)
		FAIL(_T("NameIdent A equal to B"));

	INameIdent& _A = GetNameIdent(_T("A"));
	LOG(_T("NameIdent _A = A: %s"), _A.toString().c_str());
	if (&A != &_A)
		FAIL(_T("Two NameIdent A found"));
	if (A != _A)
		FAIL(_T("NameIdent A not equal to itself"));

	ICtxNameIdent& A2 = GetCtxNameIdent(_T("A"), A);
	LOG(_T("NameIdent A2 = A.A: %s"), A2.toString().c_str());
	if (A == A2)
		FAIL(_T("NameIdent A equal to A.A"));

	ICtxNameIdent& B2 = GetCtxNameIdent(_T("B"), A);
	LOG(_T("NameIdent B2 = A.B: %s"), B2.toString().c_str());
	if (B == B2)
		FAIL(_T("NameIdent B equal to A.B"));

	ICtxNameIdent& A3 = GetCtxNameIdent(_T("A"), B);
	LOG(_T("NameIdent A3 = B.A: %s"), A3.toString().c_str());
	if (A == A3)
		FAIL(_T("NameIdent A equal to B.A"));

	ICtxNameIdent& B3 = GetCtxNameIdent(_T("B"), B);
	LOG(_T("NameIdent B3 = B.B: %s"), B3.toString().c_str());
	if (B == B3)
		FAIL(_T("NameIdent B equal to B.B"));

	ICtxNameIdent& A4 = GetCtxNameIdent(_T("A"));
	LOG(_T("NameIdent A4 = !.A: %s"), A4.toString().c_str());
	if (A == A4)
		FAIL(_T("NameIdent A equal to !.A"));

	INameIdent& _A4 = GetCtxNameIdent(_T("A"));
	LOG(_T("NameIdent _A4 = A4: %s"), _A4.toString().c_str());
	if (&A4 != &_A4)
		FAIL(_T("NameIdent !.A not equal to itself"));

	ICtxNameIdent& B4 = GetCtxNameIdent(_T("B"));
	LOG(_T("NameIdent B4 = !.B: %s"), B4.toString().c_str());
	if (B == B4)
		FAIL(_T("NameIdent B equal to !.B"));
	if (A4 == B4)
		FAIL(_T("NameIdent !.A equal to !.B"));

	class IUnmanagedNameIdent : public virtual INameIdent {
	protected:
		IUnmanagedNameIdent(void) {}
	public:
		IUnmanagedNameIdent(TString const& xName)
		{ INameIdent::_Init(xName); }
	};

	IUnmanagedNameIdent* X = new IUnmanagedNameIdent(_T("X"));
	LOG(_T("NameIdent X = X: %s"), X->toString().c_str());
	try {
		ICtxNameIdent& X1 = GetCtxNameIdent(_T("1"), *X);
		LOG(_T("NameIdent X1 = X.1: %s"), X1.toString().c_str());
	} catch (Exception *e) {
		e->Show();
		delete e;
		delete X;
	}

	INameIdent* Q = CopyCloneableAdapter<IUnmanagedNameIdent>::Create(_T("Q"));
	ICtxNameIdent& Q1 = GetCtxNameIdent(_T("1"), *Q);
	LOG(_T("NameIdent Q1 = Q.1: %s"), Q1.toString().c_str());
	delete Q;

	class IUnmanagedCNameIdent : public virtual INameIdent, public Cloneable {
	protected:
		IUnmanagedCNameIdent(void) {}
	public:
		IUnmanagedCNameIdent(TString const& xName)
		{ INameIdent::_Init(xName); }

		Cloneable* Clone(void) const {
			return new IUnmanagedCNameIdent(*this);
		}
	};

	IUnmanagedCNameIdent* Y = new IUnmanagedCNameIdent(_T("Y"));
	LOG(_T("NameIdent Y = Y: %s"), Y->toString().c_str());
	ICtxNameIdent& Y1 = GetCtxNameIdent(_T("1"), *Y);
	LOG(_T("NameIdent Y1 = Y.1: %s"), Y1.toString().c_str());
	delete Y;

	ICtxNameIdent& Z1 = GetCtxNameIdent(_T("1"), *ManagedObjAdapter<IUnmanagedCNameIdent>::Create(_T("Z")));
	LOG(_T("NameIdent Z1 = Z.1: %s"), Z1.toString().c_str());
}

void TestStringConv(void) {
	setlocale(LC_ALL, "chs");
	LOG(_T("*** Test String Conversions"));
	LOG(_T("Converting: 'This is a test...'"));
	TString Test1 = _T("This is a test...");
	std::string Test1C = TStringtoUTF8(Test1);
	TString Test1R = UTF8toTString(Test1C);
	if (Test1.compare(Test1R) != 0)
		FAIL(_T("String failed to round-trip!"));

	LOG(_T("Converting: '这是一个测试...'"));
	TString Test2 = _T("这是一个测试...");
	std::string Test2C = TStringtoUTF8(Test2);
	TString Test2R = UTF8toTString(Test2C);
	if (Test2.compare(Test2R) != 0)
		FAIL(_T("String failed to round-trip!"));
}

void TestThrottler(void) {
	LOG(_T("*** Test Thread Throttler (limit @ 1.6x CPU)"));
	ThreadThrottler T(_T("ThreadThrottler"), 1600, 1000, 10);
	BusyRunnable BusyRunner;
	TWorkerThread BZ1(_T("BusyThread1"), BusyRunner, (PVOID)7);
	TWorkerThread BZ2(_T("BusyThread2"), BusyRunner, (PVOID)11);
	TWorkerThread BZ3(_T("BusyThread3"), BusyRunner, (PVOID)17);
	TWorkerThread BZ4(_T("BusyThread4"), BusyRunner, (PVOID)23);
	BZ1.Throttler(&T);
	BZ2.Throttler(&T);
	BZ3.Throttler(&T);
	BZ4.Throttler(&T);
	LOG(_T("Enter to start throttling..."));
	system("pause");
	TEvent ThrottlerSignal;
	TWorkerThread TT(_T("ThrottlerThread"), T, &ThrottlerSignal);
	LOG(_T("Enter to stop throttling..."));
	system("pause");
	ThrottlerSignal.Set();
	TT.SignalTerminate();
	LOG(_T("Enter to stop all..."));
	system("pause");
	BZ1.SignalTerminate();
	BZ2.SignalTerminate();
	BZ3.SignalTerminate();
	BZ4.SignalTerminate();
}

int _tmain(int argc, LPCTSTR argv[], LPCTSTR envp[]) {
	LOG(_T("%s"), __REL_FILE__);
	try {
		if (argc != 2)
			FAIL(_T("Require 1 parameter: <TestType> = 'ALL' | 'Exception' / 'ErrCode' / 'SyncObj' / 'SyncQueue' / 'SyncObjPool' / 'Identifiers' / 'StringConv' / 'Throttler'"));

		bool TestAll = _tcsicmp(argv[1], _T("ALL")) == 0;
		if (TestAll || (_tcsicmp(argv[1], _T("Exception")) == 0)) {
			TestException();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("ErrCode")) == 0)) {
			TestErrCode();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("SyncObj")) == 0)) {
			TestSyncObj();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("SyncQueue")) == 0)) {
			TestSyncQueue();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("SyncObjPool")) == 0)) {
			TestSyncObjPool();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("Identifiers")) == 0)) {
			TestIdentifiers();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("StringConv")) == 0)) {
			TestStringConv();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("Throttler")) == 0)) {
			TestThrottler();
		}
	} catch (Exception *e) {
		e->Show();
		delete e;
	}

#ifdef _DEBUG
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF);
#else
#ifdef CUSMM
	// Some memory workload to test custom memory manager
	PVOID x[32768];
	for (int i = 0; i < ARRAYSIZE(x); i++)
		x[i] = malloc(1111);
	PVOID y = malloc(16384);
	PVOID z = malloc(1638400);
	for (int i = 0; i < ARRAYSIZE(x); i++)
		free(x[i]);
	LOG(_T("* %llu"), (UINT64)_msize(y));
	free(y);
	LOG(_T("* %llu"), (UINT64)_msize(z));
	free(z);
#ifdef NEDMM
	size_t MemAlloc, MemFree;
	_ned_stat(&MemAlloc, &MemFree);
	LOG(_T("* Memory allocation %.2fMB, free %.2fMB"), (double)MemAlloc / BSize_aMB, (double)MemFree / BSize_aMB);
#endif
#ifdef FASTMM
	size_t AllocSize, OverheadSize;
	_fast_summary(&AllocSize, &OverheadSize);
	LOG(_T("* Memory allocation %.2fMB, overhead %.2fMB"), (double)AllocSize / BSize_aMB, (double)OverheadSize / BSize_aMB);

	TMemoryManagerState MMDetails;
	_fast_details(&MMDetails);
	_fast_printdetails(&MMDetails);

	// Leak!
	y = malloc(16384);
	// Leak!
	z = malloc(1638400);

	TMemoryManagerState NewMMDetails;
	_fast_details(&NewMMDetails);
	_fast_printdetails(&NewMMDetails);
	_fast_comparedetails(&MMDetails, &NewMMDetails);
#endif
#endif
#endif

	if (IsDebuggerPresent())
		system("pause");

	return 0;
}
