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

// [Utilities] Nedmalloc Memory Manager Helpers
// Zhenyu Wu @ Jan 24, 2014: Initial implementation

#include "NedMM.h"

#include "../../nedmalloc/nedmalloc.h"

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL _malloc_ned(
	_In_ size_t _Size
	) {
	return nedalloc::nedmalloc(_Size);
}

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count*_Size) void *__CRTDECL _calloc_ned(
	_In_ size_t _Count,
	_In_ size_t _Size
	) {
	return nedalloc::nedcalloc(_Count, _Size);
}

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _realloc_ned(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NewSize
) {
	return nedalloc::nedrealloc(_Memory, _NewSize);
}

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _recalloc_ned
(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NumOfElements,
_In_ size_t _SizeOfElements
) {
	return nedalloc::nedrealloc2(_Memory, _NumOfElements*_SizeOfElements, 0, M2_ZERO_MEMORY | M2_RESERVE_MULT(8));
}

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _expand_ned(
	_Pre_notnull_ void * _Memory,
	_In_ size_t _NewSize
	) {
	return nedalloc::nedrealloc2(_Memory, _NewSize, 0, M2_PREVENT_MOVE | M2_RESERVE_MULT(8));
}

void __CRTDECL _free_ned(
	_Pre_maybenull_ _Post_invalid_ void * _Memory
	) {
	nedalloc::nedfree(_Memory);
}

size_t __CRTDECL _msize_ned(
	_Pre_notnull_ void * _Memory
	) {
	return nedalloc::nedmemsize(_Memory);
}

//size_t __CRTDECL _aligned_msize_ned (
//        _Pre_notnull_ void * _Memory,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset
//        );
//
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL _aligned_malloc_ned(
//        _In_ size_t _Size,
//        _In_ size_t _Alignment
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _aligned_realloc_ned(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NewSize,
//        _In_ size_t _Alignment
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _aligned_recalloc_ned
//(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NumOfElements,
//        _In_ size_t _SizeOfElements,
//        _In_ size_t _Alignment
//);
//
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL _aligned_offset_malloc_ned(
//        _In_ size_t _Size,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _aligned_offset_realloc_ned(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NewSize,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _aligned_offset_recalloc_ned
//(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NumOfElements,
//        _In_ size_t _SizeOfElements,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset
//);
//
//void __CRTDECL _aligned_free_ned(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory
//        );

void __CRTDECL _ned_tune(
	_In_ int flags
	) {
	if (flags & MM_TRIM_THREADCACHE)
		nedalloc::nedtrimthreadcache(nullptr, false);
	if (flags & MM_DROP_THREADCACHE)
		nedalloc::neddisablethreadcache(nullptr);
}

void __CRTDECL _ned_stat(
	_Inout_ size_t * allocated,
	_Inout_ size_t * free
	) {
	struct nedmallinfo _mm_info = nedalloc::nedmallinfo();
	*allocated = _mm_info.uordblks;
	*free = _mm_info.fordblks;
}

int __CRTDECL _ned_trim(
	_In_ size_t reserve
	) {
	return nedalloc::nedmalloc_trim(reserve);
}
