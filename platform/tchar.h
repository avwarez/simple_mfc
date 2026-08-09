// tchar.h -- POSIX stand-in for the Microsoft CRT's TCHAR mappings.
//
// PART OF THE Win32 PLATFORM SHIM, NOT OF THE MFC INTERFACE.
//
// This one is a clean case of the native-symbol rule: every name below is a
// Microsoft alias for a function the C standard library already has. Under
// UNICODE the _t* family maps to the wide functions, otherwise to the narrow
// ones -- exactly what the real tchar.h does, and eMule is built with UNICODE
// defined. Nothing here is emulated; the mapping IS the implementation.
//
// Not generated from the SDK headers like win32_types.h/win32_constants.h,
// because the SDK's versions map to MSVC CRT entry points (_wcsicmp and
// friends) that do not exist here. The mapping has to be re-aimed at glibc,
// and re-aiming is a decision, not a transcription.
#pragma once

#ifdef _WIN32
#error "simple_mfc/platform is for non-Windows builds only."
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <strings.h>        // strcasecmp/strncasecmp: POSIX, not ISO C

#ifdef _UNICODE

#define _tcslen     std::wcslen
#define _tcscmp     std::wcscmp
#define _tcsncmp    std::wcsncmp
#define _tcscpy     std::wcscpy
#define _tcsncpy    std::wcsncpy
#define _tcscat     std::wcscat
#define _tcsncat    std::wcsncat
#define _tcschr     std::wcschr
#define _tcsrchr    std::wcsrchr
#define _tcsstr     std::wcsstr
#define _tcstok     std::wcstok
#define _tcsspn     std::wcsspn
#define _tcscspn    std::wcscspn
#define _tcstol     std::wcstol
#define _tcstoul    std::wcstoul
#define _tcstod     std::wcstod
#define _tcstoi64   std::wcstoll
#define _tcstoui64  std::wcstoull
#define _ttoi(s)    ((int)std::wcstol((s), nullptr, 10))
#define _ttoi64(s)  std::wcstoll((s), nullptr, 10)
#define _ttof(s)    std::wcstod((s), nullptr)
#define _tprintf    std::wprintf
#define _ftprintf   std::fwprintf
#define _vftprintf  std::vfwprintf
#define _vsntprintf std::vswprintf
#define _stscanf    std::swscanf
#define _sntprintf  std::swprintf
#define _istdigit   std::iswdigit
#define _istalpha   std::iswalpha
#define _istalnum   std::iswalnum
#define _istspace   std::iswspace
#define _istupper   std::iswupper
#define _istlower   std::iswlower
#define _istpunct   std::iswpunct
#define _totupper   std::towupper
#define _totlower   std::towlower

// The case-insensitive comparisons are the one place with no ISO C wide
// counterpart: POSIX has strcasecmp for narrow strings but nothing for wide.
// wcscasecmp/wcsncasecmp are glibc extensions, which is precisely what we are
// building against, so use them rather than hand-rolling a loop that would get
// the locale wrong.
#define _tcsicmp    ::wcscasecmp
#define _tcsnicmp   ::wcsncasecmp
#define _wcsicmp    ::wcscasecmp
#define _wcsnicmp   ::wcsncasecmp

#else   // !_UNICODE

#define _tcslen     std::strlen
#define _tcscmp     std::strcmp
#define _tcsncmp    std::strncmp
#define _tcscpy     std::strcpy
#define _tcsncpy    std::strncpy
#define _tcscat     std::strcat
#define _tcsncat    std::strncat
#define _tcschr     std::strchr
#define _tcsrchr    std::strrchr
#define _tcsstr     std::strstr
#define _tcstok     std::strtok
#define _tcsspn     std::strspn
#define _tcscspn    std::strcspn
#define _tcstol     std::strtol
#define _tcstoul    std::strtoul
#define _tcstod     std::strtod
#define _tcstoi64   std::strtoll
#define _tcstoui64  std::strtoull
#define _ttoi       std::atoi
#define _ttoi64     std::atoll
#define _ttof       std::atof
#define _tprintf    std::printf
#define _ftprintf   std::fprintf
#define _vftprintf  std::vfprintf
#define _vsntprintf std::vsnprintf
#define _stscanf    std::sscanf
#define _sntprintf  std::snprintf
#define _istdigit   std::isdigit
#define _istalpha   std::isalpha
#define _istalnum   std::isalnum
#define _istspace   std::isspace
#define _istupper   std::isupper
#define _istlower   std::islower
#define _istpunct   std::ispunct
#define _totupper   std::toupper
#define _totlower   std::tolower
#define _tcsicmp    ::strcasecmp
#define _tcsnicmp   ::strncasecmp

#endif  // _UNICODE

// Narrow-only spellings eMule uses directly, regardless of UNICODE.
#define _stricmp    ::strcasecmp
#define _strnicmp   ::strncasecmp
#define _strlwr     smfc_strlwr
#define _strupr     smfc_strupr

inline char *smfc_strlwr(char *s)
{
    for (char *p = s; p != nullptr && *p != '\0'; ++p)
        *p = static_cast<char>(std::tolower(static_cast<unsigned char>(*p)));
    return s;
}

inline char *smfc_strupr(char *s)
{
    for (char *p = s; p != nullptr && *p != '\0'; ++p)
        *p = static_cast<char>(std::toupper(static_cast<unsigned char>(*p)));
    return s;
}
