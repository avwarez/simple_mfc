// initguid.h -- POSIX stand-in for the SDK header that switches DEFINE_GUID
// from "declare" to "define".
//
// PART OF THE Win32 PLATFORM SHIM, NOT OF THE MFC INTERFACE.
//
// The real header exists purely for this side effect: include it before a
// header full of DEFINE_GUID lines and that translation unit gets the storage,
// while every other one gets an extern declaration. The mechanism is
// reproduced rather than bypassed, because eMule relies on exactly one TU
// including this file -- bypassing it would give every TU its own copy and a
// pile of duplicate symbols at link time.
#pragma once

#ifdef _WIN32
#error "simple_mfc/platform is for non-Windows builds only."
#endif

#include <atlcomcli.h>          // GUID

#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    extern "C" const GUID name = {l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}
