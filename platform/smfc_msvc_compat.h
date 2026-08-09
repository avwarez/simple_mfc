// smfc_msvc_compat.h -- the Microsoft COMPILER extensions, as opposed to the
// Microsoft SDK headers that the rest of this directory stands in for.
//
// PART OF THE Win32 PLATFORM SHIM, NOT OF THE MFC INTERFACE.
//
// This file is different from every other header here in one important way:
// it is not #included by anything. It is force-included by the build (-include)
// ahead of every translation unit, because what it provides are things MSVC
// makes available with no header at all -- `__int64` is a keyword, not a
// typedef, and code uses it before it could possibly have included something.
// A header cannot retrofit a keyword after the fact; only a forced include can
// put it in place first.
//
// GCC/Clang's -fms-extensions covers part of the same ground (anonymous
// structs/unions, __unaligned, some __declspec spellings) but deliberately
// does not add the fixed-width keywords or the calling-convention keywords on
// non-x86 targets, so those are supplied here.
#pragma once

#ifdef _WIN32
#error "simple_mfc/platform is for non-Windows builds only."
#endif

// ---------------------------------------------------------------------
// Fixed-width integer keywords.
//
// MSVC spells these as keywords; there is no header to include. Mapping them
// with the compiler's own fixed-width types rather than with `long long` &c
// keeps the widths right on both LP64 (Linux) and LLP64 (Windows), which
// matters because eMule serialises these types straight onto the wire.
// ---------------------------------------------------------------------
#ifndef __int8
#define __int8   char
#endif
#ifndef __int16
#define __int16  short
#endif
#ifndef __int32
#define __int32  int
#endif
#ifndef __int64
#define __int64  long long
#endif

// ---------------------------------------------------------------------
// Calling conventions.
//
// On Windows/x86 these select an actual ABI and are part of a function's
// type. On x86-64 and on ARM there is only one calling convention, so
// Windows itself defines them away -- and GCC only recognises the attribute
// spellings on i386 at all. Empty is therefore both correct and required.
// ---------------------------------------------------------------------
#ifndef __stdcall
#define __stdcall
#endif
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __fastcall
#define __fastcall
#endif
#ifndef __thiscall
#define __thiscall
#endif
#ifndef WINAPI
#define WINAPI
#endif
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef CALLBACK
#define CALLBACK
#endif

// ---------------------------------------------------------------------
// Storage-class and inlining extensions.
//
// __declspec is swallowed rather than translated: its arguments (dllimport,
// dllexport, novtable, selectany, align(n), ...) are all either meaningless
// for a single statically-linked non-Windows binary or -- in the case of
// align -- better left to the compiler than mistranslated. Discarding it is
// the deliberate choice, not an oversight.
// ---------------------------------------------------------------------
#ifndef __declspec
#define __declspec(x)
#endif
#ifndef __forceinline
#define __forceinline inline
#endif
#ifndef __unaligned
#define __unaligned
#endif
#ifndef __pragma
#define __pragma(x)
#endif
#ifndef __noop
#define __noop(...) ((void)0)
#endif

// ---------------------------------------------------------------------
// SAL source-annotation macros. MSVC's own headers define these to nothing
// in non-analysis builds; eMule uses a handful directly.
// ---------------------------------------------------------------------
#ifndef _In_
#define _In_
#define _In_opt_
#define _Out_
#define _Out_opt_
#define _Inout_
#define _Inout_opt_
#define _Ret_maybenull_
#define _Printf_format_string_
#endif

// ---------------------------------------------------------------------
// _countof.
//
// MSVC's CRT provides it with no header, which is why it belongs here rather
// than in a generated file. The SDK's own definition leans on an
// __countof_helper template that only exists inside the MSVC/mingw CRT, so
// that spelling is not usable off Windows -- but the C++ form below is
// strictly better anyway: it fails to compile on a pointer, where the naive
// sizeof/sizeof macro silently returns a wrong count.
//
// Defined here, ahead of everything, so the #ifndef guard in the generated
// win32_constants.h leaves it alone.
// decltype(sizeof(0)) rather than std::size_t: this file is force-included
// ahead of everything, so it must not depend on a header having been read.
#if defined(__cplusplus) && !defined(_countof)
template <typename T, decltype(sizeof(0)) N>
char (&smfc_countof_helper(T (&)[N]))[N];
#define _countof(a) (sizeof(smfc_countof_helper(a)))
#endif
