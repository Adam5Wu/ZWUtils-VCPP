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

// [Utilities] Windows API Error Decoding

#include "MMSwitcher.h"

#include <stdarg.h>

#include "WinError.h"

LPTSTR __RLNTrim(LPTSTR Buf, int Len) {
	while (--Len >= 0) {
		if ((Buf[Len] != (TCHAR)'\n') && (Buf[Len] != (TCHAR)'\r'))
			break;
		Buf[Len] = 0;
	}
	return Buf;
}

LPCTSTR _DecodeError(HMODULE Module, LPTSTR Buffer, DWORD BufLen, DWORD ErrCode, va_list *Args) {
	DWORD MsgFlags = 0;
	if (Args == nullptr)
		MsgFlags |= FORMAT_MESSAGE_IGNORE_INSERTS;
	if (Module == 0)
		MsgFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
	else
		MsgFlags |= FORMAT_MESSAGE_FROM_HMODULE;

	LPTSTR MsgBuf;
	if (Buffer == nullptr) {
		MsgBuf = (LPTSTR)&MsgBuf;
		MsgFlags |= FORMAT_MESSAGE_ALLOCATE_BUFFER;
	} else
		MsgBuf = Buffer;

	DWORD Ret = FormatMessage(MsgFlags, Module, ErrCode, 0, MsgBuf, BufLen, Args);
	if (Ret == 0) {
		TCHAR ReasonMsg[__DefErrorMsgBufferLen];
		DecodeLastError(ReasonMsg, __DefErrorMsgBufferLen);
		FAIL(_T("Unable to decode error %0.8x - %s"), ErrCode, ReasonMsg);
	}
	return __RLNTrim(MsgBuf, Ret);
}

LPCTSTR DecodeLastError(va_list *Args) {
	DWORD ErrCode = GetLastError();
	return DecodeError(ErrCode, Args);
}

void DecodeLastError(LPTSTR Buffer, DWORD BufLen, va_list *Args) {
	DWORD ErrCode = GetLastError();
	DecodeError(Buffer, BufLen, ErrCode, Args);
}

LPCTSTR DecodeError(DWORD ErrCode, va_list *Args) {
	return DecodeError(0, ErrCode, Args);
}

LPCTSTR DecodeError(HMODULE Module, DWORD ErrCode, va_list *Args) {
	return _DecodeError(Module, nullptr, 0, ErrCode, Args);
}

void DecodeError(LPTSTR Buffer, DWORD BufLen, DWORD ErrCode, va_list *Args) {
	DecodeError(0, Buffer, BufLen, ErrCode, Args);
}

void DecodeError(HMODULE Module, LPTSTR Buffer, DWORD BufLen, DWORD ErrCode, va_list *Args) {
	_DecodeError(Module, Buffer, BufLen, ErrCode, Args);
}

void __FormatCtxAndDecodeError(LPTSTR CtxBuffer, DWORD CtxBufLen, HMODULE Module,
							   LPTSTR ErrBuffer, DWORD ErrBufLen, DWORD ErrCode, ...) {
	va_list _va_list;
	va_start(_va_list, ErrCode);
	__try {
		_vsntprintf_s(CtxBuffer, CtxBufLen, _TRUNCATE, _T("%s:%d"), _va_list);
		DecodeError(Module, ErrBuffer, ErrBufLen, ErrCode, &_va_list);
	} __finally {
		va_end(_va_list);
	}
}
