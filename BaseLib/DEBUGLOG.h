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
 * @brief Generic Debug Support
 * @note Define \p NO_DEBUG to supress any debug message
 * @author Zhenyu Wu
 * @date Aug 06, 2010: Initial implementation
 * @date Aug 10, 2010: Adapted for Doxygen
 * @date Aug 11, 2010: Added @link SOURCEMARK() source printing macros @endlink
 * @date Jul 26, 2013: Porting to Visual C++ 2012
 * @date Jul 29, 2013: Unicode compatibility, macro name disambiguation
 * @date Oct 20, 2013: Fixed relative source path printing for VC++ 2012/2013
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef DebugLog_H
#define DebugLog_H

#include <tchar.h>
#include <stdio.h>

#include "Misc.h"

#define __DefErrorMsgBufferLen 2048

LPCTSTR __RelPath(LPCTSTR Path);
LPCSTR __RelPath(LPCSTR Path);
LPCTSTR __PTID(void);

//! Get the relative path of source file
#define __REL_FILE__ __RelPath(_T(__FILE__))

//! Print source filename, line number and function
#define __SRCMARK__(buf,len) _sntprintf_s(buf, len, _TRUNCATE, _T("%s:%d"), __REL_FILE__, __LINE__)

//! Allocate buffer and print current source information
#define SOURCEMARK																				\
LPCTSTR __SrcMark;																				\
{																								\
	LPCTSTR __RelFile = __REL_FILE__;															\
	/* LineNum + ':' + \0 */																	\
	size_t __SrcMarkLen = _tcslen(__RelFile) + 10 + 1 + 1;										\
	__SrcMark = (LPCTSTR)malloc(__SrcMarkLen*sizeof(TCHAR));									\
	_sntprintf_s((LPTSTR)__SrcMark, __SrcMarkLen, _TRUNCATE, _T("%s:%d"), __RelFile, __LINE__);	\
}

#define LOGTIMESTAMP													\
/* 0000/00/00 00:00:00.000 */											\
size_t const __TimeStampLen = 4+2+2+2+2+2+3+6+1;						\
TCHAR const __TimeStamp[__TimeStampLen] = {0};							\
{																		\
	SYSTEMTIME SystemTime;												\
	GetSystemTime(&SystemTime);											\
	_sntprintf_s((LPTSTR)__TimeStamp, __TimeStampLen, _TRUNCATE,		\
				_T("%04d/%02d/%02d %02d:%02d:%02d.%03d"),				\
				SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay,	\
				SystemTime.wHour, SystemTime.wMinute,					\
				SystemTime.wSecond, SystemTime.wMilliseconds);			\
}

//! Print a formatted debug string message
void ERRORPRINTF(LPCTSTR Fmt, ...);

//extern LPCTSTR const CONSOLELOG;
LPCTSTR const& CONSOLELOG(void);

//! Add or remove a debug log target
void LOGTARGET(LPCTSTR Name, FILE *xTarget, LPCTSTR Message = nullptr);

#ifdef NO_DEBUG

#define UNDEBUG(s) s
#define DEBUG(s)
#define LOG(s)
#undef DEBUGV
#define DEBUGV(s)
#define LOGV
#undef DEBUGVV
#define DEBUGVV(s)
#define LOGVV(s)
#define DEBUGCHECK(s)

#else //NO_DEBUG

#define UNDEBUG(s)

//-------------- DEBUGMODULE
#ifndef DEBUG_MODULE

#define __DEBUG_MODULE(s)	s

#else //DEBUG_MODULE

#define __Str(x)	#x
#define DefStr(x)	__Str(x)

#define __DEBUG_MODULE(s)						\
{												\
	SOURCEMARK;									\
	LPCTSTR Module = _T(DefStr(DEBUG_MODULE));	\
	bool DEBUG_DO = _tcsicmp(FileName, Module);	\
	delete __SrcMark;							\
	if (DEBUG_DO == 0) s;						\
}

#endif //DEBUG_MODULE

//-------------- DEBUGMEM
#ifndef DEBUGMEM

#define DEBUGMEM(s)

#else //DEBUG_MODULE

#undef DEBUGMEM
#define DEBUGMEM(s) __DEBUG_MODULE(s)

#endif //DEBUG_MODULE

//-------------- DEBUG LOGGING
#define __DebugLog(fmt, ...)																\
__DEBUG_MODULE({																			\
	LOGTIMESTAMP																			\
	ERRORPRINTF(_T("[%s] %s | ") fmt _T("\n"), __PTID(), __TimeStamp VAWRAP(__VA_ARGS__));	\
})

#define __DebugLogSRC(fmt, ...)										\
__DEBUG_MODULE({													\
	SOURCEMARK														\
	__DebugLog(_T("@<%s> ") fmt, __SrcMark VAWRAP(__VA_ARGS__));	\
	free((PVOID)__SrcMark);											\
})

#define DEBUG(s)		__DEBUG_MODULE(s)
#define LOG(s, ...)		__DebugLog(s, __VA_ARGS__)
#define LOGS(s, ...)	__DebugLogSRC(s, __VA_ARGS__)

//-------------- DEBUGV
#ifndef DEBUGV

#define DEBUGV(s)
#define LOGV(s, ...)
#undef DEBUGVV
#define DEBUGVV(s)
#define LOGVV(s, ...)

#else //DEBUGV

#undef DEBUGV
#define DEBUGV(s)		__DEBUG_MODULE(s)
#define LOGV(s, ...)	__DebugLog(s, __VA_ARGS__)
#define LOGSV(s, ...)	__DebugLogSRC(s, __VA_ARGS__)

//-------------- DEBUGVV
#ifndef DEBUGVV

#define DEBUGVV(s, ...)
#define LOGVV(s, ...)

#else //DEBUGVV

#undef DEBUGVV
#define DEBUGVV(s)		__DEBUG_MODULE(s)
#define LOGVV(s, ...)	__DebugLog(s, __VA_ARGS__)
#define LOGSVV(s, ...)	__DebugLogSRC(s, __VA_ARGS__)

#endif //DEBUGVV
#endif //DEBUGV

#endif //NO_DEBUG

#endif //DebugLog_H
