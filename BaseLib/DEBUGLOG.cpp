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

// [Utilities] Generic Debug Support

#include "MMSwitcher.h"

#include "DebugLog.h"

#include <stdarg.h>
#include <unordered_map>
#include <initializer_list>

#include "WinError.h"
#include "Exception.h"

#include "ThreadLib/Threading.h"

#ifndef SOLUTION_PATH
#if _MSC_VER
#pragma message ("Warning: Please define the project path before compiling this file!")
#pragma message ("Warning: Hint - /D \"SOLUTION_PATH=\\\"$(SolutionDir.Replace('\\','/'))\\\"\"") 
#endif
#define SOLUTION_PATH ""
#endif //SOLUTION_PATH

#define SAFE_RELPATH

// Aquire the skip length of source code (so we can properly print relative source file paths)
LPCTSTR __RelPath(LPCTSTR Path) {
	static size_t __RelPathLen = wcslen(_T(SOLUTION_PATH));

#ifdef SAFE_RELPATH
	TCHAR __Path[1024];
	if (_tcsncpy_s(__Path, Path, 1024))
		FAIL(_T("Unable to create copy of path string"));
	while (TCHAR* PDem = _tcspbrk(__Path, _T("\\")))
		*PDem = '/';
	return Path + (_tcsnicmp(__Path, _T(SOLUTION_PATH), __RelPathLen) ? 0 : __RelPathLen);
#else
	return Path + __RelPathLen;
#endif
}

LPCSTR __RelPath(LPCSTR Path) {
	static size_t __RelPathLen = strlen(SOLUTION_PATH);

#ifdef SAFE_RELPATH
	char __Path[1024];
	if (strncpy_s(__Path, Path, 1024))
		FAIL(_T("Unable to create copy of path string"));
	while (char* PDem = strpbrk(__Path, "\\"))
		*PDem = '/';
	return Path + (_strnicmp(__Path, SOLUTION_PATH, __RelPathLen) ? 0 : __RelPathLen);
#else
	return Path + __RelPathLen;
#endif
}

LPCTSTR __PTID(void) {
	__declspec(thread) static TCHAR PTIDPrefix[12] = {NullWChar};

	if (PTIDPrefix[0] == NullWChar)
		_sntprintf_s(PTIDPrefix, 12, _TRUNCATE, _T("%5d:%-5d"), GetCurrentProcessId(), GetCurrentThreadId());
	return PTIDPrefix;
}

//LPCTSTR const CONSOLELOG = _T("Console");
LPCTSTR const& CONSOLELOG(void) {
	static LPCTSTR const __IoFU = _T("Console");
	return __IoFU;
}

//std::unordered_map<TString, FILE*> LogTargets({{CONSOLELOG, stderr}});
typedef std::vector<std::pair<TString, FILE*>> TLogTargets;
TLogTargets& LogTargets(void) {
	static TLogTargets __IoFU({{CONSOLELOG(), stderr}});
	return __IoFU;
}

void __LocaleInit(void) {
	static char const* __InitLocale = nullptr;
	if (!__InitLocale) {
		__InitLocale = ACP_LOCALE();
		setlocale(LC_ALL, __InitLocale);
	}
}

void ERRORPRINTF(LPCTSTR Fmt, ...) {
	va_list params;
	va_start(params, Fmt);
	Synchronized((*Lock_DebugLog()), {
		__LocaleInit();
		for (size_t i = 0; i < LogTargets().size(); i++) {
			_vftprintf(LogTargets()[i].second, Fmt, params);
			fflush(LogTargets()[i].second);
		}
	});
	va_end(params);
}

void LOGTARGET(LPCTSTR Name, FILE *xTarget, LPCTSTR Message) {
	Synchronized((*Lock_DebugLog()), {
		if (xTarget) {
			if (setvbuf(xTarget, nullptr, _IOLBF, 4096) != 0) {
				LOG(_T("WARNING: Unable to turn off file buffering for log target '%s'"), Name);
			}

			for (size_t i = 0; i < LogTargets().size(); i++) {
				if (LogTargets()[i].first.compare(Name) == 0) {
					LogTargets()[i].second = xTarget;
					xTarget = nullptr;
				}
			}
			if (xTarget != nullptr)
				LogTargets().emplace_back(Name, xTarget);
			if (Message)
				LOG(_T("===== %s ====="), Message);
		} else {
			for (size_t i = 0; i < LogTargets().size(); i++) {
				if (LogTargets()[i].first.compare(Name) == 0) {
					LogTargets().erase(LogTargets().cbegin() + i);
					if (Message)
						LOG(_T("===== %s ====="), Message);
				}
			}
		}
	});
}