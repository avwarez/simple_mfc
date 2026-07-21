// afx.h — part of simple_mfc. NATIVE implementation (standard C++17
// library only): same class/method names as real MFC, but with working,
// portable bodies (no Windows headers, no GUI, no PE resources). The
// GUI/socket counterparts (afxwin.h, afxsock.h...) remain declaration-only
// for now, not yet implemented — see ../README.md.
//
// What is NOT implemented here because it is inherently Windows/GUI
// specific (see ../README.md for the full list):
//   - CObject::Serialize/CArchive (MFC serialization infrastructure, tied
//     to CFile/CDocument in a Windows-specific way, out of scope)
//   - CString::LoadString (loads strings from PE .rc resources, no
//     standard C++ equivalent)
//   - CException::ReportError only prints to stderr, not a real MessageBox
#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

// Compatibility with the headers that are still "declaration-only" (afxdd_.h,
// afxwin.h...), which use these MSVC-style macros/keywords. No-op on
// non-MSVC compilers.
#ifndef _MSC_VER
#define __cdecl
#define __stdcall
#endif
#define AFXAPI __stdcall
#define AFX_CDECL __cdecl   // real MFC's calling-convention marker on static
                            // thread procs (`static UINT AFX_CDECL RunProc(...)`)

using UINT = unsigned int;
using WORD = unsigned short;
using BYTE = unsigned char;
using DWORD = unsigned long;
using HANDLE = void*;   // identical to <windows.h>'s `typedef void* HANDLE`
using LONG = long;
using LONGLONG = long long;
using ULONGLONG = unsigned long long;
using BOOL = int;
constexpr BOOL TRUE_ = 1;
constexpr BOOL FALSE_ = 0;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
// Real LPTSTR/LPCTSTR (winnt.h) are typedef chains through WCHAR*, not a
// bare wchar_t* alias -- MSVC's redefinition check treats that as a
// "different basic type" even though the two are layout-identical, so
// this collides with real <windows.h> the same way afxwin.h's
// SECURITY_ATTRIBUTES/CREATESTRUCT do (see there). Deferred to the real
// header on _WIN32. CString and friends are unconditionally wide-char
// (std::wstring-backed) regardless of the project's own charset setting,
// so UNICODE/_UNICODE are forced here too -- otherwise winnt.h's LPTSTR
// resolves to the ANSI (char*) alias and every wide-char call site in
// this library (wmemcpy, CDumpContext::operator<<...) stops matching.
#ifdef _WIN32
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
// Stop <windows.h> from dragging in the legacy <winsock.h> (Winsock 1):
// eMule/srchybrid (and our own afxsock.h) include <winsock2.h>, and the two
// redefine the same structs/functions -> thousands of C2011/C2375 errors.
// This is exactly what real MFC's afxv_w32.h does before it includes
// <windows.h>; _WINSOCKAPI_ is winsock.h's own include guard, so pre-defining
// it makes windows.h skip winsock.h and leaves the field to winsock2.h.
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <windows.h>
// <windows.h> does NOT pull in <tchar.h>, so _T()/TCHAR/_tcs* would be
// missing on a real Windows build too -- real MFC gets them because its own
// afx.h includes <tchar.h>. Match that. Under the forced _UNICODE above,
// _T maps to the L"" prefix and _tcs* to the wcs* family, matching our
// unconditionally-wide CString.
#include <tchar.h>
#else
using LPCTSTR = const wchar_t*;
using LPTSTR = wchar_t*;
// Portable (non-Windows) stand-ins for the <tchar.h> generic-text macros
// real MFC relies on. We are unconditionally a wide/UNICODE build, so these
// map to the wide-char forms.
#ifndef _T
#define _T(x) L##x
#endif
#ifndef _TEXT
#define _TEXT(x) L##x
#endif
using TCHAR = wchar_t;
using _TCHAR = wchar_t;
using PTSTR = wchar_t*;
using PCTSTR = const wchar_t*;
#endif

// ---------------------------------------------------------------------
// MFC diagnostic macros (ASSERT/VERIFY/TRACE/...). Real MFC defines these
// in its own afx.h; <windows.h> does NOT, so they are missing on a real
// Windows build unless we provide them. Semantics mirror real MFC: active
// only under _DEBUG, no-ops in a release/NDEBUG build (which is how the
// conformance and eMule-compile-check builds run). VERIFY still evaluates
// its argument in release; ASSERT/TRACE fully vanish.
// ---------------------------------------------------------------------
#ifdef _DEBUG
#ifndef ASSERT
#define ASSERT(f) assert(f)
#endif
#ifndef VERIFY
#define VERIFY(f) ASSERT(f)
#endif
#else
#ifndef ASSERT
#define ASSERT(f) ((void)0)
#endif
#ifndef VERIFY
#define VERIFY(f) ((void)(f))
#endif
#endif
#ifndef ASSERT_VALID
#define ASSERT_VALID(p) ((void)0)
#endif
#ifndef ASSERT_KINDOF
#define ASSERT_KINDOF(class_name, object) ((void)0)
#endif
#ifndef ENSURE
#define ENSURE(f) ASSERT(f)
#endif
#ifndef ENSURE_ARG
#define ENSURE_ARG(f) ASSERT(f)
#endif
#ifndef TRACE
#define TRACE(...) ((void)0)
#endif
#ifndef TRACE0
#define TRACE0(sz) ((void)0)
#endif
#ifndef TRACE1
#define TRACE1(sz, p1) ((void)0)
#endif
#ifndef TRACE2
#define TRACE2(sz, p1, p2) ((void)0)
#endif
#ifndef TRACE3
#define TRACE3(sz, p1, p2, p3) ((void)0)
#endif
// ATL diagnostic macros. eMule mixes ATL headers with MFC and uses ATLASSERT/
// ATLVERIFY/ATLTRACE (real ATL defines them in atldef.h). Mirror the same
// _DEBUG-gated semantics as the MFC ones above; #ifndef-guarded so a genuine
// <atldef.h> in the same TU still wins.
#ifndef ATLASSERT
#define ATLASSERT(expr) ASSERT(expr)
#endif
#ifndef ATLVERIFY
#define ATLVERIFY(expr) VERIFY(expr)
#endif
#ifndef ATLASSUME
#define ATLASSUME(expr) ASSERT(expr)
#endif
#ifndef ATLTRACE
#define ATLTRACE(...) ((void)0)
#endif
#ifndef ATLTRACE2
#define ATLTRACE2(...) ((void)0)
#endif

// ---------------------------------------------------------------------
// Lightweight MFC-style RTTI (does not use the compiler's typeid/
// dynamic_cast, exactly like real MFC: a chain of CRuntimeClass walkable
// at runtime). Pure standard C++ constructs, no Windows dependency.
// ---------------------------------------------------------------------
class CObject;
class CDumpContext; // full definition below, after CObject (needed for CObject::Dump)

struct CRuntimeClass
{
    const char* m_lpszClassName;
    const CRuntimeClass* m_pBaseClass;
    CObject* (*m_pfnCreateObject)(); // nullptr if the class is not "creatable by name" (DECLARE_DYNAMIC only)

    bool IsDerivedFrom(const CRuntimeClass* pBase) const noexcept
    {
        for (const CRuntimeClass* p = this; p; p = p->m_pBaseClass)
            if (p == pBase)
                return true;
        return false;
    }

    CObject* CreateObject() const { return m_pfnCreateObject ? m_pfnCreateObject() : nullptr; }
};

#define DECLARE_DYNAMIC(class_name)                                         \
public:                                                                     \
    static const CRuntimeClass classCRuntimeClass;                          \
    CRuntimeClass* GetRuntimeClass() const override                        \
    {                                                                       \
        return const_cast<CRuntimeClass*>(&class_name::classCRuntimeClass); \
    }

#define IMPLEMENT_DYNAMIC(class_name, base_class_name) \
    const CRuntimeClass class_name::classCRuntimeClass = \
        {#class_name, &base_class_name::classCRuntimeClass, nullptr};

#define DECLARE_DYNCREATE(class_name) DECLARE_DYNAMIC(class_name)

#define IMPLEMENT_DYNCREATE(class_name, base_class_name)                      \
    static CObject* AFX_CreateObject_##class_name() { return new class_name; } \
    const CRuntimeClass class_name::classCRuntimeClass =                       \
        {#class_name, &base_class_name::classCRuntimeClass, &AFX_CreateObject_##class_name};

#define RUNTIME_CLASS(class_name) (&class_name::classCRuntimeClass)

// ---------------------------------------------------------------------
// CObject — root of the hierarchy. IsSerializable/Serialize are left as
// no-op hooks: real MFC (de)serialization depends on CArchive, tied to
// CFile/CDocument in an MFC-specific way, out of scope here.
// ---------------------------------------------------------------------
class CObject
{
public:
    static const CRuntimeClass classCRuntimeClass;
    virtual CRuntimeClass* GetRuntimeClass() const { return const_cast<CRuntimeClass*>(&classCRuntimeClass); }
    BOOL IsKindOf(const CRuntimeClass* pClass) const { return GetRuntimeClass()->IsDerivedFrom(pClass) ? TRUE : FALSE; }
    virtual void AssertValid() const {}
    // Real MFC: prints the class name if the class uses IMPLEMENT_DYNAMIC/
    // IMPLEMENT_DYNCREATE/IMPLEMENT_SERIAL, otherwise prints "CObject". Here
    // GetRuntimeClass() always resolves to a real CRuntimeClass (RTTI is
    // unconditionally available in this port, see IMPLEMENT_DYNAMIC above),
    // so the class name is always printed. Unlike real MFC, not gated on
    // _DEBUG (same design choice already made for AssertValid).
    virtual void Dump(CDumpContext& dc) const;
    virtual BOOL IsSerializable() const { return FALSE; }
    virtual ~CObject() = default;
};

// ---------------------------------------------------------------------
// CDumpContext — diagnostic dump support for CObject::Dump. On top of
// std::wostream (defaults to std::wcerr: real MFC describes afxDump's
// output as "conceptually similar to the cerr stream"). Real MFC's
// constructor instead binds to a CFile* destination (afxDump is built for
// you); not implemented here since nothing in the covered eMule call
// sites ever constructs its own CDumpContext or writes to afxDump
// directly — only the Dump(dc) super-call chain is exercised, which just
// needs a working destination to forward to.
// ---------------------------------------------------------------------
class CDumpContext
{
public:
    explicit CDumpContext(std::wostream& os = std::wcerr) : m_os(os) {}

    void SetDepth(int nNewDepth) noexcept { m_nDepth = nNewDepth; }
    int GetDepth() const noexcept { return m_nDepth; }

    CDumpContext& operator<<(const char* lpsz);
    CDumpContext& operator<<(LPCTSTR lpsz);
    CDumpContext& operator<<(const CObject* pOb);
    CDumpContext& operator<<(const CObject& ob) { return *this << &ob; }
    CDumpContext& operator<<(int n);
    CDumpContext& operator<<(unsigned int u);
    CDumpContext& operator<<(long l);
    CDumpContext& operator<<(double d);
    CDumpContext& operator<<(const void* lp);

private:
    std::wostream& m_os;
    int m_nDepth = 0;
};

// Real MFC's predeclared global dump context (Debug-only there; always
// available here, consistent with Dump/AssertValid above).
extern CDumpContext afxDump;

// ---------------------------------------------------------------------
// CStringT<Ch> — a real wrapper around std::basic_string<Ch> exposing the
// standard MFC/ATL CStringT interface (only the members eMule/srchybrid
// actually uses; verified against Microsoft Learn's CStringT reference).
// The public aliases below match real ATL: CStringA (char), CStringW
// (wchar_t), CString (wide, matching a UNICODE build's TCHAR). LoadString
// (PE .rc resources) is omitted: no standard C++ equivalent, see
// ../README.md. Declared here, before CException, because
// CFileException::m_strFileName is a CString (matching real MFC).
// ---------------------------------------------------------------------
namespace mfc_detail
{
// The handful of operations that differ between the char and wchar_t
// instantiations, isolated so CStringT's body stays character-set neutral.
// (Real ATL factors these into ChTraitsCRT/StrTraitMFC; this is the
// minimal portable equivalent for simple_mfc's subset.)
template <class Ch> struct StrTraits;

template <>
struct StrTraits<wchar_t>
{
    static const wchar_t* WS() noexcept { return L" \t\r\n"; }
    static wchar_t Lower(wchar_t c) noexcept { return static_cast<wchar_t>(std::towlower(static_cast<std::wint_t>(c))); }
    static wchar_t Upper(wchar_t c) noexcept { return static_cast<wchar_t>(std::towupper(static_cast<std::wint_t>(c))); }
    static int FormatV(wchar_t* buf, size_t n, const wchar_t* fmt, va_list a) { return std::vswprintf(buf, n, fmt, a); }
};

template <>
struct StrTraits<char>
{
    static const char* WS() noexcept { return " \t\r\n"; }
    static char Lower(char c) noexcept { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    static char Upper(char c) noexcept { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
    static int FormatV(char* buf, size_t n, const char* fmt, va_list a) { return std::vsnprintf(buf, n, fmt, a); }
};

// Narrow<->wide conversion for CStringT's cross-character (YCHAR)
// constructors/assignment. ASCII maps 1:1; bytes >= 0x80 widen as Latin-1
// and non-ASCII wide chars narrow to '?' -- a portable, compile-check-grade
// stand-in for real MFC's active-code-page conversion (documented
// limitation, see ../README.md).
inline std::wstring Widen(const char* p, size_t n)
{
    std::wstring w;
    w.reserve(n);
    for (size_t i = 0; i < n; ++i)
        w.push_back(static_cast<wchar_t>(static_cast<unsigned char>(p[i])));
    return w;
}
inline std::string Narrow(const wchar_t* p, size_t n)
{
    std::string s;
    s.reserve(n);
    for (size_t i = 0; i < n; ++i)
        s.push_back(p[i] < 0x80 ? static_cast<char>(p[i]) : '?');
    return s;
}
} // namespace mfc_detail

template <class BaseType, class = void>
class CStringT
{
public:
    using XCHAR = BaseType;        // this string's character type (ATL naming)
    using PXSTR = XCHAR*;
    using PCXSTR = const XCHAR*;
    using YCHAR = std::conditional_t<std::is_same_v<XCHAR, char>, wchar_t, char>; // the "other" type
    using PCYSTR = const YCHAR*;

    CStringT() = default;
    CStringT(const CStringT&) = default;
    CStringT(CStringT&&) noexcept = default;
    CStringT(PCXSTR pszSrc) { if (pszSrc) m_data = pszSrc; }
    CStringT(PCXSTR pch, int nLength) { if (pch && nLength > 0) m_data.assign(pch, static_cast<size_t>(nLength)); }
    explicit CStringT(XCHAR ch, int nRepeat = 1) : m_data(static_cast<size_t>(nRepeat < 0 ? 0 : nRepeat), ch) {}
    // Cross-character (YCHAR) sources convert; explicit, matching real ATL's
    // CSTRING_EXPLICIT so a char/wchar_t mismatch is never silently narrowed.
    explicit CStringT(PCYSTR pszSrc) { if (pszSrc) m_data = Convert(pszSrc, std::char_traits<YCHAR>::length(pszSrc)); }
    explicit CStringT(PCYSTR pch, int nLength) { if (pch && nLength > 0) m_data = Convert(pch, static_cast<size_t>(nLength)); }
    explicit CStringT(const CStringT<YCHAR>& strSrc) { m_data = Convert(strSrc.GetString(), static_cast<size_t>(strSrc.GetLength())); }

    CStringT& operator=(const CStringT&) = default;
    CStringT& operator=(CStringT&&) noexcept = default;
    CStringT& operator=(PCXSTR pszSrc) { if (pszSrc) m_data = pszSrc; else m_data.clear(); return *this; }
    CStringT& operator=(XCHAR ch) { m_data.assign(1, ch); return *this; }
    CStringT& operator=(PCYSTR pszSrc) { m_data = pszSrc ? Convert(pszSrc, std::char_traits<YCHAR>::length(pszSrc)) : std::basic_string<XCHAR>(); return *this; }

    int GetLength() const noexcept { return static_cast<int>(m_data.size()); }
    bool IsEmpty() const noexcept { return m_data.empty(); }
    void Empty() noexcept { m_data.clear(); }
    PXSTR GetBuffer(int nMinBufferLength)
    {
        if (static_cast<size_t>(nMinBufferLength) > m_data.size())
            m_data.resize(static_cast<size_t>(nMinBufferLength));
        return m_data.data();
    }
    PXSTR GetBuffer() { return GetBuffer(GetLength()); }
    void ReleaseBuffer(int nNewLength = -1)
    {
        if (nNewLength < 0) m_data.resize(std::char_traits<XCHAR>::length(m_data.c_str()));
        else m_data.resize(static_cast<size_t>(nNewLength));
    }
    XCHAR GetAt(int iChar) const { return m_data.at(static_cast<size_t>(iChar)); }
    void SetAt(int iChar, XCHAR ch) { m_data.at(static_cast<size_t>(iChar)) = ch; }
    PCXSTR GetString() const noexcept { return m_data.c_str(); }

    void Format(PCXSTR pszFormat, ...) { va_list a; va_start(a, pszFormat); m_data = VFormat(pszFormat, a); va_end(a); }
    void AppendFormat(PCXSTR pszFormat, ...) { va_list a; va_start(a, pszFormat); m_data += VFormat(pszFormat, a); va_end(a); }
    // va_list variants (real MFC CString::FormatV/AppendFormatV) -- eMule's
    // CRichEditStream forwards a captured va_list into AppendFormatV.
    void FormatV(PCXSTR pszFormat, va_list args) { m_data = VFormat(pszFormat, args); }
    void AppendFormatV(PCXSTR pszFormat, va_list args) { m_data += VFormat(pszFormat, args); }
    void Append(PCXSTR pszSrc) { if (pszSrc) m_data += pszSrc; }
    void Append(PCXSTR pszSrc, int nLength) { if (pszSrc && nLength > 0) m_data.append(pszSrc, static_cast<size_t>(nLength)); }
    void AppendChar(XCHAR ch) { m_data += ch; }

    int Compare(PCXSTR psz) const { return m_data.compare(psz); }
    int CompareNoCase(PCXSTR psz) const
    {
        std::basic_string<XCHAR> a = m_data, b = psz ? psz : std::basic_string<XCHAR>().c_str();
        auto lower = [](XCHAR c) { return mfc_detail::StrTraits<XCHAR>::Lower(c); };
        std::transform(a.begin(), a.end(), a.begin(), lower);
        std::transform(b.begin(), b.end(), b.begin(), lower);
        return a.compare(b);
    }
    // Real MFC's Collate/CollateNoCase are locale-aware (Win32 lstrcmp/
    // CompareString); this portable build maps them to the ordinary
    // ordinal Compare/CompareNoCase (eMule's KadTagStr overrides Collate
    // and super-calls __super::Collate).
    int Collate(PCXSTR psz) const { return Compare(psz); }
    int CollateNoCase(PCXSTR psz) const { return CompareNoCase(psz); }
    int Delete(int iIndex, int nCount = 1)
    {
        if (iIndex < 0 || static_cast<size_t>(iIndex) >= m_data.size()) return GetLength();
        m_data.erase(static_cast<size_t>(iIndex), static_cast<size_t>(nCount));
        return GetLength();
    }
    int Find(PCXSTR pszSub, int iStart = 0) const
    {
        auto pos = m_data.find(pszSub, static_cast<size_t>(iStart));
        return pos == npos ? -1 : static_cast<int>(pos);
    }
    int Find(XCHAR ch, int iStart = 0) const
    {
        auto pos = m_data.find(ch, static_cast<size_t>(iStart));
        return pos == npos ? -1 : static_cast<int>(pos);
    }
    int FindOneOf(PCXSTR pszCharSet) const
    {
        auto pos = m_data.find_first_of(pszCharSet);
        return pos == npos ? -1 : static_cast<int>(pos);
    }
    int ReverseFind(XCHAR ch) const
    {
        auto pos = m_data.rfind(ch);
        return pos == npos ? -1 : static_cast<int>(pos);
    }
    int Insert(int iIndex, PCXSTR psz)
    {
        size_t i = std::min<size_t>(static_cast<size_t>(iIndex), m_data.size());
        m_data.insert(i, psz);
        return GetLength();
    }
    int Insert(int iIndex, XCHAR ch)
    {
        size_t i = std::min<size_t>(static_cast<size_t>(iIndex), m_data.size());
        m_data.insert(i, 1, ch);
        return GetLength();
    }
    int Remove(XCHAR chRemove)
    {
        size_t before = m_data.size();
        m_data.erase(std::remove(m_data.begin(), m_data.end(), chRemove), m_data.end());
        return static_cast<int>(before - m_data.size());
    }
    void Truncate(int nNewLength)
    {
        if (nNewLength >= 0 && static_cast<size_t>(nNewLength) < m_data.size())
            m_data.resize(static_cast<size_t>(nNewLength));
    }
    CStringT Left(int nCount) const
    {
        nCount = std::clamp(nCount, 0, GetLength());
        return CStringT(m_data.substr(0, static_cast<size_t>(nCount)).c_str());
    }
    CStringT Right(int nCount) const
    {
        nCount = std::clamp(nCount, 0, GetLength());
        return CStringT(m_data.substr(m_data.size() - static_cast<size_t>(nCount)).c_str());
    }
    CStringT Mid(int iFirst, int nCount) const
    {
        iFirst = std::clamp(iFirst, 0, GetLength());
        nCount = std::clamp(nCount, 0, GetLength() - iFirst);
        return CStringT(m_data.substr(static_cast<size_t>(iFirst), static_cast<size_t>(nCount)).c_str());
    }
    CStringT Mid(int iFirst) const { return Mid(iFirst, GetLength() - iFirst); }
    CStringT& MakeLower()
    {
        std::transform(m_data.begin(), m_data.end(), m_data.begin(), [](XCHAR c) { return mfc_detail::StrTraits<XCHAR>::Lower(c); });
        return *this;
    }
    CStringT& MakeUpper()
    {
        std::transform(m_data.begin(), m_data.end(), m_data.begin(), [](XCHAR c) { return mfc_detail::StrTraits<XCHAR>::Upper(c); });
        return *this;
    }
    int Replace(PCXSTR pszOld, PCXSTR pszNew)
    {
        std::basic_string<XCHAR> oldS = pszOld, newS = pszNew;
        if (oldS.empty()) return 0;
        int count = 0;
        size_t pos = 0;
        while ((pos = m_data.find(oldS, pos)) != npos)
        {
            m_data.replace(pos, oldS.size(), newS);
            pos += newS.size();
            ++count;
        }
        return count;
    }
    int Replace(XCHAR chOld, XCHAR chNew)
    {
        int count = 0;
        for (auto& c : m_data) if (c == chOld) { c = chNew; ++count; }
        return count;
    }
    CStringT SpanExcluding(PCXSTR pszCharSet) const
    {
        auto pos = m_data.find_first_of(pszCharSet);
        return pos == npos ? *this : Left(static_cast<int>(pos));
    }
    CStringT Tokenize(PCXSTR pszTokens, int& iStart) const
    {
        if (iStart < 0 || static_cast<size_t>(iStart) >= m_data.size()) { iStart = -1; return CStringT(); }
        size_t begin = m_data.find_first_not_of(pszTokens, static_cast<size_t>(iStart));
        if (begin == npos) { iStart = -1; return CStringT(); }
        size_t end = m_data.find_first_of(pszTokens, begin);
        CStringT tok(m_data.substr(begin, end == npos ? npos : end - begin).c_str());
        iStart = (end == npos) ? -1 : static_cast<int>(end + 1);
        return tok;
    }
    CStringT& Trim() { return Trim(mfc_detail::StrTraits<XCHAR>::WS()); }
    CStringT& Trim(XCHAR chTarget) { XCHAR set[2] = {chTarget, 0}; return Trim(set); }
    CStringT& Trim(PCXSTR pszTargets)
    {
        size_t b = m_data.find_first_not_of(pszTargets);
        if (b == npos) m_data.clear();
        else m_data.erase(0, b);
        return TrimRight(pszTargets);
    }
    CStringT& TrimRight() { return TrimRight(mfc_detail::StrTraits<XCHAR>::WS()); }
    CStringT& TrimRight(XCHAR chTarget) { XCHAR set[2] = {chTarget, 0}; return TrimRight(set); }
    CStringT& TrimRight(PCXSTR pszTargets)
    {
        size_t e = m_data.find_last_not_of(pszTargets);
        if (e == npos) m_data.clear();
        else m_data.erase(e + 1);
        return *this;
    }

    PCXSTR c_str() const noexcept { return m_data.c_str(); }
    operator PCXSTR() const noexcept { return m_data.c_str(); }
    XCHAR operator[](int i) const { return m_data[static_cast<size_t>(i)]; }
    const std::basic_string<XCHAR>& AsStdString() const noexcept { return m_data; }

    CStringT& operator+=(const CStringT& s) { m_data += s.m_data; return *this; }
    CStringT& operator+=(PCXSTR psz) { if (psz) m_data += psz; return *this; }
    CStringT& operator+=(XCHAR ch) { m_data += ch; return *this; }

    friend CStringT operator+(const CStringT& a, const CStringT& b) { CStringT r(a); r.m_data += b.m_data; return r; }
    friend CStringT operator+(const CStringT& a, PCXSTR b) { CStringT r(a); if (b) r.m_data += b; return r; }
    friend CStringT operator+(PCXSTR a, const CStringT& b) { CStringT r; if (a) r.m_data = a; r.m_data += b.m_data; return r; }
    friend CStringT operator+(const CStringT& a, XCHAR b) { CStringT r(a); r.m_data += b; return r; }
    friend CStringT operator+(XCHAR a, const CStringT& b) { CStringT r; r.m_data += a; r.m_data += b.m_data; return r; }

    friend bool operator==(const CStringT& a, const CStringT& b) noexcept { return a.m_data == b.m_data; }
    friend bool operator==(const CStringT& a, PCXSTR b) noexcept { return a.m_data == b; }
    friend bool operator==(PCXSTR a, const CStringT& b) noexcept { return a == b.m_data; }
    friend bool operator!=(const CStringT& a, const CStringT& b) noexcept { return a.m_data != b.m_data; }
    friend bool operator!=(const CStringT& a, PCXSTR b) noexcept { return a.m_data != b; }
    friend bool operator!=(PCXSTR a, const CStringT& b) noexcept { return a != b.m_data; }
    friend bool operator<(const CStringT& a, const CStringT& b) noexcept { return a.m_data < b.m_data; }
    friend bool operator<(const CStringT& a, PCXSTR b) noexcept { return a.m_data < b; }
    friend bool operator<(PCXSTR a, const CStringT& b) noexcept { return a < b.m_data; }
    friend bool operator>(const CStringT& a, const CStringT& b) noexcept { return a.m_data > b.m_data; }
    friend bool operator>(const CStringT& a, PCXSTR b) noexcept { return a.m_data > b; }
    friend bool operator>(PCXSTR a, const CStringT& b) noexcept { return a > b.m_data; }
    friend bool operator<=(const CStringT& a, const CStringT& b) noexcept { return a.m_data <= b.m_data; }
    friend bool operator>=(const CStringT& a, const CStringT& b) noexcept { return a.m_data >= b.m_data; }

private:
    static constexpr auto npos = std::basic_string<XCHAR>::npos;

    template <class Src>
    static std::basic_string<XCHAR> Convert(const Src* p, size_t n)
    {
        if (!p) return {};
        if constexpr (std::is_same_v<Src, XCHAR>)
            return std::basic_string<XCHAR>(p, n);
        else if constexpr (std::is_same_v<XCHAR, wchar_t>)
            return mfc_detail::Widen(p, n);
        else
            return mfc_detail::Narrow(p, n);
    }

    static std::basic_string<XCHAR> VFormat(PCXSTR fmt, va_list args)
    {
        size_t size = 256;
        std::vector<XCHAR> buf(size);
        for (;;)
        {
            va_list ap;
            va_copy(ap, args);
            int n = mfc_detail::StrTraits<XCHAR>::FormatV(buf.data(), size, fmt, ap);
            va_end(ap);
            if (n >= 0 && static_cast<size_t>(n) < size)
                return std::basic_string<XCHAR>(buf.data(), static_cast<size_t>(n));
            if (size > (1u << 20))
                return std::basic_string<XCHAR>(buf.data(), size - 1);
            size *= 2;
            buf.resize(size);
        }
    }

    std::basic_string<XCHAR> m_data;
};

using CStringA = CStringT<char>;    // ANSI/char
using CStringW = CStringT<wchar_t>; // wide
using CString = CStringW;           // TCHAR under UNICODE, matching real MFC

namespace std
{
template <class Ch, class Tr>
struct hash<CStringT<Ch, Tr>>
{
    size_t operator()(const CStringT<Ch, Tr>& s) const noexcept { return std::hash<std::basic_string<Ch>>{}(s.AsStdString()); }
};
} // namespace std

// ---------------------------------------------------------------------
// CException — real hierarchy, genuine C++ exceptions (used with standard
// throw/catch, not with the original MFC pointer+Delete() pattern, even
// though Delete() is still available for interface compatibility with
// code written against real MFC).
// ---------------------------------------------------------------------
class CException : public CObject
{
    DECLARE_DYNAMIC(CException)
public:
    explicit CException(BOOL bAutoDelete) : m_bAutoDelete(bAutoDelete) {}
    void Delete() { if (m_bAutoDelete) delete this; }
    virtual int ReportError(UINT nType = 0, UINT nMessageID = 0);

private:
    BOOL m_bAutoDelete;
};

// Abstract base for "resource-critical" exceptions. In real MFC
// GetErrorMessage is virtual with a body (not pure); here it is made pure
// to enforce abstractness at compile time (an explicit design choice), so
// every concrete subclass must provide its own override.
class CSimpleException : public CException
{
    DECLARE_DYNAMIC(CSimpleException)
public:
    CSimpleException() : CException(FALSE) {}
    explicit CSimpleException(BOOL bAutoDelete) : CException(bAutoDelete) {}
    virtual BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) const = 0;
};

class CMemoryException : public CSimpleException
{
    DECLARE_DYNAMIC(CMemoryException)
public:
    CMemoryException() : CSimpleException(FALSE) {}
    BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) const override;
};

class CFileException : public CException
{
    DECLARE_DYNAMIC(CFileException)
public:
    enum Cause
    {
        none, genericException, fileNotFound, badPath, tooManyOpenFiles,
        accessDenied, invalidFile, removeCurrentDir, directoryFull, badSeek,
        hardIO, sharingViolation, lockViolation, diskFull, endOfFile
    };

    explicit CFileException(int cause = none, LONG lOsError = -1, LPCTSTR lpszFileName = nullptr)
        : CException(TRUE), m_cause(cause), m_lOsError(lOsError),
          m_strFileName(lpszFileName ? lpszFileName : L"") {}

    BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) const;

    // Static factory: maps an OS-specific error code to a Cause (falling
    // back to genericException for anything not recognized, matching real
    // MFC's documented fallback) and throws the resulting CFileException by
    // pointer, exactly like AfxThrowFileException. Real MFC also exposes
    // OsErrorToException/ErrnoToException/ThrowErrno as separate public
    // static methods; not added here since eMule/srchybrid only ever calls
    // ThrowOsError itself (see mfc_scan_srchybrid.md blind-spot findings).
    [[noreturn]] static void ThrowOsError(LONG lOsError, LPCTSTR lpszFileName = nullptr);

    int m_cause;
    LONG m_lOsError;
    CString m_strFileName;
};

// Global exception-throwing functions. In real MFC, AfxThrowInvalidArgException,
// AfxThrowNotSupportedException, AfxThrowResourceException and
// AfxThrowUserException throw classes (CInvalidArgException, etc.) that are
// not implemented here — see ../README.md for why only the two below are
// provided.
[[noreturn]] void AfxThrowFileException(int cause, LONG lOsError = -1, LPCTSTR lpszFileName = nullptr);
[[noreturn]] void AfxThrowMemoryException();
[[noreturn]] void AfxAbort();

// ---------------------------------------------------------------------
// CFile / CStdioFile / CMemFile — built on std::fstream / an in-memory
// buffer, fully portable (no Win32 HANDLE).
// ---------------------------------------------------------------------
struct CFileStatus
{
    ULONGLONG m_size = 0;
};

class CFile : public CObject
{
    DECLARE_DYNAMIC(CFile)
public:
    enum OpenFlags
    {
        modeRead = 0x0000, modeWrite = 0x0001, modeReadWrite = 0x0002,
        modeCreate = 0x1000, modeNoTruncate = 0x2000,
        shareExclusive = 0x0010, shareDenyNone = 0x0040,
        typeBinary = 0x0000, typeText = 0x4000
    };
    enum SeekPosition { begin = 0, current = 1, end = 2 };

    CFile() = default;
    CFile(LPCTSTR lpszFileName, UINT nOpenFlags) { Open(lpszFileName, nOpenFlags); }
    virtual ~CFile() { if (m_stream.is_open()) m_stream.close(); }

    virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError = nullptr);
    virtual void Abort() { if (m_stream.is_open()) m_stream.close(); }
    virtual void Close() { if (m_stream.is_open()) m_stream.close(); }
    virtual UINT Read(void* lpBuf, UINT nCount);
    virtual void Write(const void* lpBuf, UINT nCount);
    virtual ULONGLONG Seek(LONGLONG lOff, UINT nFrom);
    void SeekToBegin() { Seek(0, begin); }
    ULONGLONG SeekToEnd() { return Seek(0, end); }
    virtual ULONGLONG GetLength() const;
    virtual void SetLength(ULONGLONG dwNewLen);
    virtual ULONGLONG GetPosition() const;
    virtual void Flush() { m_stream.flush(); }
    virtual CString GetFileName() const { return CString(std::filesystem::path(m_path).filename().wstring().c_str()); }
    virtual CString GetFilePath() const { return CString(m_path.c_str()); }
    BOOL GetStatus(CFileStatus& rStatus) const;
    static BOOL GetStatus(LPCTSTR lpszFileName, CFileStatus& rStatus);
    static void Remove(LPCTSTR lpszFileName);
    static void Rename(LPCTSTR lpszOldName, LPCTSTR lpszNewName);

protected:
    std::fstream m_stream;
    std::wstring m_path;
};

class CStdioFile : public CFile
{
    DECLARE_DYNAMIC(CStdioFile)
public:
    CStdioFile() = default;
    CStdioFile(LPCTSTR lpszFileName, UINT nOpenFlags) : CFile(lpszFileName, nOpenFlags) {}

    virtual LPTSTR ReadString(LPTSTR lpsz, UINT nMax);
    virtual BOOL ReadString(CString& rString);
    virtual void WriteString(LPCTSTR lpsz);
};

// CMemFile — an entirely in-memory file (std::vector<uint8_t>), no methods
// of its own beyond the ones it inherits.
class CMemFile : public CFile
{
    DECLARE_DYNAMIC(CMemFile)
public:
    CMemFile() = default;
    // Real MFC's CMemFile(UINT nGrowBytes) constructor. eMule's CSafeMemFile
    // forwards to it (SafeFile.h:98). nGrowBytes only tunes reallocation
    // growth, which this std::vector-backed implementation handles
    // automatically, so the argument is accepted and ignored.
    explicit CMemFile(UINT /*nGrowBytes*/) {}
    // Real MFC's attach-a-buffer constructor CMemFile(BYTE*, UINT, UINT).
    // eMule's CSafeMemFile(const BYTE*, UINT) forwards to it (SafeFile.h:104).
    CMemFile(BYTE* lpBuffer, UINT nBufferSize, UINT /*nGrowBytes*/ = 0)
        : m_buffer(lpBuffer, lpBuffer + nBufferSize) {}
    UINT Read(void* lpBuf, UINT nCount) override;
    void Write(const void* lpBuf, UINT nCount) override;
    ULONGLONG Seek(LONGLONG lOff, UINT nFrom) override;
    ULONGLONG GetLength() const override { return m_buffer.size(); }
    void SetLength(ULONGLONG dwNewLen) override { m_buffer.resize(static_cast<size_t>(dwNewLen)); }
    ULONGLONG GetPosition() const override { return m_pos; }

protected:
    // Real MFC's protected data members. eMule's CSafeMemFile reads m_lpBuffer
    // directly (SafeFile.h:110). This std::vector-backed CMemFile keeps its
    // real state in m_buffer/m_pos below and does NOT mirror it into these --
    // they exist to satisfy derived-class member access (the eMule
    // compile-check is compile-only; a runtime-faithful mirror is out of scope).
    BYTE*     m_lpBuffer = nullptr;
    UINT      m_nGrowBytes = 0;
    ULONGLONG m_nPosition = 0;
    ULONGLONG m_nBufferSize = 0;
    ULONGLONG m_nFileSize = 0;
    BOOL      m_bAutoDelete = TRUE;

private:
    std::vector<uint8_t> m_buffer;
    size_t m_pos = 0;
};

// ---------------------------------------------------------------------
// CFileFind — built on std::filesystem (standard C++17, no FindFirstFile).
// FindNextFile is also a real winbase.h macro (expands to FindNextFileW/A)
// -- undefined here, the same way real MFC's own afx.h does it, so the
// member keeps its true name instead of being silently rewritten to
// something CFileFind doesn't have. Once undef'd it stays gone for the
// rest of the translation unit, so later call sites (this file's own
// afx.cpp, and any including code such as eMule's) see the same name too.
// ---------------------------------------------------------------------
#undef FindNextFile
class CFileFind : public CObject
{
    DECLARE_DYNAMIC(CFileFind)
public:
    virtual BOOL FindFile(LPCTSTR pstrName = nullptr, DWORD dwUnused = 0);
    virtual BOOL FindNextFile();
    void Close() { m_it = std::filesystem::directory_iterator(); m_pending.reset(); }
    BOOL IsDirectory() const;
    virtual BOOL IsDots() const;
    virtual CString GetFileName() const;
    virtual CString GetFilePath() const;
    ULONGLONG GetLength() const;
    virtual CString GetRoot() const;

private:
    bool AdvanceToNextMatch();

    std::filesystem::path m_dir;
    std::wstring m_root;
    std::wstring m_pattern;
    std::filesystem::directory_iterator m_it;
    std::optional<std::filesystem::directory_entry> m_pending;
    std::filesystem::directory_entry m_current;
};
