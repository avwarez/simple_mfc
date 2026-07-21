// atlconv.h — NATIVE implementation (standard C++17 library only).
// ATL's string conversion helpers. Only the UTF-8 encoder is declared:
// it is the one eMule/srchybrid names (StringConversion.cpp, 7 calls,
// in the two-pass "measure then fill" form the signature supports).
//
// Real atlconv.h also carries the USES_CONVERSION / W2A / A2W / CT2A
// macro family; none is declared here, because eMule uses none of them.
// Signature verified against the Microsoft Learn ATL text encoding
// functions page.
#pragma once
#include "afx.h"

// Off Windows these narrow/wide pointer aliases do not exist (afx.h only
// defines the TCHAR-shaped ones); on Windows they come from <windows.h>.
#ifndef _WIN32
using LPCWSTR = const wchar_t*;
using LPSTR = char*;
#endif

// Returns the number of bytes required when szDest is NULL and nDest 0,
// which is exactly how eMule sizes its buffer before the second call.
int AtlUnicodeToUTF8(LPCWSTR wszSrc, int nSrc, LPSTR szDest, int nDest) noexcept;

// The code page ATL's conversion macros use. Internal (leading
// underscore) and undocumented, but eMule names it directly -- twice as a
// default argument in StringConversion.h, so it must be declared here.
UINT _AtlGetConversionACP() noexcept;
