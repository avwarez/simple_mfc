// atlbase.h — the ATL umbrella header, as in real ATL: it declares almost
// nothing itself and pulls in the headers that do.
//
// Why simple_mfc carries ATL headers at all: ATL is a separate library
// that ships with MSVC whether or not MFC is used, so the project's rule
// has been to defer to the real one. That stopped working once these
// headers shadowed MFC's, because real ATL's atlstr.h then contributes
// its own ATL::CString while simple_mfc contributes ::CString, and the
// automatic "using namespace ATL" makes every unqualified use ambiguous
// (C2872). Shadowing the ATL surface eMule/srchybrid actually reaches
// removes the collision and keeps the compile check self-contained.
//
// Deliberately NOT provided, because eMule names none of them:
// CComModule, CComObject and the object-map machinery,
// CComCriticalSection, the conversion macros (USES_CONVERSION/W2A/...),
// CAtlList/CAtlArray/CAtlMap, and ATL's own string classes.
//
// CRegKey and AtlUnicodeToUTF8 were on that list until the first CI round
// proved otherwise. The lesson: build the list by sweeping eMule for
// every Atl*/C*-shaped ATL name, not by checking a candidate list.
#pragma once

#include "atldef.h"      // ATLASSERT & co
#include "atlcomcli.h"   // CComPtr, CComQIPtr, CComBSTR, CComVariant
#include "atlsimpcoll.h" // CSimpleArray
#include "atlconv.h"     // AtlUnicodeToUTF8

// Registry key wrapper. Real ATL declares CRegKey in atlbase.h itself.
// eMule uses it for the shell/file-association settings; signatures
// verified against the Microsoft Learn CRegKey class page.
#ifndef _WIN32
using HKEY = void*;
using REGSAM = unsigned long;
using LPSECURITY_ATTRIBUTES = void*;
using LPDWORD = unsigned long*;
using ULONG = unsigned long;
using LONG = long;
#define REG_NONE 0
#define REG_SZ 1
#define REG_OPTION_NON_VOLATILE 0
#define KEY_READ 0x20019
#define KEY_WRITE 0x20006
#endif

class CRegKey
{
public:
    CRegKey() noexcept;
    ~CRegKey() noexcept;

    HKEY m_hKey;
    operator HKEY() const noexcept;

    LONG Create(HKEY hKeyParent, LPCTSTR lpszKeyName, LPTSTR lpszClass = (LPTSTR)REG_NONE,
                DWORD dwOptions = REG_OPTION_NON_VOLATILE,
                REGSAM samDesired = KEY_READ | KEY_WRITE,
                LPSECURITY_ATTRIBUTES lpSecAttr = nullptr,
                LPDWORD lpdwDisposition = nullptr) noexcept;
    LONG Open(HKEY hKeyParent, LPCTSTR lpszKeyName,
              REGSAM samDesired = KEY_READ | KEY_WRITE) noexcept;
    LONG Close() noexcept;

    LONG QueryStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) noexcept;
    LONG QueryDWORDValue(LPCTSTR pszValueName, DWORD& dwValue) noexcept;
    LONG SetStringValue(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD dwType = REG_SZ) noexcept;
    LONG DeleteValue(LPCTSTR lpszValue) noexcept;
    LONG RecurseDeleteKey(LPCTSTR lpszKey) noexcept;
};
