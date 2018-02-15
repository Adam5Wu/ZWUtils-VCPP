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
 * @addtogroup Modeling Modeling Support Utilities
 * @file
 * @brief Identifiers and Identifier Pools
 * @author Zhenyu Wu
 * @date Sep 24, 2013: Uplift from a child project
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef Identifier_H
#define Identifier_H

#include <stddef.h>
#include <unordered_map>
#include <utility>

#include "BaseLib/Misc.h"
#include "BaseLib/Exception.h"
#include "BaseLib/DebugLog.h"
#include "BaseLib/Allocator.h"
#include "BaseLib/ManagedObj.h"
#include "BaseLib/ManagedRef.h"
#include "ThreadLib/SyncObjs.h"

//======== Interface: Identifier ========
class IIdentifier {
	friend SimpleAllocator < IIdentifier > ;
	friend SimpleAllocator < IIdentifier const > ;
protected:
	IIdentifier(void) {}
	virtual ~IIdentifier(void) {}

	// Replacement constructor
	void _Init(void) {}
public:
	virtual bool equalto(IIdentifier const &xIdentifier) const
	{ FAIL(_T("Abstract function")); }
	virtual size_t hashcode(void) const
	{ FAIL(_T("Abstract function")); }
	virtual TString toString(void) const
	{ FAIL(_T("Abstract function")); }
};

bool operator==(IIdentifier const &A, IIdentifier const &B);
bool operator!=(IIdentifier const &A, IIdentifier const &B);
size_t operator~(IIdentifier const &A);

template<>
struct std::hash < IIdentifier > {
	std::size_t operator()(IIdentifier const& Target)
	{ return ~Target; }
};

typedef std::reference_wrapper<IIdentifier const> RIdentifier;
template<>
struct std::hash < RIdentifier > {
	std::size_t operator()(IIdentifier const& Target)
	{ return ~Target; }
};

typedef ManagedRef<IIdentifier const> MRIdentifier;
template<>
struct std::hash < MRIdentifier > {
	std::size_t operator()(IIdentifier const& Target)
	{ return ~Target; }
};

//extern IIdentifier const &RootIdent;
IIdentifier const& RootIdent(void);

//======== Interface: IdentPool ========
template<class TKey, class TIdentifier, class TKeyHasher = std::hash<TKey>>
class IIdentPool : public TLockable {
	ENFORCE_DERIVE(IIdentifier, TIdentifier);
protected:
	typedef std::unordered_map<TKey, ManagedRef<TIdentifier>, TKeyHasher> _Idents;
	TSyncObj<_Idents> Idents;

	template<typename... Params>
	bool _FindOrCreateIdent(_Idents& Pool, TKey const &xKey, ManagedRef<TIdentifier>& xMRef, Params&&... xParams) {
		auto iRet = Pool.find(xKey);
		if (iRet == Pool.end()) {
			iRet = Pool.emplace(std::piecewise_construct, std::forward_as_tuple(xKey),
								std::forward_as_tuple(EMPLACE_CONSTRUCT, xParams...)).first;
			return (xMRef = (*iRet).second, true);
		}
		return (xMRef = (*iRet).second, false);
	}

	bool _FindIdent(_Idents& Pool, TKey const &xKey, ManagedRef<TIdentifier>& xMRef) {
		_Idents::iterator iRet = Pool.find(xKey);
		if (iRet != Pool.end()) {
			return (xMRef = (*iRet).second, true);
		}
		return false;
	}

	bool _RemoveIdent(_Idents& Pool, TKey const &xKey, ManagedRef<TIdentifier>& xMRef) {
		_Idents::iterator iRet = Pool.find(xKey);
		if (iRet != Pool.end()) {
			xMRef = (*iRet).second;
			return (Pool.erase(iRet), true);
		}
		return false;
	}

public:
	typedef typename _Idents::iterator iterator;

	IIdentPool(void) {}
	virtual ~IIdentPool(void) {}

	template<typename... Params>
	bool FindOrCreateIdent(TKey const &xKey, ManagedRef<TIdentifier>& xMRef, Params&&... xParams) {
		auto Pool(Idents.Pickup());
		return _FindOrCreateIdent(Pool, xKey, xMRef, xParams...);
	}

	bool FindIdent(TKey const &xKey, ManagedRef<TIdentifier> &xMRef) {
		auto Pool(Idents.Pickup());
		return _FindIdent(Pool, xKey, xMRef);
	}

	bool RemoveIdent(TKey const &xKey, ManagedRef<TIdentifier> &xMRef) {
		auto Pool(Idents.Pickup());
		return _RemoveIdent(Pool, xKey, xMRef);
	}

	size_t Flush(void) {
		auto Pool(Idents.Pickup());
		auto Ret = Pool->size();
		Pool->clear();
		return Ret;
	}

	size_t size(void) {
		auto Pool(Idents.Pickup());
		return Pool->size();
	}
};

//======== Interface: NameIdent ========

class INameIdent : public virtual IIdentifier {
protected:
	// Default constructor for virtual inheritance
	INameIdent(void) {}

	// Replacement constructor
	void _Init(TString const &xName)
	{ const_cast<TString*>(&Name)->assign(xName); }
	void _Init(TString &&xName)
	{ const_cast<TString*>(&Name)->assign(std::move(xName)); }
public:
	TString const Name;

	INameIdent(TString const &xName)
	{ _Init(xName); }
	INameIdent(TString &&xName)
	{ _Init(std::move(xName)); }

	void operator =(INameIdent const &xNameIdent)
	{ const_cast<TString*>(&Name)->assign(xNameIdent.Name); }
	void operator =(INameIdent &&xNameIdent)
	{ const_cast<TString*>(&Name)->assign(std::move(*const_cast<TString*>(&xNameIdent.Name))); }

	bool equalto(IIdentifier const &xIdentifier) const override;
	size_t hashcode(void) const override;
	TString toString(void) const override;

	bool equalto(INameIdent const &xNameIdent) const;
};
typedef ManagedRef<INameIdent const> MRNameIdent;

INameIdent& GetNameIdent(TString const& xName);

//======== Interface: NameDelegatedIdent ========
template<class TIdent>
class INameDelegatedIdent : public virtual INameIdent, public virtual TIdent {
	ENFORCE_DERIVE(IIdentifier, TIdent);
protected:
	// Default constructor for virtual inheritance
	INameDelegatedIdent(void) {}

	// Replacement constructor
	template<typename O, typename... Params,
		typename = std::enable_if<!std::is_same<DEFAULT_CONSTRUCT_T const&, O>::value>::type>
		void _Init(TString const& xName, O &&xO, Params&&... xParams)
	{ INameIdent::_Init(xName); TIdent::_Init(xO, xParams...); }

	void _Init(TString const& xName, DEFAULT_CONSTRUCT_T const&)
	{ INameIdent::_Init(xName); }
public:
	template<typename... Params>
	INameDelegatedIdent(TString const& xName, Params&&... xParams)
	{ _Init(xName, xParams...); }

	bool equalto(IIdentifier const &xIdentifier) const override
	{ return typeid(xIdentifier) == typeid(*this) ? equalto(dynamic_cast<INameDelegatedIdent const&>(xIdentifier)) : false; }
	bool equalto(TIdent const &xTIdent) const /*override*/
	{ return typeid(xTIdent) == typeid(*this) ? equalto(dynamic_cast<INameDelegatedIdent const&>(xTIdent)) : false; }
	virtual bool equalto(INameDelegatedIdent const &xNameDelegatedIdent) const
	{ return INameIdent::equalto(xNameDelegatedIdent); }
	size_t hashcode(void) const override
	{ return INameIdent::hashcode(); }
	TString toString(void) const override
	{ return INameIdent::toString(); }
};

//======== Interface: ContextIdent ========
template<class TIdent>
class IContextIdent : public virtual TIdent {
protected:
	// Default constructor for virtual inheritance
	IContextIdent(void) {}

	// Replacement constructor
	template<typename... Params>
	void _Init(IIdentifier const &xContext, Params&&... xParams)
	{ TIdent::_Init(xParams...); *const_cast<MRIdentifier*>(~rContext) = &xContext; }
public:
	MRIdentifier const rContext;

	template<typename... Params>
	IContextIdent(CONTEXT_CONSTRUCT_T const&, IIdentifier const &xContext, Params&&... xParams)
	{ _Init(xContext, xParams...); }

	bool equalto(IIdentifier const &xIdentifier) const override
	{ return typeid(xIdentifier) == typeid(*this) ? equalto(dynamic_cast<IContextIdent const&>(xIdentifier)) : false; }
	size_t hashcode(void) const override
	{ return rContext->hashcode() ^ TIdent::hashcode(); }
	TString toString(void) const override
	{ return rContext->toString() + _T('.') + TIdent::toString(); }

	bool equalto(IContextIdent const &xContextIdent) const
	{ return xContextIdent.rContext->equalto(*rContext) && TIdent::equalto(xContextIdent); }
};

template<>
class IContextIdent<IIdentifier> : public virtual IIdentifier{
protected:
	// Default constructor for virtual inheritance
	IContextIdent(void) {}

	// Replacement constructor
	void _Init(IIdentifier const &xContext)
	{ *const_cast<MRIdentifier*>(~rContext) = &xContext; }
public:
	MRIdentifier const rContext;

	IContextIdent(CONTEXT_CONSTRUCT_T const&, IIdentifier const &xContext)
	{ _Init(xContext); }

	bool equalto(IIdentifier const &xIdentifier) const override
	{ return typeid(xIdentifier) == typeid(*this) ? equalto(dynamic_cast<IContextIdent const&>(xIdentifier)) : false; }
	size_t hashcode(void) const override
	{ return rContext->hashcode(); }
	TString toString(void) const override
	{ return rContext->toString(); }

	bool equalto(IContextIdent const &xContextIdent) const
	{ return xContextIdent.rContext->equalto(*rContext); }
};

//======== Interface: CtxIdentPool ========
template<class TKey, class TIdentifier, class TContext = IIdentifier, class TKeyHasher = std::hash<TKey>>
class ICtxIdentPool : protected IIdentPool<TKey, TIdentifier, TKeyHasher>, public IIdentifier {
	ENFORCE_DERIVE(IIdentifier, TContext);
protected:
	typedef ManagedRef<TContext const> MRContext;

	// Default constructor for virtual inheritance
	ICtxIdentPool(void) {}

	// Replacement constructor
	void _Init(TContext const &xContext)
	{ *const_cast<MRContext*>(~rContext) = &xContext; }

public:
	MRContext const rContext;

	ICtxIdentPool(TContext const &xContext, CONTEXT_CONSTRUCT_T const&)
	{ _Init(xContext); }

	template<typename... Params>
	bool FindOrCreateIdent(TKey const &xKey, ManagedRef<TIdentifier>& xMRef, Params&&... xParams)
	{ return IIdentPool::FindOrCreateIdent(xKey, xMRef, *rContext, xParams...); }

	using IIdentPool::FindIdent;
	using IIdentPool::RemoveIdent;
	using IIdentPool::Flush;
	using IIdentPool::size;

	TString toString(void) const override
	{ return rContext->toString() + _T("{}"); }
};

//======== Interface: CtxNameIdent ========
class ICtxNameIdent : public virtual IContextIdent < INameIdent > {
protected:
	using IContextIdent::IContextIdent;
};

ICtxNameIdent& GetCtxNameIdent(TString const& xName, IIdentifier const &xContext = RootIdent());

//======== Interface: IAnnotation ========
template<class TNote>
class IAnnotation {
protected:
	// Default constructor for virtual inheritance
	IAnnotation(void) {}
	virtual ~IAnnotation(void) {}

	// Replacement constructor
	void _Init(TNote const &xNote)
	{ *const_cast<TNote*>(&Note) = xNote; }
public:
	TNote const Note;

	IAnnotation(TNote const &xNote)
	{ _Init(xNote); }

	virtual TString toString(void) const
	{ return Note.toString(); }
};

//======== Interface: AnnotatedIdent ========
template<class TIdent, class TNote>
class IAnnotatedIdent : public virtual TIdent, public IAnnotation < TNote > {
	ENFORCE_DERIVE(IIdentifier, TIdent);
protected:
	// Default constructor for virtual inheritance
	IAnnotatedIdent(void) {}

	template<typename... Params>
	void _Init(TNote const &xNote, Params&&... xParams)
	{ IAnnotation::_Init(xNote); TIdent::_Init(xParams...); }
public:
	template<typename... Params>
	IAnnotatedIdent(TNote const &xNote, Params&&... xParams)
	{ _Init(xNote, xParams...); }

	TString toString(void) const override
	{ return TIdent::toString() + _T('{') + IAnnotation::toString() + _T('}'); }
};

#endif //Identifier_H