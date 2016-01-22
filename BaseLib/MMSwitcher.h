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
 * @addtogroup Utilities Memory Management Switcher
 * @file
 * @brief Memory Management Switcher
 * @author Zhenyu Wu
 * @date Jan 27, 2014: Initial implementation
 * @date Jan 22, 2016: Initial Public Release
 **/

// HOW TO USE: Include at the beginning of every (non-template) .cpp file!

#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "BaseLib/DebugLog.h"
#define new new(_NORMAL_BLOCK, __RelPath(__FILE__), __LINE__)

#else

#ifdef NEDMM
#include "BaseLib/NedMM.h"
#endif
#ifdef FASTMM
#include "BaseLib/FastMM.h"
#endif
#ifdef STATMM
#include "BaseLib/DebugLog.h"
#include "BaseLib/StatMM.h"
#endif
#ifdef RECMM
#include "BaseLib/DebugLog.h"
#include "BaseLib/RecMM.h"
#endif

#ifdef __MM_MAIN__

#include <Windows.h>

static void FunctionPatcher(void* dest, void* address) {
#ifdef _WIN64
#define INST_MAX 6
#else
#define INST_MAX 1
#endif

#define PATCH_SIZE INST_MAX+sizeof(void*)
	// unprotect memory
	DWORD OriProtect;
	VirtualProtect(dest, PATCH_SIZE, PAGE_READWRITE, &OriProtect);

	// Get offset for relative jmp
	size_t offset = (size_t)((char*)address - (char*)dest - (1 + sizeof(int)));

#ifdef _WIN64
	if (((INT64)offset > MAXINT) || ((INT64)offset < MININT)) {
		// write absolute indirect jmp
		((unsigned char*)dest)[0] = 0xff;
		((unsigned char*)dest)[1] = 0x25;
		*(int*)((char*)dest + 2) = 0;
		*(void**)((char*)dest + INST_MAX) = address;
	} else
#endif
	{
		// write relative jmp
		((unsigned char*)dest)[0] = 0xe9;
		*(int*)((char*)dest + 1) = (int)offset;
	}

	// protect memory
	VirtualProtect(dest, PATCH_SIZE, OriProtect, &OriProtect);
}

#define PATCH(target, repl) void target(); FunctionPatcher((void*)(target), (void*)(repl))

#if defined(STATMM) || defined(RECMM)
int __real_tmain(int argc, LPCTSTR argv[], LPCTSTR envp[]);
#endif//STATMM || RECMM

extern "C" {

#ifdef _DLL
#error "Not yet supported!"
#else

#pragma comment(linker, "/entry:__MM_EntryPoint")

#ifdef STATMM

	void __MM_ExitPoint(void);

	int _tmain(int argc, LPCTSTR argv[], LPCTSTR envp[]) {
		// NOTE: The following line is needed to ensure logging resource allocated at exit callback
		LOG(_T("* Memory Manager Statistics Enabled!"));
		atexit(&__MM_ExitPoint);
		return __real_tmain(argc, argv, envp);
	}

#undef _tmain
#define _tmain __real_tmain

#endif//STATMM

#ifdef RECMM

	void __MM_ExitPoint(void);

	int _tmain(int argc, LPCTSTR argv[], LPCTSTR envp[]) {
		// NOTE: The following line is needed to ensure logging resource allocated at exit callback
		LOG(_T("* Memory Operation Recorder Enabled!"));
		atexit(&__MM_ExitPoint);
		return __real_tmain(argc, argv, envp);
	}

#undef _tmain
#define _tmain __real_tmain

#endif//RECMM

	int __MM_EntryPoint(void)
	{
#ifdef _UNICODE
#define tmainCRTStartup wmainCRTStartup
#else
#define tmainCRTStartup mainCRTStartup
#endif

		int tmainCRTStartup();

#if defined(STATMM) || defined(RECMM)
#pragma push_macro("malloc")
#undef malloc
#pragma push_macro("calloc")
#undef calloc
#pragma push_macro("realloc")
#undef realloc
#pragma push_macro("_recalloc")
#undef _recalloc
#endif

#ifdef NEDMM
		PATCH(malloc, _malloc_ned);
		PATCH(_malloc_crt, _malloc_ned);
		PATCH(calloc, _calloc_ned);
		PATCH(_calloc_crt, _calloc_ned);
		PATCH(_msize, _msize_ned);
		PATCH(_heap_alloc, _malloc_ned);
		//#if !defined(STATMM) && !defined(RECMM)
		PATCH(realloc, _realloc_ned);
		PATCH(_realloc_crt, _realloc_ned);
		PATCH(_recalloc, _recalloc_ned);
		PATCH(_recalloc_crt, _recalloc_ned);
		PATCH(free, _free_ned);
		PATCH(_expand, _expand_ned);
		//#endif
#endif

#ifdef FASTMM
		PATCH(malloc, _malloc_fast);
		PATCH(_malloc_crt, _malloc_fast);
		PATCH(calloc, _calloc_fast);
		PATCH(_calloc_crt, _calloc_fast);
		PATCH(_msize, _msize_fast);
		PATCH(_heap_alloc, _malloc_fast);
		//#if !defined(STATMM) && !defined(RECMM)
		PATCH(realloc, _realloc_fast);
		PATCH(_realloc_crt, _realloc_fast);
		PATCH(_recalloc, _recalloc_fast);
		PATCH(_recalloc_crt, _recalloc_fast);
		PATCH(free, _free_fast);
		PATCH(_expand, _expand_fast);
		//#endif
#endif

#ifdef STATMM
		PATCH(malloc, _malloc_stat);
		PATCH(_malloc_crt, _malloc_stat);
		PATCH(calloc, _calloc_stat);
		PATCH(_calloc_crt, _calloc_stat);
		//PATCH(_msize, _msize_stat);
		PATCH(_heap_alloc, _malloc_stat);

		PATCH(realloc, _realloc_stat_h);
		PATCH(_realloc_crt, _realloc_stat_h);
		PATCH(_recalloc, _recalloc_stat_h);
		PATCH(_recalloc_crt, _recalloc_stat_h);
		PATCH(free, _free_stat_h);
		PATCH(_expand, _expand_stat_h);
#endif

#ifdef RECMM
		PATCH(malloc, _malloc_rec);
		PATCH(_malloc_crt, _malloc_rec);
		PATCH(calloc, _calloc_rec);
		PATCH(_calloc_crt, _calloc_rec);
		//PATCH(_msize, _msize_rec);
		PATCH(_heap_alloc, _malloc_rec);

		PATCH(realloc, _realloc_rec);
		PATCH(_realloc_crt, _realloc_rec);
		PATCH(_recalloc, _recalloc_rec);
		PATCH(_recalloc_crt, _recalloc_rec);
		PATCH(free, _free_rec);
		PATCH(_expand, _expand_rec);
#endif

#if defined(STATMM) || defined(RECMM)
#pragma pop_macro("_recalloc")
#pragma pop_macro("realloc")
#pragma pop_macro("calloc")
#pragma pop_macro("malloc")

#if !defined(STATMM_LT) || defined(RECMM)
		__MM_INIT();
#endif
#endif

		return tmainCRTStartup();
	}

#ifdef STATMM
	void __MM_ExitPoint(void) {
		if (_MM_AllocSize() > 0)
			LOG(_T("! Leaked Memory: %.2fKB"), (double)_MM_AllocSize() / BSize_aKB);
#ifndef STATMM_LT
		_MM_DumpDetails();
#else
#endif//STATMM_LT
	}
#endif//STATMM

#ifdef RECMM
	void __MM_ExitPoint(void) {
		__MM_FINIT();
	}
#endif//RECMM
}

#endif//_DLL

#endif//__MM_MAIN__

#endif//_DEBUG
