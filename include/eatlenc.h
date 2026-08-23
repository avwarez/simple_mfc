#pragma once
#include "eafx.h"

#ifndef _WIN32
using LPSTR = char*;
#endif

#ifndef EATL_BASE64_FLAG_NONE
#define EATL_BASE64_FLAG_NONE   0
#define EATL_BASE64_FLAG_NOPAD  1
#define EATL_BASE64_FLAG_NOCRLF 2
#endif

int EBase64EncodeGetRequiredLength(int nSrcLen, DWORD dwFlags = EATL_BASE64_FLAG_NONE) noexcept;

BOOL EBase64Encode(const BYTE* pbSrcData, int nSrcLen, LPSTR szDest,
                  int* pnDestLen, DWORD dwFlags = EATL_BASE64_FLAG_NONE) noexcept;
