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
 * @brief Generic Worker Thread
 * @author Zhenyu Wu
 * @date Feb 28, 2005: Initial implementation
 * @date Jul 31, 2013: Port to Visual C++ 2012
 * @date Nov 18, 2013: Major modeling upgrade
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef WorkerThread_H
#define WorkerThread_H

#include "Threading.h"

class TWorkerThread;

/**
* @ingroup Threading
* @brief Runnable template
*
* Defines the required function call interfaces for TWorkerThread wrapper
**/
class TRunnable {
	friend TWorkerThread;
protected:
	virtual ~TRunnable(void) {}

	/**
	* The "main" function of the runnable object
	**/
	virtual void* Run(TWorkerThread &WorkerThread, void *Data) {
		FAIL(_T("Abstract function"));
	}

	/**
	* Notify the stop request from worker thread
	**/
	virtual void StopNotify(void) {}
	/**
	* Discard the input data resource
	**/
	virtual void DiscardInput(void *InputData) {}
	/**
	* Discard the return data resource
	**/
	virtual void DiscardReturn(void *RetData) {}
};

class ThreadThrottler;

/**
* @ingroup Threading
* @brief Generic worker thread template
*
* Can wrap "runnable" class instance into a thread
**/
class TWorkerThread : public TWaitable {
	friend ThreadThrottler;
public:
	static enum class State {
		Initialzing,
		Running,
		Terminating,
		Terminated,
	};
	LPCTSTR STR_State(State const &xState);

private:
	void __ThreadStart(SIZE_T StackSize);
	int CollectSEHException(struct _EXCEPTION_POINTERS *SEH);
	DWORD __CallForwarder_Outer(void);
	void __CallForwarder_Inner(void);

protected:
	TRunnable& rRunnable;
	void* rInputData;
	void* rReturnData;
	BOOL rSelfFree;

	HANDLE rThread;
	Exception* rException;
	TSyncInt rState;
	TSyncObj<ThreadThrottler*> rThrottler;

public:
	TString const Name;
	DWORD const ThreadID;

	/**
	* Create and start the thread for executing the Runnable object instance
	* @note The thread is started as soon as its creation, so any initialization MUST be done before calling this function
	**/
	template<class IRunnable>
	TWorkerThread(TString const &xName, IRunnable &Runnable, void* InputData, SIZE_T StackSize = 0, bool SelfFree = false) :
		rRunnable(Runnable), rInputData(InputData), rReturnData(nullptr), rSelfFree(SelfFree),
		rThread(nullptr), rException(nullptr), rState((__ARC_INT)State::Initialzing), rThrottler(nullptr),
		Name(xName), ThreadID(0) {
		__ThreadStart(StackSize);
	}

	~TWorkerThread(void) override;

	/**
	* Signal the thread to terminate
	* @return Previous worker thread state
	**/
	State SignalTerminate(void);
	State CurrentState(void);

	/**
	* Mark thread as self-free (Exercise caution when use this function!)
	**/
	void SelfFree(void);

	/**
	* Join or leave a thread throttler group
	**/
	void Throttler(ThreadThrottler *xThrottler);

	/**
	* Get the return data of the thread
	* @note Must be called AFTER worker thread goes to wtsTerminated
	* @note The caller does NOT own the retrieved data, DO NOT FREE!
	**/
	void* getReturnData(void);
	/**
	* Get the Exception object that terminated the thread
	* @note Must be called AFTER worker thread goes to wtsTerminated
	* @note The caller does NOT own the retrieved exception, DO NOT FREE!
	**/
	Exception* getException(void);

	HANDLE CreateWaitHandle(void) const override;
};

//! @ingroup Threading
//! Worker thread specific exception
class TWorkerThreadException : public Exception {
public:
	TString const WorkerThreadName;

	template<typename... Params>
	TWorkerThreadException(TWorkerThread const &xWorkerThread, LPCTSTR &&xSource, LPCTSTR ReasonFmt, Params&&... xParams) :
		WorkerThreadName(xWorkerThread.Name), Exception(std::move(xSource), ReasonFmt, xParams...) {}

	template<typename... Params>
	static TWorkerThreadException* Create(TWorkerThread const &xWorkerThread, LPCTSTR &&xSource, LPCTSTR ReasonFmt, Params&&... xParams)
	{ return new TWorkerThreadException(xWorkerThread, std::move(xSource), ReasonFmt, xParams...); }

	LPCTSTR Why(void) const override;
};

//! @ingroup Threading
//! Worker thread SEH specific exception
class TWorkerThreadSEHException : public TWorkerThreadException {
public:
	DWORD const ExceptionCode;

	template<typename... Params>
	TWorkerThreadSEHException(DWORD xExceptionCode, TWorkerThread const &xWorkerThread, LPCTSTR &&xSource, LPCTSTR ReasonFmt, Params&&... xParams) :
		ExceptionCode(xExceptionCode), TWorkerThreadException(xWorkerThread, std::move(xSource), ReasonFmt, xParams...) {}

	LPCTSTR Why(void) const override;
};

typedef std::function<void(TWorkerThread &, bool StartStop) throw()> TWorkerThreadLiveCycle;
void WorkerThread_StartupEvent_Register(LPCTSTR Name, TWorkerThreadLiveCycle const& Func);
void WorkerThread_StartupEvent_Unregister(LPCTSTR Name);
void WorkerThread_ShutdownEvent_Register(LPCTSTR Name, TWorkerThreadLiveCycle const& Func);
void WorkerThread_ShutdownEvent_Unregister(LPCTSTR Name);

#endif //WorkerThread_H

