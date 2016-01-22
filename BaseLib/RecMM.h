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
 * @brief Memory Operation Recorder
 * @author Zhenyu Wu
 * @date Jul 14, 2014: Initial implementation
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef RecMM_H
#define RecMM_H

#include <sal.h>
#include <crtdefs.h>

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL _malloc_rec(
	_In_ size_t _Size
	);

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count*_Size) void *__CRTDECL _calloc_rec(
	_In_ size_t _Count,
	_In_ size_t _Size
	);

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _expand_rec(
	_Pre_notnull_ void * _Memory,
	_In_ size_t _NewSize
	);

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _realloc_rec(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NewSize
);

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _recalloc_rec(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NumOfElements,
_In_ size_t _SizeOfElements
);

void __CRTDECL _free_rec(
	_Pre_maybenull_ _Post_invalid_ void * _Memory
	);

//size_t __CRTDECL _msize_rec(
//	      _Pre_notnull_ void * _Memory
//	      );
//
//size_t __CRTDECL _aligned_msize_rec (
//        _Pre_notnull_ void * _Memory,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset
//        );
//
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL _aligned_malloc_rec(
//        _In_ size_t _Size,
//        _In_ size_t _Alignment
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _aligned_realloc_rec(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NewSize,
//        _In_ size_t _Alignment
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _aligned_recalloc_rec(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NumOfElements,
//        _In_ size_t _SizeOfElements,
//        _In_ size_t _Alignment
//);
//
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL _aligned_offset_malloc_rec(
//        _In_ size_t _Size,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _aligned_offset_realloc_rec(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NewSize,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _aligned_offset_recalloc_rec(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NumOfElements,
//        _In_ size_t _SizeOfElements,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset
//);
//
//void __CRTDECL _aligned_free_rec(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory
//        );

//_Check_return_ _Ret_maybenull_z_ char *__CRTDECL _strdup_rec(
//	_In_opt_z_ const char * _Str
//	);
//
//_Check_return_ _Ret_maybenull_z_ TCHAR *__CRTDECL _wcsdup_rec(
//	_In_opt_z_ const wchar_t * _Str
//	);

//_Check_return_ _Ret_maybenull_z_ char *__CRTDECL _tempnam_rec(
//        _In_opt_z_ const TCHAR * _DirName,
//        _In_opt_z_ const TCHAR * _FilePrefix
//        );
//
//_Check_return_ _Ret_maybenull_z_ TCHAR *__CRTDECL _wtempnam_rec(
//        _In_opt_z_ const TCHAR * _DirName,
//        _In_opt_z_ const TCHAR * _FilePrefix
//        );
//
//_Check_return_ _Ret_maybenull_z_ char *__CRTDECL _fullpath_rec(
//        _Out_writes_opt_z_(_SizeInBytes) char * _FullPath,
//        _In_z_ const TCHAR * _Path,
//        _In_ size_t _SizeInBytes
//        );
//
//_Check_return_ _Ret_maybenull_z_ TCHAR *__CRTDECL _wfullpath_rec(
//        _Out_writes_opt_z_(_SizeInWords) TCHAR * _FullPath,
//        _In_z_ const TCHAR * _Path,
//        _In_ size_t _SizeInWords
//        );
//
//_Check_return_ _Ret_maybenull_z_ char *__CRTDECL _getcwd_rec(
//        _Out_writes_opt_z_(_SizeInBytes) char * _DstBuf,
//        _In_ int _SizeInBytes
//        );
//
//_Check_return_ _Ret_maybenull_z_ TCHAR *__CRTDECL _wgetcwd_rec(
//        _Out_writes_opt_z_(_SizeInWords) TCHAR * _DstBuf,
//        _In_ int _SizeInWords
//        );
//
//_Check_return_ _Ret_maybenull_z_ char *__CRTDECL _getdcwd_rec(
//        _In_ int _Drive,
//        _Out_writes_opt_z_(_SizeInBytes) char * _DstBuf,
//        _In_ int _SizeInBytes
//        );
//
//_Check_return_ _Ret_maybenull_z_ TCHAR *__CRTDECL _wgetdcwd_rec(
//        _In_ int _Drive,
//        _Out_writes_opt_z_(_SizeInWords) TCHAR * _DstBuf,
//        _In_ int _SizeInWords
//        );
//
//_Check_return_ _Ret_maybenull_z_ char *__CRTDECL _getdcwd_lk_rec(
//        _In_ int _Drive,
//        _Out_writes_opt_z_(_SizeInBytes) char * _DstBuf,
//        _In_ int _SizeInBytes
//        );
//
//_Check_return_ _Ret_maybenull_z_ TCHAR *__CRTDECL _wgetdcwd_lk_rec(
//        _In_ int _Drive,
//        _Out_writes_opt_z_(_SizeInWords) TCHAR * _DstBuf,
//        _In_ int _SizeInWords
//        );
//
//_Check_return_wat_ errno_t __CRTDECL _dupenv_s_rec(
//        _Outptr_result_buffer_maybenull_(*_PBufferSizeInBytes) _Outptr_result_z_ char ** _PBuffer,
//        _Out_opt_ size_t * _PBufferSizeInBytes,
//        _In_z_ const TCHAR * _VarName
//        );
//
//_Check_return_wat_ errno_t __CRTDECL _wdupenv_s_rec(
//        _Outptr_result_buffer_maybenull_(*_PBufferSizeInWords) _Outptr_result_z_ TCHAR ** _PBuffer,
//        _Out_opt_ size_t * _PBufferSizeInWords,
//        _In_z_ const TCHAR * _VarName
//        );

#ifndef _STATMM_IMPL

#ifdef _DEBUG
#pragma message ("Warning: This module only applies in non-debug release mode!")
#endif

#include <malloc.h>
#include <string.h>

#define   malloc(s)             _malloc_rec(s)
#define   calloc(c, s)          _calloc_rec(c, s)
//#define   realloc(p, s)         _realloc_rec(p, s)
//#define   _recalloc(p, c, s)    _recalloc_rec(p, c, s)
//#define   _expand(p, s)         _expand_rec(p, s)
//#define   free(p)               _free_rec(p)
//#define   _msize(p)             _msize_rec(p)
//#define   _aligned_msize(p, a, o)                   _aligned_msize_rec(p, a, o)
//#define   _aligned_malloc(s, a)                     _aligned_malloc_rec(s, a)
//#define   _aligned_realloc(p, s, a)                 _aligned_realloc_rec(p, s, a)
//#define   _aligned_recalloc(p, c, s, a)             _aligned_recalloc_rec(p, c, s, a)
//#define   _aligned_offset_malloc(s, a, o)           _aligned_offset_malloc_rec(s, a, o)
//#define   _aligned_offset_realloc(p, s, a, o)       _aligned_offset_realloc_rec(p, s, a, o)
//#define   _aligned_offset_recalloc(p, c, s, a, o)   _aligned_offset_recalloc_rec(p, c, s, a, o)
//#define   _aligned_free(p)  _aligned_free_rec(p)

#undef _malloca
#define   _malloca(s)        _malloc_rec(s)
//#define   _freea(p)          _free_rec(p)

//#define   _strdup(s)         _strdup_rec(s)
//#define   _wcsdup(s)         _wcsdup_rec(s)
//#define   _mbsdup(s)         _strdup_rec(s)
//#define   _tempnam(s1, s2)   _tempnam_rec(s1, s2)
//#define   _wtempnam(s1, s2)  _wtempnam_rec(s1, s2)
//#define   _fullpath(s1, s2, le)     _fullpath_rec(s1, s2, le)
//#define   _wfullpath(s1, s2, le)    _wfullpath_rec(s1, s2, le)
//#define   _getcwd(s, le)      _getcwd_rec(s, le)
//#define   _wgetcwd(s, le)     _wgetcwd_rec(s, le)
//#define   _getdcwd(d, s, le)  _getdcwd_rec(d, s, le)
//#define   _wgetdcwd(d, s, le) _wgetdcwd_rec(d, s, le)
//#define   _dupenv_s(ps1, size, s2)      _dupenv_s_rec(ps1, size, s2)
//#define   _wdupenv_s(ps1, size, s2)     _wdupenv_s_rec(ps1, size, s2)

#if !__STDC__
//#define   strdup(s)          _strdup_rec(s)
//#define   wcsdup(s)          _wcsdup_rec(s)
//#define   tempnam(s1, s2)    _tempnam_rec(s1, s2)
//#define   getcwd(s, le)      _getcwd_rec(s, le)
#endif  /* !__STDC__ */

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

}

#endif//_STATMM_IMPL

void __MM_INIT(void);
void __MM_FINIT(void);

#endif//RecMM_H
