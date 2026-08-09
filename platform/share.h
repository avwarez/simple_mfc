// share.h -- POSIX stand-in for the Microsoft CRT's file-sharing header.
//
// PART OF THE Win32 PLATFORM SHIM, NOT OF THE MFC INTERFACE.
//
// The _SH_* constants are generated into win32_constants.h from the SDK. What
// this file adds is the one function eMule reaches for, _tfsopen, and an
// honest account of what happens to its third argument.
//
// On Windows the share flag is MANDATORY locking enforced by the kernel: a
// second open that violates it fails. Linux file locking is ADVISORY -- an
// unrelated process that never asks for a lock is not stopped by one. There is
// no way to make fopen behave like _wfsopen, so the flag is accepted and
// dropped rather than half-implemented with flock(), which would add a lock
// eMule never asked to be released and change the failure mode instead of
// matching it.
//
// This matters in exactly one place worth knowing about: eMule opens its
// part.met and known.met with _SH_DENYWR to keep a second instance from
// corrupting them. Off Windows that protection is gone, and a second instance
// is a user error rather than something the file layer will catch.
#pragma once

#ifdef _WIN32
#error "simple_mfc/platform is for non-Windows builds only."
#endif

#include <cstdio>
#include <cwchar>

#include <win32_constants.h>

inline std::FILE *_wfsopen(const wchar_t *filename, const wchar_t *mode, int /*shflag*/)
{
    char path[4096];
    char m[16];
    if (std::wcstombs(path, filename, sizeof path) == static_cast<std::size_t>(-1))
        return nullptr;
    if (std::wcstombs(m, mode, sizeof m) == static_cast<std::size_t>(-1))
        return nullptr;
    return std::fopen(path, m);
}

inline std::FILE *_fsopen(const char *filename, const char *mode, int /*shflag*/)
{
    return std::fopen(filename, mode);
}

#ifdef _UNICODE
#define _tfsopen _wfsopen
#else
#define _tfsopen _fsopen
#endif
