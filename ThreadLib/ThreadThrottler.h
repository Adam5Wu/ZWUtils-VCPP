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
 * @brief Thread Throttling Support
 * @author Zhenyu Wu
 * @date Nov 13, 2013: Initial implementation
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef ThreadThrottler_H
#define ThreadThrottler_H

#include <Windows.h>

#include "BaseLib/Misc.h"

#include "Threading.h"
#include "WorkerThread.h"

#include <vector>

class ThreadThrottler : public TRunnable {
	friend TWorkerThread;
public:
	static DWORD const MIntervalMin;
	static DWORD const MIntervalMax;
	static DWORD const TIntervalMin;
	static DWORD const TIntervalMax;
	static DWORD const MTRatioSmall;
	static DWORD const MTRatioLarge;
	static DWORD const MTRatioMin;
	static DWORD const MTRatioMax;
	static DWORD const MAXLOAD;

	static enum class State {
		Monitoring,
		Activiated,
		Throttling,
	};
	LPCTSTR STR_State(State const &xState);

protected:
	DWORD LoadLimit;
	DWORD MInterval;
	DWORD TInterval;
	std::vector<std::pair<TWorkerThread*, UINT64>> Threads;

	State rState;
	UINT64 ThrottleCount;
	UINT64 ThrottleTime;
	TLockableCS RunLock;
	TEvent WaitEvent;

	DWORD CurLoad;
	struct TSample {
		UINT64 Dur;
		UINT64 RL;
	};
	int const SampleCount;
	std::vector<TSample> Samples;
	int SamplePtr;

	void RegisterWorker(TWorkerThread *Worker);
	void UnregisterWorker(TWorkerThread *Worker);

	void StateCheck(DWORD &WaitTime);
	void* Run(TWorkerThread &WorkerThread, void* NoUse) override;
	void StopNotify(void) override;

public:
	TString const Name;

	ThreadThrottler(TString const &xName, DWORD xLoadLimit, DWORD xMInterval, DWORD xTInterval);

	void Bump(void);

	double GetLoadFactor(void) const;
	State CurrentState(void) const;
	UINT64 GetThrottleCount(void) const;
	UINT64 GetThrottleTime(void) const;
};

//! @ingroup Threading
//! Thread throttler specific exception
class ThreadThrottlerException : public Exception {
public:
	TString const ThreadThrottlerName;

	template<typename... Params>
	ThreadThrottlerException(ThreadThrottler const &xThrottler, LPCTSTR &&xSource, LPCTSTR ReasonFmt, Params&&... xParams) :
		ThreadThrottlerName(xThrottler.Name), Exception(std::move(xSource), ReasonFmt, xParams...) {}

	LPCTSTR Why(void) const override;
};

#endif//ThreadThrottler_H
