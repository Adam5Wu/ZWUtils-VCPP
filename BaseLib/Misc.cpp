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

// [Utilities] Misc utilities

#include "MMSwitcher.h"

#include <tchar.h>
#include <iomanip>

#include "Misc.h"
#include "WinError.h"

#pragma comment(lib, "ws2_32.lib")

//std::string const EMPTY_CSTRING(EmptyAText);
//TString const EMPTY_TSTRING()(EmptyWText);
std::string const& EMPTY_CSTRING(void) {
	static std::string const __IoFU(EmptyAText);
	return __IoFU;
}

TString const& EMPTY_TSTRING(void) {
	static TString const __IoFU(EmptyWText);
	return __IoFU;
}

std::string TStringtoCPString(UINT CodePage, TString const &Str, TString &ErrMessage) {
	ErrMessage.clear();

	if (Str.length() == 0)
		return EMPTY_CSTRING();

	DWORD dwConversionFlags = 0;
#if (WINVER >= 0x0600)
	// Only applicable to UTF-8 and GB18030
	if ((CodePage == CP_UTF8) || (CodePage == 54936))
		dwConversionFlags = WC_ERR_INVALID_CHARS;
#endif
	BOOL IsDefaultCharUsed = FALSE;
	LPBOOL UsedDefaultChar = &IsDefaultCharUsed;
	// Not supported by UTF-7 or UTF-8
	if ((CodePage == CP_UTF7) || (CodePage == CP_UTF8))
		UsedDefaultChar = NULL;

	// Get size of destination buffer, in CHAR's (= bytes)
	int cbCP = WideCharToMultiByte(
		CodePage,				// convert to UTF-8
		dwConversionFlags,		// specify conversion behavior
		Str.data(),				// source UTF-16 string
		(int)Str.length(),		// null terminated string
		NULL,					// unused - no conversion required in this step
		0,						// request buffer size
		NULL,					// use system default char
		UsedDefaultChar			// check if default char is used
		);
	if (cbCP == 0) {
#if (WINVER >= 0x0600)
		DWORD ErrCode = GetLastError();
		// Only applicable to UTF-8 and GB18030
		if (((CodePage == CP_UTF8) || (CodePage == 54936)) && (ErrCode == ERROR_NO_UNICODE_TRANSLATION)) {
			TCHAR ErrMsg[__DefErrorMsgBufferLen];
			DecodeError(ErrMsg, __DefErrorMsgBufferLen, ErrCode);
			ErrMessage = ErrMsg;
			// Retry with less strict conversion behavior
			cbCP = WideCharToMultiByte(
				CodePage,				// convert to UTF-8
				0,						// specify conversion behavior
				Str.data(),				// source UTF-16 string
				(int)Str.length(),		// null terminated string
				NULL,					// unused - no conversion required in this step
				0,						// request buffer size
				NULL,					// use system default char
				UsedDefaultChar			// check if default char is used
				);
		}
		if (cbCP == 0)
#endif
		{
			DWORD ErrCode = GetLastError();
			ErrMessage.assign(__DefErrorMsgBufferLen, '\0');
			DecodeError(const_cast<TCHAR*>(ErrMessage.data()), __DefErrorMsgBufferLen, ErrCode);
			return EMPTY_CSTRING();
		}
	}
	if ((UsedDefaultChar != NULL) && (IsDefaultCharUsed)) {
		ErrMessage.assign(_T("Unicode string contains incompatible codepoint for code page #%d, replaced with default char"), CodePage);
	}

	//
	// Allocate destination buffer for UTF-8 string
	//
	std::string Ret(cbCP, NullAChar);

	//
	// Do the conversion from UTF-16 to UTF-8
	//
	int result = WideCharToMultiByte(
		CodePage,				// convert to UTF-8
		0,						// specify conversion behavior
		Str.data(),				// source UTF-16 string
		(int)Str.length(),		// null terminated string
		&Ret.front(),			// destination buffer
		cbCP,					// destination buffer size, in bytes
		NULL, NULL				// unused
		);
	if (result == 0)
		SYSFAIL(_T("Failed to convert unicode string '%s'"), Str.c_str());

	// Return resulting UTF-8 string
	return std::move(Ret);
}

TString CPStringtoTString(UINT CodePage, std::string const &Str, TString &ErrMessage) {
	if (Str.length() == 0)
		return EMPTY_TSTRING();

	//
	// Get size of destination UTF-16 buffer, in WCHAR's
	//
	int cchUTF16 = MultiByteToWideChar(
		CodePage,				// convert from UTF-8
		MB_ERR_INVALID_CHARS,	// error on invalid chars
		Str.data(),				// source UTF-8 string
		(int)Str.length(),		// total length of source UTF-8 string,
		NULL,					// unused - no conversion done in this step
		0						// request size of destination buffer, in WCHAR's
		);
	if (cchUTF16 == 0) {
		DWORD ErrCode = GetLastError();
		ErrMessage.assign(__DefErrorMsgBufferLen, '\0');
		DecodeError(const_cast<TCHAR*>(ErrMessage.data()), __DefErrorMsgBufferLen, ErrCode);
		return EMPTY_TSTRING();
	}

	//
	// Allocate destination buffer to store UTF-16 string
	//
	TString Ret(cchUTF16, NullWChar);

	//
	// Do the conversion from UTF-8 to UTF-16
	//
	int result = ::MultiByteToWideChar(
		CodePage,				// convert from UTF-8
		MB_ERR_INVALID_CHARS,	// error on invalid chars
		Str.data(),				// source UTF-8 string
		(int)Str.length(),		// total length of source UTF-8 string,
		&Ret.front(),			// destination buffer
		cchUTF16				// size of destination buffer, in WCHAR's
		);
	if (result == 0)
		SYSFAIL(_T("Failed to convert codepage #%u string '%s'"), CodePage, Str.c_str());

	// Return resulting UTF-8 string
	return std::move(Ret);
}

std::string TStringtoUTF8(TString const &Str) {
	TString ErrMessage;
	std::string Ret = TStringtoCPString(CP_UTF8, Str, ErrMessage);
	if (!ErrMessage.empty())
		LOG(_T("WARNING: Unsafe conversion from unicode to UTF-8 - %s"), ErrMessage.c_str());
	return std::move(Ret);
}

std::string TStringtoUTF8_Check(TString const &Str) {
	TString ErrMessage;
	std::string Ret = TStringtoCPString(CP_UTF8, Str, ErrMessage);
	if (!ErrMessage.empty())
		FAIL(_T("Unsafe conversion from unicode to UTF-8 - %s"), ErrMessage.c_str());
	return std::move(Ret);
}

std::string TStringtoUTF8(TString const &Str, TString &ErrMessage) {
	return std::move(TStringtoCPString(CP_UTF8, Str, ErrMessage));
}

TString UTF8toTString(std::string const &Str) {
	TString ErrMessage;
	TString Ret = CPStringtoTString(CP_UTF8, Str, ErrMessage);
	if (!ErrMessage.empty())
		LOG(_T("WARNING: Unable to convert from UTF-8 to unicode - %s"), ErrMessage.c_str());
	return std::move(Ret);
}

TString TStringtoUTF8_Check(std::string const &Str) {
	TString ErrMessage;
	TString Ret = CPStringtoTString(CP_UTF8, Str, ErrMessage);
	if (!ErrMessage.empty())
		FAIL(_T("Unable to convert from UTF-8 to unicode - %s"), ErrMessage.c_str());
	return std::move(Ret);
}

TString UTF8toTString(std::string const &Str, TString &ErrMessage) {
	return std::move(CPStringtoTString(CP_UTF8, Str, ErrMessage));
}

// Flatten_FILETIME
TString Flatten_FILETIME::toString(void) const {
	SYSTEMTIME SysTime;
	if (!FileTimeToSystemTime(&FileTime, &SysTime))
		SYSFAIL(_T("Could not convert timestamp"));
	return TStringCast(std::setfill(_T('0')) << std::setw(4) << SysTime.wYear << '/' << std::setw(2) << SysTime.wMonth << '/' << SysTime.wDay
					   << '+' << SysTime.wHour << ':' << SysTime.wMinute << ':' << SysTime.wSecond << '.' << std::setw(3) << SysTime.wMilliseconds);
}

Flatten_FILETIME GetCurrentSystemTime(void) {
	Flatten_FILETIME Ret;
	GetSystemTimeAsFileTime(&Ret.FileTime);
	return Ret;
}

// Flatten_INET4
size_t INET4::hashcode(void) const {
	return std::hash<UINT32>()(Addr);
}

TString INET4::toString(void) const {
	return TStringCast(A << _T('.') << B << _T('.') << C << _T('.') << D);
}

bool INET4::equalto(INET4 const &T) const {
	return Addr == T.Addr;
}

// UINT128
TString UINT128::toString(Format const &Fmt) const {
	static TCHAR const* _STR_Format[] = {
		_T("HEX64"),
		_T("HEX32"),
		_T("HEX16"),
		_T("HEX8"),
		_T("INET6"),
	};

	switch (Fmt) {
		case Format::HEX64:
			return TStringCast(std::hex << std::uppercase << _T('[') << U64B << _T('$') << U64A << _T(']'));
		case Format::HEX32:
			return TStringCast(std::hex << std::uppercase << _T('[') << U32D << _T('$') << U32C
							   << _T('$') << U32B << _T('$') << U32A << _T(']'));
		case Format::HEX16:
			return TStringCast(std::hex << std::uppercase << _T('[') << U16[7] << _T('$') << U16[6] << _T('$') << U16[5] << _T('$') << U16[4]
							   << _T('$') << U16[3] << _T('$') << U16[2] << _T('$') << U16[1] << _T('$') << U16[0] << _T(']'));
		case Format::HEX8:
			return TStringCast(std::hex << std::uppercase << _T('[') << U8[15] << _T('$') << U8[14] << _T('$') << U8[13] << _T('$') << U8[12]
							   << _T('$') << U8[11] << _T('$') << U8[10] << _T('$') << U8[9] << _T('$') << U8[8]
							   << _T('$') << U8[7] << _T('$') << U8[6] << _T('$') << U8[5] << _T('$') << U8[4]
							   << _T('$') << U8[3] << _T('$') << U8[2] << _T('$') << U8[1] << _T('$') << U8[0] << _T(']'));
		case Format::INET6:
			return TStringCast(std::hex << std::setfill(_T('0')) << htons(U16[0]) << _T(':') << htons(U16[1]) << _T(':') << htons(U16[2]) << _T(':') << htons(U16[3])
							   << _T(':') << htons(U16[4]) << _T(':') << htons(U16[5]) << _T(':') << htons(U16[6]) << _T(':') << htons(U16[7]));
		default:
			FAIL(_T("Unknown format %s"), _STR_Format[(int)Fmt]);
	}
}

size_t UINT128::hashcode(void) const {
	return std::hash<UINT64>()(U64A) ^ std::hash<UINT64>()(U64B);
}

bool UINT128::equalto(UINT128 const &T) const {
	return (T.U64A == U64A) && (T.U64B == U64B);
}

//UINT128 const UINT128_ZERO{0, 0, 0, 0};
UINT128 const& UINT128_ZERO(void) {
	static UINT128 const __IoFU{0, 0, 0, 0};
	return __IoFU;
}

// HASH256
TString HASH256::toString(void) const {
	return TStringCast(std::hex << _T('{') << U64D << _T('$') << U64C << _T('$') << U64B << _T('$') << U64A << _T('}'));
}

size_t HASH256::hashcode(void) const {
	return std::hash<UINT64>()(U64A) ^ std::hash<UINT64>()(U64B) ^ std::hash<UINT64>()(U64C) ^ std::hash<UINT64>()(U64D);
}

bool HASH256::equalto(HASH256 const &T) const {
	return (T.U64A == U64A) && (T.U64B == U64B) && (T.U64C == U64C) && (T.U64D == U64D);
}

bool StrToInt(LPCTSTR String, int &i, int base) {
	LPTSTR ParsePtr;
	errno = 0;
	i = _tcstol(String, &ParsePtr, base);
	return ((errno == 0) && (ParsePtr[0] == 0));
}

bool StrToDouble(LPCTSTR String, double &d) {
	LPTSTR ParsePtr;
	errno = 0;
	d = _tcstod(String, &ParsePtr);
	return ((errno == 0) && (ParsePtr[0] == 0));
}

UINT32 CountBits(UINT32 const &Mask) {
	UINT32 Ret = Mask - ((Mask >> 1) & 0x55555555);
	Ret = (Ret & 0x33333333) + ((Ret >> 2) & 0x33333333);
	Ret = (Ret + (Ret >> 4)) & 0x0f0f0f0f;
	return (Ret * 0x01010101) >> 24;
}

UINT32 CountBits(UINT64 const &Mask) {
	UINT64 Ret = Mask - ((Mask >> 1) & 0x5555555555555555);
	Ret = (Ret & 0x3333333333333333) + ((Ret >> 2) & 0x3333333333333333);
	Ret = (Ret + (Ret >> 4)) & 0x0f0f0f0f0f0f0f0f;
	return (Ret * 0x0101010101010101) >> 56;
}

CONTEXT_CONSTRUCT_T const CONTEXT_CONSTRUCT;
EMPLACE_CONSTRUCT_T const EMPLACE_CONSTRUCT;
HANDOFF_CONSTRUCT_T const HANDOFF_CONSTRUCT;
DEFAULT_CONSTRUCT_T const DEFAULT_CONSTRUCT;

