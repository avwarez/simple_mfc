#pragma once

#ifdef _WIN32
#include <tchar.h>
using EWCHAR = wchar_t;
#else
using EWCHAR = char16_t;
using TCHAR = EWCHAR;
using LPCTSTR = const TCHAR*;
using LPTSTR = TCHAR*;
#ifndef _T
#define _T(x) u##x
#endif
#endif
