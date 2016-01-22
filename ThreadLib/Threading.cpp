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

// [Threading] Basic Threading Support
// Zhenyu Wu @ Jul 26, 2013: Porting to Visual C++ 2012

#include "BaseLib/MMSwitcher.h"

#include "Threading.h"

//static TLockableCS CSLock_ConsoleIn;
//static TLockableCS CSLock_ConsoleOut;
//static TLockableCS CSLock_ConsoleErr;
//static TLockableCS CSLock_DebugLog;
//static TLockableCS CSLock_Global;
//
//static TLockable* Lock_ConsoleIn = &CSLock_ConsoleIn;
//static TLockable* Lock_ConsoleOut = &CSLock_ConsoleOut;
//static TLockable* Lock_ConsoleErr = &CSLock_ConsoleErr;
//static TLockable* Lock_DebugLog = &CSLock_DebugLog;
//static TLockable* Lock_Global = &CSLock_Global;

PLockable& Lock_ConsoleIn(void) {
	static TLockableCS __IoFU_T;
	static PLockable __IoFU = &__IoFU_T;
	return __IoFU;
}

PLockable& Lock_ConsoleOut(void) {
	static TLockableCS __IoFU_T;
	static PLockable __IoFU = &__IoFU_T;
	return __IoFU;
}

PLockable& Lock_ConsoleErr(void) {
	static TLockableCS __IoFU_T;
	static PLockable __IoFU = &__IoFU_T;
	return __IoFU;
}

PLockable& Lock_DebugLog(void) {
	static TLockableCS __IoFU_T;
	static PLockable __IoFU = &__IoFU_T;
	return __IoFU;
}

PLockable& Lock_Global(void) {
	static TLockableCS __IoFU_T;
	static PLockable __IoFU = &__IoFU_T;
	return __IoFU;
}
