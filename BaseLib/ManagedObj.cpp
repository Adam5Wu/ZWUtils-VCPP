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

// [Utilities] Managed Object Helpers
// Zhenyu Wu @ Oct 27, 2013: Split off template class

#include "MMSwitcher.h"

#include "ManagedObj.h"

DEBUGMEM(bool MEMDEBUG = true);

ManagedObj::~ManagedObj(void) {
	DEBUG({
		if (~RefCount != 0)
		LOG(_T("WARNING: Destruction on non-zero reference count!"));
	});
}

void ManagedObj::_AddRef(void) {
	int iRef = ++RefCount;
	DEBUGMEM(if (MEMDEBUG) LOGVV(_T("$MRef+ %s @%d"), toString().c_str(), iRef));
}

bool ManagedObj::_RemoveRef(void) {
	int iRef = --RefCount;
	DEBUGMEM(if (MEMDEBUG) LOGVV(_T("$MRef- %s @%d"), toString().c_str(), iRef));
	return iRef == 0;
}
