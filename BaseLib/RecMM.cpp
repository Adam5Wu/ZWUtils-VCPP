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

// [Utilities] Memory Operation Recorder

#ifdef RECMM

#define _RECMM_IMPL

#include "RecMM.h"

#ifdef NEDMM

#include "NedMM.h"

#define _malloc_rec_l _malloc_ned
#define _msize_rec_l _msize_ned
#define _calloc_rec_l _calloc_ned
#define _realloc_rec_l _realloc_ned
#define _recalloc_rec_l _recalloc_ned
#define _expand_rec_l _expand_ned
#define _free_rec_l _free_ned

#endif

#ifdef FASTMM

#include "FastMM.h"

#define _malloc_rec_l _malloc_fast
#define _msize_rec_l _msize_fast
#define _calloc_rec_l _calloc_fast
#define _realloc_rec_l _realloc_fast
#define _recalloc_rec_l _recalloc_fast
#define _expand_rec_l _expand_fast
#define _free_rec_l _free_fast

#endif

#include <vector>
#include <allocators>
#include <stdio.h>

#include "BaseLib/DebugLog.h"
#include "ThreadLib/SyncObjs.h"

static TLockableCS* StatLock = nullptr;

using namespace std;

struct __MemOp_Entry {
	UINT64 _TS;
	void* _Addr;
	size_t _Size;
};

// TEMPLATE CLASS allocator
template<class _Ty>
class malloc_allocator
	: public _Allocator_base < _Ty > {	// generic allocator for objects of class _Ty
public:
	typedef malloc_allocator<_Ty> other;

	typedef _Allocator_base<_Ty> _Mybase;
	typedef typename _Mybase::value_type value_type;

	typedef value_type *pointer;
	typedef const value_type *const_pointer;
	typedef void *void_pointer;
	typedef const void *const_void_pointer;

	typedef value_type& reference;
	typedef const value_type& const_reference;

	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	typedef false_type propagate_on_container_copy_assignment;
	typedef false_type propagate_on_container_move_assignment;
	typedef false_type propagate_on_container_swap;

	malloc_allocator<_Ty> select_on_container_copy_construction() const
	{	// return this allocator
		return (*this);
	}

	template<class _Other>
	struct rebind {	// convert this type to allocator<_Other>
		typedef malloc_allocator<_Other> other;
	};

	pointer address(reference _Val) const _NOEXCEPT
	{	// return address of mutable _Val
		return (_STD addressof(_Val));
	}

		const_pointer address(const_reference _Val) const _NOEXCEPT
	{	// return address of nonmutable _Val
		return (_STD addressof(_Val));
	}

		malloc_allocator() _THROW0()
	{	// construct default allocator (do nothing)
	}

	malloc_allocator(const malloc_allocator<_Ty>&) _THROW0()
	{	// construct by copying (do nothing)
	}

	template<class _Other>
	malloc_allocator(const malloc_allocator<_Other>&) _THROW0()
	{	// construct from a related allocator (do nothing)
	}

	template<class _Other>
	malloc_allocator<_Ty>& operator=(const malloc_allocator<_Other>&)
	{	// assign from a related allocator (do nothing)
		return (*this);
	}

	void deallocate(pointer _Ptr, size_type)
	{	// deallocate object at _Ptr, ignore size
		_free_rec_l(_Ptr);
	}

	pointer allocate(size_type _Count)
	{	// allocate array of _Count elements
		if (void*_ret = _malloc_rec_l(sizeof(value_type)* _Count))
			return (pointer)_ret;
		if (_Count) throw bad_alloc();
		return nullptr;
	}

	pointer allocate(size_type _Count, const void *)
	{	// allocate array of _Count elements, ignore hint
		return (allocate(_Count));
	}

	void construct(_Ty *_Ptr)
	{	// default construct object at _Ptr
		::new ((void *)_Ptr) _Ty();
	}

	void construct(_Ty *_Ptr, const _Ty& _Val)
	{	// construct object at _Ptr with value _Val
		::new ((void *)_Ptr) _Ty(_Val);
	}

	template<class _Objty,
	class... _Types>
		void construct(_Objty *_Ptr, _Types&&... _Args)
	{	// construct _Objty(_Types...) at _Ptr
		::new ((void *)_Ptr) _Objty(_STD forward<_Types>(_Args)...);
	}


	template<class _Uty>
	void destroy(_Uty *_Ptr)
	{	// destroy object at _Ptr
		_Ptr->~_Uty();
	}

	size_t max_size() const _THROW0()
	{	// estimate maximum array size
		return ((size_t)(-1) / sizeof(_Ty));
	}
};

#define TMemOp_Log vector<__MemOp_Entry, malloc_allocator<__MemOp_Entry>>
static TMemOp_Log* __MemOp_Log = nullptr;
static FILE* __LogFile = nullptr;
static const DWORD __LogFlushCnt = 1024 * 1024;
static const LPCTSTR __LogFileName = _T("MemOps.log");

void InsertLog(void* Addr, size_t Size) {
	Flatten_FILETIME CurTS;
	GetSystemTimeAsFileTime(&CurTS.FileTime);
	__MemOp_Log->push_back({CurTS.U64, Addr, Size});
}

void CheckFlushLog(DWORD Threshold) {
	if (__MemOp_Log->size() > Threshold) {
		if (__LogFile == nullptr) {
			// Open log file
			LOGVV(_T("* Opening memory operation log file..."));
			errno_t ErrCode = _tfopen_s(&__LogFile, __LogFileName, _T("w+b"));
			if (ErrCode != 0)
				FAIL(_T("WARNING: Unable to open memory operation log file (%d)"), ErrCode);
			// Identify platform address size
			unsigned char const PtrWidth = sizeof(void*);
			fwrite(&PtrWidth, sizeof(unsigned char), 1, __LogFile);
#ifdef _WIN64
			size_t const ByteOrder = 0x1020304050607080L;
#else
			size_t const ByteOrder = 0x10203040L;
#endif
			fwrite(&ByteOrder, sizeof(size_t), 1, __LogFile);
		}
		LOGVV(_T("* Flushing %u memory operation log entries..."), __MemOp_Log->size());
		fwrite(__MemOp_Log->data(), sizeof(__MemOp_Entry), __MemOp_Log->size(), __LogFile);
		__MemOp_Log->clear();
	}
}

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL _malloc_rec(
	_In_ size_t _Size
	) {
	if (void *_ret = _malloc_rec_l(_Size)) {
		size_t _size = _msize_rec_l(_ret);
		auto Lock = StatLock->SyncLock();
		InsertLog(_ret, _Size);
		return _ret;
	}
	return nullptr;
}

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count*_Size) void *__CRTDECL _calloc_rec(
	_In_ size_t _Count,
	_In_ size_t _Size
	) {
	if (void *_ret = _calloc_rec_l(_Count, _Size)) {
		size_t _size = _msize_rec_l(_ret);
		auto Lock = StatLock->SyncLock();
		InsertLog(_ret, _Size);
		return _ret;
	}
	return nullptr;
}

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _realloc_rec(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NewSize
) {
	if (void *_ret = _realloc_rec_l(_Memory, _NewSize)) {
		size_t _size = _ret ? _msize_rec_l(_ret) : 0;
		auto Lock = StatLock->SyncLock();
		bool Wild = false;
		if (_Memory != _ret) {
			if (_Memory)
				InsertLog(_Memory, 0);;
			InsertLog(_ret, _NewSize);
		} else {
			InsertLog(_Memory, _NewSize);
		}
		return _ret;
	}
	return nullptr;
}

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _recalloc_rec(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NumOfElements,
_In_ size_t _SizeOfElements
) {
	if (void *_ret = _recalloc_rec_l(_Memory, _NumOfElements, _SizeOfElements)) {
		size_t _size = _ret ? _msize_rec_l(_ret) : 0;
		auto Lock = StatLock->SyncLock();
		bool Wild = false;
		if (_Memory != _ret) {
			if (_Memory)
				InsertLog(_Memory, 0);;
			InsertLog(_ret, _NumOfElements*_SizeOfElements);
		} else {
			InsertLog(_Memory, _NumOfElements*_SizeOfElements);
		}
		return _ret;
	}
	return nullptr;
}

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _expand_rec(
	_Pre_notnull_ void * _Memory,
	_In_ size_t _NewSize
	) {
	if (void *_ret = _expand_rec_l(_Memory, _NewSize)) {
		size_t _size = _msize_rec_l(_ret);
		auto Lock = StatLock->SyncLock();
		DEBUGV(if (_Memory != _ret) FAIL(_T("Expansion changes allocation record")));
		InsertLog(_Memory, _NewSize);
		return _ret;
	}
	return nullptr;
}

void __CRTDECL _free_rec(
	_Pre_maybenull_ _Post_invalid_ void * _Memory
	) {
	if (_Memory) {
		auto Lock = StatLock->SyncLock();
		InsertLog(_Memory, 0);;
		_free_rec_l(_Memory);
		// Performance: Only check for log flush when freeing
		CheckFlushLog(__LogFlushCnt);
	}
}

//size_t __CRTDECL _msize_rec(
//        _Pre_notnull_ void * _Memory
//        );
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
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _aligned_recalloc_rec
//(
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
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _aligned_offset_recalloc_rec
//(
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
//
//_Check_return_ _Ret_maybenull_z_ char *__CRTDECL _strdup_rec(
//	_In_opt_z_ const char * _Str
//	);
//
//_Check_return_ _Ret_maybenull_z_ TCHAR *__CRTDECL _wcsdup_rec(
//	_In_opt_z_ const wchar_t * _Str
//	);
//
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

extern "C++" {

#include <new>

	// Throw version
	_Ret_notnull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new(size_t _Size) _THROW1(_STD bad_alloc) {
		if (void *_ret = _malloc_rec(_Size))
			return _ret;
		throw _STD bad_alloc();
	}

	_Ret_notnull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new[](size_t _Size) _THROW1(_STD bad_alloc) {
		if (void *_ret = _malloc_rec(_Size))
			return _ret;
		throw _STD bad_alloc();
	}

	void __CRTDECL operator delete(void * _P) _THROW0() {
		_free_rec(_P);
	}

	void __CRTDECL operator delete[](void * _P) _THROW0() {
		_free_rec(_P);
	}

	// No-throw version
	_Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new(
		size_t _Size,
		const _STD nothrow_t&
		) _THROW0() {
		return _malloc_rec(_Size);
	}

	_Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new[](
		size_t _Size,
		const _STD nothrow_t&
		) _THROW0() {
		return _malloc_rec(_Size);
	}

	void __CRTDECL operator delete(
		void * _P,
		const _STD nothrow_t&
		) _THROW0() {
		_free_rec(_P);
	}

	void __CRTDECL operator delete[](
		void * _P,
		const _STD nothrow_t&
		) _THROW0() {
		_free_rec(_P);
	}

}

void __MM_INIT(void) {
	// These resources are never deallocated
	StatLock = new (_malloc_rec_l(sizeof TLockableCS)) TLockableCS();
	__MemOp_Log = new (_malloc_rec_l(sizeof TMemOp_Log)) TMemOp_Log();
	// Note: we cannot open log file here because CRT has not initialized!
}

void __MM_FINIT(void) {
	CheckFlushLog(0);
	if (__LogFile != nullptr) {
		LOGVV(_T("* Closing memory operation log file..."));
		fclose(__LogFile);
	}
}

#endif//RECMM
