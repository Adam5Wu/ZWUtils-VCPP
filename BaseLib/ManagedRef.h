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
 * @brief Managed Reference Helpers
 * @author Zhenyu Wu
 * @date Oct 25, 2013: Refactored from a ManagedObj
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef ManagedRef_H
#define ManagedRef_H

#include "ManagedObj.h"

#include "Misc.h"
#include "Exception.h"
#include "Allocator.h"

template<class T, class TAllocator = SimpleAllocator<T>>
class ManagedRef {
private:
	T* volatile _Obj = nullptr;

protected:
	static T* _RefObj(T *Obj);
	static T* _RelObj(T *Obj);
	static T* _DupObj(T *Obj, bool Smart = true);

	T* _Assign(T *Obj);
public:
	ManagedRef(void) {}

	ManagedRef(T *Obj, HANDOFF_CONSTRUCT_T const&) : _Obj(Obj) {}
	ManagedRef(T *Obj, ASSIGN_CONSTRUCT_T const&) : _Obj(_RefObj(Obj)) {}
	ManagedRef(T *Obj, bool SmartCopy = true) : _Obj(_DupObj(Obj, SmartCopy)) {}

	template<typename... Params>
	ManagedRef(EMPLACE_CONSTRUCT_T const&, Params&&... xParams) :
		_Obj(_RefObj(TAllocator::Create(xParams...))) {}

	// Move constructor
	ManagedRef(ManagedRef &&xMR) : _Obj(&xMR)
	{ xMR._Obj = nullptr; }
	// Copy constructor
	ManagedRef(ManagedRef const &xMR) : _Obj(_DupObj(&xMR)) {}

	virtual ~ManagedRef(void)
	{ Clear(); }

	inline T* operator&(void) const
	{ return _Obj; }
	inline T& operator*(void) const
	{ return *&*this; }
	inline T* operator->(void) const
	{ return &*this; }
	inline operator T&() const
	{ return **this; }
	inline bool operator==(ManagedRef const &xMR) const
	{ return (T&)*this == (T&)xMR; }

	ManagedRef& operator=(ManagedRef &&xMR);
	inline T* Assign(T *Obj, bool SmartCopy = true)
	{ return &(*this = ManagedRef(Obj, SmartCopy)); }
	inline T* operator=(ManagedRef const &xMR)
	{ return Assign(&xMR); }
	inline T* operator=(T *Obj)
	{ return Assign(Obj); }

	inline ManagedRef* operator~(void)
	{ return this; }
	inline ManagedRef const* operator~(void) const
	{ return this; }

	inline void Clear(void)
	{ _Assign(nullptr); }
	inline bool Empty(void) const
	{ return _Obj == nullptr; }
};

// HINT: In order for this wrapper to work, class T MUST have a virtual destructor!
template<class T, class U = void>
class ManagedObjAdapter final : public T, public ManagedObj {
	typedef ManagedObjAdapter _this;
	friend SimpleAllocator < _this > ;
	friend SimpleAllocator < _this const > ;
	friend decltype(U());
public:
	ManagedObjAdapter(void) {}

	template<typename A, typename... Params,
		typename = std::enable_if<!std::is_same<DEFAULT_CONSTRUCT_T const&, A>::value>::type,
		typename = std::enable_if<!std::is_same<ManagedObjAdapter const&, A>::value>::type,
		typename = std::enable_if<!std::is_same<ManagedObjAdapter&&, A>::value>::type>
		ManagedObjAdapter(A &&xA, Params&&... xParams)
	{ T::_Init(xA, xParams...); }

	ManagedObjAdapter(DEFAULT_CONSTRUCT_T const&)
	{ T::_Init(); }

	// No copy or move construction
	ManagedObjAdapter(ManagedObjAdapter const&) = delete;
	ManagedObjAdapter(ManagedObjAdapter&&) = delete;

	template<typename... Params>
	inline static T* Create(Params&&... xParams)
	{ return new ManagedObjAdapter(xParams...); }

	inline ManagedObjAdapter& operator=(ManagedObjAdapter const &xObj)
	{ return T::operator=(xObj); }
	inline ManagedObjAdapter& operator=(ManagedObjAdapter &&xObj)
	{ return T::operator=(std::move(xObj)); }

	auto toString(void) const -> decltype(((T&)_this()).toString(), TString()) override
	{ return T::toString(); }
};

template<class T>
class CopyCloneableAdapter final : public T, public Cloneable {
	typedef CopyCloneableAdapter _this;
	friend SimpleAllocator < _this > ;
	friend SimpleAllocator < _this const > ;
public:
	CopyCloneableAdapter(void) {}

	template<typename A, typename... Params,
		typename = std::enable_if<!std::is_same<DEFAULT_CONSTRUCT_T const&, A>::value>::type,
		typename = std::enable_if<!std::is_same<CopyCloneableAdapter const&, A>::value>::type,
		typename = std::enable_if<!std::is_same<CopyCloneableAdapter&&, A>::value>::type>
		CopyCloneableAdapter(A &&xA, Params&&... xParams)
	{ T::_Init(xA, xParams...); }

	CopyCloneableAdapter(DEFAULT_CONSTRUCT_T const&)
	{ T::_Init(); }

	inline Cloneable* Clone(void) const
	{ return new CopyCloneableAdapter(*this); }

	template<typename... Params>
	static auto Create(Params&&... xParams) -> decltype(_this()._Init(xParams...), (T*)&_this())
	{ return new CopyCloneableAdapter(xParams...); }

	inline static T* Create(void)
	{ return new CopyCloneableAdapter(DEFAULT_CONSTRUCT); }
};

template<class T, class TAllocator>
T* ManagedRef<T, TAllocator>::_RefObj(T *Obj) {
	if (auto MRef = ManagedObj::Cast(*Obj))
		const_cast<ManagedObj*>(MRef)->_AddRef();
	return Obj;
}

template<class T, class TAllocator>
T* ManagedRef<T, TAllocator>::_RelObj(T *Obj) {
	if (auto MRef = ManagedObj::Cast(*Obj))
		return const_cast<ManagedObj*>(MRef)->_RemoveRef() ? Obj : nullptr;
	return Obj;
}

template<class T, class TAllocator>
T* ManagedRef<T, TAllocator>::_DupObj(T *Obj, bool Smart) {
	if (Smart) {
		if (auto MRef = ManagedObj::Cast(*Obj)) {
			const_cast<ManagedObj*>(MRef)->_AddRef();
			return Obj;
		}
	}
	if (auto cObj = Cloneable::GetClone(*Obj))
		return dynamic_cast<T*>(cObj);

	if (Obj == nullptr)
		return nullptr;

	FAIL(_T("Must be ManagedObj or Cloneable to apply in this context"));
}

template<class T, class TAllocator>
T* ManagedRef<T, TAllocator>::_Assign(T *Obj) {
	TAllocator::Destroy(_RelObj((T*)InterlockedExchangePointer((PVOID*)&_Obj, (PVOID)_RefObj(Obj))));
	return Obj;
}

template<class T, class TAllocator>
ManagedRef<T, TAllocator>& ManagedRef<T, TAllocator>:: operator=(ManagedRef &&xMR) {
	TAllocator::Destroy(_RelObj((T*)InterlockedExchangePointer((PVOID*)&_Obj, InterlockedExchangePointer((PVOID*)&xMR._Obj, nullptr))));
	return *this;
}

#endif //ManagedRef_H