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
 * @addtogroup Utilities Basic Supporting Utilities
 * @file
 * @brief Generic %Exception Support
 * @author Zhenyu Wu
 * @date Feb 28, 2005: Initial implementation
 * @date Aug 06, 2010: Adapted for SAPlacement
 * @date Aug 10, 2010: Adapted for Doxygen
 * @date Aug 11, 2010: Uses source printing function in DEBUG.h
 * @date Jul 26, 2013: Porting to Visual C++ 2012
 * @date Jul 29, 2013: Unicode compatibility, interface cleanup
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef Exception_H
#define Exception_H

#include <Windows.h>

#include "DebugLog.h"

/**
 * @ingroup Utilities
 * @brief Generic exception class
 *
 * Contains useful information about the exception:
 *  source, reason and a helper integer.
 **/
class Exception {
public:
	static LPCTSTR const ExceptSourceNone;
	static LPCTSTR const ExceptReasonNone;
	static LPCTSTR const FExceptionMessage;

protected:
	TString const rWhy;

public:
	TString const Source;
	TString const Reason;

	/**
	 * Create an exception object with given source, reason, and an optional helper integer
	 *
	 * @note Use the @link FAIL() \p FAIL* @endlink macros to retrive source info automatically
	 **/
	template<typename... Params>
	Exception(LPCTSTR &&xSource, LPCTSTR ReasonFmt, Params&&... xParams) :
		Source(xSource ? xSource : EmptyWText), Reason(EmptyWText), rWhy(EmptyWText) {
		if (xSource) free((PVOID)xSource);
		if (ReasonFmt != nullptr) {
			const_cast<TString*>(&Reason)->assign(__DefErrorMsgBufferLen, NullWChar);
			int MsgLen = _sntprintf_s((TCHAR*)&Reason.front(), __DefErrorMsgBufferLen, _TRUNCATE, ReasonFmt, xParams...);
			if (MsgLen >= 0)
				const_cast<TString*>(&Reason)->resize(MsgLen);
		}
	}

	template<typename... Params>
	static Exception* Create(LPCTSTR &&xSource, LPCTSTR ReasonFmt, Params&&... xParams)
	{ return new Exception(std::move(xSource), ReasonFmt, xParams...); }

	virtual ~Exception(void) {}

	/**
	 * Returns a description of the exception
	 * @note The caller does NOT have ownership of the returned string
	 **/
	virtual LPCTSTR Why(void) const;

	/**
	 * Prints the description of the exception via @link ERRORPRINT() DEBUG printing functions @endlink
	 **/
	virtual void Show(void) const;
};

//! @ingroup Utilities
//! Raise an exception with a formatted string message
#define FAIL(fmt, ...)						\
{											\
	SOURCEMARK								\
	FAIL_SRC(__SrcMark, fmt, __VA_ARGS__);	\
}
#define FAIL_SRC(src, fmt, ...) throw Exception::Create(std::move(src), fmt VAWRAP(__VA_ARGS__));

class SystemError : public Exception {
public:
	static LPCTSTR const FSystemErrorMessage;

protected:
	TString const rErrorMsg;

public:
	DWORD const ErrorCode;

	template<typename... Params>
	SystemError(LPCTSTR &&xSource, DWORD xErrorCode, LPCTSTR ReasonFmt, Params&&... xParams) :
		Exception(std::move(xSource), ReasonFmt, xParams...), ErrorCode(xErrorCode), rErrorMsg(EmptyWText) {
		// Nothing
	}

	template<typename... Params>
	static SystemError* Create(LPCTSTR &&xSource, DWORD xErrorCode, LPCTSTR ReasonFmt, Params&&... xParams)
	{ return new SystemError(std::move(xSource), xErrorCode, ReasonFmt, xParams...); }

	virtual LPCTSTR ErrorMessage(void) const;

	LPCTSTR Why(void) const override;

};

//! @ingroup Utilities
//! Raise a system error exception with a formatted string message
#define SYSERRFAIL(errcode, fmt, ...)						\
{															\
	SOURCEMARK												\
	SYSERRFAIL_SRC(__SrcMark, errcode, fmt, __VA_ARGS__);	\
}
#define SYSERRFAIL_SRC(src, errcode, fmt, ...) throw SystemError::Create(std::move(src), errcode, fmt VAWRAP(__VA_ARGS__));

#endif //Exception_H
