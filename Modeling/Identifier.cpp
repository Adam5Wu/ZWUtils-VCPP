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

// [Modeling] Identifiers and Identifier Pools

#include "BaseLib/MMSwitcher.h"

#include "Identifier.h"

// IIdentifier
bool operator==(IIdentifier const &A, IIdentifier const &B) {
	return A.equalto(B);
}

bool operator!=(IIdentifier const &A, IIdentifier const &B) {
	return !A.equalto(B);
}

size_t operator~(IIdentifier const &A) {
	return A.hashcode();
}

// RootIdent
class RootIdentifier final : public IIdentifier, public ManagedObj {
public:
	bool equalto(IIdentifier const &xIdentifier) const override
	{ return &xIdentifier == this; }
	size_t hashcode(void) const override
	{ return 0; }
	TString toString(void) const override
	{ return _T("!"); }
};

IIdentifier const& RootIdent(void) {
	static ManagedRef<RootIdentifier> __IoFU_T(EMPLACE_CONSTRUCT);
	return __IoFU_T;
}


// INameIdent
bool INameIdent::equalto(IIdentifier const &xIdentifier) const {
	return typeid(xIdentifier) == typeid(*this) ? equalto(dynamic_cast<INameIdent const&>(xIdentifier)) : false;
}

bool INameIdent::equalto(INameIdent const &xNameIdent) const {
	return xNameIdent.Name.compare(Name) == 0;
}

size_t INameIdent::hashcode(void) const {
	return std::hash<TString>()(Name);
}

TString INameIdent::toString(void) const {
	return Name;
}

// NameIdentPool
typedef ManagedObjAdapter<INameIdent> IPoolNameIdent;
class INameIdentPool final : public IIdentPool < TString, IPoolNameIdent > {};

INameIdent& GetNameIdent(TString const &xName) {
	static INameIdentPool NameIdentPool;
	ManagedRef<IPoolNameIdent> Ret;
	return (NameIdentPool.FindOrCreateIdent(xName, Ret, xName), Ret);
}

// CtxNameIdentPool
typedef ManagedObjAdapter<ICtxNameIdent> IPoolCtxNameIdent;
//Converted to class definition due to C4503
//typedef ManagedObjAdapter<ICtxIdentPool<TString, IPoolCtxNameIdent>> ICtxNameIdentPool;
class ICtxNameIdentPool final : public ICtxIdentPool<TString, IPoolCtxNameIdent>, public ManagedObj {
public:
	ICtxNameIdentPool(IIdentifier const &xContext) :
		ICtxIdentPool(xContext, CONTEXT_CONSTRUCT) {}

	TString toString(void) const override
	{ return ICtxIdentPool::toString(); }
};
class ICtxNameIdentPools final : public IIdentPool < MRIdentifier, ICtxNameIdentPool > {};

ICtxNameIdentPool& GetCtxNameIdentPool(IIdentifier const &xContext) {
	static ICtxNameIdentPools CtxNameIdentPools;
	ManagedRef<ICtxNameIdentPool> Ret;
	return (CtxNameIdentPools.FindOrCreateIdent(MRIdentifier(&xContext), Ret, xContext), Ret);
}

ICtxNameIdent& GetCtxNameIdent(TString const &xName, IIdentifier const &xContext) {
	ICtxNameIdentPool &CtxNameIdentPool = GetCtxNameIdentPool(xContext);
	ManagedRef<IPoolCtxNameIdent> Ret;
	return (CtxNameIdentPool.FindOrCreateIdent(xName, Ret, xName), Ret);
}
