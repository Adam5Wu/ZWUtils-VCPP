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

// [Threading] Thread Throttling Support
// Zhenyu Wu @ Nov 13, 2005: Initial implementation

#include "BaseLib/MMSwitcher.h"

#include "ThreadThrottler.h"
#include <thread>

#define TTLogTag _T("T.Throttler '%s'")
#define TTLogHeader _T("{") TTLogTag _T("} ")

//! @ingroup Threading
//! Raise an exception within a thread throttler with formatted string message
#define TTFAIL(...)																	\
{																					\
	SOURCEMARK;																		\
	throw new ThreadThrottlerException(*this, std::move(__SrcMark), __VA_ARGS__);	\
}

//! Perform logging within a synchronized queue
#define TTLOG(s, ...) LOG(TTLogHeader s, Name.c_str(), __VA_ARGS__)
#define TTLOGV(s, ...) LOGV(TTLogHeader s, Name.c_str(), __VA_ARGS__)
#define TTLOGVV(s, ...) LOGVV(TTLogHeader s, Name.c_str(), __VA_ARGS__)

DWORD const ThreadThrottler::MIntervalMin = 100;
DWORD const ThreadThrottler::MIntervalMax = 30000;
DWORD const ThreadThrottler::TIntervalMin = 10;
DWORD const ThreadThrottler::TIntervalMax = 100;
DWORD const ThreadThrottler::MTRatioSmall = 10;
DWORD const ThreadThrottler::MTRatioLarge = 100;
DWORD const ThreadThrottler::MTRatioMin = 5;
DWORD const ThreadThrottler::MTRatioMax = 500;
DWORD const ThreadThrottler::MAXLOAD = std::thread::hardware_concurrency() * MSTime_aSecond;

ThreadThrottler::ThreadThrottler(TString const &xName, DWORD xLoadLimit, DWORD xMInterval, DWORD xTInterval) :
Name(xName), LoadLimit(xLoadLimit), MInterval(xMInterval), TInterval(xTInterval),
rState(State::Monitoring), ThrottleCount(0), ThrottleTime(0), WaitEvent(true),
CurLoad(0), SampleCount(xMInterval / xTInterval), SamplePtr(0), Samples(SampleCount) {
	if ((xMInterval < MIntervalMin) || (xMInterval > MIntervalMax))
		TTFAIL(_T("Invliad monitoring interval %d (must be between %d and %d)"), xMInterval, MIntervalMin, MIntervalMax);
	if ((xTInterval < TIntervalMin) || (xTInterval > TIntervalMax))
		TTFAIL(_T("Invliad throttling interval %d (must be between %d and %d)"), xTInterval, TIntervalMin, TIntervalMax);
	if (SampleCount < MTRatioMin)
		TTFAIL(_T("Invliad throttling interval %d (must be smaller than %d)"), xTInterval, xMInterval / MTRatioMin);
	if (SampleCount > MTRatioMax)
		TTFAIL(_T("Invliad throttling interval %d (must be larger than %d)"), xTInterval, xMInterval / MTRatioMax);
	if (SampleCount < MTRatioSmall)
		TTFAIL(_T("WARNING: Throttling interval %d larger than recommended value %d"), xTInterval, xMInterval / MTRatioSmall);
	if (SampleCount > MTRatioLarge)
		TTFAIL(_T("WARNING: Throttling interval %d smaller than recommended value %d"), xTInterval, xMInterval / MTRatioLarge);
	if (xLoadLimit >= MAXLOAD)
		TTLOG(_T("WARNING: Load limit %d is higher than the maximum hardware concurrency (%d)"), xLoadLimit, MAXLOAD);
}

LPCTSTR ThreadThrottler::STR_State(State const &xState) {
	static LPCTSTR const _STR_State[] = {
		_T("Monitoring"),
		_T("Activated"),
		_T("Throttling"),
	};
	return _STR_State[(int)xState];
}

void ThreadThrottler::RegisterWorker(TWorkerThread *Worker) {
	HANDLE WThread = Worker->rThread;
	Synchronized(RunLock, {
		for (size_t i = 0; i < Threads.size(); i++) {
			HANDLE TThread = Threads[i].first->rThread;
			if (TThread == WThread)
				FAIL(_T("{Throttler %s} WorkerThread '%s' already registered"), Name.c_str(), Worker->Name.c_str());
		}

		Flatten_FILETIME TTimes[4];
		GetThreadTimes(WThread, &TTimes[0].FileTime, &TTimes[1].FileTime, &TTimes[2].FileTime, &TTimes[3].FileTime);
		Threads.emplace_back(Worker, TTimes[2].U64 + TTimes[3].U64);
	});
	LOGV(_T("{Throttler %s} Registered WorkerThread '%s'"), Name.c_str(), Worker->Name.c_str());
}

void ThreadThrottler::UnregisterWorker(TWorkerThread *Worker) {
	HANDLE WThread = Worker->rThread;
	Synchronized(RunLock, {
		for (size_t i = 0; i < Threads.size(); i++) {
			HANDLE TThread = Threads[i].first->rThread;
			if (TThread == WThread) {
				Threads.erase(Threads.cbegin() + i);
				LOGV(_T("{Throttler %s} Unregistered WorkerThread '%s'"), Name.c_str(), Worker->Name.c_str());
				if (rState == State::Throttling)
					ResumeThread(WThread);
				Worker = nullptr;
				break;
			}
		}
	});
	if (Worker) {
		LOGV(_T("{Throttler %s} WARNING: WorkerThread '%s' not registered"), Name.c_str(), Worker->Name.c_str());
	}
}

void ThreadThrottler::Bump(void) {
	WaitEvent.Pulse();
}

double ThreadThrottler::GetLoadFactor(void) const {
	Synchronized((*const_cast<TLockableCS*>(&RunLock)), {
		return LoadLimit ? (double)CurLoad / LoadLimit : INFINITY;
	});
}

ThreadThrottler::State ThreadThrottler::CurrentState(void) const {
	Synchronized((*const_cast<TLockableCS*>(&RunLock)), {
		return rState;
	});
}

UINT64 ThreadThrottler::GetThrottleCount(void) const {
	Synchronized((*const_cast<TLockableCS*>(&RunLock)), {
		return ThrottleCount;
	});
}

UINT64 ThreadThrottler::GetThrottleTime(void) const {
	Synchronized((*const_cast<TLockableCS*>(&RunLock)), {
		return ThrottleTime;
	});
}

void ThreadThrottler::StateCheck(DWORD &WaitTime) {
	Flatten_FILETIME PrevTS;
	Flatten_FILETIME CurTS;
	UINT64 RawLoad;
	UINT64 DeltaTime;
	DWORD TMultiplier = 0;

	DWORD HistDuration = MInterval * MSTime_o100ns;
	DWORD const LoadSentinal = LoadLimit * 95 / 100;

	GetSystemTimeAsFileTime(&PrevTS.FileTime);
	Samples[SamplePtr].Dur = PrevTS.U64;
	Samples[SamplePtr].RL = 0;

	Synchronized(RunLock, {
		switch (rState) {
			case State::Monitoring:
			case State::Activiated:
				// Record the sample time
				GetSystemTimeAsFileTime(&CurTS.FileTime);
				DeltaTime = CurTS.U64 - PrevTS.U64;
				if (DeltaTime == 0)
					return;
				PrevTS.U64 = CurTS.U64;

				// Record the load time since last sample
				RawLoad = 0;
				for (size_t i = 0; i < Threads.size(); i++) {
					Flatten_FILETIME TTimes[4];
					if (GetThreadTimes(Threads[i].first->rThread, &TTimes[0].FileTime, &TTimes[1].FileTime, &TTimes[2].FileTime, &TTimes[3].FileTime)) {
						UINT64 LTime = TTimes[2].U64 + TTimes[3].U64;
						RawLoad += LTime - Threads[i].second;
						Threads[i].second = LTime;
					} else {
						LOGSYSERR(_T("WARNING: Failed to query time information for WorkerThread '%s'"), Threads[i].first->Name.c_str());
					}
				}

				SamplePtr = (SamplePtr + 1) % SampleCount;
				Samples[SamplePtr].Dur = DeltaTime;
				Samples[SamplePtr].RL = RawLoad;

				for (int SampleIdx = SamplePtr ? SamplePtr - 1 : SampleCount - 1; SampleIdx != SamplePtr; SampleIdx = SampleIdx ? SampleIdx - 1 : SampleCount - 1) {
					if (DeltaTime + Samples[SampleIdx].Dur < HistDuration) {
						DeltaTime += Samples[SampleIdx].Dur;
						RawLoad += Samples[SampleIdx].RL;
					} else break;
				}

				// Derive the x1000 load factor
				CurLoad = (DWORD)(RawLoad * MSTime_aSecond / DeltaTime);

				if (CurLoad <= LoadSentinal) {
					if (TMultiplier > 1) {
						HistDuration = max(TInterval, MInterval / TMultiplier) * MSTime_o100ns;
						WaitTime = TInterval * --TMultiplier;
						//TTLOGVV(_T("Throttle less @%d"), WaitTime);
					} else {
						if (TMultiplier) {
							--TMultiplier;
							//WaitTime = TInterval;
							//TTLOGVV(_T("Watch start @%d"), WaitTime);
							HistDuration = MInterval * MSTime_o100ns;
						} else {
							//DEBUGVV({if (WaitTime < MInterval) TTLOG(_T("Watch Relax @%d"), min(MInterval, WaitTime * 2));});
							WaitTime = min(MInterval, WaitTime + TInterval);
						}
					}
				} else if (CurLoad <= LoadLimit) {
					if (TMultiplier > 0) {
						WaitTime = TInterval * TMultiplier;
					} else {
						//DEBUGVV({if (WaitTime > TInterval) TTLOG(_T("Watch Tight @%d"), max(TInterval, WaitTime / 2)); });
						WaitTime = max(TInterval, WaitTime / 2);
					}
				} else {
					WaitTime = TInterval * ++TMultiplier;
					//DEBUGVV({if (TMultiplier > 1) TTLOGVV(_T("Throttle More @%d"), WaitTime) else TTLOGVV(_T("Throttle Start @%d"), WaitTime)});
					HistDuration = max(TInterval, MInterval / (TMultiplier + 1)) * MSTime_o100ns;
				}

				if (TMultiplier) {
					rState = State::Throttling;
					for (size_t i = 0; i < Threads.size(); i++)
						SuspendThread(Threads[i].first->rThread);
					++ThrottleCount;
					ThrottleTime += WaitTime;
				} else
					rState = State::Monitoring;
				break;

			case State::Throttling:
				for (size_t i = 0; i < Threads.size(); i++)
					ResumeThread(Threads[i].first->rThread);
				rState = State::Activiated;
				WaitTime = TInterval;
				break;
		}
	});
}

void* ThreadThrottler::Run(TWorkerThread& WorkerThread, void* NoUse) {
	DWORD WaitTime = MInterval;

	__try {
		while (WorkerThread.CurrentState() == TWorkerThread::State::Running) {
			WaitEvent.WaitFor(WaitTime);
			StateCheck(WaitTime);
		}
	} __finally {
		if (rState == State::Throttling) {
			for (size_t i = 0; i < Threads.size(); i++)
				ResumeThread(Threads[i].first->rThread);
		}
	}
	return nullptr;
}

void ThreadThrottler::StopNotify(void) {
	WaitEvent.Set();
}

LPCTSTR ThreadThrottlerException::Why(void) const {
	if (rWhy.length() == 0) {
		TString tWhy = Exception::Why();
		const_cast<TString*>(&rWhy)->resize(__DefErrorMsgBufferLen, NullWChar);
		int MsgLen = _sntprintf_s((TCHAR*)&rWhy.front(), __DefErrorMsgBufferLen, _TRUNCATE, TTLogHeader _T("%s"), ThreadThrottlerName.c_str(), tWhy.c_str());
		if (MsgLen >= 0)
			const_cast<TString*>(&rWhy)->resize(MsgLen);
	}
	return rWhy.c_str();
}

#undef TTFAIL
#undef TTLOG
#undef TTLOGV
#undef TTLOGVV
