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
 * @brief Misc utilities
 * @author Zhenyu Wu
 * @date Sep 25, 2013: Uplift from a child project
 * @date Jan 22, 2016: Initial Public Release
 **/

#ifndef Misc_H
#define Misc_H

#include <Windows.h>

#define VAWRAP(...) ,##__VA_ARGS__

#include <string>
#include <sstream>

#define EmptyAText	""
#define EmptyWText	_T("")
#define NullAChar	'\0'
#define NullWChar	_T('\0')

typedef std::basic_string<TCHAR> TString;
typedef std::basic_stringstream<TCHAR> TStringStream;

std::string TStringtoCPString(UINT CodePage, TString const &Str, TString &ErrMessage);
TString CPStringtoTString(UINT CodePage, std::string const &Str, TString &ErrMessage);

std::string TStringtoUTF8(TString const &Str);
std::string TStringtoUTF8_Check(TString const &Str);
std::string TStringtoUTF8(TString const &Str, TString &ErrMessage);
TString UTF8toTString(std::string const &Str);
TString UTF8toTString_Check(std::string const &Str);
TString UTF8toTString(std::string const &Str, TString &ErrMessage);

#define TStringCast(exp) dynamic_cast<TStringStream&>(TStringStream() << exp).str()
#define CStringCast(exp) dynamic_cast<std::stringstream&>(std::stringstream() << exp).str()

//extern std::string const EMPTY_CSTRING;
//extern TString const EMPTY_TSTRING;
std::string const& EMPTY_CSTRING(void);
TString const& EMPTY_TSTRING(void);

char const* ACP_LOCALE(void);

// Time Conversion Factors
#define HTime_aDay		24
#define MTime_anHour	60
#define STime_aMinute	60
#define MSTime_aSecond	1000
#define MSTime_o100ns	10000
#define MSTime_aNs		MSTime_o100ns * 100
#define MSTime_aMinute	(MSTime_aSecond*STime_aMinute)
#define MSTime_anHour	((UINT64)MSTime_aMinute*MTime_anHour)

#define W32UNIX_TSDIFF	11644473600000ULL
#define W32TStoUNIXMS(TS)	W32MStoUNIXMS(TS / MSTime_o100ns)
#define W32MStoUNIXMS(MS)	(MS - W32UNIX_TSDIFF)
#define W32TStoMSNS(TS)		TS % MSTime_o100ns * (MSTime_aNs / MSTime_o100ns)

// DataSize Conversion Factors
#define DataSize_KiloUnit	1024
#define BSize_aKB		DataSize_KiloUnit
#define BSize_aMB		(BSize_aKB*DataSize_KiloUnit)
#define BSize_aGB		(BSize_aMB*DataSize_KiloUnit)
#define BSize_aTB		((UINT64)BSize_aGB*DataSize_KiloUnit)

union Flatten_FILETIME {
	FILETIME FileTime;
	ULONG64 U64;
	TString toString(void) const;
};

Flatten_FILETIME GetCurrentSystemTime(void);

union INET4 {
	UINT32 Addr;
	struct {
		UINT8 A, B, C, D;
	};
	INET4(UINT32 T) : Addr(T) {}
	size_t hashcode(void) const;
	TString toString(void) const;
	bool equalto(INET4 const &T) const;
};

inline bool operator ==(INET4 const &A, INET4 const &B)
{ return A.equalto(B); }
inline bool operator !=(INET4 const &A, INET4 const &B)
{ return !(A == B); }

union UINT128 {
	struct { UINT64 U64A, U64B; };
	struct { UINT32 U32A, U32B, U32C, U32D; };
	struct { UINT16 U16[8]; };
	struct { UINT8 U8[16]; };
	UINT128(UINT64 A, UINT64 B) :
		U64A(A), U64B(B) {}
	UINT128(UINT32 A, UINT32 B, UINT32 C, UINT32 D) :
		U32A(A), U32B(B), U32C(C), U32D(D) {}
	size_t hashcode(void) const;
	static enum class Format {
		HEX64, HEX32, HEX16, HEX8, INET6
	};
	TString toString(Format const &Fmt = Format::HEX64) const;
	bool equalto(UINT128 const &T) const;
};

//extern UINT128 const UINT128_ZERO;
UINT128 const& UINT128_ZERO(void);

inline bool operator ==(UINT128 const &A, UINT128 const &B)
{ return A.equalto(B); }
inline bool operator !=(UINT128 const &A, UINT128 const &B)
{ return !(A == B); }

union HASH256 {
	struct { UINT64 U64A, U64B, U64C, U64D; };
	struct { UINT32 U32A, U32B, U32C, U32D, U32E, U32F, U32G, U32H; };
	struct { UINT16 U16[16]; };
	struct { UINT8 U8[32]; };
	HASH256(UINT64 A, UINT64 B, UINT64 C, UINT64 D) :
		U64A(A), U64B(B), U64C(C), U64D(D) {}
	size_t hashcode(void) const;
	TString toString(void) const;
	bool equalto(HASH256 const &T) const;
};

inline bool operator ==(HASH256 const &A, HASH256 const &B)
{ return A.equalto(B); }
inline bool operator !=(HASH256 const &A, HASH256 const &B)
{ return !(A == B); }

bool StrToInt(LPCTSTR String, int &i, int base = 10);
bool StrToDouble(LPCTSTR String, double &d);

UINT32 CountBits32(UINT32 Mask);
UINT32 CountBits64(UINT64 Mask);
#ifdef _WIN64
#define CountBits CountBits64
#else
#define CountBits CountBits32
#endif

int LOG2(UINT32 Value);
int LOG2(UINT64 Value);

#define ENFORCE_DERIVE(Base, Derived) \
	static_assert(std::is_base_of<Base, Derived>::value, "Type inheritance constraint violation")

// For use in Identifier disambiguation consutrction
struct CONTEXT_CONSTRUCT_T { CONTEXT_CONSTRUCT_T(void) {} };
extern CONTEXT_CONSTRUCT_T const CONTEXT_CONSTRUCT;

// For use in SyncObj emplace consutrction
struct EMPLACE_CONSTRUCT_T { EMPLACE_CONSTRUCT_T(void) {} };
extern EMPLACE_CONSTRUCT_T const EMPLACE_CONSTRUCT;

// For use in ManagedObj dereferencing construction
struct HANDOFF_CONSTRUCT_T { HANDOFF_CONSTRUCT_T(void) {} };
extern HANDOFF_CONSTRUCT_T const HANDOFF_CONSTRUCT;

// For use in ManagedObj assignment construction
struct ASSIGN_CONSTRUCT_T { ASSIGN_CONSTRUCT_T(void) {} };
extern ASSIGN_CONSTRUCT_T const ASSIGN_CONSTRUCT;

// For use in ManagedObj default construction
struct DEFAULT_CONSTRUCT_T { DEFAULT_CONSTRUCT_T(void) {} };
extern DEFAULT_CONSTRUCT_T const DEFAULT_CONSTRUCT;

#endif //Misc_H