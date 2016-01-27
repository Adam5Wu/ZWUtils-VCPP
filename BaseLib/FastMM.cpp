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

// [Utilities] FastMM Memory Manager Helpers

#include "FastMM.h"

#include <Windows.h>

extern "C" {
	__declspec(dllimport) void* __fastcall FastGetMem(size_t _Size);
	__declspec(dllimport) size_t __fastcall FastMemSize(void * _Memory);
	__declspec(dllimport) int __fastcall FastFreeMem(void * _Memory);
	__declspec(dllimport) void* __fastcall FastReallocMem(void * _Memory, size_t _NewSize);
	__declspec(dllimport) void* __fastcall FastAllocMem(size_t _Size);

	__declspec(dllimport) void __fastcall InitializeMemoryManager(void);
	__declspec(dllimport) void __fastcall FinalizeMemoryManager(void);

	struct TMemoryManagerUsageSummary {
		size_t AllocatedBytes;
		size_t OverheadBytes;
		double EfficiencyPercentage;
	};

	__declspec(dllimport) void __fastcall GetMemoryManagerUsageSummary(TMemoryManagerUsageSummary * _Info);
	__declspec(dllimport) void __fastcall GetMemoryManagerState(TMemoryManagerState * _Info);
}

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL _malloc_fast(
	_In_ size_t _Size
	) {
	return FastGetMem(_Size);
}

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count*_Size) void *__CRTDECL _calloc_fast(
	_In_ size_t _Count,
	_In_ size_t _Size
	) {
	return FastAllocMem(_Count*_Size);
}

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _realloc_fast(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NewSize
) {
	return FastReallocMem(_Memory, _NewSize);
}

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _recalloc_fast
(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NumOfElements,
_In_ size_t _SizeOfElements
) {
	size_t _OldSize = FastMemSize(_Memory);
	size_t _NewSize = _NumOfElements*_SizeOfElements;
	void* _NewMemory = FastReallocMem(_Memory, _NewSize);
	if (_NewSize > _OldSize) {
		// NOTE: The logic is incomplete, let's hope nothing bad happens... :{
		ZeroMemory(((byte*)_NewMemory) + _OldSize, _NewSize - _OldSize);
	}
	return _NewMemory;
}

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _expand_fast(
	_Pre_notnull_ void * _Memory,
	_In_ size_t _NewSize
	) {
	// Expand is not supported!
	return nullptr;
}

void __CRTDECL _free_fast(
	_Pre_maybenull_ _Post_invalid_ void * _Memory
	) {
	// FastMEM do not handle free of nullptr
	if (_Memory)
		FastFreeMem(_Memory);
}

size_t __CRTDECL _msize_fast(
	_Pre_notnull_ void * _Memory
	) {
	return FastMemSize(_Memory);
}

//size_t __CRTDECL _aligned_msize_fast (
//        _Pre_notnull_ void * _Memory,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset
//        );
//
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL _aligned_malloc_fast(
//        _In_ size_t _Size,
//        _In_ size_t _Alignment
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _aligned_realloc_fast(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NewSize,
//        _In_ size_t _Alignment
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _aligned_recalloc_fast
//(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NumOfElements,
//        _In_ size_t _SizeOfElements,
//        _In_ size_t _Alignment
//);
//
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL _aligned_offset_malloc_fast(
//        _In_ size_t _Size,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _aligned_offset_realloc_fast(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NewSize,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset
//        );
//
//_Success_(return!=0)
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _aligned_offset_recalloc_fast
//(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory,
//        _In_ size_t _NumOfElements,
//        _In_ size_t _SizeOfElements,
//        _In_ size_t _Alignment,
//        _In_ size_t _Offset
//);
//
//void __CRTDECL _aligned_free_fast(
//        _Pre_maybenull_ _Post_invalid_ void * _Memory
//        );

void __CRTDECL _fast_summary(
	_Inout_ size_t * allocated,
	_Inout_ size_t * overhead
	) {
	TMemoryManagerUsageSummary Info;
	GetMemoryManagerUsageSummary(&Info);
	*allocated = Info.AllocatedBytes;
	*overhead = Info.OverheadBytes;
}

void __CRTDECL _fast_details(
	_Inout_ TMemoryManagerState * info
	) {
	GetMemoryManagerState(info);
}

#include "DEBUGLOG.h"

static LPCTSTR _FormatSize(UINT64 Size, LPTSTR Buffer, size_t BufLen) {
	static const int UnitSizes[] = {1024, 1024, 1024, 1024};
	static LPCTSTR UnitNames[] = {_T("KB"), _T("MB"), _T("GB"), _T("TB"), _T("PB")};

	int UnitIdx = 0;
	UINT64 BaseUnit = UnitSizes[UnitIdx];
	while (UnitIdx < ARRAYSIZE(UnitSizes)) {
		if (Size < BaseUnit * UnitSizes[UnitIdx + 1]) break;
		BaseUnit *= UnitSizes[++UnitIdx];
	}

	_stprintf_s(Buffer, BufLen, _T("%.2f%s"), (double)Size / BaseUnit, UnitNames[UnitIdx]);
	return Buffer;
}

void _fast_printdetails(
	_In_ TMemoryManagerState * info
	) {
	LOG(_T("======== Memory Manager Details ========"))
		TCHAR STRBUF_Alloc[16], STRBUF_Reserve[16];
	for (int i = 0; i < SmallBlockTypeCount; i++) {
		if (info->SmallBlockStates[i].Reserved > 0) {
			_FormatSize(info->SmallBlockStates[i].Reserved, STRBUF_Reserve, 16);
			LOG(_T("| SmallBlock < %d bytes: %llu (%s, efficiency %.2f%%)"), info->SmallBlockStates[i].UseableSize,
				(UINT64)info->SmallBlockStates[i].AllocCount, STRBUF_Reserve,
				info->SmallBlockStates[i].AllocCount * info->SmallBlockStates[i].BlockSize * 100 / (double)info->SmallBlockStates[i].Reserved);
		}
	}
	if (info->MediumBlockAlloc > 0) {
		_FormatSize(info->MediumBlockAlloc, STRBUF_Alloc, 16);
		_FormatSize(info->MediumBlockReserve, STRBUF_Reserve, 16);
		LOG(_T("| MediumBlocks: %llu (%s / %s, efficiency %.2f%%)"), (UINT64)info->MediumBlockCount,
			STRBUF_Alloc, STRBUF_Reserve, info->MediumBlockAlloc * 100 / (double)info->MediumBlockReserve);
	}
	if (info->LargeBlockAlloc > 0) {
		_FormatSize(info->LargeBlockAlloc, STRBUF_Alloc, 16);
		_FormatSize(info->LargeBlockReserve, STRBUF_Reserve, 16);
		LOG(_T("| LargeBlocks: %llu (%s / %s, efficiency %.2f%%)"), (UINT64)info->LargeBlockCount,
			STRBUF_Alloc, STRBUF_Reserve, info->LargeBlockAlloc * 100 / (double)info->LargeBlockReserve);
	}
}

void _fast_comparedetails(
	_In_ TMemoryManagerState * from,
	_In_ TMemoryManagerState * to
	) {
	LOG(_T("======== Memory Manager Detail Changes ========"))
		TCHAR STRBUF_Alloc[16], STRBUF_Reserve[16];
	for (int i = 0; i < SmallBlockTypeCount; i++) {
		if ((from->SmallBlockStates[i].AllocCount != to->SmallBlockStates[i].AllocCount) ||
			(from->SmallBlockStates[i].Reserved != to->SmallBlockStates[i].Reserved)) {
			STRBUF_Reserve[0] = (from->SmallBlockStates[i].Reserved > to->SmallBlockStates[i].Reserved) ? _T('-') : _T('+');
			_FormatSize(abs((INT64)(from->SmallBlockStates[i].Reserved - to->SmallBlockStates[i].Reserved)), STRBUF_Reserve + 1, 15);
			LOG(_T("| SmallBlock < %d bytes: %+lld (%s)"), to->SmallBlockStates[i].UseableSize,
				(INT64)to->SmallBlockStates[i].AllocCount - from->SmallBlockStates[i].AllocCount, STRBUF_Reserve);
		}
	}
	if ((from->MediumBlockAlloc != to->MediumBlockAlloc) || (from->MediumBlockReserve != to->MediumBlockReserve)) {
		STRBUF_Alloc[0] = (from->MediumBlockAlloc > to->MediumBlockAlloc) ? _T('-') : _T('+');
		_FormatSize(abs((INT64)(from->MediumBlockAlloc - to->MediumBlockAlloc)), STRBUF_Alloc + 1, 15);
		STRBUF_Reserve[0] = (from->MediumBlockReserve > to->MediumBlockReserve) ? _T('-') : _T('+');
		_FormatSize(abs((INT64)(from->MediumBlockReserve - to->MediumBlockReserve)), STRBUF_Reserve + 1, 15);
		LOG(_T("| MediumBlocks: %+lld (%s / %s)"), (INT64)to->MediumBlockCount - from->MediumBlockCount, STRBUF_Alloc, STRBUF_Reserve);
	}
	if ((from->LargeBlockAlloc != to->LargeBlockAlloc) || (from->LargeBlockReserve != to->LargeBlockReserve)) {
		STRBUF_Alloc[0] = (from->LargeBlockAlloc > to->LargeBlockAlloc) ? _T('-') : _T('+');
		_FormatSize(abs((INT64)(from->LargeBlockAlloc - to->LargeBlockAlloc)), STRBUF_Alloc + 1, 15);
		STRBUF_Reserve[0] = (from->LargeBlockReserve > to->LargeBlockReserve) ? _T('-') : _T('+');
		_FormatSize(abs((INT64)(from->LargeBlockReserve - to->LargeBlockReserve)), STRBUF_Reserve + 1, 15);
		LOG(_T("| LargeBlocks: %+lld (%s / %s)"), (INT64)to->LargeBlockCount - from->LargeBlockCount, STRBUF_Alloc, STRBUF_Reserve);
	}
}
