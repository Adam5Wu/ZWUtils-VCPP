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
 * @brief Managed Object Helpers
 * @author Zhenyu Wu
 * @date Oct 25, 2013: Refactored from a deprecated module
 * @date Oct 27, 2013: Split off template class
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef ManagedObj_H
#define ManagedObj_H

#include "Misc.h"
#include "Exception.h"
#include "ThreadLib/SyncPrems.h"

DEBUGMEM(extern bool MEMDEBUG);

class Cloneable {
protected:
	virtual ~Cloneable(void) {}
public:
	virtual Cloneable* Clone(void) const
	{ FAIL(_T("Abstract function")); }

	template<class T,
		typename = std::enable_if<std::is_base_of<Cloneable, T>::value>::type>
		static Cloneable* GetClone(T const &Obj)
	{ return Obj->Clone(); }

	template<class T,
		typename = std::enable_if<!std::is_base_of<Cloneable, T>::value>::type,
		typename = std::enable_if<std::is_polymorphic<T>::value>::type>
		static Cloneable* GetClone(T const &Obj) {
		if (auto CRef = dynamic_cast<Cloneable const*>(&Obj))
			return CRef->Clone();
		return nullptr;
	}

	template<class T,
		typename = std::enable_if<!std::is_base_of<Cloneable, T>::value>::type,
		typename = std::enable_if<!std::is_polymorphic<T>::value>::type,
		typename = void>
		static Cloneable* GetClone(T const&)
	{ return nullptr; }
};

class ManagedObj {
private:
	TInterlockedSyncOrdinal32<int> RefCount;
protected:
	ManagedObj(unsigned int RefInit) : RefCount(RefInit) {}
	virtual ~ManagedObj(void);
public:
	ManagedObj(void) : RefCount(0) {}

	void _AddRef(void);
	bool _RemoveRef(void);

	inline int _RefCount(void)
	{ return ~RefCount; }

	virtual TString toString(void) const
	{ return TStringCast(_T("ManagedObj(") << (void*)this << _T(')')); }

	template<class T,
		typename = std::enable_if<std::is_base_of<ManagedObj, T>::value>::type>
		static ManagedObj const* Cast(T const &Obj)
	{ return &Obj; }

	template<class T,
		typename = std::enable_if<!std::is_base_of<ManagedObj, T>::value>::type,
		typename = std::enable_if<std::is_polymorphic<T>::value>::type>
		static ManagedObj const* Cast(T const &Obj)
	{ return dynamic_cast<ManagedObj const*>(&Obj); }

	template<class T,
		typename = std::enable_if<!std::is_base_of<ManagedObj, T>::value>::type,
		typename = std::enable_if<!std::is_polymorphic<T>::value>::type,
		typename = void>
		static ManagedObj const* Cast(T const&)
	{ return nullptr; }
};

#endif //ManagedObj_H