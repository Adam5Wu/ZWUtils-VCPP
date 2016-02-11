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
 * @brief Basic UUID Operations
 * @author Zhenyu Wu
 * @date Sep 24, 2013: Uplift from a child project
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef UUIDUtils_H
#define UUIDUtils_H

#include <Windows.h>
#include <functional>

void UUIDToString(UUID const &Value, LPTSTR String);
UUID UUIDFromString(LPCTSTR const &String);

union Flatten_UUID {
	UUID Value;
	struct { UINT64 U64A, U64B; };
	struct { UINT32 U32A, U32B, U32C, U32D; };

	Flatten_UUID(UUID const &xValue) :
		Value(xValue) {}
	Flatten_UUID(UINT64 A, UINT64 B) :
		U64A(A), U64B(B) {}
	Flatten_UUID(UINT32 A, UINT32 B, UINT32 C, UINT32 D) :
		U32A(A), U32B(B), U32C(C), U32D(D) {}
};

extern Flatten_UUID const UUID_NULL;

int UUIDCompare(UUID const &A, UUID const &B);
int UUIDCompare(Flatten_UUID const &A, Flatten_UUID const &B);

template<>
struct std::hash < Flatten_UUID > {
	size_t operator ()(Flatten_UUID const& T)
	{ return hash<UINT64>()(T.U64A) ^ hash<UINT64>()(T.U64B); }
};

bool operator ==(Flatten_UUID const& A, Flatten_UUID const& B);
inline bool operator !=(Flatten_UUID const& A, Flatten_UUID const& B)
{ return !(A == B); }

#endif //UUIDUtils_H