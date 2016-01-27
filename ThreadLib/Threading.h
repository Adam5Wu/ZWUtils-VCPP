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
 * @addtogroup Threading Threading Support Utilities
 * @file
 * @brief Basic Threading Support
 * @author Zhenyu Wu
 * @date Mar 01, 2005: Initial implementation
 * @date Jul 31, 2013: Port to Visual C++ 2012
 * @date Jan 29, 2014: Make the code C++ style (avoid performance hit of __try __finally)
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef Threading_H
#define Threading_H

#include <Windows.h>

#include "SyncPrems.h"
#include "SyncObjs.h"

typedef TInterlockedSyncOrdinal32<INT32> TSyncInt32;
typedef TInterlockedSyncOrdinal64<INT64> TSyncInt64;
typedef TInterlockedSyncOrdinal32<UINT32> TSyncUInt32;
typedef TInterlockedSyncOrdinal64<UINT64> TSyncUInt64;
typedef TInterlockedSyncOrdinal32<BOOL> TSyncBool;
typedef TInterlockedSyncOrdinal<PVOID> TSyncPtr;

#ifdef _WIN64
typedef INT64 __ARC_INT;
typedef UINT64 __ARC_UINT;
typedef TSyncInt64 TSyncInt;
#else
typedef INT32 __ARC_INT;
typedef UINT32 __ARC_UINT;
typedef TSyncInt32 TSyncInt;
#endif //_WIN64

/**
 * @ingroup Threading
* @brief Synchronized Code Wrapper
*
* Make a piece of code synchronized with respect to a lockable object
**/
#define Synchronized(lockable,code) { auto __Lock__ = lockable.SyncLock(); code; }

//extern TLockable* Lock_ConsoleIn;
//extern TLockable* Lock_ConsoleOut;
//extern TLockable* Lock_ConsoleErr;
//extern TLockable* Lock_DebugLog;
//extern TLockable* Lock_Global;
PLockable& Lock_ConsoleIn(void);
PLockable& Lock_ConsoleOut(void);
PLockable& Lock_ConsoleErr(void);
PLockable& Lock_DebugLog(void);
PLockable& Lock_Global(void);

#endif //Threading_H
