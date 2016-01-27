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

// [Utilities] Generic Exception Support

#include "MMSwitcher.h"

#include "Exception.h"

#include <tchar.h>
#include "DebugLog.h"
#include "WinError.h"

LPCTSTR const Exception::ExceptSourceNone = _T("(unknown source)");
LPCTSTR const Exception::ExceptReasonNone = _T("(unknown reason)");
LPCTSTR const Exception::FExceptionMessage = _T("Exception @ [%s]: %s");

LPCTSTR Exception::Why(void) const {
	if (rWhy.length() == 0) {
		LPCTSTR dSource = (Source.length() > 0) ? Source.c_str() : ExceptSourceNone;
		LPCTSTR dReason = (Reason.length() > 0) ? Reason.c_str() : ExceptReasonNone;
		const_cast<TString*>(&rWhy)->assign(__DefErrorMsgBufferLen, NullWChar);
		int MsgLen = _sntprintf_s((TCHAR*)&rWhy.front(), __DefErrorMsgBufferLen, _TRUNCATE, FExceptionMessage, dSource, dReason);
		if (MsgLen >= 0)
			const_cast<TString*>(&rWhy)->resize(MsgLen);
	}
	return rWhy.c_str();
}

// Display the error message via DEBUG output
void Exception::Show(void) const {
	LOG(_T("%s"), Why());
}

LPCTSTR const SystemError::FSystemErrorMessage = _T("%s (Error %0.8X: %s)");

LPCTSTR SystemError::ErrorMessage(void) const {
	if (rErrorMsg.length() == 0) {
		const_cast<TString*>(&rErrorMsg)->assign(__DefErrorMsgBufferLen, NullWChar);
		DecodeError((TCHAR*)&rErrorMsg.front(), __DefErrorMsgBufferLen, ErrorCode);
		const_cast<TString*>(&rErrorMsg)->resize(wcslen(&rErrorMsg.front()));
	}
	return rErrorMsg.c_str();
}

LPCTSTR SystemError::Why(void) const {
	if (rWhy.length() == 0) {
		LPCTSTR dWhy = Exception::Why();
		TString nWhy(__DefErrorMsgBufferLen, NullWChar);

		int MsgLen = _sntprintf_s((TCHAR*)&nWhy.front(), __DefErrorMsgBufferLen, _TRUNCATE, FSystemErrorMessage, dWhy, ErrorCode, ErrorMessage());
		if (MsgLen >= 0) {
			nWhy.resize(MsgLen);
			const_cast<TString*>(&rWhy)->assign(std::move(nWhy));
		}
	}
	return rWhy.c_str();
}
