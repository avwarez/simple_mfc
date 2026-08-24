#pragma once
#include "eafx.h"

#ifndef _WIN32
using LPCWSTR = const EWCHAR*;
using LPSTR = char*;
#endif

int EAtlUnicodeToUTF8(LPCWSTR wszSrc, int nSrc, LPSTR szDest, int nDest) noexcept;

UINT E_AtlGetConversionACP() noexcept;
