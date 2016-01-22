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
 * @brief Windows API Error Decoding
 * @author Zhenyu Wu
 * @date Jul 29, 2013: Initial Implementation
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef WinError_H
#define WinError_H

#include <Windows.h>

#include "DebugLog.h"
#include "Exception.h"

//! @ingroup Utilities
//! Decode the error code in GetLastError and allocate a buffer for the message (free with LocalFree)
LPCTSTR DecodeLastError(va_list *Args = nullptr);

//! @ingroup Utilities
//! Decode the error code in GetLastError using a pre-allocated message buffer
void DecodeLastError(LPTSTR Buffer, DWORD BufLen, va_list *Args = nullptr);

//! @ingroup Utilities
//! Decode the specified system error code and allocate a buffer for the message (free with LocalFree)
LPCTSTR DecodeError(DWORD ErrCode, va_list *Args = nullptr);

//! @ingroup Utilities
//! Decode the specified module error code and allocate a buffer for the message (free with LocalFree)
LPCTSTR DecodeError(HMODULE Module, DWORD ErrCode, va_list *Args = nullptr);

//! @ingroup Utilities
//! Decode the specified system error code using a pre-allocated message buffer
void DecodeError(LPTSTR Buffer, DWORD BufLen, DWORD ErrCode, va_list *Args = nullptr);

//! @ingroup Utilities
//! Decode the specified module error code using a pre-allocated message buffer
void DecodeError(HMODULE Module, LPTSTR Buffer, DWORD BufLen, DWORD ErrCode, va_list *Args = nullptr);

void __FormatCtxAndDecodeError(LPTSTR CtxBuffer, DWORD CtxBufLen, HMODULE Module,
							   LPTSTR ErrBuffer, DWORD ErrBufLen, DWORD ErrCode, ...);

//! @ingroup Utilities
//! Raise an exception after a failed system call
#define SYSFAIL(ctx, ...)		SYSERRFAIL(GetLastError(), ctx, __VA_ARGS__)

//! @ingroup Utilities
//! Log a failed system call
#define LOGSYSERR(ctx, ...)		LOGERR(GetLastError(), ctx, __VA_ARGS__)
#define LOGSYSERR_ARG(ctx, ...)	LOGERR_ARG(GetLastError(), ctx, __VA_ARGS__)

//! @ingroup Utilities
//! Log a failure with a system error code
#define LOGERR(errcode, ctx, ...)							\
{															\
	TCHAR ErrMsg[__DefErrorMsgBufferLen];					\
	DecodeError(ErrMsg, __DefErrorMsgBufferLen, errcode);	\
	LOG(_T("%s (") ctx _T(")"), ErrMsg, __VA_ARGS__);		\
}
#define LOGERR_ARG(errcode, ctx, argc, ...)							\
{																	\
	TCHAR CtxMsg[__DefErrorMsgBufferLen];							\
	TCHAR ErrMsg[__DefErrorMsgBufferLen];							\
	__FormatCtxAndDecodeError(CtxMsg, __DefErrorMsgBufferLen,		\
							  0, ErrMsg, __DefErrorMsgBufferLen,	\
							  errcode, __VA_ARGS__);				\
	LOG(_T("%s (%s)"), ErrMsg, CtxMsg);								\
}

#endif //WinError_H