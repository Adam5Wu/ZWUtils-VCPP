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
 * @brief Statistics Memory Manager Helpers
 * @author Zhenyu Wu
 * @date Jan 16, 2014: Initial implementation
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef StatMM_H
#define StatMM_H

#include <sal.h>
#include <crtdefs.h>
#include <tchar.h>

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL _malloc_stat(
#ifndef STATMM_LT
	_In_ size_t _Size,
	_In_opt_z_ const TCHAR * _Filename,
	_In_ int _LineNumber
#else
	_In_ size_t _Size
#endif
	);

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count*_Size) void *__CRTDECL _calloc_stat(
#ifndef STATMM_LT
	_In_ size_t _Count,
	_In_ size_t _Size,
	_In_opt_z_ const TCHAR * _Filename,
	_In_ int _LineNumber
#else
	_In_ size_t _Count,
	_In_ size_t _Size
#endif
	);

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _realloc_stat_h(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NewSize
);

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _recalloc_stat_h(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NumOfElements,
_In_ size_t _SizeOfElements
);

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _expand_stat_h(
	_Pre_notnull_ void * _Memory,
	_In_ size_t _NewSize
	);

#ifndef STATMM_LT

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _realloc_stat(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NewSize,
_In_opt_z_ const TCHAR * _Filename,
_In_ int _LineNumber
);

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _recalloc_stat(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NumOfElements,
_In_ size_t _SizeOfElements,
_In_opt_z_ const TCHAR * _Filename,
_In_ int _LineNumber
);

#endif

void __CRTDECL _free_stat_h(
	_Pre_maybenull_ _Post_invalid_ void * _Memory
	);

//size_t __CRTDECL _msize_stat(
//	      _Pre_notnull_ void * _Memory
//	      );
//
//size_t __CRTDECL _aligned_msize_stat (
//        _Pre_notnull_ void * _Memory,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset
//        );
//
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL _aligned_malloc_stat(
//        _In_ size_t _Size,
//        _In_ size_t _Alignment,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _aligned_realloc_stat(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NewSize,
//        _In_ size_t _Alignment,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _aligned_recalloc_stat(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NumOfElements,
//        _In_ size_t _SizeOfElements,
//        _In_ size_t _Alignment,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//);
//
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL _aligned_offset_malloc_stat(
//        _In_ size_t _Size,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _aligned_offset_realloc_stat(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NewSize,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _aligned_offset_recalloc_stat(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NumOfElements,
//        _In_ size_t _SizeOfElements,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//);
//
//void __CRTDECL _aligned_free_stat(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory
//        );

//_Check_return_ _Ret_maybenull_z_ char *__CRTDECL _strdup_stat(
//	_In_opt_z_ const char * _Str,
//#ifndef STATMM_LT
//	_In_opt_z_ const TCHAR * _Filename,
//	_In_ int _LineNumber
//#endif
//	);
//
//_Check_return_ _Ret_maybenull_z_ TCHAR *__CRTDECL _wcsdup_stat(
//	_In_opt_z_ const wchar_t * _Str,
//#ifndef STATMM_LT
//	_In_opt_z_ const TCHAR * _Filename,
//	_In_ int _LineNumber
//#endif
//	);

//_Check_return_ _Ret_maybenull_z_ char *__CRTDECL _tempnam_stat(
//        _In_opt_z_ const TCHAR * _DirName,
//        _In_opt_z_ const TCHAR * _FilePrefix,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );
//
//_Check_return_ _Ret_maybenull_z_ TCHAR *__CRTDECL _wtempnam_stat(
//        _In_opt_z_ const TCHAR * _DirName,
//        _In_opt_z_ const TCHAR * _FilePrefix,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );
//
//_Check_return_ _Ret_maybenull_z_ char *__CRTDECL _fullpath_stat(
//        _Out_writes_opt_z_(_SizeInBytes) char * _FullPath,
//        _In_z_ const TCHAR * _Path,
//        _In_ size_t _SizeInBytes,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );
//
//_Check_return_ _Ret_maybenull_z_ TCHAR *__CRTDECL _wfullpath_stat(
//        _Out_writes_opt_z_(_SizeInWords) TCHAR * _FullPath,
//        _In_z_ const TCHAR * _Path,
//        _In_ size_t _SizeInWords,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );
//
//_Check_return_ _Ret_maybenull_z_ char *__CRTDECL _getcwd_stat(
//        _Out_writes_opt_z_(_SizeInBytes) char * _DstBuf,
//        _In_ int _SizeInBytes,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );
//
//_Check_return_ _Ret_maybenull_z_ TCHAR *__CRTDECL _wgetcwd_stat(
//        _Out_writes_opt_z_(_SizeInWords) TCHAR * _DstBuf,
//        _In_ int _SizeInWords,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );
//
//_Check_return_ _Ret_maybenull_z_ char *__CRTDECL _getdcwd_stat(
//        _In_ int _Drive,
//        _Out_writes_opt_z_(_SizeInBytes) char * _DstBuf,
//        _In_ int _SizeInBytes,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );
//
//_Check_return_ _Ret_maybenull_z_ TCHAR *__CRTDECL _wgetdcwd_stat(
//        _In_ int _Drive,
//        _Out_writes_opt_z_(_SizeInWords) TCHAR * _DstBuf,
//        _In_ int _SizeInWords,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );
//
//_Check_return_ _Ret_maybenull_z_ char *__CRTDECL _getdcwd_lk_stat(
//        _In_ int _Drive,
//        _Out_writes_opt_z_(_SizeInBytes) char * _DstBuf,
//        _In_ int _SizeInBytes,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );
//
//_Check_return_ _Ret_maybenull_z_ TCHAR *__CRTDECL _wgetdcwd_lk_stat(
//        _In_ int _Drive,
//        _Out_writes_opt_z_(_SizeInWords) TCHAR * _DstBuf,
//        _In_ int _SizeInWords,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );
//
//_Check_return_wat_ errno_t __CRTDECL _dupenv_s_stat(
//        _Outptr_result_buffer_maybenull_(*_PBufferSizeInBytes) _Outptr_result_z_ char ** _PBuffer,
//        _Out_opt_ size_t * _PBufferSizeInBytes,
//        _In_z_ const TCHAR * _VarName,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );
//
//_Check_return_wat_ errno_t __CRTDECL _wdupenv_s_stat(
//        _Outptr_result_buffer_maybenull_(*_PBufferSizeInWords) _Outptr_result_z_ TCHAR ** _PBuffer,
//        _Out_opt_ size_t * _PBufferSizeInWords,
//        _In_z_ const TCHAR * _VarName,
//#ifndef STATMM_LT
//        _In_opt_z_ const TCHAR * _Filename,
//        _In_ int _LineNumber
//#endif
//        );

#ifndef _STATMM_IMPL

#ifdef _DEBUG
#pragma message ("Warning: This module only applies in non-debug release mode!")
#endif

#include <malloc.h>
#include <string.h>

#ifndef STATMM_LT

#include "BaseLib/DebugLog.h"

#define   malloc(s)             _malloc_stat(s, __REL_FILE__, __LINE__)
#define   calloc(c, s)          _calloc_stat(c, s, __REL_FILE__, __LINE__)
#define   realloc(p, s)         _realloc_stat(p, s, __REL_FILE__, __LINE__)
#define   _recalloc(p, c, s)    _recalloc_stat(p, c, s, __REL_FILE__, __LINE__)
//#define   _expand(p, s)         _expand_stat(p, s, __REL_FILE__, __LINE__)
//#define   free(p)               _free_stat(p)
//#define   _msize(p)             _msize_stat(p)
//#define   _aligned_msize(p, a, o)                   _aligned_msize_stat(p, a, o)
//#define   _aligned_malloc(s, a)                     _aligned_malloc_stat(s, a, __REL_FILE__, __LINE__)
//#define   _aligned_realloc(p, s, a)                 _aligned_realloc_stat(p, s, a, __REL_FILE__, __LINE__)
//#define   _aligned_recalloc(p, c, s, a)             _aligned_recalloc_stat(p, c, s, a, __REL_FILE__, __LINE__)
//#define   _aligned_offset_malloc(s, a, o)           _aligned_offset_malloc_stat(s, a, o, __REL_FILE__, __LINE__)
//#define   _aligned_offset_realloc(p, s, a, o)       _aligned_offset_realloc_stat(p, s, a, o, __REL_FILE__, __LINE__)
//#define   _aligned_offset_recalloc(p, c, s, a, o)   _aligned_offset_recalloc_stat(p, c, s, a, o, __REL_FILE__, __LINE__)
//#define   _aligned_free(p)  _aligned_free_stat(p)

#undef _malloca
#define   _malloca(s)        _malloc_stat(s, __REL_FILE__, __LINE__)
//#define   _freea(p)          _free_stat(p)

//#define   _strdup(s)         _strdup_stat(s, __REL_FILE__, __LINE__)
//#define   _wcsdup(s)         _wcsdup_stat(s, __REL_FILE__, __LINE__)
//#define   _mbsdup(s)         _strdup_stat(s, __REL_FILE__, __LINE__)
//#define   _tempnam(s1, s2)   _tempnam_stat(s1, s2, __REL_FILE__, __LINE__)
//#define   _wtempnam(s1, s2)  _wtempnam_stat(s1, s2, __REL_FILE__, __LINE__)
//#define   _fullpath(s1, s2, le)     _fullpath_stat(s1, s2, le, __REL_FILE__, __LINE__)
//#define   _wfullpath(s1, s2, le)    _wfullpath_stat(s1, s2, le, __REL_FILE__, __LINE__)
//#define   _getcwd(s, le)      _getcwd_stat(s, le, __REL_FILE__, __LINE__)
//#define   _wgetcwd(s, le)     _wgetcwd_stat(s, le, __REL_FILE__, __LINE__)
//#define   _getdcwd(d, s, le)  _getdcwd_stat(d, s, le, __REL_FILE__, __LINE__)
//#define   _wgetdcwd(d, s, le) _wgetdcwd_stat(d, s, le, __REL_FILE__, __LINE__)
//#define   _dupenv_s(ps1, size, s2)      _dupenv_s_stat(ps1, size, s2, __REL_FILE__, __LINE__)
//#define   _wdupenv_s(ps1, size, s2)     _wdupenv_s_stat(ps1, size, s2, __REL_FILE__, __LINE__)

#if !__STDC__
//#define   strdup(s)          _strdup_stat(s, __REL_FILE__, __LINE__)
//#define   wcsdup(s)          _wcsdup_stat(s, __REL_FILE__, __LINE__)
//#define   tempnam(s1, s2)    _tempnam_stat(s1, s2, __REL_FILE__, __LINE__)
//#define   getcwd(s, le)      _getcwd_stat(s, le, __REL_FILE__, __LINE__)
#endif  /* !__STDC__ */

#else//STATMM_LT

#define   malloc(s)             _malloc_stat(s)
#define   calloc(c, s)          _calloc_stat(c, s)
//#define   realloc(p, s)         _realloc_stat(p, s)
//#define   _recalloc(p, c, s)    _recalloc_stat(p, c, s)
//#define   _expand(p, s)         _expand_stat(p, s)
//#define   free(p)               _free_stat(p)
//#define   _msize(p)             _msize_stat(p)
//#define   _aligned_msize(p, a, o)                   _aligned_msize_stat(p, a, o)
//#define   _aligned_malloc(s, a)                     _aligned_malloc_stat(s, a)
//#define   _aligned_realloc(p, s, a)                 _aligned_realloc_stat(p, s, a)
//#define   _aligned_recalloc(p, c, s, a)             _aligned_recalloc_stat(p, c, s, a)
//#define   _aligned_offset_malloc(s, a, o)           _aligned_offset_malloc_stat(s, a, o)
//#define   _aligned_offset_realloc(p, s, a, o)       _aligned_offset_realloc_stat(p, s, a, o)
//#define   _aligned_offset_recalloc(p, c, s, a, o)   _aligned_offset_recalloc_stat(p, c, s, a, o)
//#define   _aligned_free(p)  _aligned_free_stat(p)

#undef _malloca
#define   _malloca(s)        _malloc_stat(s)
//#define   _freea(p)          _free_stat(p)

//#define   _strdup(s)         _strdup_stat(s)
//#define   _wcsdup(s)         _wcsdup_stat(s)
//#define   _mbsdup(s)         _strdup_stat(s)
//#define   _tempnam(s1, s2)   _tempnam_stat(s1, s2)
//#define   _wtempnam(s1, s2)  _wtempnam_stat(s1, s2)
//#define   _fullpath(s1, s2, le)     _fullpath_stat(s1, s2, le)
//#define   _wfullpath(s1, s2, le)    _wfullpath_stat(s1, s2, le)
//#define   _getcwd(s, le)      _getcwd_stat(s, le)
//#define   _wgetcwd(s, le)     _wgetcwd_stat(s, le)
//#define   _getdcwd(d, s, le)  _getdcwd_stat(d, s, le)
//#define   _wgetdcwd(d, s, le) _wgetdcwd_stat(d, s, le)
//#define   _dupenv_s(ps1, size, s2)      _dupenv_s_stat(ps1, size, s2)
//#define   _wdupenv_s(ps1, size, s2)     _wdupenv_s_stat(ps1, size, s2)

#if !__STDC__
//#define   strdup(s)          _strdup_stat(s)
//#define   wcsdup(s)          _wcsdup_stat(s)
//#define   tempnam(s1, s2)    _tempnam_stat(s1, s2)
//#define   getcwd(s, le)      _getcwd_stat(s, le)
#endif  /* !__STDC__ */

#endif//STATMM_LT

extern "C++" {

#include <new>

	// Throw version
#pragma warning (suppress: 4985)
	_Ret_notnull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new(size_t _Size) _THROW1(_STD bad_alloc);
	_Ret_notnull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new[](size_t _Size) _THROW1(_STD bad_alloc);
	void __CRTDECL operator delete(void * _P) _THROW0();
	void __CRTDECL operator delete[](void * _P) _THROW0();

	// No-throw version
	_Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new(
		size_t _Size,
		const _STD nothrow_t&
		) _THROW0();

	_Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new[](
		size_t _Size,
		const _STD nothrow_t&
		) _THROW0();

	void __CRTDECL operator delete(
		void * _P,
		const _STD nothrow_t&
		) _THROW0();

	void __CRTDECL operator delete[](
		void * _P,
		const _STD nothrow_t&
		) _THROW0();

#ifndef STATMM_LT

	// Throw version
#pragma warning (suppress: 4985)
	_Ret_notnull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new(
		size_t _Size,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW1(_STD bad_alloc);

	_Ret_notnull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new[](
		size_t _Size,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW1(_STD bad_alloc);

	void __CRTDECL operator delete(
		void * _P,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW0();

	void __CRTDECL operator delete[](
		void * _P,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW0();

	// No-throw version
	_Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new(
		size_t _Size,
		const _STD nothrow_t&,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW0();

	_Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new[](
		size_t _Size,
		const _STD nothrow_t&,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW0();

	void __CRTDECL operator delete(
		void * _P,
		const _STD nothrow_t&,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW0();

	void __CRTDECL operator delete[](
		void * _P,
		const _STD nothrow_t&,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW0();

#include "BaseLib/DebugLog.h"

#define new new(__REL_FILE__, __LINE__)

#endif//STATMM_LT
}

#endif//_STATMM_IMPL

void __MM_INIT(void);

size_t _MM_AllocSize(void);

#ifndef STATMM_LT

struct _MM_Stats {
	size_t __Alloc_Size = 0;
	size_t __Alloc_Count = 0;
	size_t __Dealloc_Count = 0;
	size_t __Alloc_Cumulative = 0;
	size_t __Dealloc_Cumulative = 0;
};
_MM_Stats _MM_AllocStats(void);
void _MM_DumpDetails(void);

#endif//STATMM_LT

#endif//StatMM_H
