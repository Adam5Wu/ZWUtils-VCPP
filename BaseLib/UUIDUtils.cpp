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

// [Utilities] Basic UUID Operations

#include "MMSwitcher.h"

#include <errno.h>

#include "UUIDUtils.h"
#include "Exception.h"

void UUIDToString(UUID const &Value, LPTSTR String) {
	int result = _sntprintf_s(String, 36 + 1, 36 + 1, _T("%08X-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X"),
							  Value.Data1, Value.Data2, Value.Data3,
							  Value.Data4[0], Value.Data4[1], Value.Data4[2], Value.Data4[3],
							  Value.Data4[4], Value.Data4[5], Value.Data4[6], Value.Data4[7]);
	if (result < 0)
		FAIL(_T("Converting from GUID to string failed with error code %d"), errno);
}

UUID UUIDFromString(LPCTSTR const &String) {
	UUID output;

	// Work around VC++ missing support for "hhX"
	int tail8[8];
	int result = _stscanf_s(String, _T("%8X-%4hX-%4hX-%2X%2X-%2X%2X%2X%2X%2X%2X"),
							&output.Data1, &output.Data2, &output.Data3,
							&tail8[0], &tail8[1], &tail8[2], &tail8[3],
							&tail8[4], &tail8[5], &tail8[6], &tail8[7]);
	if (result < 0)
		FAIL(_T("Converting from string to GUID failed with error code %d"), errno);
	if (result < 11)
		FAIL(_T("Converting from string to GUID failed strating from %d"), result + 1);
	// Work around VC++ missing support for "hhX"
	for (int i = 0; i < 8; i++)
		output.Data4[i] = tail8[i];
	return output;
}

Flatten_UUID const UUID_NULL{0, 0};

int UUIDCompare(UUID const &A, UUID const &B) {
	return UUIDCompare(*((Flatten_UUID*)&A), *((Flatten_UUID*)&B));
}

int UUIDCompare(Flatten_UUID const &A, Flatten_UUID const &B) {
	return ((A.U32A ^ B.U32A) | (A.U32B ^ B.U32B) | (A.U32C ^ B.U32C) | (A.U32D ^ B.U32D)) != 0;
}

bool operator ==(Flatten_UUID const& A, Flatten_UUID const& B) {
	return (A.U64A == B.U64A) && (A.U64B == B.U64B);
}
