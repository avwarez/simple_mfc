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
#include <string_view>
#include <type_traits>
#include <vector>

// Compatibility with the headers that are still "declaration-only" (afxdd_.h,
// afxwin.h...), which use these MSVC-style macros/keywords. No-op on
// non-MSVC compilers.
#ifndef _MSC_VER
#define __cdecl
#define __stdcall
#endif
#define EAFXAPI __stdcall
#define EAFX_CDECL __cdecl   // real MFC's calling-convention marker on static
                            // thread procs (`static UINT AFX_CDECL RunProc(...)`)

// On Windows (LLP64) `long` is 32 bits, so Win32's DWORD/LONG are exactly
// 32 bits AND are distinct types from UINT/int -- which is what lets MFC
// overload on both (CArchive::operator<< has separate UINT and DWORD
// overloads). On a 64-bit Unix (LP64) `long` is 64 bits, so the two
// properties become mutually exclusive: standard C++ offers no second
// 32-bit integer type distinct from `int`. One of them has to go.
//
// WIDTH WINS off Windows. DWORD is 32 bits by definition in Win32, and
// every consequence of getting that wrong is silent: struct layouts shift,
// and above all serialized bytes change. Measured, not assumed -- the
// golden conformance run caught CArchive writing 47 bytes where real MFC
// writes 39, because `long` and `DWORD` each went out 8 bytes wide. eMule's
// on-disk formats (known.met and friends) are read by other clients, so a
// widened field is a corrupt file, not an internal detail.
//
// The cost is exactly two overload pairs -- CArchive's UINT/DWORD in each
// direction -- which collapse into one off Windows and are #ifdef'd out
// there accordingly. Nothing else in the library depended on the
// distinction. The Windows interface is untouched, so what eMule compiles
// against is unchanged.
using UINT = unsigned int;
using WORD = unsigned short;
using BYTE = unsigned char;
#ifdef _WIN32
using DWORD = unsigned long;
#else
using DWORD = unsigned int;
#endif
using HANDLE = void*;   // identical to <windows.h>'s `typedef void* HANDLE`
#ifdef _WIN32
using LONG = long;
#else
using LONG = int;
#endif
using LONGLONG = long long;
using ULONGLONG = unsigned long long;
using BOOL = int;
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
// Required of the BUILD, never defined here. Defining a charset macro from
// inside a header sets it for everything parsed after this point and leaves
// everything parsed before it on the other setting -- in a translation unit
// that also holds real MFC (which eMule's migration keeps for as long as it
// runs, one replaced symbol at a time), that means afx.h parsed as ANSI and
// afxwin.h parsed as Unicode, with CString a different type in each. The
// library's own CMakeLists.txt puts these on the target as PUBLIC, so a
// consumer inherits them; a consumer with its own build system is told here
// rather than silently switched.
#if !defined(UNICODE) || !defined(_UNICODE)
#error "simple_mfc is unconditionally wide-char: build with UNICODE and _UNICODE defined."
#endif
// Stop <windows.h> from dragging in the legacy <winsock.h> (Winsock 1):
// eMule/srchybrid (and our own afxsock.h) include <winsock2.h>, and the two
// redefine the same structs/functions -> thousands of C2011/C2375 errors.
// This is exactly what real MFC's afxv_w32.h does before it includes
// <windows.h>; _WINSOCKAPI_ is winsock.h's own include guard, so pre-defining
// it makes windows.h skip winsock.h and leaves the field to winsock2.h.
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
// ...and put it back afterwards, which is the half of the trick that
// matters in a TU that also holds real MFC. afxwin.h hard-errors ("MFC
// requires use of Winsock2.h") when it finds _WINSOCKAPI_ defined without
// _WINSOCK2API_ alongside it -- i.e. when someone suppressed winsock.h and
// then did not bring winsock2.h in. Real MFC's afxv_w32.h defines the macro
// only for the duration of its own <windows.h> include and undefines it
// again (its _AFX_NO_WINSOCK_UNDEF flag); leaving it defined, as this
// header used to, broke every real MFC header included after ours. Found by
// the per-header pair matrix, and only there: the umbrella coexistence probe
// includes afxsock.h too, whose winsock2.h defines _WINSOCK2API_ and hides it.
#define ESIMPLE_MFC_UNDEF_WINSOCKAPI
#endif
#include <windows.h>
#ifdef ESIMPLE_MFC_UNDEF_WINSOCKAPI
#undef _WINSOCKAPI_
#undef ESIMPLE_MFC_UNDEF_WINSOCKAPI
#endif
// <windows.h> does NOT pull in <tchar.h>, so _T()/TCHAR/_tcs* would be
// missing on a real Windows build too -- real MFC gets them because its own
// afx.h includes <tchar.h>. Match that. Under the forced _UNICODE above,
// _T maps to the L"" prefix and _tcs* to the wcs* family, matching our
// unconditionally-wide CString.
#include <tchar.h>
// BSTR/SysAllocString (CStringT::AllocSysString below): normally pulled
// in transitively by <windows.h>, but not when a consuming project
// defines WIN32_LEAN_AND_MEAN (as this library's own CMakeLists.txt
// does) -- that strips COM/OLE headers from windows.h's default set, so
// BSTR is otherwise an incomplete/unknown type here regardless of
// whether AllocSysString is ever called (a class TEMPLATE's declarations
// are still name-looked-up when the template itself is parsed, not only
// on instantiation, since BSTR does not depend on the template
// parameter). Include explicitly rather than relying on the consumer's
// windows.h configuration.
#include <oleauto.h>
#else
using LPCTSTR = const wchar_t*;
using LPTSTR = wchar_t*;
// Portable (non-Windows) stand-ins for the <tchar.h> generic-text macros
// real MFC relies on. We are unconditionally a wide/UNICODE build, so these
// map to the wide-char forms.
#ifndef _T
#define _T(x) L##x
#endif
using TCHAR = wchar_t;
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
#ifndef EASSERT
#define EASSERT(f) assert(f)
#endif
#ifndef EVERIFY
#define EVERIFY(f) EASSERT(f)
#endif
// DEBUG_ONLY wraps code that must exist only in a debug build -- unlike
// VERIFY it discards the expression entirely in release, so it is the one
// place where a side effect is meant to disappear.
#ifndef EDEBUG_ONLY
#define EDEBUG_ONLY(f) (f)
#endif
#else
#ifndef EASSERT
#define EASSERT(f) ((void)0)
#endif
#ifndef EVERIFY
#define EVERIFY(f) ((void)(f))
#endif
#ifndef EDEBUG_ONLY
#define EDEBUG_ONLY(f) ((void)0)
#endif
#endif
#ifndef EASSERT_VALID
#define EASSERT_VALID(p) ((void)0)
#endif
#ifndef EASSERT_KINDOF
#define EASSERT_KINDOF(class_name, object) ((void)0)
#endif
#ifndef ETRACE
#ifdef _MSC_VER
#define ETRACE __noop   // valid called AND bare, as real MFC's is
#else
#define ETRACE(...) ((void)0)
#endif
#endif
// ATL diagnostic macros. eMule mixes ATL headers with MFC and uses ATLASSERT
// (real ATL defines it in atldef.h). Mirror the same _DEBUG-gated semantics
// as the MFC ones above; #ifndef-guarded so a genuine <atldef.h> in the same
// TU still wins.
#ifndef EATLASSERT
#define EATLASSERT(expr) EASSERT(expr)
#endif
#ifndef EATLTRACE2
#define EATLTRACE2(...) ((void)0)
#endif

// Split a 64-bit value into its two halves. eMule uses them wherever a
// file size has to be handed to a Win32 API that still takes the low and
// high DWORDs separately.
#ifndef LODWORD
#define LODWORD(l) ((DWORD)((unsigned long long)(l)&0xFFFFFFFFULL))
#endif
#ifndef HIDWORD
#define HIDWORD(l) ((DWORD)(((unsigned long long)(l) >> 32) & 0xFFFFFFFFULL))
#endif

// ---------------------------------------------------------------------
// Lightweight MFC-style RTTI (does not use the compiler's typeid/
// dynamic_cast, exactly like real MFC: a chain of CRuntimeClass walkable
// at runtime). Pure standard C++ constructs, no Windows dependency.
// ---------------------------------------------------------------------
class ECObject;
class ECDumpContext; // full definition below, after CObject (needed for CObject::Dump)

struct ECRuntimeClass
{
    const char* m_lpszClassName;
    const ECRuntimeClass* m_pBaseClass;
    ECObject* (*m_pfnCreateObject)(); // nullptr if the class is not "creatable by name" (DECLARE_DYNAMIC only)

    bool IsDerivedFrom(const ECRuntimeClass* pBase) const noexcept
    {
        for (const ECRuntimeClass* p = this; p; p = p->m_pBaseClass)
            if (p == pBase)
                return true;
        return false;
    }

    ECObject* CreateObject() const { return m_pfnCreateObject ? m_pfnCreateObject() : nullptr; }
};

#define EDECLARE_DYNAMIC(class_name)                                         \
public:                                                                     \
    static const ECRuntimeClass classCRuntimeClass;                          \
    ECRuntimeClass* GetRuntimeClass() const override                        \
    {                                                                       \
        return const_cast<ECRuntimeClass*>(&class_name::classCRuntimeClass); \
    }

#define EIMPLEMENT_DYNAMIC(class_name, base_class_name) \
    const ECRuntimeClass class_name::classCRuntimeClass = \
        {#class_name, &base_class_name::classCRuntimeClass, nullptr};

// CreateObject must be a STATIC MEMBER, not a free function: the classes
// created this way (eMule's worker threads -- CAICHSyncThread,
// CPreviewThread, ...) deliberately keep their constructor protected so
// nothing but AfxBeginThread(RUNTIME_CLASS(...)) can instantiate them. A
// free function has no access to it (C2248); a member of the class does.
// Real MFC declares it inside the class for exactly this reason.
#define EDECLARE_DYNCREATE(class_name)                                          \
    EDECLARE_DYNAMIC(class_name)                                                \
public:                                                                        \
    static ECObject* CreateObject();

#define EIMPLEMENT_DYNCREATE(class_name, base_class_name)                       \
    ECObject* class_name::CreateObject() { return new class_name; }             \
    const ECRuntimeClass class_name::classCRuntimeClass =                       \
        {#class_name, &base_class_name::classCRuntimeClass, &class_name::CreateObject};

// Real MFC hands back a non-const CRuntimeClass*, and eMule stores the
// result in plain CRuntimeClass* variables and passes it to APIs typed
// that way, so the const has to be cast off here rather than at 30 call
// sites.
#define ERUNTIME_CLASS(class_name) (const_cast<ECRuntimeClass*>(&class_name::classCRuntimeClass))

// Checked downcast. DYNAMIC_DOWNCAST tests the runtime class and yields
// NULL on a mismatch.
ECObject* EAfxDynamicDownCast(ECRuntimeClass* pClass, ECObject* pObject);
#define EDYNAMIC_DOWNCAST(class_name, pObject)                                  \
    ((class_name*)EAfxDynamicDownCast(ERUNTIME_CLASS(class_name), pObject))

// ---------------------------------------------------------------------
// CObject — root of the hierarchy. IsSerializable/Serialize are left as
// no-op hooks: real MFC (de)serialization depends on CArchive, tied to
// CFile/CDocument in an MFC-specific way, out of scope here.
// ---------------------------------------------------------------------
class ECObject
{
public:
    static const ECRuntimeClass classCRuntimeClass;
    virtual ECRuntimeClass* GetRuntimeClass() const { return const_cast<ECRuntimeClass*>(&classCRuntimeClass); }
    BOOL IsKindOf(const ECRuntimeClass* pClass) const { return GetRuntimeClass()->IsDerivedFrom(pClass) ? TRUE : FALSE; }
    virtual void AssertValid() const {}
    // Real MFC: prints the class name if the class uses IMPLEMENT_DYNAMIC/
    // IMPLEMENT_DYNCREATE/IMPLEMENT_SERIAL, otherwise prints "CObject". Here
    // GetRuntimeClass() always resolves to a real CRuntimeClass (RTTI is
    // unconditionally available in this port, see IMPLEMENT_DYNAMIC above),
    // so the class name is always printed. Unlike real MFC, not gated on
    // _DEBUG (same design choice already made for AssertValid).
    virtual void Dump(ECDumpContext& dc) const;
    virtual BOOL IsSerializable() const { return FALSE; }
    virtual ~ECObject() = default;
};

// ---------------------------------------------------------------------
// CDumpContext — diagnostic dump support for CObject::Dump. On top of
// std::wostream (defaults to std::wcerr: real MFC describes its global
// afxDump instance's output as "conceptually similar to the cerr
// stream"). Real MFC's constructor instead binds to a CFile* destination;
// not implemented here since nothing in the covered eMule call sites ever
// constructs its own CDumpContext directly — only the Dump(dc) super-call
// chain is exercised, which just needs a working destination to forward to.
// ---------------------------------------------------------------------
class ECDumpContext
{
public:
    explicit ECDumpContext(std::wostream& os = std::wcerr) : m_os(os) {}

    void SetDepth(int nNewDepth) noexcept { m_nDepth = nNewDepth; }
    int GetDepth() const noexcept { return m_nDepth; }

    ECDumpContext& operator<<(const char* lpsz);
    ECDumpContext& operator<<(LPCTSTR lpsz);
    ECDumpContext& operator<<(const ECObject* pOb);
    ECDumpContext& operator<<(const ECObject& ob) { return *this << &ob; }
    ECDumpContext& operator<<(int n);
    ECDumpContext& operator<<(unsigned int u);
    ECDumpContext& operator<<(long l);
    ECDumpContext& operator<<(double d);
    ECDumpContext& operator<<(const void* lp);

private:
    std::wostream& m_os;
    int m_nDepth = 0;
};

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
// Declared here, ahead of CStringT, because CStringT::AllocSysString
// throws it on an allocation failure exactly as real MFC does. The
// definition (and the matching declaration among the other Afx throwers)
// is further down; a member of a class template resolves a non-dependent
// name like this one at definition time, so it has to be visible already.
[[noreturn]] void EAfxThrowMemoryException();

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

// ---------------------------------------------------------------------
// Format-string dialect translation (POSIX only).
//
// MSVC's CRT and the C standard disagree about what %s means inside a
// WIDE format string, and the disagreement is silent:
//
//     swprintf(buf, n, L"Hello, %s!", L"world")
//         MSVC   -> "Hello, world!"   (%s is wchar_t* in a wide format)
//         glibc  -> "Hello, w!"       (%s is char*; the wide string's
//                                      first byte pair reads as "w\0")
//
// Application code is written against the MSVC dialect - eMule alone has
// ~2800 %s in Format calls - so every one of them would silently produce
// the first letter only. The format string is therefore rewritten into
// the C-standard dialect before it reaches vswprintf/vsnprintf:
//
//     wide format:    %s -> %ls    %S -> %s     %c -> %lc    %C -> %c
//     narrow format:  %s -> %s     %S -> %ls    %c -> %c     %C -> %lc
//     both:           %I64 -> %ll  %I32 -> %l   %w{s,c} -> wide
//
// An explicit h/l/w length modifier already says which width is meant, so
// it wins over the conversion letter's default. On MSVC nothing is
// rewritten: there the application's dialect is already the CRT's.
// ---------------------------------------------------------------------
template <class Ch>
std::basic_string<Ch> TranslateFormat(const Ch* fmt)
{
    std::basic_string<Ch> out;
    if (!fmt) return out;
    constexpr bool wideFormat = std::is_same_v<Ch, wchar_t>;
    auto C = [](char c) { return static_cast<Ch>(c); };

    for (const Ch* p = fmt; *p; ++p)
    {
        if (*p != C('%')) { out += *p; continue; }
        out += *p;
        ++p;
        if (*p == C('%')) { out += *p; continue; }   // literal %%

        // Flags, width and precision pass through untouched.
        while (*p && (*p == C('-') || *p == C('+') || *p == C(' ')
                      || *p == C('#') || *p == C('0')))
            out += *p++;
        while (*p && ((*p >= C('0') && *p <= C('9')) || *p == C('*')))
            out += *p++;
        if (*p == C('.')) {
            out += *p++;
            while (*p && ((*p >= C('0') && *p <= C('9')) || *p == C('*')))
                out += *p++;
        }

        // Length modifiers. Collected rather than emitted, because for the
        // string/char conversions the modifier and the conversion letter
        // together decide the argument width.
        enum class Width { Default, Narrow, Wide } w = Width::Default;
        std::basic_string<Ch> lengthMods;
        for (;;) {
            if (*p == C('h')) {
                w = Width::Narrow; ++p;
                if (*p == C('h')) { lengthMods += C('h'); lengthMods += C('h'); ++p; }
                else lengthMods += C('h');
            } else if (*p == C('l')) {
                w = Width::Wide; ++p;
                if (*p == C('l')) { lengthMods += C('l'); lengthMods += C('l'); ++p; }
                else lengthMods += C('l');
            } else if (*p == C('w')) {          // MSVC: wide
                w = Width::Wide; lengthMods += C('l'); ++p;
            } else if (*p == C('L') || *p == C('j') || *p == C('z') || *p == C('t')) {
                lengthMods += *p++;
            } else if (*p == C('I')) {          // MSVC: I64 / I32 / I
                ++p;
                if (*p == C('6') && *(p + 1) == C('4')) { lengthMods += C('l'); lengthMods += C('l'); p += 2; }
                else if (*p == C('3') && *(p + 1) == C('2')) { p += 2; }
                else { lengthMods += C('z'); }   // plain %I is size_t-sized
            } else {
                break;
            }
        }
        if (!*p) { out += lengthMods; break; }   // malformed: leave as written

        const Ch conv = *p;
        if (conv == C('s') || conv == C('S') || conv == C('c') || conv == C('C'))
        {
            const bool upper = (conv == C('S') || conv == C('C'));
            // Default width: %s/%c match the format's own character type,
            // %S/%C mean the other one. An explicit h/l/w overrides both.
            bool wide = upper ? !wideFormat : wideFormat;
            if (w == Width::Narrow) wide = false;
            else if (w == Width::Wide) wide = true;

            if (wide) out += C('l');
            out += (conv == C('c') || conv == C('C')) ? C('c') : C('s');
        }
        else
        {
            out += lengthMods;
            out += conv;
        }
    }
    return out;
}


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
class ECStringT
{
public:
    using XCHAR = BaseType;        // this string's character type (ATL naming)
    using PXSTR = XCHAR*;
    using PCXSTR = const XCHAR*;
    using YCHAR = std::conditional_t<std::is_same_v<XCHAR, char>, wchar_t, char>; // the "other" type
    using PCYSTR = const YCHAR*;

    ECStringT() = default;
    ECStringT(const ECStringT&) = default;
    ECStringT(ECStringT&&) noexcept = default;
    ECStringT(PCXSTR pszSrc) { if (pszSrc) m_data = pszSrc; }
    ECStringT(PCXSTR pch, int nLength) { if (pch && nLength > 0) m_data.assign(pch, static_cast<size_t>(nLength)); }
    explicit ECStringT(XCHAR ch, int nRepeat = 1) : m_data(static_cast<size_t>(nRepeat < 0 ? 0 : nRepeat), ch) {}
    // Cross-character (YCHAR) sources convert, but explicitly: making them
    // implicit gives some call sites two equally good user-conversion
    // paths (CStringA -> PCSTR -> this, or CStringA -> this) which the
    // compiler rejects as ambiguous, and it also makes eMule's own
    // OptUtf8ToStr(const CStringA&)/OptUtf8ToStr(const CStringW&) pair
    // ambiguous. The cross-width assignment, append and comparison
    // operators below cover what eMule actually needs, each with a single
    // exact match and no conversion at all.
    explicit ECStringT(PCYSTR pszSrc) { if (pszSrc) m_data = Convert(pszSrc, std::char_traits<YCHAR>::length(pszSrc)); }
    explicit ECStringT(PCYSTR pch, int nLength) { if (pch && nLength > 0) m_data = Convert(pch, static_cast<size_t>(nLength)); }
    explicit ECStringT(const ECStringT<YCHAR>& strSrc) { m_data = Convert(strSrc.GetString(), static_cast<size_t>(strSrc.GetLength())); }

    ECStringT& operator=(const ECStringT&) = default;
    ECStringT& operator=(ECStringT&&) noexcept = default;
    ECStringT& operator=(PCXSTR pszSrc) { if (pszSrc) m_data = pszSrc; else m_data.clear(); return *this; }
    ECStringT& operator=(XCHAR ch) { m_data.assign(1, ch); return *this; }
    ECStringT& operator=(PCYSTR pszSrc) { m_data = pszSrc ? Convert(pszSrc, std::char_traits<YCHAR>::length(pszSrc)) : std::basic_string<XCHAR>(); return *this; }
    // NOTE: there is deliberately NO operator=(const CStringT<YCHAR>&).
    // Real ATL has no such overload either -- the other width's string
    // class is assigned through its operator PCXSTR and the PCYSTR
    // overload right above, and the cross-width constructor at line 418
    // is explicit so it never competes. Adding one looks harmless but
    // changes overload resolution for anything DERIVED from CStringA:
    // binding a derived object to "const CStringA&" beats a user-defined
    // conversion, so it won every time -- including where the derivation
    // is private/protected, which is a hard error rather than a
    // fallback. That is exactly Kademlia's CKadTagNameString ("class
    // CKadTagNameString : protected CStringA", Tag.h:46), which exposes
    // its content through an explicit operator PCXSTR; "strName =
    // pTag->m_name;" (MetaDataDlg.cpp:311) failed with C2243, an
    // inaccessible-base conversion, while compiling fine against real
    // MFC. (The overload was originally added for "m_pProxyPeerHost =
    // sAscii;", which the explicit constructor above already covers.)

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
    // Same as ReleaseBuffer, but the length is known to be exact: real MFC
    // skips the strlen rather than allowing the -1 "measure it" form.
    void ReleaseBufferSetLength(int nNewLength)
    {
        m_data.resize(static_cast<size_t>(nNewLength));
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
    // Replaces the whole contents, the counted form included (which is how
    // eMule copies out of a fixed-size buffer that may not be terminated).
    void SetString(PCXSTR pszSrc) { m_data = pszSrc ? pszSrc : std::basic_string<XCHAR>(); }
    void SetString(PCXSTR pszSrc, int nLength)
    {
        if (pszSrc && nLength > 0)
            m_data.assign(pszSrc, static_cast<size_t>(nLength));
        else
            m_data.clear();
    }

    // Windows-only interop: both hand off to Win32 (LoadStringW /
    // SysAllocString) and have no portable meaning, so they exist only on
    // Windows -- exactly as real MFC does not offer them anywhere else.
    // eMule loads its UI strings from a language DLL, hence the
    // explicit-module and explicit-language overloads.
    //
    // These were declaration-only until the conformance suite went to
    // full method coverage and found that eMule names both (LoadString in
    // the language-DLL path, AllocSysString in the COM interop) against a
    // library that never defined them: a link error waiting to happen, not
    // just an untestable method.
#ifdef _WIN32
    BOOL LoadString(UINT nID)
    {
        // Real MFC resolves the module the same way: whatever
        // AfxGetResourceHandle() currently points at. Without MFC's module
        // state, the process image is the equivalent default -- and
        // GetModuleHandle(nullptr) is what MFC itself falls back to.
        return LoadString(::GetModuleHandleW(nullptr), nID);
    }

    BOOL LoadString(HINSTANCE hInstance, UINT nID)
    {
        return LoadStringForLangId(hInstance, nID, /*useLangId*/ false, 0);
    }

    BOOL LoadString(HINSTANCE hInstance, UINT nID, WORD wLanguageID)
    {
        return LoadStringForLangId(hInstance, nID, /*useLangId*/ true, wLanguageID);
    }

    // Returns a BSTR the caller owns (SysFreeString), as real MFC does.
    // A BSTR is always UTF-16, so the char instantiation widens first.
    BSTR AllocSysString() const
    {
        BSTR bstr = nullptr;
        if constexpr (std::is_same_v<XCHAR, wchar_t>)
        {
            bstr = ::SysAllocStringLen(reinterpret_cast<const OLECHAR*>(m_data.data()),
                                       static_cast<UINT>(m_data.size()));
        }
        else
        {
            const int nSrc = static_cast<int>(m_data.size());
            const int nWide = nSrc == 0
                                  ? 0
                                  : ::MultiByteToWideChar(CP_ACP, 0, m_data.data(), nSrc, nullptr, 0);
            bstr = ::SysAllocStringLen(nullptr, static_cast<UINT>(nWide < 0 ? 0 : nWide));
            if (bstr != nullptr && nWide > 0)
                ::MultiByteToWideChar(CP_ACP, 0, m_data.data(), nSrc, bstr, nWide);
        }
        // Real MFC throws rather than returning a null BSTR on failure.
        if (bstr == nullptr)
            EAfxThrowMemoryException();
        return bstr;
    }
#endif

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
    ECStringT Left(int nCount) const
    {
        nCount = std::clamp(nCount, 0, GetLength());
        return ECStringT(m_data.substr(0, static_cast<size_t>(nCount)).c_str());
    }
    ECStringT Right(int nCount) const
    {
        nCount = std::clamp(nCount, 0, GetLength());
        return ECStringT(m_data.substr(m_data.size() - static_cast<size_t>(nCount)).c_str());
    }
    ECStringT Mid(int iFirst, int nCount) const
    {
        iFirst = std::clamp(iFirst, 0, GetLength());
        nCount = std::clamp(nCount, 0, GetLength() - iFirst);
        return ECStringT(m_data.substr(static_cast<size_t>(iFirst), static_cast<size_t>(nCount)).c_str());
    }
    ECStringT Mid(int iFirst) const { return Mid(iFirst, GetLength() - iFirst); }
    ECStringT& MakeLower()
    {
        std::transform(m_data.begin(), m_data.end(), m_data.begin(), [](XCHAR c) { return mfc_detail::StrTraits<XCHAR>::Lower(c); });
        return *this;
    }
    ECStringT& MakeUpper()
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
    ECStringT SpanExcluding(PCXSTR pszCharSet) const
    {
        auto pos = m_data.find_first_of(pszCharSet);
        return pos == npos ? *this : Left(static_cast<int>(pos));
    }
    ECStringT Tokenize(PCXSTR pszTokens, int& iStart) const
    {
        if (iStart < 0 || static_cast<size_t>(iStart) >= m_data.size()) { iStart = -1; return ECStringT(); }
        size_t begin = m_data.find_first_not_of(pszTokens, static_cast<size_t>(iStart));
        if (begin == npos) { iStart = -1; return ECStringT(); }
        size_t end = m_data.find_first_of(pszTokens, begin);
        ECStringT tok(m_data.substr(begin, end == npos ? npos : end - begin).c_str());
        iStart = (end == npos) ? -1 : static_cast<int>(end + 1);
        return tok;
    }
    ECStringT& Trim() { return Trim(mfc_detail::StrTraits<XCHAR>::WS()); }
    ECStringT& Trim(XCHAR chTarget) { XCHAR set[2] = {chTarget, 0}; return Trim(set); }
    ECStringT& Trim(PCXSTR pszTargets)
    {
        size_t b = m_data.find_first_not_of(pszTargets);
        if (b == npos) m_data.clear();
        else m_data.erase(0, b);
        return TrimRight(pszTargets);
    }
    ECStringT& TrimRight() { return TrimRight(mfc_detail::StrTraits<XCHAR>::WS()); }
    ECStringT& TrimRight(XCHAR chTarget) { XCHAR set[2] = {chTarget, 0}; return TrimRight(set); }
    ECStringT& TrimRight(PCXSTR pszTargets)
    {
        size_t e = m_data.find_last_not_of(pszTargets);
        if (e == npos) m_data.clear();
        else m_data.erase(e + 1);
        return *this;
    }

    // c_str() and AsStdString() used to sit here: simple_mfc's own
    // additions, with no CStringT counterpart in real MFC and no user in
    // eMule. Nothing could compare them, so they are gone -- GetString()
    // is MFC's own spelling of the first, and the second was only ever a
    // shortcut past the class's interface.
    operator PCXSTR() const noexcept { return m_data.c_str(); }
    XCHAR operator[](int i) const { return m_data[static_cast<size_t>(i)]; }

    ECStringT& operator+=(const ECStringT& s) { m_data += s.m_data; return *this; }
    ECStringT& operator+=(PCXSTR psz) { if (psz) m_data += psz; return *this; }
    ECStringT& operator+=(XCHAR ch) { m_data += ch; return *this; }
    ECStringT& operator+=(PCYSTR psz) { if (psz) m_data += Convert(psz, std::char_traits<YCHAR>::length(psz)); return *this; }
    ECStringT& operator+=(const ECStringT<YCHAR>& s) { m_data += Convert(s.GetString(), static_cast<size_t>(s.GetLength())); return *this; }

    friend ECStringT operator+(const ECStringT& a, const ECStringT& b) { ECStringT r(a); r.m_data += b.m_data; return r; }
    friend ECStringT operator+(const ECStringT& a, PCXSTR b) { ECStringT r(a); if (b) r.m_data += b; return r; }
    friend ECStringT operator+(PCXSTR a, const ECStringT& b) { ECStringT r; if (a) r.m_data = a; r.m_data += b.m_data; return r; }
    friend ECStringT operator+(const ECStringT& a, XCHAR b) { ECStringT r(a); r.m_data += b; return r; }
    friend ECStringT operator+(XCHAR a, const ECStringT& b) { ECStringT r; r.m_data += a; r.m_data += b.m_data; return r; }

    friend bool operator==(const ECStringT& a, const ECStringT& b) noexcept { return a.m_data == b.m_data; }
    friend bool operator==(const ECStringT& a, PCXSTR b) noexcept { return a.m_data == b; }
    friend bool operator==(PCXSTR a, const ECStringT& b) noexcept { return a == b.m_data; }
    friend bool operator!=(const ECStringT& a, const ECStringT& b) noexcept { return a.m_data != b.m_data; }
    friend bool operator!=(const ECStringT& a, PCXSTR b) noexcept { return a.m_data != b; }
    friend bool operator!=(PCXSTR a, const ECStringT& b) noexcept { return a != b.m_data; }
    // Against the other width's literal, which is how eMule tests a wide
    // CString against the narrow constants in Opcodes.h.
    friend bool operator==(const ECStringT& a, PCYSTR b) noexcept { return b != nullptr && a.m_data == Convert(b, std::char_traits<YCHAR>::length(b)); }
    friend bool operator==(PCYSTR a, const ECStringT& b) noexcept { return b == a; }
    friend bool operator!=(const ECStringT& a, PCYSTR b) noexcept { return !(a == b); }
    friend bool operator!=(PCYSTR a, const ECStringT& b) noexcept { return !(b == a); }
    friend bool operator<(const ECStringT& a, const ECStringT& b) noexcept { return a.m_data < b.m_data; }
    friend bool operator<(const ECStringT& a, PCXSTR b) noexcept { return a.m_data < b; }
    friend bool operator<(PCXSTR a, const ECStringT& b) noexcept { return a < b.m_data; }
    friend bool operator>(const ECStringT& a, const ECStringT& b) noexcept { return a.m_data > b.m_data; }
    friend bool operator>(const ECStringT& a, PCXSTR b) noexcept { return a.m_data > b; }
    friend bool operator>(PCXSTR a, const ECStringT& b) noexcept { return a > b.m_data; }
    friend bool operator<=(const ECStringT& a, const ECStringT& b) noexcept { return a.m_data <= b.m_data; }
    friend bool operator>=(const ECStringT& a, const ECStringT& b) noexcept { return a.m_data >= b.m_data; }

private:
    static constexpr auto npos = std::basic_string<XCHAR>::npos;

#ifdef _WIN32
    // Shared body of the three LoadString overloads. LoadStringW has no
    // language-aware form, so the explicit-language overload goes through
    // FindResourceEx/LoadResource and decodes the string table block by
    // hand -- which is what MFC's own AfxLoadString does for a language
    // DLL. A string table resource holds 16 strings per block; block
    // number is nID/16 + 1 and the entry within it is nID%16, each entry
    // being a WORD length followed by that many UTF-16 units.
    BOOL LoadStringForLangId(HINSTANCE hInstance, UINT nID, bool useLangId, WORD wLanguageID)
    {
        if (!useLangId)
        {
            // The CRT-side loader: returns a pointer INTO the resource, and
            // the count of characters, without needing a caller buffer.
            const wchar_t* pStr = nullptr;
            const int n = ::LoadStringW(hInstance, nID, reinterpret_cast<LPWSTR>(&pStr), 0);
            // A failed load leaves the string ALONE -- real MFC does not
            // clear it, and a caller that pre-seeded a default relies on
            // that. Emptying it was this branch's invention.
            if (n <= 0)
                return FALSE;
            m_data = Convert(pStr, static_cast<size_t>(n));
            return TRUE;
        }

        const HRSRC hRes = ::FindResourceExW(hInstance, RT_STRING,
                                             MAKEINTRESOURCEW(nID / 16 + 1), wLanguageID);
        if (hRes == nullptr) { return FALSE; }
        const HGLOBAL hMem = ::LoadResource(hInstance, hRes);
        if (hMem == nullptr) { return FALSE; }
        const wchar_t* p = static_cast<const wchar_t*>(::LockResource(hMem));
        if (p == nullptr) { return FALSE; }

        const DWORD cbRes = ::SizeofResource(hInstance, hRes);
        const wchar_t* const end = p + cbRes / sizeof(wchar_t);
        for (UINT i = 0; i < nID % 16; ++i)
        {
            if (p >= end) { return FALSE; }
            p += 1 + static_cast<size_t>(*p); // length prefix, then the text
        }
        if (p >= end || *p == 0) { return FALSE; }
        const size_t len = static_cast<size_t>(*p);
        if (p + 1 + len > end) { return FALSE; }
        m_data = Convert(p + 1, len);
        return TRUE;
    }
#endif

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
#ifdef _MSC_VER
        // The application's dialect IS the CRT's here: nothing to rewrite.
        PCXSTR fmtUsed = fmt;
#else
        // Elsewhere the CRT follows the C standard, in which %s inside a wide
        // format means char* -- so a wide argument would silently format as
        // its first character. See TranslateFormat above.
        const std::basic_string<XCHAR> fmtHeld = mfc_detail::TranslateFormat(fmt);
        PCXSTR fmtUsed = fmtHeld.c_str();
#endif
        size_t size = 256;
        std::vector<XCHAR> buf(size);
        for (;;)
        {
            va_list ap;
            va_copy(ap, args);
            int n = mfc_detail::StrTraits<XCHAR>::FormatV(buf.data(), size, fmtUsed, ap);
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

using ECStringA = ECStringT<char>;    // ANSI/char
using ECStringW = ECStringT<wchar_t>; // wide
using ECString = ECStringW;           // TCHAR under UNICODE, matching real MFC

namespace std
{
template <class Ch, class Tr>
struct hash<ECStringT<Ch, Tr>>
{
    // std::hash<basic_string_view<Ch>> is required to produce the same
    // value as std::hash<basic_string<Ch>> for the same characters, so
    // this is the identical hash without reaching into the object.
    size_t operator()(const ECStringT<Ch, Tr>& s) const noexcept
    {
        return std::hash<std::basic_string_view<Ch>>{}(
            std::basic_string_view<Ch>(s.GetString(), static_cast<size_t>(s.GetLength())));
    }
};
} // namespace std

// ---------------------------------------------------------------------
// CException — real hierarchy, genuine C++ exceptions (used with standard
// throw/catch, not with the original MFC pointer+Delete() pattern, even
// though Delete() is still available for interface compatibility with
// code written against real MFC).
// ---------------------------------------------------------------------
class ECException : public ECObject
{
public:
    // Declared on the base in real MFC, which is how eMule can call it
    // through a plain CException*.
    virtual BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) const;

    EDECLARE_DYNAMIC(ECException)
public:
    // The default constructor is what eMule's own exception classes call
    // implicitly (Exceptions.h derives from CException without naming a base
    // initializer). Real MFC defaults m_bAutoDelete to TRUE here: an exception
    // built without an explicit choice is heap-allocated and self-deleting.
    ECException() : m_bAutoDelete(TRUE) {}
    explicit ECException(BOOL bAutoDelete) : m_bAutoDelete(bAutoDelete) {}
    void Delete() { if (m_bAutoDelete) delete this; }
    // ReportError is deliberately absent: real MFC's opens a Win32
    // MessageBox, which no headless build can produce and no conformance
    // probe could ever compare -- and eMule never calls it.

private:
    BOOL m_bAutoDelete;
};

// Abstract base for "resource-critical" exceptions. In real MFC
// GetErrorMessage is virtual with a body (not pure); here it is made pure
// to enforce abstractness at compile time (an explicit design choice), so
// every concrete subclass must provide its own override.
class ECSimpleException : public ECException
{
    EDECLARE_DYNAMIC(ECSimpleException)
public:
    ECSimpleException() : ECException(FALSE) {}
    explicit ECSimpleException(BOOL bAutoDelete) : ECException(bAutoDelete) {}
    virtual BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) const override = 0;
};

// Thrown for an operation a class does not implement; eMule's Kademlia
// I/O layer uses it for the unimplemented halves of its stream classes.
class ECNotSupportedException : public ECSimpleException
{
    EDECLARE_DYNAMIC(ECNotSupportedException)
public:
    ECNotSupportedException() = default;
    BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) const override;
};

class ECMemoryException : public ECSimpleException
{
    EDECLARE_DYNAMIC(ECMemoryException)
public:
    ECMemoryException() : ECSimpleException(FALSE) {}
    BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) const override;
};

class ECFileException : public ECException
{
    EDECLARE_DYNAMIC(ECFileException)
public:
    enum Cause
    {
        none, genericException, fileNotFound, badPath, tooManyOpenFiles,
        accessDenied, invalidFile, removeCurrentDir, directoryFull, badSeek,
        hardIO, sharingViolation, lockViolation, diskFull, endOfFile
    };

    explicit ECFileException(int cause = none, LONG lOsError = -1, LPCTSTR lpszFileName = nullptr)
        : ECException(TRUE), m_cause(cause), m_lOsError(lOsError),
          m_strFileName(lpszFileName ? lpszFileName : L"") {}

    BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) const override;

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
    ECString m_strFileName;
};

// Global exception-throwing functions. In real MFC, AfxThrowInvalidArgException,
// AfxThrowNotSupportedException, AfxThrowResourceException and
// AfxThrowUserException throw classes (CInvalidArgException, etc.) that are
// not implemented here — see ../README.md for why only the two below are
// provided.
[[noreturn]] void EAfxThrowFileException(int cause, LONG lOsError = -1, LPCTSTR lpszFileName = nullptr);
[[noreturn]] void EAfxThrowMemoryException();

// ---------------------------------------------------------------------
// CFile / CStdioFile / CMemFile — built on std::fstream / an in-memory
// buffer, fully portable (no Win32 HANDLE).
// ---------------------------------------------------------------------
// CFileStatus is owned by afx.h (as in real MFC), but carries CTime
// members, so CFile can only take it by reference here -- a forward
// declaration covers that. Its full definition is at the very bottom of
// this header, after #include "eatltime.h" has made CTime complete.
// (atltime.h no longer includes afx.h back, so this is a one-way
// dependency now, no cycle.)
class ECTime;
struct ECFileStatus;
#ifndef _WIN32
struct FILETIME;
#endif

class ECFile : public ECObject
{
    EDECLARE_DYNAMIC(ECFile)
public:
    // The path this file was opened with. Protected in real MFC, and
    // eMule's CSafeFile passes it to AfxThrowFileException.
    ECString m_strFileName;

    // "Usually contains the operating-system file handle" (Learn). Public
    // in real MFC, with a conversion operator alongside it, which is how
    // eMule hands a CFile straight to a Win32 call.
    HANDLE m_hFile = nullptr;
    operator HANDLE() const { return m_hFile; }

    enum OpenFlags
    {
        modeRead = 0x0000, modeWrite = 0x0001, modeReadWrite = 0x0002,
        modeCreate = 0x1000, modeNoTruncate = 0x2000,
        shareDenyWrite = 0x0020, shareDenyNone = 0x0040,
        // Cache hint, passed by eMule when it streams a file end to end.
        // Value is real MFC's, which is why it sits far above the mode
        // flags rather than continuing the sequence.
        osSequentialScan = 0x80000,
        typeBinary = 0x0000, typeText = 0x4000
    };
    enum SeekPosition { begin = 0, current = 1, end = 2 };

    ECFile() = default;
    ECFile(LPCTSTR lpszFileName, UINT nOpenFlags) { Open(lpszFileName, nOpenFlags); }
    virtual ~ECFile() { if (m_stream.is_open()) m_stream.close(); }

    virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags, ECFileException* pError = nullptr);
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
    virtual ECString GetFileName() const { return ECString(std::filesystem::path(m_path).filename().wstring().c_str()); }
    virtual ECString GetFilePath() const { return ECString(m_path.c_str()); }
    BOOL GetStatus(ECFileStatus& rStatus) const;
    static BOOL GetStatus(LPCTSTR lpszFileName, ECFileStatus& rStatus);
    static void Remove(LPCTSTR lpszFileName);
    static void Rename(LPCTSTR lpszOldName, LPCTSTR lpszNewName);

protected:
    std::fstream m_stream;
    std::wstring m_path;
};

class ECStdioFile : public ECFile
{
    EDECLARE_DYNAMIC(ECStdioFile)
public:
    ECStdioFile() = default;
    ECStdioFile(LPCTSTR lpszFileName, UINT nOpenFlags) : ECFile(lpszFileName, nOpenFlags) {}
    // Real MFC also wraps an already-open FILE*.
    explicit ECStdioFile(FILE* pOpenStream) : m_pStream(pOpenStream) {}

    // The underlying stream, a public member in real MFC. eMule's
    // CSafeBufferedFile reads it directly to fflush/setvbuf the buffer.
    FILE* m_pStream = nullptr;

    virtual LPTSTR ReadString(LPTSTR lpsz, UINT nMax);
    virtual BOOL ReadString(ECString& rString);
    virtual void WriteString(LPCTSTR lpsz);
};

// CMemFile — an entirely in-memory file (std::vector<uint8_t>), no methods
// of its own beyond the ones it inherits.
class ECMemFile : public ECFile
{
    EDECLARE_DYNAMIC(ECMemFile)
public:
    ECMemFile() = default;
    // Real MFC's CMemFile(UINT nGrowBytes) constructor. eMule's CSafeMemFile
    // forwards to it (SafeFile.h:98). nGrowBytes only tunes reallocation
    // growth, which this std::vector-backed implementation handles
    // automatically, so the argument is accepted and ignored.
    explicit ECMemFile(UINT /*nGrowBytes*/) {}
    // Real MFC's attach-a-buffer constructor CMemFile(BYTE*, UINT, UINT).
    // eMule's CSafeMemFile(const BYTE*, UINT) forwards to it (SafeFile.h:104).
    ECMemFile(BYTE* lpBuffer, UINT nBufferSize, UINT /*nGrowBytes*/ = 0)
        : m_buffer(lpBuffer, lpBuffer + nBufferSize) {}
    UINT Read(void* lpBuf, UINT nCount) override;
    void Write(const void* lpBuf, UINT nCount) override;
    ULONGLONG Seek(LONGLONG lOff, UINT nFrom) override;
    ULONGLONG GetLength() const override { return m_buffer.size(); }
    void SetLength(ULONGLONG dwNewLen) override { m_buffer.resize(static_cast<size_t>(dwNewLen)); }
    ULONGLONG GetPosition() const override { return m_pos; }
    // Hands the memory buffer over to the caller, who becomes responsible
    // for free()ing it (eMule does exactly that in CEncryptedStreamSocket).
    // Declaration-only: the vector-backed storage above has no detachable
    // malloc'd block to give away, and the compile-check never links.
    BYTE* Detach();
    void Attach(BYTE* lpBuffer, UINT nBufferSize, UINT nGrowBytes = 0);
    // Reserves space ahead of a write. Protected in real MFC too; eMule's
    // CSafeMemFile calls it from its own override.
    virtual void GrowFile(ULONGLONG dwNewLen);

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
//
// FindNextFile is also a winbase.h macro (FindNextFileW under UNICODE), so
// on Windows the member below is declared -- and every call to it compiled
// -- under the substituted name. That substitution is left alone on
// purpose: it is exactly what happens to real MFC's own CFileFind, whose
// member is therefore CFileFind::FindNextFileW, and undefining the macro
// here would silence it for the whole rest of the translation unit,
// including any real-MFC header included after this one. Off Windows there
// is no macro and the name stays literal.
// ---------------------------------------------------------------------
class ECFileFind : public ECObject
{
    EDECLARE_DYNAMIC(ECFileFind)
public:
    virtual BOOL FindFile(LPCTSTR pstrName = nullptr, DWORD dwUnused = 0);
    virtual BOOL FindNextFile();
    void Close() { m_it = std::filesystem::directory_iterator(); m_pending.reset(); }
    BOOL IsDirectory() const;
    virtual BOOL IsDots() const;
    // Attribute queries. std::filesystem exposes no notion of the Windows
    // "system"/"hidden"/"archive" bits, so off Windows these answer FALSE
    // rather than guessing (a leading dot is a convention, not an
    // attribute). eMule uses IsSystem to skip system folders when scanning
    // shared directories.
    BOOL IsSystem() const;
    BOOL IsHidden() const;
    BOOL IsReadOnly() const;
    virtual ECString GetFileName() const;
    virtual ECString GetFilePath() const;
    ULONGLONG GetLength() const;
    virtual ECString GetRoot() const;
    // The find-data timestamps. CTime is incomplete here (see the
    // CFileStatus note above), which is enough for a reference parameter.
    virtual BOOL GetLastWriteTime(ECTime& refTime) const;
    virtual BOOL GetCreationTime(ECTime& refTime) const;
    virtual BOOL GetLastAccessTime(ECTime& refTime) const;
    // Real MFC offers each timestamp in raw FILETIME form as well, which
    // is what eMule uses when it only needs to compare two of them.
    virtual BOOL GetLastWriteTime(FILETIME* pTimeStamp) const;
    virtual BOOL GetCreationTime(FILETIME* pTimeStamp) const;
    virtual BOOL GetLastAccessTime(FILETIME* pTimeStamp) const;
    BOOL IsTemporary() const;
    BOOL IsArchived() const;
    BOOL IsCompressed() const;

private:
    bool AdvanceToNextMatch();

    std::filesystem::path m_dir;
    std::wstring m_root;
    std::wstring m_pattern;
    std::filesystem::directory_iterator m_it;
    std::optional<std::filesystem::directory_entry> m_pending;
    std::filesystem::directory_entry m_current;
};

// ---------------------------------------------------------------------
// CArchive — the serialization stream MFC layers over a CFile. eMule
// reads its saved part-file metadata through one:
//
//   CArchive ar(&sdFile, CArchive::load);
//   ar >> nTotal >> nRemaining >> nFragments;
//   ar.Read(pMD4, sizeof pMD4);
//   ar.Close();
//
// NATIVE implementation (afx.cpp): forwards Read/Write to the underlying
// CFile. The primitive-type operators (BYTE/WORD/int/UINT/long/DWORD/
// float/double/ULONGLONG) write/read exactly sizeof(T) raw bytes, with no
// length prefix -- this is real MFC's actual wire format for these types
// too, so an archive containing only primitives (like eMule's part-file
// metadata above) round-trips against a file written by real MFC. The
// CString operators are a documented exception: they use a
// self-consistent 32-bit length prefix + raw wchar_t payload that is NOT
// necessarily byte-identical to real MFC's CString::Serialize format
// (which also encodes a narrow/wide flag and a variable-length count) --
// eMule's own CArchive usage never reaches this path, so treat CString
// (de)serialization as round-trip-correct within simple_mfc only.
// ---------------------------------------------------------------------
class ECArchive
{
public:
    enum Mode { store = 0, load = 1 };

    ECArchive(ECFile* pFile, UINT nMode, int nBufSize = 4096, void* lpBuf = nullptr);
    ~ECArchive();

    BOOL IsLoading() const;
    BOOL IsStoring() const;
    ECFile* GetFile() const;
    void Close();
    void Flush();
    UINT Read(void* lpBuf, UINT nMax);
    void Write(const void* lpBuf, UINT nMax);

    ECArchive& operator>>(BYTE& by);
    ECArchive& operator>>(WORD& w);
    ECArchive& operator>>(int& i);
    ECArchive& operator>>(UINT& u);
    ECArchive& operator>>(long& l);
#ifdef _WIN32
    // Off Windows DWORD is `unsigned int`, i.e. the same type as UINT, so
    // this overload would redeclare the one above. See the typedef block at
    // the top of this header for why width wins there.
    ECArchive& operator>>(DWORD& dw);
#endif
    ECArchive& operator>>(float& f);
    ECArchive& operator>>(double& d);
    ECArchive& operator>>(ULONGLONG& dwdw);
    ECArchive& operator>>(ECString& str);

    ECArchive& operator<<(BYTE by);
    ECArchive& operator<<(WORD w);
    ECArchive& operator<<(int i);
    ECArchive& operator<<(UINT u);
    ECArchive& operator<<(long l);
#ifdef _WIN32
    ECArchive& operator<<(DWORD dw);   // == UINT off Windows, see above
#endif
    ECArchive& operator<<(float f);
    ECArchive& operator<<(double d);
    ECArchive& operator<<(ULONGLONG dwdw);
    ECArchive& operator<<(const ECString& str);

private:
    ECFile* m_pFile;
    UINT m_nMode;
};

// Thrown when an archive operation fails; eMule catches it by pointer.
class ECArchiveException : public ECException
{
    EDECLARE_DYNAMIC(ECArchiveException)
public:
    enum Cause
    {
        none, genericException, readOnly, endOfFile, writeOnly, badIndex,
        badClass, badSchema, bufferFull
    };

    explicit ECArchiveException(int cause = ECArchiveException::none, LPCTSTR lpszArchiveName = nullptr);
    int m_cause;
    ECString m_strFileName;
};

// The time classes. Last, because atltime.h builds on CString. It also
// makes CTime complete and supplies _MAX_PATH, both needed by CFileStatus.
#include "eatltime.h"

// CFileStatus -- defined here (its real MFC home) now that atltime.h has
// made CTime complete; forward-declared far above, where CFile::GetStatus
// takes it by reference.
struct ECFileStatus
{
    ECTime m_ctime;               // creation
    ECTime m_mtime;               // last modification
    ECTime m_atime;               // last access
    ULONGLONG m_size = 0;
    BYTE m_attribute = 0;
    BYTE m_padding = 0;
    TCHAR m_szFullName[_MAX_PATH] = {};
};
