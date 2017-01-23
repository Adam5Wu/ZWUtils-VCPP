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

// [Threading] Generic Worker Thread

#include "BaseLib/MMSwitcher.h"

#include <unordered_map>

#include "BaseLib/Exception.h"
#include "WorkerThread.h"
#include "ThreadThrottler.h"
#include "StackWalker.h"

using namespace std;

#define WTLogTag _T("W.Thread '%s'")
#define WTLogHeader _T("{") WTLogTag _T("} ")

struct WorkerThreadForwardRec {
	TWorkerThread *WorkerThread;
	DWORD(TWorkerThread::*Run)(void);
	BOOL &SelfFree;
};

TSyncObj<unordered_map<TString, TWorkerThreadLiveCycle>> SyncWorkerThreadStartups;
TSyncObj<unordered_map<TString, TWorkerThreadLiveCycle>> SyncWorkerThreadShutdowns;

static void __WorkerThread_Startup(TWorkerThread &WT) {
	auto WorkerThreadStartups(SyncWorkerThreadStartups.Pickup());
	for (auto &entry : *WorkerThreadStartups) {
		LOGVV(WTLogHeader _T("Startup event '%s'..."), WT.Name.c_str(), entry.first.c_str());
		entry.second(WT, true);
	}
}

static void __WorkerThread_Shutdown(TWorkerThread &WT) {
	auto WorkerThreadShutdowns(SyncWorkerThreadShutdowns.Pickup());
	for (auto &entry : *WorkerThreadShutdowns) {
		LOGVV(WTLogHeader _T("Shutdown event '%s'..."), WT.Name.c_str(), entry.first.c_str());
		entry.second(WT, false);
	}
}

static DWORD WINAPI ThreadProc(LPVOID ForwardRec) {
	WorkerThreadForwardRec WTR = *(WorkerThreadForwardRec*)ForwardRec;
	delete ForwardRec;

	__WorkerThread_Startup(*WTR.WorkerThread);
	__try {
		return (WTR.WorkerThread->*WTR.Run)();
	} __finally {
		__WorkerThread_Shutdown(*WTR.WorkerThread);
		// Check and perform self-free if requested
		if (WTR.SelfFree)
			delete WTR.WorkerThread;
	}
}

void WorkerThread_StartupEvent_Register(LPCTSTR Name, TWorkerThreadLiveCycle const& Func) {
	auto WorkerThreadStartups(SyncWorkerThreadStartups.Pickup());
	if (!WorkerThreadStartups->emplace(Name, Func).second) {
		FAIL(_T("WorkerThread startup event '%s' already registered!"), Name);
	} else {
		LOGVV(_T("Registered WorkerThread startup event '%s'"), Name);
	}
}

void WorkerThread_StartupEvent_Unregister(LPCTSTR Name) {
	auto WorkerThreadStartups(SyncWorkerThreadStartups.Pickup());
	if (WorkerThreadStartups->erase(Name) == 0) {
		LOG(_T("WARNING: WorkerThread startup event '%s' not registered!"), Name);
	} else {
		LOGVV(_T("Unregistered WorkerThread startup event '%s'"), Name);
	}
}

void WorkerThread_ShutdownEvent_Register(LPCTSTR Name, TWorkerThreadLiveCycle const& Func) {
	auto WorkerThreadShutdowns(SyncWorkerThreadShutdowns.Pickup());
	if (!WorkerThreadShutdowns->emplace(Name, Func).second) {
		FAIL(_T("WorkerThread shutdown event '%s' already registered!"), Name);
	} else {
		LOGVV(_T("Registered WorkerThread shutdown event '%s'"), Name);
	}
}

void WorkerThread_ShutdownEvent_Unregister(LPCTSTR Name) {
	auto WorkerThreadShutdowns(SyncWorkerThreadShutdowns.Pickup());
	if (WorkerThreadShutdowns->erase(Name) == 0) {
		LOG(_T("WARNING: WorkerThread shutdown event '%s' not registered!"), Name);
	} else {
		LOGVV(_T("Unregistered WorkerThread shutdown event '%s'"), Name);
	}
}

//! @ingroup Threading
//! Raise an exception within a worker thread with formatted string message
#define WTFAIL(...)																	\
{																					\
	SOURCEMARK;																		\
	throw TWorkerThreadException::Create(*this, std::move(__SrcMark), __VA_ARGS__);	\
}

//! Perform logging within a worker thread
#define WTLOG(s, ...) LOG(WTLogHeader s, Name.c_str(), __VA_ARGS__)
#define WTLOGV(s, ...) LOGV(WTLogHeader s, Name.c_str(), __VA_ARGS__)
#define WTLOGVV(s, ...) LOGVV(WTLogHeader s, Name.c_str(), __VA_ARGS__)

LPCTSTR TWorkerThread::STR_State(State const &xState) {
	static LPCTSTR _STR_State[] = {
		_T("Initialzing"),
		_T("Running"),
		_T("Terminating"),
		_T("Terminated"),
	};
	return _STR_State[(int)xState];
}

TWorkerThread::~TWorkerThread(void) {
	DWORD ThisThreadId = GetCurrentThreadId();
	if (ThisThreadId == ThreadID) {
		if (CurrentState() != State::Terminated) {
			rSelfFree = true;
			WTFAIL(_T("Running worker thread self-destruction, marking as self-free..."));
		}
		WTLOGV(_T("Self destruction in progress..."));
	} else {
		WTLOGVV(_T("Destruction in progress..."));
		State PrevState = SignalTerminate();
		if ((PrevState == State::Running) || (PrevState == State::Terminating)) {
			WTLOGV(_T("WARNING: Waiting for worker termination..."));
			WaitFor();
		}
		// Leave throttler group
		Throttler(nullptr);
	}

	// Free input data resource
	rRunnable.DiscardInput(rInputData);
	// Free caught exception object
	rRunnable.DiscardReturn(rReturnData);
	// Free caught exception object
	if (rException != nullptr)
		delete rException;
}

void TWorkerThread::__ThreadStart(SIZE_T StackSize) {
	auto ForwardRec = new WorkerThreadForwardRec{this, &TWorkerThread::__CallForwarder_Outer, rSelfFree};
	rThread = CreateThread(nullptr, StackSize, &ThreadProc, ForwardRec, 0, const_cast<LPDWORD>(&ThreadID));
	if (rThread == nullptr) {
		DWORD ErrCode = GetLastError();
		delete ForwardRec;
		SYSERRFAIL(ErrCode, _T("Failed to create thread for worker '%s'"), Name.c_str());
	}
}

class WTStackWalker : public StackWalker {
public:
	TString const& Name;
	WTStackWalker(TWorkerThread const* WT) : StackWalker(), Name(WT->Name) {}
	virtual void OnOutput(LPCSTR szText) override {
		TString TraceStr = UTF8toTString(szText);
		if (TraceStr.length() > 1) {
			TraceStr.pop_back();
			WTLOGV(_T("%s"), TraceStr.c_str());
		}
	}
};

int TWorkerThread::CollectSEHException(struct _EXCEPTION_POINTERS *SEH) {
	static const LPCTSTR UnhandledSEHException = _T("Terminated due to unhandled SEH Exception");

	DWORD ExceptionCode = SEH->ExceptionRecord->ExceptionCode;
	WTLOG(_T("WARNING: Abnormal termination due to unhanded SEH Exception (0x%0.8X)"), ExceptionCode);
	rException = new TWorkerThreadSEHException(ExceptionCode, *this, nullptr, UnhandledSEHException);

	WTLOG(_T("Stacktrace:"));
	WTStackWalker SW(this);
	SW.ShowCallstack(GetCurrentThread(), SEH->ContextRecord);

	return EXCEPTION_EXECUTE_HANDLER;
}

DWORD TWorkerThread::__CallForwarder_Outer(void) {
	DWORD Ret = 0;
	auto iCurState = (State)rState.CompareAndSwap((__ARC_INT)State::Initialzing, (__ARC_INT)State::Running);
	if (iCurState == State::Initialzing) {
		__try {
			__CallForwarder_Inner();
		} __except (CollectSEHException(GetExceptionInformation())) {
			Ret = GetExceptionCode();
		}
	} else
		WTLOGV(_T("WARNING: Status changed to '%s' before running start"), STR_State(iCurState));

	rState = (__ARC_INT)State::Terminated;
	return Ret;
}

void TWorkerThread::__CallForwarder_Inner(void) {
	WTLOGV(_T("Started"));
	try {
		rReturnData = rRunnable.Run(*this, rInputData);
	} catch (Exception *e) {
		WTLOGV(_T("WARNING: Abnormal termination due to unhanded ZWUtils Exception - %s"), e->Why());
		rException = e;
	}
	WTLOGV(_T("Finished"));
}

void TWorkerThread::SelfFree(void) {
	auto iThrottler(rThrottler.Pickup());
	if (*iThrottler)
		WTFAIL(_T("Cannot set self-free when joined a throttler group"));

	WTLOGV(_T("Marking thread as self-free"));
	rSelfFree = true;
}

void TWorkerThread::Throttler(ThreadThrottler *xThrottler) {
	auto iThrottler(rThrottler.Pickup());
	if (rSelfFree)
		WTFAIL(_T("Cannot join a throttler group when marked as self-free"));

	if (*iThrottler) {
		(*iThrottler)->UnregisterWorker(this);
		*iThrottler = nullptr;
	}
	if (xThrottler) {
		xThrottler->RegisterWorker(this);
		*iThrottler = xThrottler;
	}
}

TWorkerThread::State TWorkerThread::SignalTerminate(void) {
	auto iCurState = rState.CompareAndSwap((__ARC_INT)State::Initialzing, (__ARC_INT)State::Terminating);
	if (iCurState != (__ARC_INT)State::Initialzing) {
		iCurState = rState.CompareAndSwap((__ARC_INT)State::Running, (__ARC_INT)State::Terminating);
		rRunnable.StopNotify();

		auto iThrottler(rThrottler.Pickup());
		if (*iThrottler) {
			// Nudge the throttler so that the request get acknowledged faster (best effort)
			if ((*iThrottler)->CurrentState() == ThreadThrottler::State::Throttling)
				(*iThrottler)->Bump();
		}
	}
	return (State)iCurState;
}

TWorkerThread::State TWorkerThread::CurrentState(void) {
	return (State)~rState;
}

void* TWorkerThread::getReturnData(void) {
	State iCurState = (State)~rState;
	if (iCurState != State::Terminated)
		WTFAIL(_T("Could not get return data while thread is in state '%s'"), STR_State(iCurState));
	return rReturnData;
}

Exception* TWorkerThread::getException(void) {
	State iCurState = (State)~rState;
	if (iCurState != State::Terminated)
		WTFAIL(_T("Could not get return data while thread is in state '%s'"), STR_State(iCurState));
	return rException;
}

HANDLE TWorkerThread::CreateWaitHandle(void) const {
	HANDLE WaitHandle;
	if (DuplicateHandle(GetCurrentProcess(), rThread, GetCurrentProcess(), &WaitHandle, SYNCHRONIZE, FALSE, 0) == 0)
		SYSFAIL(_T("Failed to duplicate thread handle"));
	return WaitHandle;
}


LPCTSTR TWorkerThreadException::Why(void) const {
	if (rWhy.length() == 0) {
		TString tWhy = Exception::Why();
		const_cast<TString*>(&rWhy)->resize(__DefErrorMsgBufferLen, NullWChar);
		int MsgLen = _sntprintf_s((TCHAR*)&rWhy.front(), __DefErrorMsgBufferLen, _TRUNCATE, WTLogHeader _T("%s"), WorkerThreadName.c_str(), tWhy.c_str());
		if (MsgLen >= 0)
			const_cast<TString*>(&rWhy)->resize(MsgLen);
	}
	return rWhy.c_str();
}

LPCTSTR TWorkerThreadSEHException::Why(void) const {
	if (rWhy.length() == 0) {
		TString tWhy = TWorkerThreadException::Why();
		const_cast<TString*>(&rWhy)->resize(__DefErrorMsgBufferLen, NullWChar);
		int MsgLen = _sntprintf_s((TCHAR*)&rWhy.front(), __DefErrorMsgBufferLen, _TRUNCATE, _T("%s [SEH %0.8X]"), tWhy.c_str(), ExceptionCode);
		if (MsgLen >= 0)
			const_cast<TString*>(&rWhy)->resize(MsgLen);
	}
	return rWhy.c_str();
}

#undef WTFAIL
#undef WTLOG
#undef WTLOGV
#undef WTLOGVV
