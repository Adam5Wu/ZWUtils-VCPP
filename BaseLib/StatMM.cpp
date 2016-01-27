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

// [Utilities] Statistics Memory Manager Helpers

#ifdef STATMM

#define _STATMM_IMPL

#include "StatMM.h"

#ifdef NEDMM

#include "NedMM.h"

#define _malloc_stat_l _malloc_ned
#define _msize_stat_l _msize_ned
#define _calloc_stat_l _calloc_ned
#define _realloc_stat_l _realloc_ned
#define _recalloc_stat_l _recalloc_ned
#define _expand_stat_l _expand_ned
#define _free_stat_l _free_ned

#endif

#ifdef FASTMM

#include "FastMM.h"

#define _malloc_stat_l _malloc_fast
#define _msize_stat_l _msize_fast
#define _calloc_stat_l _calloc_fast
#define _realloc_stat_l _realloc_fast
#define _recalloc_stat_l _recalloc_fast
#define _expand_stat_l _expand_fast
#define _free_stat_l _free_fast

#endif

#ifndef STATMM_LT

#include <string>
#include <unordered_map>
#include <allocators>

#include "BaseLib/DebugLog.h"
#include "ThreadLib/SyncObjs.h"

static TLockableCS* StatLock = nullptr;

using namespace std;

struct __Alloc_Rec {
	const TCHAR * _Filename;
	int _LineNumber;

	size_t _Size;
};

template<>
struct hash < __Alloc_Rec > {
	size_t operator()(__Alloc_Rec const &T)
	{ return (size_t)T._Filename ^ (size_t)T._LineNumber; }
};

bool operator ==(__Alloc_Rec const &A, __Alloc_Rec const &B)
{ return (A._Filename == B._Filename) && (A._LineNumber == B._LineNumber); }

bool operator !=(__Alloc_Rec const &A, __Alloc_Rec const &B)
{ return (A._Filename != B._Filename) || (A._LineNumber != B._LineNumber); }

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
		_free_stat_l(_Ptr);
	}

	pointer allocate(size_type _Count)
	{	// allocate array of _Count elements
		if (void*_ret = _malloc_stat_l(sizeof(value_type)* _Count))
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

#define TAlloc_Map unordered_map<__Alloc_Rec, size_t, hash<__Alloc_Rec>, equal_to<__Alloc_Rec>, malloc_allocator<pair<__Alloc_Rec const, size_t>>>
static TAlloc_Map* __Alloc_Map = nullptr;
#define TDealloc_Map unordered_map<void*, __Alloc_Rec, hash<void*>, equal_to<void*>, malloc_allocator<pair<void* const, __Alloc_Rec>>>
static TDealloc_Map* __Dealloc_Map = nullptr;

static size_t __Alloc_Size = 0;
static UINT64 __Alloc_Count = 0;
static UINT64 __Dealloc_Count = 0;
static UINT64 __Alloc_Cumulative = 0;
static UINT64 __Dealloc_Cumulative = 0;
static UINT64 __Dealloc_Wild = 0;

#else//STATMM_LT

#include "ThreadLib/Threading.h"
#include "ThreadLib/SyncPrems.h"

static TSyncInt __Alloc_Size = 0;

#endif//STATMM_LT

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL _malloc_stat(
#ifndef STATMM_LT
	_In_ size_t _Size,
	_In_opt_z_ const TCHAR * _Filename,
	_In_ int _LineNumber
#else
	_In_ size_t _Size
#endif
	) {
	if (void *_ret = _malloc_stat_l(_Size)) {
		size_t _size = _msize_stat_l(_ret);
#ifndef STATMM_LT
		auto Lock = StatLock->SyncLock();
		__Alloc_Rec _addr{_Filename, _LineNumber, _size};
		auto idxiter = __Alloc_Map->find(_addr);
		if (idxiter != __Alloc_Map->end()) {
			*const_cast<size_t*>(&idxiter->first._Size) += _size;
			idxiter->second++;
		} else
			__Alloc_Map->insert(make_pair(_addr, 1));
		__Alloc_Count++;

		auto insrec = __Dealloc_Map->insert(make_pair(_ret, _addr));
		if (!insrec.second) {
			FAIL(_T("{malloc} Duplicate allocation record '%s (%d)' -> '%s (%d)'"),
				 insrec.first->second._Filename, insrec.first->second._LineNumber,
				 _Filename, _LineNumber);
		}
		__Alloc_Cumulative += _size;
#endif//STATMM_LT
		__Alloc_Size += _size;
		return _ret;
	}
	return nullptr;
}

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
	) {
	if (void *_ret = _calloc_stat_l(_Count, _Size)) {
		size_t _size = _msize_stat_l(_ret);
#ifndef STATMM_LT
		auto Lock = StatLock->SyncLock();
		__Alloc_Rec _addr{_Filename, _LineNumber, _size};
		auto idxiter = __Alloc_Map->find(_addr);
		if (idxiter != __Alloc_Map->end()) {
			*const_cast<size_t*>(&idxiter->first._Size) += _size;
			idxiter->second++;
		} else
			__Alloc_Map->insert(make_pair(_addr, 1));
		__Alloc_Count++;

		auto insrec = __Dealloc_Map->insert(make_pair(_ret, _addr));
		if (!insrec.second) {
			FAIL(_T("{calloc} Duplicate allocation record '%s (%d)' -> '%s (%d)'"),
				 insrec.first->second._Filename, insrec.first->second._LineNumber,
				 _Filename, _LineNumber);
		}
		__Alloc_Cumulative += _size;
#endif//STATMM_LT
		__Alloc_Size += _size;
		return _ret;
	}
	return nullptr;
}

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _realloc_stat_h(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NewSize
) {
	size_t _OldSize = _Memory ? _msize_stat_l(_Memory) : 0;
	if (void *_ret = _realloc_stat_l(_Memory, _NewSize)) {
		size_t _size = _ret ? _msize_stat_l(_ret) : 0;
#ifndef STATMM_LT
		auto Lock = StatLock->SyncLock();
		bool Wild = false;
		if (_Memory != _ret) {
			if (_Memory) {
				auto iter = __Dealloc_Map->find(_Memory);
				if (iter != __Dealloc_Map->end()) {
					DEBUGV(if (iter->second._Size != _OldSize)
						   FAIL(_T("{realloc} Allocation deallocation size unbalanced '%s (%d)' (%d != %d)"),
						   iter->second._Filename, iter->second._LineNumber, (int)iter->second._Size, (int)_OldSize);
					);

					if (_ret == nullptr) {
						auto cntiter = __Alloc_Map->find(iter->second);
						if (cntiter == __Alloc_Map->end())
							FAIL(_T("Missing allocation record"));
						if (--cntiter->second == 0) {
							__Alloc_Map->erase(cntiter);
						} else {
							*const_cast<size_t*>(&cntiter->first._Size) += _size - _OldSize;
						}
					} else {
						__Alloc_Count++;
						__Alloc_Rec _addr{iter->second._Filename, iter->second._LineNumber, _size};
						auto insrec = __Dealloc_Map->insert(make_pair(_ret, _addr));
						if (!insrec.second) {
							FAIL(_T("{realloc} Duplicate allocation record '%s (%d)' -> '%s (%d)'"),
								 insrec.first->second._Filename, insrec.first->second._LineNumber,
								 _addr._Filename, _addr._LineNumber);
						}
					}
					__Dealloc_Map->erase(iter);
					__Dealloc_Count++;
				} else {
					Wild = true;
					__Dealloc_Wild++;
				}
			} else {
				if (_ret != nullptr) {
					__Alloc_Rec _addr{nullptr, 0, _size};
					auto idxiter = __Alloc_Map->find(_addr);
					if (idxiter != __Alloc_Map->end()) {
						*const_cast<size_t*>(&idxiter->first._Size) += _size;
						idxiter->second++;
					} else
						__Alloc_Map->insert(make_pair(_addr, 1));
					__Alloc_Count++;

					auto insrec = __Dealloc_Map->insert(make_pair(_ret, _addr));
					if (!insrec.second) {
						FAIL(_T("{realloc} Duplicate allocation record '%s (%d)' -> '%s (%d)'"),
							 insrec.first->second._Filename, insrec.first->second._LineNumber,
							 _addr._Filename, _addr._LineNumber);
					}
				}
			}
		}
		if (!Wild) {
			__Dealloc_Cumulative += _OldSize;
			__Alloc_Cumulative += _size;
		}
		if (!Wild)
#endif//STATMM_LT
			__Alloc_Size += _size - _OldSize;
		return _ret;
	}
	return nullptr;
}

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _recalloc_stat_h(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NumOfElements,
_In_ size_t _SizeOfElements
) {
	size_t _OldSize = _Memory ? _msize_stat_l(_Memory) : 0;
	if (void *_ret = _recalloc_stat_l(_Memory, _NumOfElements, _SizeOfElements)) {
		size_t _size = _ret ? _msize_stat_l(_ret) : 0;
#ifndef STATMM_LT
		auto Lock = StatLock->SyncLock();
		bool Wild = false;
		if (_Memory != _ret) {
			if (_Memory) {
				auto iter = __Dealloc_Map->find(_Memory);
				if (iter != __Dealloc_Map->end()) {
					DEBUGV(if (iter->second._Size != _OldSize)
						   FAIL(_T("{recalloc} Allocation deallocation size unbalanced '%s (%d)' (%d != %d)"),
						   iter->second._Filename, iter->second._LineNumber, (int)iter->second._Size, (int)_OldSize);
					);

					if (_ret == nullptr) {
						auto cntiter = __Alloc_Map->find(iter->second);
						if (cntiter == __Alloc_Map->end())
							FAIL(_T("Missing allocation record"));
						if (--cntiter->second == 0) {
							__Alloc_Map->erase(cntiter);
						} else {
							*const_cast<size_t*>(&cntiter->first._Size) += _size - _OldSize;
						}
					} else {
						__Alloc_Count++;
						__Alloc_Rec _addr{iter->second._Filename, iter->second._LineNumber, _size};
						auto insrec = __Dealloc_Map->insert(make_pair(_ret, _addr));
						if (!insrec.second) {
							FAIL(_T("{recalloc} Duplicate allocation record '%s (%d)' -> '%s (%d)'"),
								 insrec.first->second._Filename, insrec.first->second._LineNumber,
								 _addr._Filename, _addr._LineNumber);
						}
					}
					__Dealloc_Map->erase(iter);
					__Dealloc_Count++;
				} else {
					Wild = true;
					__Dealloc_Wild++;
				}
			} else {
				if (_ret != nullptr) {
					__Alloc_Rec _addr{nullptr, 0, _size};
					auto idxiter = __Alloc_Map->find(_addr);
					if (idxiter != __Alloc_Map->end()) {
						*const_cast<size_t*>(&idxiter->first._Size) += _size;
						idxiter->second++;
					} else
						__Alloc_Map->insert(make_pair(_addr, 1));
					__Alloc_Count++;

					auto insrec = __Dealloc_Map->insert(make_pair(_ret, _addr));
					if (!insrec.second) {
						FAIL(_T("{recalloc} Duplicate allocation record '%s (%d)' -> '%s (%d)'"),
							 insrec.first->second._Filename, insrec.first->second._LineNumber,
							 _addr._Filename, _addr._LineNumber);
					}
				}
			}
		}
		if (!Wild) {
			__Dealloc_Cumulative += _OldSize;
			__Alloc_Cumulative += _size;
		}
		if (!Wild)
#endif//STATMM_LT
			__Alloc_Size += _size - _OldSize;
		return _ret;
	}
	return nullptr;
}

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _expand_stat_h(
	_Pre_notnull_ void * _Memory,
	_In_ size_t _NewSize
	) {
	size_t _OldSize = _Memory ? _msize_stat_l(_Memory) : 0;
	if (void *_ret = _expand_stat_l(_Memory, _NewSize)) {
		size_t _size = _msize_stat_l(_ret);
#ifndef STATMM_LT
		auto Lock = StatLock->SyncLock();
		DEBUGV(if (_Memory != _ret) FAIL(_T("Expansion changes allocation record")));
		auto iter = __Dealloc_Map->find(_Memory);
		if (iter != __Dealloc_Map->end()) {
			iter->second._Size += _size - _OldSize;
			auto cntiter = __Alloc_Map->find(iter->second);
			if (cntiter == __Alloc_Map->end())
				FAIL(_T("Missing allocation record"));
			*const_cast<size_t*>(&cntiter->first._Size) += _size - _OldSize;
			__Dealloc_Cumulative += _OldSize;
			__Alloc_Cumulative += _size;
#endif//STATMM_LT
			__Alloc_Size += _size - _OldSize;
#ifndef STATMM_LT
		}
#endif//STATMM_LT
		return _ret;
	}
	return nullptr;
}

#ifndef STATMM_LT

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) void *__CRTDECL _realloc_stat(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NewSize,
_In_opt_z_ const TCHAR * _Filename,
_In_ int _LineNumber
) {
	size_t _OldSize = _Memory ? _msize_stat_l(_Memory) : 0;
	if (void *_ret = _realloc_stat_l(_Memory, _NewSize)) {
		size_t _size = _ret ? _msize_stat_l(_ret) : 0;
#ifndef STATMM_LT
		auto Lock = StatLock->SyncLock();
		bool Wild = false;
		if (_Memory != _ret) {
			if (_Memory) {
				auto iter = __Dealloc_Map->find(_Memory);
				if (iter != __Dealloc_Map->end()) {
					DEBUGV(if (iter->second._Size != _OldSize)
						   FAIL(_T("{realloc} Allocation deallocation size unbalanced '%s (%d)' (%d != %d)"),
						   iter->second._Filename, iter->second._LineNumber, (int)iter->second._Size, (int)_OldSize);
					);

					if (_ret == nullptr) {
						auto cntiter = __Alloc_Map->find(iter->second);
						if (cntiter == __Alloc_Map->end())
							FAIL(_T("Missing allocation record"));
						if (--cntiter->second == 0) {
							__Alloc_Map->erase(cntiter);
						} else {
							*const_cast<size_t*>(&cntiter->first._Size) += _size - _OldSize;
						}
					} else {
						__Alloc_Count++;
						__Alloc_Rec _addr{iter->second._Filename, iter->second._LineNumber, _size};
						auto insrec = __Dealloc_Map->insert(make_pair(_ret, _addr));
						if (!insrec.second) {
							FAIL(_T("{realloc} Duplicate allocation record '%s (%d)' -> '%s (%d)'"),
								 insrec.first->second._Filename, insrec.first->second._LineNumber,
								 _addr._Filename, _addr._LineNumber);
						}
					}
					__Dealloc_Map->erase(iter);
					__Dealloc_Count++;
				} else {
					Wild = true;
					__Dealloc_Wild++;
				}
			} else {
				if (_ret != nullptr) {
					__Alloc_Rec _addr{_Filename, _LineNumber, _size};
					auto idxiter = __Alloc_Map->find(_addr);
					if (idxiter != __Alloc_Map->end()) {
						*const_cast<size_t*>(&idxiter->first._Size) += _size;
						idxiter->second++;
					} else
						__Alloc_Map->insert(make_pair(_addr, 1));
					__Alloc_Count++;

					auto insrec = __Dealloc_Map->insert(make_pair(_ret, _addr));
					if (!insrec.second) {
						FAIL(_T("{realloc} Duplicate allocation record '%s (%d)' -> '%s (%d)'"),
							 insrec.first->second._Filename, insrec.first->second._LineNumber,
							 _addr._Filename, _addr._LineNumber);
					}
				}
			}
		}
		if (!Wild) {
			__Dealloc_Cumulative += _OldSize;
			__Alloc_Cumulative += _size;
		}
		if (!Wild)
#endif//STATMM_LT
			__Alloc_Size += _size - _OldSize;
		return _ret;
	}
	return nullptr;
}

_Success_(return != 0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _recalloc_stat(
_Pre_maybenull_ _Post_invalid_ void * _Memory,
_In_ size_t _NumOfElements,
_In_ size_t _SizeOfElements,
_In_opt_z_ const TCHAR * _Filename,
_In_ int _LineNumber
) {
	size_t _OldSize = _Memory ? _msize_stat_l(_Memory) : 0;
	if (void *_ret = _recalloc_stat_l(_Memory, _NumOfElements, _SizeOfElements)) {
		size_t _size = _ret ? _msize_stat_l(_ret) : 0;
#ifndef STATMM_LT
		auto Lock = StatLock->SyncLock();
		bool Wild = false;
		if (_Memory != _ret) {
			if (_Memory) {
				auto iter = __Dealloc_Map->find(_Memory);
				if (iter != __Dealloc_Map->end()) {
					DEBUGV(if (iter->second._Size != _OldSize)
						   FAIL(_T("{recalloc} Allocation deallocation size unbalanced '%s (%d)' (%d != %d)"),
						   iter->second._Filename, iter->second._LineNumber, (int)iter->second._Size, (int)_OldSize);
					);

					if (_ret == nullptr) {
						auto cntiter = __Alloc_Map->find(iter->second);
						if (cntiter == __Alloc_Map->end())
							FAIL(_T("Missing allocation record"));
						if (--cntiter->second == 0) {
							__Alloc_Map->erase(cntiter);
						} else {
							*const_cast<size_t*>(&cntiter->first._Size) += _size - _OldSize;
						}
					} else {
						__Alloc_Count++;
						__Alloc_Rec _addr{iter->second._Filename, iter->second._LineNumber, _size};
						auto insrec = __Dealloc_Map->insert(make_pair(_ret, _addr));
						if (!insrec.second) {
							FAIL(_T("{recalloc} Duplicate allocation record '%s (%d)' -> '%s (%d)'"),
								 insrec.first->second._Filename, insrec.first->second._LineNumber,
								 _addr._Filename, _addr._LineNumber);
						}
					}
					__Dealloc_Map->erase(iter);
					__Dealloc_Count++;
				} else {
					Wild = true;
					__Dealloc_Wild++;
				}
			} else {
				if (_ret != nullptr) {
					__Alloc_Rec _addr{_Filename, _LineNumber, _size};
					auto idxiter = __Alloc_Map->find(_addr);
					if (idxiter != __Alloc_Map->end()) {
						*const_cast<size_t*>(&idxiter->first._Size) += _size;
						idxiter->second++;
					} else
						__Alloc_Map->insert(make_pair(_addr, 1));
					__Alloc_Count++;

					auto insrec = __Dealloc_Map->insert(make_pair(_ret, _addr));
					if (!insrec.second) {
						FAIL(_T("{recalloc} Duplicate allocation record '%s (%d)' -> '%s (%d)'"),
							 insrec.first->second._Filename, insrec.first->second._LineNumber,
							 _addr._Filename, _addr._LineNumber);
					}
				}
			}
		}
		if (!Wild) {
			__Dealloc_Cumulative += _OldSize;
			__Alloc_Cumulative += _size;
		}
		if (!Wild)
#endif//STATMM_LT
			__Alloc_Size += _size - _OldSize;
		return _ret;
	}
	return nullptr;
}

#endif

void __CRTDECL _free_stat_h(
	_Pre_maybenull_ _Post_invalid_ void * _Memory
	) {
	if (_Memory) {
		size_t _OldSize = _msize_stat_l(_Memory);
#ifndef STATMM_LT
		auto Lock = StatLock->SyncLock();
		bool Wild = false;
		auto iter = __Dealloc_Map->find(_Memory);
		if (iter != __Dealloc_Map->end()) {
			DEBUGV(if (iter->second._Size != _OldSize)
				   FAIL(_T("{free} Allocation deallocation size unbalanced '%s (%d)' (%d != %d)"),
				   iter->second._Filename, iter->second._LineNumber, (int)iter->second._Size, (int)_OldSize);
			);

			auto cntiter = __Alloc_Map->find(iter->second);
			if (cntiter == __Alloc_Map->end())
				FAIL(_T("Missing allocation record"));
			if (--cntiter->second == 0) {
				__Alloc_Map->erase(cntiter);
			} else {
				*const_cast<size_t*>(&cntiter->first._Size) -= _OldSize;
			}
			__Dealloc_Map->erase(iter);
			__Dealloc_Count++;
			__Dealloc_Cumulative += _OldSize;
		} else {
			Wild = true;
			__Dealloc_Wild++;
		}
		if (!Wild)
#endif//STATMM_LT
			__Alloc_Size -= _OldSize;
		_free_stat_l(_Memory);
	}
}

//size_t __CRTDECL _msize_stat(
//        _Pre_notnull_ void * _Memory
//        );
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
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _aligned_recalloc_stat
//(
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
//_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) void *__CRTDECL _aligned_offset_recalloc_stat
//(
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
//
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

#ifndef STATMM_LT

void __CRTDECL _free_stat_x(
	_Pre_maybenull_ _Post_invalid_ void * _Memory,
	_In_opt_z_ const TCHAR * _Filename,
	_In_ int _LineNumber
	) {
	if (_Memory) {
		size_t _OldSize = _msize_stat_l(_Memory);
#ifndef STATMM_LT
		auto Lock = StatLock->SyncLock();
		auto iter = __Dealloc_Map->find(_Memory);
		if (iter == __Dealloc_Map->end())
			FAIL(_T("Missing allocation record"));

		DEBUGV(if (iter->second._Size != _OldSize)
			   FAIL(_T("{free} Allocation deallocation size unbalanced '%s (%d)' (%d != %d)"),
			   iter->second._Filename, iter->second._LineNumber, (int)iter->second._Size, (int)_OldSize);
		);

		DEBUGV(if (iter->second != __Alloc_Rec{_Filename, _LineNumber}) FAIL(_T("Free site different from allocation")));
		auto cntiter = __Alloc_Map->find(iter->second);
		if (cntiter == __Alloc_Map->end())
			FAIL(_T("Missing allocation record"));
		if (--cntiter->second == 0) {
			__Alloc_Map->erase(cntiter);
		} else {
			*const_cast<size_t*>(&cntiter->first._Size) -= _OldSize;
		}
		__Dealloc_Map->erase(_Memory);
		__Dealloc_Count++;
		__Dealloc_Cumulative += _OldSize;
#endif//STATMM_LT
		__Alloc_Size -= _OldSize;
		free(_Memory);
	}
}

#endif//STATMM_LT

extern "C++" {

#include <new>

	// Throw version
	_Ret_notnull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new(size_t _Size) _THROW1(_STD bad_alloc) {
#ifndef STATMM_LT
		if (void *_ret = _malloc_stat(_Size, nullptr, 0))
#else
		if (void *_ret = _malloc_stat(_Size))
#endif
			return _ret;
		throw _STD bad_alloc();
	}

	_Ret_notnull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new[](size_t _Size) _THROW1(_STD bad_alloc) {
#ifndef STATMM_LT
		if (void *_ret = _malloc_stat(_Size, nullptr, 0))
#else
		if (void *_ret = _malloc_stat(_Size))
#endif
			return _ret;
		throw _STD bad_alloc();
	}

	void __CRTDECL operator delete(void * _P) _THROW0() {
		_free_stat_h(_P);
	}

	void __CRTDECL operator delete[](void * _P) _THROW0() {
		_free_stat_h(_P);
	}

	// No-throw version
	_Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new(
		size_t _Size,
		const _STD nothrow_t&
		) _THROW0() {
#ifndef STATMM_LT
		return _malloc_stat(_Size, nullptr, 0);
#else
		return _malloc_stat(_Size);
#endif
	}

	_Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new[](
		size_t _Size,
		const _STD nothrow_t&
		) _THROW0() {
#ifndef STATMM_LT
		return _malloc_stat(_Size, nullptr, 0);
#else
		return _malloc_stat(_Size);
#endif
	}

	void __CRTDECL operator delete(
		void * _P,
		const _STD nothrow_t&
		) _THROW0() {
		_free_stat_h(_P);
	}

	void __CRTDECL operator delete[](
		void * _P,
		const _STD nothrow_t&
		) _THROW0() {
		_free_stat_h(_P);
	}

#ifndef STATMM_LT

	_Ret_notnull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new(
		size_t _Size,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW1(_STD bad_alloc) {
		if (void *_ret = _malloc_stat(_Size, _Filename, _LineNumber))
			return _ret;
		throw _STD bad_alloc();
	}

	_Ret_notnull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new[](
		size_t _Size,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW1(_STD bad_alloc) {
		if (void *_ret = _malloc_stat(_Size, _Filename, _LineNumber))
			return _ret;
		throw _STD bad_alloc();
	}

	void __CRTDECL operator delete(
		void * _P,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW0() {
		_free_stat_x(_P, _Filename, _LineNumber);
	}

	void __CRTDECL operator delete[](
		void * _P,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW0() {
		_free_stat_x(_P, _Filename, _LineNumber);
	}

	// No-throw version
	_Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new(
		size_t _Size,
		const _STD nothrow_t&,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW0() {
		return _malloc_stat(_Size, _Filename, _LineNumber);
	}

	_Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new[](
		size_t _Size,
		const _STD nothrow_t&,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW0() {
		return _malloc_stat(_Size, _Filename, _LineNumber);
	}

	void __CRTDECL operator delete(
		void * _P,
		const _STD nothrow_t&,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW0() {
		_free_stat_x(_P, _Filename, _LineNumber);
	}

	void __CRTDECL operator delete[](
		void * _P,
		const _STD nothrow_t&,
		_In_opt_z_ const TCHAR * _Filename,
		_In_ int _LineNumber
		) _THROW0() {
		_free_stat_x(_P, _Filename, _LineNumber);
	}

#endif//STATMM_LT

}

#ifndef STATMM_LT

void __MM_INIT(void) {
	// These resources are never deallocated
	StatLock = new (_malloc_stat_l(sizeof TLockableCS)) TLockableCS();
	__Alloc_Map = new (_malloc_stat_l(sizeof TAlloc_Map)) TAlloc_Map();
	__Dealloc_Map = new (_malloc_stat_l(sizeof TDealloc_Map)) TDealloc_Map();
}

size_t _MM_AllocSize(void) {
	Synchronized((*StatLock), {
		return __Alloc_Size;
	});
}

_MM_Stats _MM_AllocStats(void) {
	_MM_Stats _ret;
	Synchronized((*StatLock), {
		_ret.__Alloc_Size = __Alloc_Size;
		_ret.__Alloc_Count = __Alloc_Count;
		_ret.__Dealloc_Count = __Dealloc_Count;
		_ret.__Alloc_Cumulative = __Alloc_Cumulative;
		_ret.__Dealloc_Cumulative = __Dealloc_Cumulative;
		return _ret;
	});
}

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

void _MM_DumpDetails(void) {
	Synchronized((*StatLock), {
		LOG(_T("========== Memory Manager Statistics =========="));
		TCHAR STRBUF_Alloc[16]; TCHAR STRBUF_Dealloc[16]; TCHAR STRBUF_Occupy[16];
		LOG(_T("Allocations: %s allocations, %s deallocations (%s occupied)"),
			_FormatSize(__Alloc_Cumulative, STRBUF_Alloc, 16),
			_FormatSize(__Dealloc_Cumulative, STRBUF_Dealloc, 16),
			_FormatSize(__Alloc_Size, STRBUF_Occupy, 16));
		LOG(_T("Operations: %lld allocations, %lld deallocations (%lld pending)"),
			__Alloc_Count, __Dealloc_Count, __Alloc_Count - __Dealloc_Count);
		if (__Dealloc_Wild > 0)
			LOG(_T("Wild deallocations: %lld"), __Dealloc_Wild);
		for (auto &entry : *__Alloc_Map) {
			TCHAR STRBUF[16];
			if (entry.first._Filename) {
				LOG(_T("* %s (%d): %lld [%s]"),
					entry.first._Filename, entry.first._LineNumber,
					(UINT64)entry.second, _FormatSize(entry.first._Size, STRBUF, 16));
			} else {
				LOG(_T("* <Anonymous>: %lld [%s]"),
					(UINT64)entry.second, _FormatSize(entry.first._Size, STRBUF, 16));
			}
		}
	});
}

#else

size_t _MM_AllocSize(void) {
	return ~__Alloc_Size;
}

#endif//STATMM_LT

#endif//STATMM
