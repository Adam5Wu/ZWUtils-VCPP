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
 * @brief Object Allocation Helpers
 * @author Zhenyu Wu
 * @date Oct 27, 2013: Refactored from SyncObjs
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef Allocator_H
#define Allocator_H

#include "Misc.h"
#include "Exception.h"

#include <functional>

template<class T>
struct IAllocator {};

/**
 * @ingroup Utilities
 * @brief Object allocator template
 *
 * Manages the life cycle of an object
 **/
template<typename T>
struct SimpleAllocator : public IAllocator < T > {
	/**
	 * Create an object
	 **/
	template<typename... Params>
	inline static T* Create(Params&&... xParams)
	{ return new T(xParams...); }

	/**
	 * Destroy an object
	 **/
	inline static void Destroy(T *Obj)
	{ delete Obj; }
};

/**
 * @ingroup Utilities
 * @brief Dummy object allocator
 *
 * A dummy object allocator that will NOT perform any managing
 * @note Useful for working with externally managed objects
 **/
template<typename T>
struct DummyAllocator : public IAllocator < T > {
	template<typename... Params>
	inline static T* Create(Params&&... xParams)
	{ FAIL(_T("Should not reach")); }

	inline static void Destroy(T *Obj) {}
};

/**
 * @ingroup Utilities
 * @brief Lambda object allocator template
 *
 * Manages the life cycle of an object with lambda function
 **/
template<typename T>
struct LambdaAllocator : public IAllocator < T > {
	std::function<T*()> const Create;
	std::function<void(T*)> const Destroy;
	LambdaAllocator(std::function<T*()> A, std::function<void(T*)> B) :
		Create(A), Destroy(B) {}
	LambdaAllocator(std::function<T*()> A) :
		Create(A), Destroy([](T*) {FAIL(_T("Should not reach")); }) {}
};

#endif //Allocator_H