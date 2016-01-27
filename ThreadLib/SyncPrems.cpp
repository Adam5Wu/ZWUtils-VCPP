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

// [Threading] Synchronization Premitive Classes

#include "BaseLib/MMSwitcher.h"

#include "SyncPrems.h"

#include "BaseLib/DebugLog.h"
#include "BaseLib/WinError.h"

using namespace std;

inline WaitResult __FilterWaitResult(DWORD Ret, size_t ObjCnt, bool WaitAll) {
	if ((Ret >= WAIT_OBJECT_0) && (Ret < WAIT_OBJECT_0 + ObjCnt)) {
		// High frequency event
		//LOGVV(_T("NOTE: Wait Succeed"));
		return WaitAll ? WaitResult::Signaled : (WaitResult)(WaitResult::Signaled_0 + Ret - WAIT_OBJECT_0);
	} if (Ret == WAIT_OBJECT_0 + ObjCnt) {
		// High frequency event
		//LOGVV(_T("NOTE: Wait Msg"));
		return WaitResult::Message;
	} else if ((Ret >= WAIT_ABANDONED_0) && (Ret < WAIT_ABANDONED_0 + ObjCnt)) {
		LOGVV(_T("WARNING: Wait Succeed (with abandoned mutex)"));
		return WaitAll ? WaitResult::Abandoned : (WaitResult)(WaitResult::Abandoned_0 + Ret - WAIT_ABANDONED_0);
	} if (Ret == WAIT_IO_COMPLETION) {
		// High frequency event
		//LOGVV(_T("NOTE: Wait APC"));
		return WaitResult::APC;
	} else if (Ret == WAIT_TIMEOUT) {
		// High frequency event
		//LOGVV(_T("NOTE: Wait timed out"));
		return WaitResult::TimedOut;
	} else if (Ret == WAIT_FAILED) {
		DEBUGV({
			DWORD ErrCode = GetLastError();
			TCHAR ErrMsg[__DefErrorMsgBufferLen];
			DecodeError(ErrMsg, __DefErrorMsgBufferLen, ErrCode);
			LOGV(_T("WARNING: Wait failed - %s"), ErrMsg);
			SetLastError(ErrCode);
		});
		return WaitResult::Error;
	} else {
		LOG(_T("WARNING: Wait possibly failed - Unrecognized return value (%d)"), Ret);
		return WaitResult::Error;
	}
}

WaitResult MsgWaitMultiple(vector<HANDLE> const &WaitHandles, bool WaitAll, DWORD WakeMask, DWORD Timeout, bool WaitAPC) {
	size_t ObjCnt = WaitHandles.size();
	DWORD WaitFlag = (WaitAll ? MWMO_WAITALL : 0) | (WaitAPC ? MWMO_ALERTABLE : 0) | (WakeMask != 0 ? MWMO_INPUTAVAILABLE : 0);
	DWORD Ret = MsgWaitForMultipleObjectsEx((DWORD)ObjCnt, &WaitHandles.front(), Timeout, WakeMask, WaitFlag);
	return __FilterWaitResult(Ret, ObjCnt, WaitAll);
}

WaitResult WaitMultiple(vector<HANDLE> const &WaitHandles, bool WaitAll, DWORD Timeout, bool WaitAPC, bool WaitMsg) {
	if (WaitMsg) {
		return MsgWaitMultiple(WaitHandles, WaitAll, QS_ALLEVENTS, Timeout, WaitAPC);
	} else {
		size_t ObjCnt = WaitHandles.size();
		DWORD Ret = WaitForMultipleObjectsEx((DWORD)ObjCnt, &WaitHandles.front(), WaitAll, Timeout, WaitAPC);
		return __FilterWaitResult(Ret, ObjCnt, WaitAll);
	}
}

class TWaitHandles : public vector < HANDLE > {
protected:
	void DiscardHandles(void) {
		for (size_type i = 0; i < size(); i++)
			if (CloseHandle(at(i)) == 0)
				LOGSYSERR(_T("WARNING: Failed to close wait handle"));
	}
public:
	~TWaitHandles(void) {
		DiscardHandles();
	}
};

WaitResult WaitMultiple(vector<reference_wrapper<TWaitable const>> const&Waitables, bool WaitAll, DWORD Timeout, bool WaitAPC, bool WaitMsg) {
	TWaitHandles WaitHandles;
	for (TWaitable const &Waitable : Waitables)
		WaitHandles.push_back(Waitable.CreateWaitHandle());
	return WaitMultiple(WaitHandles, WaitAll, Timeout, WaitAPC, WaitMsg);
}

WaitResult WaitSingle(HANDLE WaitHandle, DWORD Timeout, bool WaitAPC, bool WaitMsg) {
	if (WaitMsg) {
		vector<HANDLE> WaitHandles;
		WaitHandles.push_back(WaitHandle);
		return WaitMultiple(WaitHandles, true, Timeout, WaitAPC, WaitMsg);
	} else {
		DWORD Ret = WaitForSingleObjectEx(WaitHandle, Timeout, WaitAPC);
		return __FilterWaitResult(Ret, 1, true);
	}
}

WaitResult WaitSingle(const TWaitable &Waitable, DWORD Timeout, bool WaitAPC, bool WaitMsg) {
	TWaitHandles WaitHandles;
	WaitHandles.push_back(Waitable.CreateWaitHandle());
	return WaitSingle(WaitHandles[0], Timeout, WaitAPC, WaitMsg);
}

// TSemaphore
TSemaphore::TSemaphore(LONG Initial, LONG Maximum, LPCTSTR Name) {
	rSemaphore = CreateSemaphore(nullptr, Initial, Maximum, Name);
	if (rSemaphore == nullptr)
		SYSFAIL(_T("Failed to create semaphore"));
	if (Name != nullptr) {
		DWORD ErrCode = GetLastError();
		if (ErrCode == ERROR_ALREADY_EXISTS) {
			LOGV(_T("WARNING: Named semaphore '%s' already exists!"), Name);
		}
	}
}

TSemaphore::~TSemaphore(void) {
	if (CloseHandle(rSemaphore) == 0)
		LOGSYSERR(_T("WARNING: Failed to close semaphore"));
}

LONG TSemaphore::Signal(LONG Count) {
	LONG PrevCnt;
	if (ReleaseSemaphore(rSemaphore, Count, &PrevCnt) == 0)
		SYSFAIL(_T("Failed to signal semaphore"));
	return PrevCnt;
}

HANDLE TSemaphore::CreateWaitHandle(void) const {
	HANDLE WaitHandle;
	if (DuplicateHandle(GetCurrentProcess(), rSemaphore, GetCurrentProcess(), &WaitHandle, SYNCHRONIZE, FALSE, 0) == 0)
		SYSFAIL(_T("Failed to duplicate semaphore handle"));
	return WaitHandle;
}

// TMutex
TMutex::TMutex(bool Acquired, LPCTSTR Name) {
	rMutex = CreateMutex(nullptr, Acquired, Name);
	if (rMutex == nullptr)
		SYSFAIL(_T("Failed to create mutex"));
	if (Name != nullptr) {
		DWORD ErrCode = GetLastError();
		if (ErrCode == ERROR_ALREADY_EXISTS) {
			LOGV(_T("WARNING: Named mutex '%s' already exists!"), Name);
		}
	}
}

TMutex::~TMutex(void) {
	if (CloseHandle(rMutex) == 0)
		LOGSYSERR(_T("WARNING: Failed to close mutex"));
}

void TMutex::Release(void) {
	if (ReleaseMutex(rMutex) == 0)
		SYSFAIL(_T("Failed to release mutex"));
}

HANDLE TMutex::CreateWaitHandle(void) const {
	HANDLE WaitHandle;
	if (DuplicateHandle(GetCurrentProcess(), rMutex, GetCurrentProcess(), &WaitHandle, SYNCHRONIZE, FALSE, 0) == 0)
		SYSFAIL(_T("Failed to duplicate mutex handle"));
	return WaitHandle;
}

// TEvent
TEvent::TEvent(bool ManualReset, bool Initial, LPCTSTR Name) {
	rEvent = CreateEvent(nullptr, ManualReset, Initial, Name);
	if (rEvent == nullptr)
		SYSFAIL(_T("Failed to create event"));
	if (Name != nullptr) {
		DWORD ErrCode = GetLastError();
		if (ErrCode == ERROR_ALREADY_EXISTS) {
			LOGV(_T("WARNING: Named event '%s' already exists!"), Name);
		}
	}
}

TEvent::~TEvent(void) {
	if (CloseHandle(rEvent) == 0)
		LOGSYSERR(_T("WARNING: Failed to close event"));
}

void TEvent::Set(void) {
	if (SetEvent(rEvent) == 0)
		SYSFAIL(_T("Failed to set event"));
}

void TEvent::Reset(void) {
	if (ResetEvent(rEvent) == 0)
		SYSFAIL(_T("Failed to reset event"));
}

void TEvent::Pulse(void) {
	if (PulseEvent(rEvent) == 0)
		SYSFAIL(_T("Failed to pulse event"));
}

HANDLE TEvent::CreateWaitHandle(void) const {
	HANDLE WaitHandle;
	if (DuplicateHandle(GetCurrentProcess(), rEvent, GetCurrentProcess(), &WaitHandle, SYNCHRONIZE, FALSE, 0) == 0)
		SYSFAIL(_T("Failed to duplicate event handle"));
	return WaitHandle;
}

// TCriticalSection
TCriticalSection::TCriticalSection(bool Entered, DWORD SpinCount) {
	InitializeCriticalSectionAndSpinCount(&rCriticalSection, SpinCount);
	DEBUGVV(OwnerThreadID = 0);
	if (Entered)
		Enter();
}

TCriticalSection::~TCriticalSection(void) {
	DEBUG(if (!TryEnter()) LOG(_T("WARNING: Freeing an acquired critical section!")));
	UNDEBUG(Enter(void));
	DeleteCriticalSection(&rCriticalSection);
}

void TCriticalSection::Enter(void) {
	EnterCriticalSection(&rCriticalSection);
	DEBUGVV(OwnerThreadID = GetCurrentThreadId());
}

bool TCriticalSection::TryEnter(void) {
	BOOL Acquired = TryEnterCriticalSection(&rCriticalSection);
	DEBUGVV(if (Acquired != 0) OwnerThreadID = GetCurrentThreadId());
	return Acquired != 0;
}

void TCriticalSection::Leave(void) {
	DEBUGVV(OwnerThreadID = 0);
	LeaveCriticalSection(&rCriticalSection);
}
