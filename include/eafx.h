#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstddef>
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

#ifndef _MSC_VER
#define __cdecl
#define __stdcall
#endif
#define EAFXAPI __stdcall
#define EAFX_CDECL __cdecl

using UINT = unsigned int;
using WORD = unsigned short;
using BYTE = unsigned char;
#ifdef _WIN32
using DWORD = unsigned long;
#else
using DWORD = unsigned int;
#endif
using HANDLE = void*;
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
#ifdef _WIN32
#if !defined(UNICODE) || !defined(_UNICODE)
#error "simple_mfc is unconditionally wide-char: build with UNICODE and _UNICODE defined."
#endif
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#define ESIMPLE_MFC_UNDEF_WINSOCKAPI
#endif
#include <windows.h>
#ifdef ESIMPLE_MFC_UNDEF_WINSOCKAPI
#undef _WINSOCKAPI_
#undef ESIMPLE_MFC_UNDEF_WINSOCKAPI
#endif
#include <oleauto.h>
#endif
#include "etchar.h"

#ifdef _DEBUG
#ifndef EASSERT
#define EASSERT(f) assert(f)
#endif
#ifndef EVERIFY
#define EVERIFY(f) EASSERT(f)
#endif
#ifndef EDEBUG_ONLY
#define EDEBUG_ONLY(f) (f)
#endif
#ifndef EASSERT_VALID
#define EASSERT_VALID(pOb) (EAfxAssertValidObject(pOb, __FILE__, __LINE__))
#endif
#ifndef EASSERT_KINDOF
#define EASSERT_KINDOF(class_name, object) \
    EASSERT((object)->IsKindOf(ERUNTIME_CLASS(class_name)))
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
#ifndef EASSERT_VALID
#define EASSERT_VALID(pOb) ((void)0)
#endif
#ifndef EASSERT_KINDOF
#define EASSERT_KINDOF(class_name, object) ((void)0)
#endif
#endif
#ifndef ETRACE
#ifdef _MSC_VER
#define ETRACE __noop
#else
#define ETRACE(...) ((void)0)
#endif
#endif
#ifndef EATLASSERT
#define EATLASSERT(expr) EASSERT(expr)
#endif
#ifndef EATLTRACE2
#define EATLTRACE2(...) ((void)0)
#endif

#ifndef LODWORD
#define LODWORD(l) ((DWORD)((unsigned long long)(l)&0xFFFFFFFFULL))
#endif
#ifndef HIDWORD
#define HIDWORD(l) ((DWORD)(((unsigned long long)(l) >> 32) & 0xFFFFFFFFULL))
#endif

class ECObject;
class ECDumpContext;

struct ECRuntimeClass
{
    const char* m_lpszClassName;
    const ECRuntimeClass* m_pBaseClass;
    ECObject* (*m_pfnCreateObject)();

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

#define EDECLARE_DYNCREATE(class_name)                                          \
    EDECLARE_DYNAMIC(class_name)                                                \
public:                                                                        \
    static ECObject* CreateObject();

#define EIMPLEMENT_DYNCREATE(class_name, base_class_name)                       \
    ECObject* class_name::CreateObject() { return new class_name; }             \
    const ECRuntimeClass class_name::classCRuntimeClass =                       \
        {#class_name, &base_class_name::classCRuntimeClass, &class_name::CreateObject};

#define ERUNTIME_CLASS(class_name) (const_cast<ECRuntimeClass*>(&class_name::classCRuntimeClass))

ECObject* EAfxDynamicDownCast(ECRuntimeClass* pClass, ECObject* pObject);
#define EDYNAMIC_DOWNCAST(class_name, pObject)                                  \
    ((class_name*)EAfxDynamicDownCast(ERUNTIME_CLASS(class_name), pObject))

class ECObject
{
public:
    static const ECRuntimeClass classCRuntimeClass;
    virtual ECRuntimeClass* GetRuntimeClass() const { return const_cast<ECRuntimeClass*>(&classCRuntimeClass); }
    BOOL IsKindOf(const ECRuntimeClass* pClass) const { return GetRuntimeClass()->IsDerivedFrom(pClass) ? TRUE : FALSE; }
    virtual void AssertValid() const {}
    virtual void Dump(ECDumpContext& dc) const;
    virtual BOOL IsSerializable() const { return FALSE; }
    virtual ~ECObject() = default;
};

#ifdef _DEBUG
void EAFXAPI EAfxAssertValidObject(const ECObject* pOb, const char* lpszFileName, int nLine);
#endif

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

[[noreturn]] void EAfxThrowMemoryException();

#define ESIMPLE_MFC_ERROR_SHARING_VIOLATION 32L

namespace mfc_detail
{
void ReleaseShare(const void* owner);

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

inline void Utf16AppendNarrow(std::u16string& out, const char* p, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        out.push_back(static_cast<char16_t>(static_cast<unsigned char>(p[i])));
}

inline void Utf16AppendPadded(std::u16string& out, const std::u16string& s,
                              int width, bool leftAlign)
{
    const int pad = width > static_cast<int>(s.size())
                        ? width - static_cast<int>(s.size())
                        : 0;
    if (!leftAlign) out.append(static_cast<size_t>(pad), u' ');
    out += s;
    if (leftAlign) out.append(static_cast<size_t>(pad), u' ');
}

inline std::u16string Utf16FormatBuild(const char16_t* fmt, va_list ap)
{
    std::u16string out;
    if (!fmt) return out;

    for (const char16_t* p = fmt; *p; ++p)
    {
        if (*p != u'%') { out.push_back(*p); continue; }
        ++p;
        if (!*p) break;
        if (*p == u'%') { out.push_back(u'%'); continue; }

        std::string spec("%");
        bool leftAlign = false;
        while (*p == u'-' || *p == u'+' || *p == u' ' || *p == u'#' || *p == u'0')
        {
            if (*p == u'-') leftAlign = true;
            spec.push_back(static_cast<char>(*p));
            ++p;
        }

        int width = -1;
        if (*p == u'*')
        {
            ++p;
            int w = va_arg(ap, int);
            if (w < 0) { leftAlign = true; w = -w; spec.push_back('-'); }
            width = w;
            spec += std::to_string(w);
        }
        else
        {
            std::string digits;
            while (*p >= u'0' && *p <= u'9') digits.push_back(static_cast<char>(*p++));
            if (!digits.empty())
            {
                width = 0;
                for (char c : digits) width = width * 10 + (c - '0');
                spec += digits;
            }
        }

        int prec = -1;
        if (*p == u'.')
        {
            ++p;
            std::string digits;
            if (*p == u'*')
            {
                ++p;
                const int pr = va_arg(ap, int);
                if (pr >= 0) { prec = pr; digits = std::to_string(pr); }
            }
            else
            {
                while (*p >= u'0' && *p <= u'9') digits.push_back(static_cast<char>(*p++));
                prec = 0;
                for (char c : digits) prec = prec * 10 + (c - '0');
            }
            if (prec >= 0) { spec.push_back('.'); spec += digits; }
        }

        std::string mods;
        while (*p == u'h' || *p == u'l' || *p == u'L'
               || *p == u'j' || *p == u'z' || *p == u't')
            mods.push_back(static_cast<char>(*p++));
        if (!*p) break;

        const char conv = static_cast<char>(*p);
        const bool wideArg = mods.find('l') != std::string::npos;

        if (conv == 's')
        {
            std::u16string arg;
            if (wideArg)
            {
                const char16_t* v = va_arg(ap, const char16_t*);
                if (v) arg.assign(v);
                else Utf16AppendNarrow(arg, "(null)", 6);
            }
            else
            {
                const char* v = va_arg(ap, const char*);
                if (!v) v = "(null)";
                Utf16AppendNarrow(arg, v, std::char_traits<char>::length(v));
            }
            if (prec >= 0 && static_cast<size_t>(prec) < arg.size())
                arg.resize(static_cast<size_t>(prec));
            Utf16AppendPadded(out, arg, width, leftAlign);
            continue;
        }

        if (conv == 'c')
        {
            const int v = va_arg(ap, int);
            const std::u16string arg(1, wideArg
                ? static_cast<char16_t>(v)
                : static_cast<char16_t>(static_cast<unsigned char>(v)));
            Utf16AppendPadded(out, arg, width, leftAlign);
            continue;
        }

        spec += mods;
        spec.push_back(conv);

        auto emit = [&out, &spec](auto value) {
            char stackBuf[256];
            const int n = std::snprintf(stackBuf, sizeof stackBuf, spec.c_str(), value);
            if (n < 0) return;
            if (static_cast<size_t>(n) < sizeof stackBuf)
            {
                Utf16AppendNarrow(out, stackBuf, static_cast<size_t>(n));
                return;
            }
            std::vector<char> heapBuf(static_cast<size_t>(n) + 1);
            std::snprintf(heapBuf.data(), heapBuf.size(), spec.c_str(), value);
            Utf16AppendNarrow(out, heapBuf.data(), static_cast<size_t>(n));
        };

        switch (conv)
        {
        case 'd': case 'i':
            if (mods == "ll" || mods == "j")   emit(va_arg(ap, long long));
            else if (mods == "l")              emit(va_arg(ap, long));
            else if (mods == "z" || mods == "t") emit(va_arg(ap, std::ptrdiff_t));
            else                               emit(va_arg(ap, int));
            break;
        case 'u': case 'o': case 'x': case 'X':
            if (mods == "ll" || mods == "j")   emit(va_arg(ap, unsigned long long));
            else if (mods == "l")              emit(va_arg(ap, unsigned long));
            else if (mods == "z" || mods == "t") emit(va_arg(ap, std::size_t));
            else                               emit(va_arg(ap, unsigned int));
            break;
        case 'f': case 'F': case 'e': case 'E':
        case 'g': case 'G': case 'a': case 'A':
            if (mods == "L") emit(va_arg(ap, long double));
            else             emit(va_arg(ap, double));
            break;
        case 'p':
            emit(va_arg(ap, void*));
            break;
        default:
            break;
        }
    }
    return out;
}

template <>
struct StrTraits<char16_t>
{
    static const char16_t* WS() noexcept { return u" \t\r\n"; }
    static char16_t Lower(char16_t c) noexcept { return static_cast<char16_t>(std::towlower(static_cast<std::wint_t>(c))); }
    static char16_t Upper(char16_t c) noexcept { return static_cast<char16_t>(std::towupper(static_cast<std::wint_t>(c))); }
    static int FormatV(char16_t* buf, size_t n, const char16_t* fmt, va_list a)
    {
        const std::u16string s = Utf16FormatBuild(fmt, a);
        if (buf && n)
        {
            const size_t copy = s.size() < n - 1 ? s.size() : n - 1;
            std::char_traits<char16_t>::copy(buf, s.data(), copy);
            buf[copy] = 0;
        }
        return static_cast<int>(s.size());
    }
};

template <class Ch>
std::basic_string<Ch> TranslateFormat(const Ch* fmt)
{
    std::basic_string<Ch> out;
    if (!fmt) return out;
    constexpr bool wideFormat = !std::is_same_v<Ch, char>;
    auto C = [](char c) { return static_cast<Ch>(c); };

    for (const Ch* p = fmt; *p; ++p)
    {
        if (*p != C('%')) { out += *p; continue; }
        out += *p;
        ++p;
        if (*p == C('%')) { out += *p; continue; }

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
            } else if (*p == C('w')) {
                w = Width::Wide; lengthMods += C('l'); ++p;
            } else if (*p == C('L') || *p == C('j') || *p == C('z') || *p == C('t')) {
                lengthMods += *p++;
            } else if (*p == C('I')) {
                ++p;
                if (*p == C('6') && *(p + 1) == C('4')) { lengthMods += C('l'); lengthMods += C('l'); p += 2; }
                else if (*p == C('3') && *(p + 1) == C('2')) { p += 2; }
                else { lengthMods += C('z'); }
            } else {
                break;
            }
        }
        if (!*p) { out += lengthMods; break; }

        const Ch conv = *p;
        if (conv == C('s') || conv == C('S') || conv == C('c') || conv == C('C'))
        {
            const bool upper = (conv == C('S') || conv == C('C'));
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

template <class Ch>
inline unsigned long CodeUnitValue(Ch c) noexcept
{
    if constexpr (sizeof(Ch) == 2)
        return static_cast<unsigned long>(static_cast<std::uint16_t>(c));
    else
        return static_cast<unsigned long>(static_cast<std::uint32_t>(c));
}

template <class Dst, class Src>
inline std::basic_string<Dst> WideToWide(const Src* p, size_t n)
{
    std::basic_string<Dst> out;
    if (!p) return out;
    out.reserve(n);
    if constexpr (sizeof(Dst) == sizeof(Src))
    {
        for (size_t i = 0; i < n; ++i)
            out.push_back(static_cast<Dst>(CodeUnitValue(p[i])));
    }
    else if constexpr (sizeof(Dst) > sizeof(Src))
    {
        for (size_t i = 0; i < n; ++i)
        {
            unsigned long cp = CodeUnitValue(p[i]);
            if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < n)
            {
                const unsigned long lo = CodeUnitValue(p[i + 1]);
                if (lo >= 0xDC00 && lo <= 0xDFFF)
                {
                    cp = 0x10000u + ((cp - 0xD800u) << 10) + (lo - 0xDC00u);
                    ++i;
                }
            }
            out.push_back(static_cast<Dst>(cp));
        }
    }
    else
    {
        for (size_t i = 0; i < n; ++i)
        {
            const unsigned long cp = CodeUnitValue(p[i]);
            if (cp < 0x10000u)
            {
                out.push_back(static_cast<Dst>(cp));
            }
            else if (cp <= 0x10FFFFu)
            {
                const unsigned long v = cp - 0x10000u;
                out.push_back(static_cast<Dst>(0xD800u + (v >> 10)));
                out.push_back(static_cast<Dst>(0xDC00u + (v & 0x3FFu)));
            }
            else
            {
                out.push_back(static_cast<Dst>(0xFFFDu));
            }
        }
    }
    return out;
}

template <class Ch>
inline std::basic_string<Ch> Widen(const char* p, size_t n)
{
    std::basic_string<Ch> w;
    w.reserve(n);
    for (size_t i = 0; i < n; ++i)
        w.push_back(static_cast<Ch>(static_cast<unsigned char>(p[i])));
    return w;
}
template <class Ch>
inline std::string Narrow(const Ch* p, size_t n)
{
    std::string s;
    s.reserve(n);
    for (size_t i = 0; i < n; ++i)
        s.push_back(p[i] < 0x80 ? static_cast<char>(p[i]) : '?');
    return s;
}
}

template <class BaseType, class = void>
class ECStringT
{
public:
    using XCHAR = BaseType;
    using PXSTR = XCHAR*;
    using PCXSTR = const XCHAR*;
    using YCHAR = std::conditional_t<std::is_same_v<XCHAR, char>, EWCHAR, char>;
    using PCYSTR = const YCHAR*;

    ECStringT() noexcept : m_pData(Nil()) {}
    ECStringT(const ECStringT& strSrc) noexcept : m_pData(strSrc.m_pData) { AddRef(m_pData); }
    ECStringT(ECStringT&& strSrc) noexcept : m_pData(strSrc.m_pData) { strSrc.m_pData = Nil(); }
    ECStringT(PCXSTR pszSrc) : m_pData(Nil())
    {
        if (pszSrc) Assign(pszSrc, std::char_traits<XCHAR>::length(pszSrc));
    }
    ECStringT(PCXSTR pch, int nLength) : m_pData(Nil())
    {
        if (pch && nLength > 0) Assign(pch, static_cast<size_t>(nLength));
    }
    explicit ECStringT(XCHAR ch, int nRepeat = 1) : m_pData(Nil())
    {
        if (nRepeat > 0)
        {
            PrepareWrite(nRepeat);
            std::char_traits<XCHAR>::assign(Chars(), static_cast<size_t>(nRepeat), ch);
            SetLength(nRepeat);
        }
    }
    explicit ECStringT(PCYSTR pszSrc) : m_pData(Nil())
    {
        if (pszSrc) AssignStr(Convert(pszSrc, std::char_traits<YCHAR>::length(pszSrc)));
    }
    explicit ECStringT(PCYSTR pch, int nLength) : m_pData(Nil())
    {
        if (pch && nLength > 0) AssignStr(Convert(pch, static_cast<size_t>(nLength)));
    }
    explicit ECStringT(const ECStringT<YCHAR>& strSrc) : m_pData(Nil())
    {
        AssignStr(Convert(strSrc.GetString(), static_cast<size_t>(strSrc.GetLength())));
    }

    ~ECStringT() { Release(m_pData); }

    ECStringT& operator=(const ECStringT& strSrc) noexcept
    {
        if (m_pData != strSrc.m_pData)
        {
            StringData* pNew = strSrc.m_pData;
            AddRef(pNew);
            Release(m_pData);
            m_pData = pNew;
        }
        return *this;
    }
    ECStringT& operator=(ECStringT&& strSrc) noexcept
    {
        if (this != &strSrc)
        {
            Release(m_pData);
            m_pData = strSrc.m_pData;
            strSrc.m_pData = Nil();
        }
        return *this;
    }
    ECStringT& operator=(PCXSTR pszSrc)
    {
        if (pszSrc) Assign(pszSrc, std::char_traits<XCHAR>::length(pszSrc));
        else Empty();
        return *this;
    }
    ECStringT& operator=(XCHAR ch) { Assign(&ch, 1); return *this; }
    ECStringT& operator=(PCYSTR pszSrc)
    {
        if (pszSrc) AssignStr(Convert(pszSrc, std::char_traits<YCHAR>::length(pszSrc)));
        else Empty();
        return *this;
    }

    int GetLength() const noexcept { return m_pData->nDataLength; }
    bool IsEmpty() const noexcept { return m_pData->nDataLength == 0; }
    void Empty() noexcept { Release(m_pData); m_pData = Nil(); }
    PXSTR GetBuffer(int nMinBufferLength) { PrepareWrite(nMinBufferLength); return Chars(); }
    PXSTR GetBuffer() { PrepareWrite(0); return Chars(); }
    void ReleaseBuffer(int nNewLength = -1)
    {
        if (nNewLength < 0)
            nNewLength = static_cast<int>(std::char_traits<XCHAR>::length(GetString()));
        PrepareWrite(nNewLength);
        SetLength(nNewLength);
    }
    void ReleaseBufferSetLength(int nNewLength)
    {
        if (nNewLength < 0) nNewLength = 0;
        PrepareWrite(nNewLength);
        SetLength(nNewLength);
    }
    XCHAR GetAt(int iChar) const
    {
        if (iChar < 0 || iChar >= GetLength()) throw std::out_of_range("ECStringT::GetAt");
        return GetString()[iChar];
    }
    void SetAt(int iChar, XCHAR ch)
    {
        if (iChar < 0 || iChar >= GetLength()) throw std::out_of_range("ECStringT::SetAt");
        PrepareWrite(0);
        Chars()[iChar] = ch;
    }
    PCXSTR GetString() const noexcept { return m_pData->buf.c_str(); }

    void Format(PCXSTR pszFormat, ...) { va_list a; va_start(a, pszFormat); AssignStr(VFormat(pszFormat, a)); va_end(a); }
    void AppendFormat(PCXSTR pszFormat, ...) { va_list a; va_start(a, pszFormat); AppendStr(VFormat(pszFormat, a)); va_end(a); }
    void FormatV(PCXSTR pszFormat, va_list args) { AssignStr(VFormat(pszFormat, args)); }
    void AppendFormatV(PCXSTR pszFormat, va_list args) { AppendStr(VFormat(pszFormat, args)); }
    void Append(PCXSTR pszSrc) { if (pszSrc) AppendChars(pszSrc, std::char_traits<XCHAR>::length(pszSrc)); }
    void Append(PCXSTR pszSrc, int nLength) { if (pszSrc && nLength > 0) AppendChars(pszSrc, static_cast<size_t>(nLength)); }
    void AppendChar(XCHAR ch) { AppendChars(&ch, 1); }
    void SetString(PCXSTR pszSrc)
    {
        if (pszSrc) Assign(pszSrc, std::char_traits<XCHAR>::length(pszSrc));
        else Empty();
    }
    void SetString(PCXSTR pszSrc, int nLength)
    {
        if (pszSrc && nLength > 0) Assign(pszSrc, static_cast<size_t>(nLength));
        else Empty();
    }

#ifdef _WIN32
    BOOL LoadString(UINT nID)
    {
        return LoadString(::GetModuleHandleW(nullptr), nID);
    }

    BOOL LoadString(HINSTANCE hInstance, UINT nID)
    {
        return LoadStringForLangId(hInstance, nID,   false, 0);
    }

    BOOL LoadString(HINSTANCE hInstance, UINT nID, WORD wLanguageID)
    {
        return LoadStringForLangId(hInstance, nID,   true, wLanguageID);
    }

    BSTR AllocSysString() const
    {
        BSTR bstr = nullptr;
        if constexpr (!std::is_same_v<XCHAR, char>)
        {
            bstr = ::SysAllocStringLen(reinterpret_cast<const OLECHAR*>(GetString()),
                                       static_cast<UINT>(GetLength()));
        }
        else
        {
            const int nSrc = GetLength();
            const int nWide = nSrc == 0
                                  ? 0
                                  : ::MultiByteToWideChar(CP_ACP, 0, GetString(), nSrc, nullptr, 0);
            bstr = ::SysAllocStringLen(nullptr, static_cast<UINT>(nWide < 0 ? 0 : nWide));
            if (bstr != nullptr && nWide > 0)
                ::MultiByteToWideChar(CP_ACP, 0, GetString(), nSrc, bstr, nWide);
        }
        if (bstr == nullptr)
            EAfxThrowMemoryException();
        return bstr;
    }
#endif

    int Compare(PCXSTR psz) const { return View().compare(psz); }
    int CompareNoCase(PCXSTR psz) const
    {
        std::basic_string<XCHAR> a(View());
        std::basic_string<XCHAR> b;
        if (psz) b = psz;
        auto lower = [](XCHAR c) { return mfc_detail::StrTraits<XCHAR>::Lower(c); };
        std::transform(a.begin(), a.end(), a.begin(), lower);
        std::transform(b.begin(), b.end(), b.begin(), lower);
        return a.compare(b);
    }
    int Collate(PCXSTR psz) const { return Compare(psz); }
    int CollateNoCase(PCXSTR psz) const { return CompareNoCase(psz); }
    int Delete(int iIndex, int nCount = 1)
    {
        if (iIndex < 0 || iIndex >= GetLength()) return GetLength();
        std::basic_string<XCHAR> tmp(View());
        tmp.erase(static_cast<size_t>(iIndex), static_cast<size_t>(nCount));
        AssignStr(tmp);
        return GetLength();
    }
    int Find(PCXSTR pszSub, int iStart = 0) const
    {
        auto pos = View().find(pszSub, static_cast<size_t>(iStart));
        return pos == npos ? -1 : static_cast<int>(pos);
    }
    int Find(XCHAR ch, int iStart = 0) const
    {
        auto pos = View().find(ch, static_cast<size_t>(iStart));
        return pos == npos ? -1 : static_cast<int>(pos);
    }
    int FindOneOf(PCXSTR pszCharSet) const
    {
        auto pos = View().find_first_of(pszCharSet);
        return pos == npos ? -1 : static_cast<int>(pos);
    }
    int ReverseFind(XCHAR ch) const
    {
        auto pos = View().rfind(ch);
        return pos == npos ? -1 : static_cast<int>(pos);
    }
    int Insert(int iIndex, PCXSTR psz)
    {
        std::basic_string<XCHAR> tmp(View());
        size_t i = std::min<size_t>(static_cast<size_t>(iIndex), tmp.size());
        tmp.insert(i, psz);
        AssignStr(tmp);
        return GetLength();
    }
    int Insert(int iIndex, XCHAR ch)
    {
        std::basic_string<XCHAR> tmp(View());
        size_t i = std::min<size_t>(static_cast<size_t>(iIndex), tmp.size());
        tmp.insert(i, 1, ch);
        AssignStr(tmp);
        return GetLength();
    }
    int Remove(XCHAR chRemove)
    {
        std::basic_string<XCHAR> tmp(View());
        const size_t before = tmp.size();
        tmp.erase(std::remove(tmp.begin(), tmp.end(), chRemove), tmp.end());
        const int removed = static_cast<int>(before - tmp.size());
        if (removed > 0) AssignStr(tmp);
        return removed;
    }
    void Truncate(int nNewLength)
    {
        if (nNewLength >= 0 && nNewLength < GetLength())
        {
            PrepareWrite(0);
            SetLength(nNewLength);
        }
    }
    ECStringT Left(int nCount) const
    {
        nCount = std::clamp(nCount, 0, GetLength());
        return ECStringT(GetString(), nCount);
    }
    ECStringT Right(int nCount) const
    {
        nCount = std::clamp(nCount, 0, GetLength());
        return ECStringT(GetString() + (GetLength() - nCount), nCount);
    }
    ECStringT Mid(int iFirst, int nCount) const
    {
        iFirst = std::clamp(iFirst, 0, GetLength());
        nCount = std::clamp(nCount, 0, GetLength() - iFirst);
        return ECStringT(GetString() + iFirst, nCount);
    }
    ECStringT Mid(int iFirst) const { return Mid(iFirst, GetLength() - iFirst); }
    ECStringT& MakeLower()
    {
        PrepareWrite(0);
        PXSTR p = Chars();
        for (int i = 0, n = GetLength(); i < n; ++i)
            p[i] = mfc_detail::StrTraits<XCHAR>::Lower(p[i]);
        return *this;
    }
    ECStringT& MakeUpper()
    {
        PrepareWrite(0);
        PXSTR p = Chars();
        for (int i = 0, n = GetLength(); i < n; ++i)
            p[i] = mfc_detail::StrTraits<XCHAR>::Upper(p[i]);
        return *this;
    }
    int Replace(PCXSTR pszOld, PCXSTR pszNew)
    {
        std::basic_string<XCHAR> oldS = pszOld, newS = pszNew;
        if (oldS.empty()) return 0;
        std::basic_string<XCHAR> tmp(View());
        int count = 0;
        size_t pos = 0;
        while ((pos = tmp.find(oldS, pos)) != npos)
        {
            tmp.replace(pos, oldS.size(), newS);
            pos += newS.size();
            ++count;
        }
        if (count > 0) AssignStr(tmp);
        return count;
    }
    int Replace(XCHAR chOld, XCHAR chNew)
    {
        int count = 0;
        const int n = GetLength();
        for (int i = 0; i < n; ++i) if (GetString()[i] == chOld) ++count;
        if (count == 0) return 0;
        PrepareWrite(0);
        PXSTR p = Chars();
        for (int i = 0; i < n; ++i) if (p[i] == chOld) p[i] = chNew;
        return count;
    }
    ECStringT SpanExcluding(PCXSTR pszCharSet) const
    {
        auto pos = View().find_first_of(pszCharSet);
        return pos == npos ? *this : Left(static_cast<int>(pos));
    }
    ECStringT Tokenize(PCXSTR pszTokens, int& iStart) const
    {
        if (iStart < 0 || iStart >= GetLength()) { iStart = -1; return ECStringT(); }
        const auto v = View();
        const size_t begin = v.find_first_not_of(pszTokens, static_cast<size_t>(iStart));
        if (begin == npos) { iStart = -1; return ECStringT(); }
        const size_t end = v.find_first_of(pszTokens, begin);
        const size_t n = (end == npos) ? v.size() - begin : end - begin;
        ECStringT tok(GetString() + begin, static_cast<int>(n));
        iStart = (end == npos) ? -1 : static_cast<int>(end + 1);
        return tok;
    }
    ECStringT& Trim() { return Trim(mfc_detail::StrTraits<XCHAR>::WS()); }
    ECStringT& Trim(XCHAR chTarget) { XCHAR set[2] = {chTarget, 0}; return Trim(set); }
    ECStringT& Trim(PCXSTR pszTargets)
    {
        const size_t b = View().find_first_not_of(pszTargets);
        if (b == npos) { Empty(); return *this; }
        if (b > 0)
        {
            std::basic_string<XCHAR> tmp(View().substr(b));
            AssignStr(tmp);
        }
        return TrimRight(pszTargets);
    }
    ECStringT& TrimRight() { return TrimRight(mfc_detail::StrTraits<XCHAR>::WS()); }
    ECStringT& TrimRight(XCHAR chTarget) { XCHAR set[2] = {chTarget, 0}; return TrimRight(set); }
    ECStringT& TrimRight(PCXSTR pszTargets)
    {
        const size_t e = View().find_last_not_of(pszTargets);
        if (e == npos) { Empty(); return *this; }
        PrepareWrite(0);
        SetLength(static_cast<int>(e + 1));
        return *this;
    }

    operator PCXSTR() const noexcept { return GetString(); }
    XCHAR operator[](int i) const { return GetString()[i]; }

    ECStringT& operator+=(const ECStringT& s) { AppendChars(s.GetString(), static_cast<size_t>(s.GetLength())); return *this; }
    ECStringT& operator+=(PCXSTR psz) { if (psz) AppendChars(psz, std::char_traits<XCHAR>::length(psz)); return *this; }
    ECStringT& operator+=(XCHAR ch) { AppendChars(&ch, 1); return *this; }
    ECStringT& operator+=(PCYSTR psz) { if (psz) AppendStr(Convert(psz, std::char_traits<YCHAR>::length(psz))); return *this; }
    ECStringT& operator+=(const ECStringT<YCHAR>& s) { AppendStr(Convert(s.GetString(), static_cast<size_t>(s.GetLength()))); return *this; }

    friend ECStringT operator+(const ECStringT& a, const ECStringT& b) { ECStringT r(a); r += b; return r; }
    friend ECStringT operator+(const ECStringT& a, PCXSTR b) { ECStringT r(a); if (b) r += b; return r; }
    friend ECStringT operator+(PCXSTR a, const ECStringT& b) { ECStringT r; if (a) r = a; r += b; return r; }
    friend ECStringT operator+(const ECStringT& a, XCHAR b) { ECStringT r(a); r += b; return r; }
    friend ECStringT operator+(XCHAR a, const ECStringT& b) { ECStringT r; r += a; r += b; return r; }

    friend bool operator==(const ECStringT& a, const ECStringT& b) noexcept { return a.View() == b.View(); }
    friend bool operator==(const ECStringT& a, PCXSTR b) noexcept { return a.View() == ViewOf(b); }
    friend bool operator==(PCXSTR a, const ECStringT& b) noexcept { return ViewOf(a) == b.View(); }
    friend bool operator!=(const ECStringT& a, const ECStringT& b) noexcept { return !(a == b); }
    friend bool operator!=(const ECStringT& a, PCXSTR b) noexcept { return !(a == b); }
    friend bool operator!=(PCXSTR a, const ECStringT& b) noexcept { return !(a == b); }
    friend bool operator==(const ECStringT& a, PCYSTR b) noexcept
    {
        return b != nullptr && a.View() == std::basic_string_view<XCHAR>(Convert(b, std::char_traits<YCHAR>::length(b)));
    }
    friend bool operator==(PCYSTR a, const ECStringT& b) noexcept { return b == a; }
    friend bool operator!=(const ECStringT& a, PCYSTR b) noexcept { return !(a == b); }
    friend bool operator!=(PCYSTR a, const ECStringT& b) noexcept { return !(b == a); }
    friend bool operator<(const ECStringT& a, const ECStringT& b) noexcept { return a.View() < b.View(); }
    friend bool operator<(const ECStringT& a, PCXSTR b) noexcept { return a.View() < ViewOf(b); }
    friend bool operator<(PCXSTR a, const ECStringT& b) noexcept { return ViewOf(a) < b.View(); }
    friend bool operator>(const ECStringT& a, const ECStringT& b) noexcept { return a.View() > b.View(); }
    friend bool operator>(const ECStringT& a, PCXSTR b) noexcept { return a.View() > ViewOf(b); }
    friend bool operator>(PCXSTR a, const ECStringT& b) noexcept { return ViewOf(a) > b.View(); }
    friend bool operator<=(const ECStringT& a, const ECStringT& b) noexcept { return a.View() <= b.View(); }
    friend bool operator>=(const ECStringT& a, const ECStringT& b) noexcept { return a.View() >= b.View(); }

private:
    static constexpr auto npos = std::basic_string<XCHAR>::npos;

    struct StringData
    {
        std::atomic<long> nRefs;
        int nDataLength;
        std::basic_string<XCHAR> buf;
    };

    static StringData* Nil() noexcept
    {
        static StringData nil{1, 0, std::basic_string<XCHAR>(1, XCHAR())};
        nil.nRefs.store(-1, std::memory_order_relaxed);
        return &nil;
    }
    static void AddRef(StringData* p) noexcept
    {
        if (p->nRefs.load(std::memory_order_relaxed) >= 0)
            p->nRefs.fetch_add(1, std::memory_order_relaxed);
    }
    static void Release(StringData* p) noexcept
    {
        if (p->nRefs.load(std::memory_order_relaxed) < 0) return;
        if (p->nRefs.fetch_sub(1, std::memory_order_acq_rel) == 1) delete p;
    }
    static StringData* NewData(int nAlloc)
    {
        if (nAlloc < 0) nAlloc = 0;
        return new StringData{1, 0, std::basic_string<XCHAR>(static_cast<size_t>(nAlloc) + 1, XCHAR())};
    }
    static std::basic_string_view<XCHAR> ViewOf(PCXSTR p) noexcept
    {
        return p ? std::basic_string_view<XCHAR>(p) : std::basic_string_view<XCHAR>();
    }

    bool IsShared() const noexcept { return m_pData->nRefs.load(std::memory_order_relaxed) > 1; }
    int AllocLength() const noexcept { return static_cast<int>(m_pData->buf.size()) - 1; }
    PXSTR Chars() noexcept { return &m_pData->buf[0]; }
    std::basic_string_view<XCHAR> View() const noexcept
    {
        return std::basic_string_view<XCHAR>(m_pData->buf.data(), static_cast<size_t>(m_pData->nDataLength));
    }
    bool Owns(PCXSTR p) const noexcept
    {
        const PCXSTR base = m_pData->buf.data();
        return p >= base && p <= base + m_pData->buf.size();
    }

    void Fork(int nAlloc)
    {
        StringData* pOld = m_pData;
        const int n = pOld->nDataLength;
        if (nAlloc < n) nAlloc = n;
        StringData* pNew = NewData(nAlloc);
        if (n > 0)
            std::char_traits<XCHAR>::copy(&pNew->buf[0], pOld->buf.data(), static_cast<size_t>(n));
        pNew->nDataLength = n;
        pNew->buf[static_cast<size_t>(n)] = XCHAR();
        m_pData = pNew;
        Release(pOld);
    }
    void PrepareWrite(int nLength)
    {
        if (nLength < m_pData->nDataLength) nLength = m_pData->nDataLength;
        if (m_pData == Nil() || IsShared())
        {
            if (m_pData == Nil() && nLength == 0) return;
            Fork(nLength);
        }
        else if (AllocLength() < nLength)
        {
            m_pData->buf.resize(static_cast<size_t>(nLength) + 1, XCHAR());
        }
    }
    void SetLength(int n)
    {
        if (m_pData == Nil()) return;
        if (n < 0) n = 0;
        if (n > AllocLength()) n = AllocLength();
        m_pData->nDataLength = n;
        m_pData->buf[static_cast<size_t>(n)] = XCHAR();
    }
    void Assign(PCXSTR p, size_t n)
    {
        if (n == 0) { Empty(); return; }
        if (Owns(p))
        {
            const std::basic_string<XCHAR> tmp(p, n);
            Assign(tmp.data(), tmp.size());
            return;
        }
        PrepareWrite(static_cast<int>(n));
        std::char_traits<XCHAR>::copy(Chars(), p, n);
        SetLength(static_cast<int>(n));
    }
    void AssignStr(const std::basic_string<XCHAR>& s) { Assign(s.data(), s.size()); }
    void AppendChars(PCXSTR p, size_t n)
    {
        if (n == 0) return;
        if (Owns(p))
        {
            const std::basic_string<XCHAR> tmp(p, n);
            AppendChars(tmp.data(), tmp.size());
            return;
        }
        const int oldLength = m_pData->nDataLength;
        const int newLength = oldLength + static_cast<int>(n);
        PrepareWrite(newLength);
        std::char_traits<XCHAR>::copy(Chars() + oldLength, p, n);
        SetLength(newLength);
    }
    void AppendStr(const std::basic_string<XCHAR>& s) { AppendChars(s.data(), s.size()); }

#ifdef _WIN32
    BOOL LoadStringForLangId(HINSTANCE hInstance, UINT nID, bool useLangId, WORD wLanguageID)
    {
        if (!useLangId)
        {
            const wchar_t* pStr = nullptr;
            const int n = ::LoadStringW(hInstance, nID, reinterpret_cast<LPWSTR>(&pStr), 0);
            if (n <= 0)
                return FALSE;
            AssignStr(Convert(pStr, static_cast<size_t>(n)));
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
            p += 1 + static_cast<size_t>(*p);
        }
        if (p >= end || *p == 0) { return FALSE; }
        const size_t len = static_cast<size_t>(*p);
        if (p + 1 + len > end) { return FALSE; }
        AssignStr(Convert(p + 1, len));
        return TRUE;
    }
#endif

    template <class Src>
    static std::basic_string<XCHAR> Convert(const Src* p, size_t n)
    {
        if (!p) return {};
        if constexpr (std::is_same_v<Src, XCHAR>)
            return std::basic_string<XCHAR>(p, n);
        else if constexpr (std::is_same_v<Src, char>)
            return mfc_detail::Widen<XCHAR>(p, n);
        else if constexpr (std::is_same_v<XCHAR, char>)
            return mfc_detail::Narrow(p, n);
        else
            return mfc_detail::WideToWide<XCHAR>(p, n);
    }

    static std::basic_string<XCHAR> VFormat(PCXSTR fmt, va_list args)
    {
#ifdef _MSC_VER
        constexpr bool translate =
            !std::is_same_v<XCHAR, char> && !std::is_same_v<XCHAR, wchar_t>;
#else
        constexpr bool translate = true;
#endif
        std::basic_string<XCHAR> fmtHeld;
        PCXSTR fmtUsed = fmt;
        if constexpr (translate)
        {
            fmtHeld = mfc_detail::TranslateFormat(fmt);
            fmtUsed = fmtHeld.c_str();
        }
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
    StringData* m_pData;
};

using ECStringA = ECStringT<char>;
using ECStringW = ECStringT<EWCHAR>;
using ECString = ECStringW;

namespace std
{
template <class Ch, class Tr>
struct hash<ECStringT<Ch, Tr>>
{
    size_t operator()(const ECStringT<Ch, Tr>& s) const noexcept
    {
        return std::hash<std::basic_string_view<Ch>>{}(
            std::basic_string_view<Ch>(s.GetString(), static_cast<size_t>(s.GetLength())));
    }
};
}

class ECException : public ECObject
{
public:
    virtual BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) const;

    EDECLARE_DYNAMIC(ECException)
public:
    ECException() : m_bAutoDelete(TRUE) {}
    explicit ECException(BOOL bAutoDelete) : m_bAutoDelete(bAutoDelete) {}
    void Delete() { if (m_bAutoDelete) delete this; }

private:
    BOOL m_bAutoDelete;
};

class ECSimpleException : public ECException
{
    EDECLARE_DYNAMIC(ECSimpleException)
public:
    ECSimpleException() : ECException(FALSE) {}
    explicit ECSimpleException(BOOL bAutoDelete) : ECException(bAutoDelete) {}
    virtual BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) const override = 0;
};

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
          m_strFileName(lpszFileName ? lpszFileName : _T("")) {}

    BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) const override;

    [[noreturn]] static void ThrowOsError(LONG lOsError, LPCTSTR lpszFileName = nullptr);

    int m_cause;
    LONG m_lOsError;
    ECString m_strFileName;
};

[[noreturn]] void EAfxThrowFileException(int cause, LONG lOsError = -1, LPCTSTR lpszFileName = nullptr);
[[noreturn]] void EAfxThrowMemoryException();

class ECTime;
struct ECFileStatus;
#ifndef _WIN32
struct FILETIME;
#endif

class ECFile : public ECObject
{
    EDECLARE_DYNAMIC(ECFile)
public:
    HANDLE m_hFile = nullptr;
    operator HANDLE() const { return m_hFile; }

    enum OpenFlags
    {
        modeRead = 0x0000, modeWrite = 0x0001, modeReadWrite = 0x0002,
        shareCompat = 0x0000, shareExclusive = 0x0010, shareDenyWrite = 0x0020,
        shareDenyRead = 0x0030, shareDenyNone = 0x0040,
        modeNoInherit = 0x0080,
        modeCreate = 0x1000, modeNoTruncate = 0x2000,
        typeText = 0x4000, typeBinary = 0x8000,
        osNoBuffer = 0x10000, osWriteThrough = 0x20000,
        osRandomAccess = 0x40000, osSequentialScan = 0x80000
    };
    enum SeekPosition { begin = 0, current = 1, end = 2 };

    ECFile() = default;
    ECFile(LPCTSTR lpszFileName, UINT nOpenFlags);
    virtual ~ECFile() { if (m_stream.is_open()) m_stream.close(); mfc_detail::ReleaseShare(this); }

    virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags, ECFileException* pError = nullptr);
    virtual void Abort() { if (m_stream.is_open()) m_stream.close(); mfc_detail::ReleaseShare(this); }
    virtual void Close() { if (m_stream.is_open()) m_stream.close(); mfc_detail::ReleaseShare(this); }
    virtual UINT Read(void* lpBuf, UINT nCount);
    virtual void Write(const void* lpBuf, UINT nCount);
    virtual ULONGLONG Seek(LONGLONG lOff, UINT nFrom);
    void SeekToBegin() { Seek(0, begin); }
    ULONGLONG SeekToEnd() { return Seek(0, end); }
    virtual ULONGLONG GetLength() const;
    virtual void SetLength(ULONGLONG dwNewLen);
    virtual ULONGLONG GetPosition() const;
    virtual void Flush() { m_stream.flush(); }
    virtual ECString GetFileName() const { return ECString(std::filesystem::path(m_path).filename().string<TCHAR>().c_str()); }
    virtual ECString GetFilePath() const { return ECString(m_path.c_str()); }
    BOOL GetStatus(ECFileStatus& rStatus) const;
    static BOOL GetStatus(LPCTSTR lpszFileName, ECFileStatus& rStatus);
    static void Remove(LPCTSTR lpszFileName);
    static void Rename(LPCTSTR lpszOldName, LPCTSTR lpszNewName);

protected:
    ECString m_strFileName;
    std::fstream m_stream;
    std::basic_string<TCHAR> m_path;
    UINT m_nOpenFlags = 0;
};

class ECStdioFile : public ECFile
{
    EDECLARE_DYNAMIC(ECStdioFile)
public:
    ECStdioFile() = default;
    ECStdioFile(LPCTSTR lpszFileName, UINT nOpenFlags) : ECFile(lpszFileName, nOpenFlags) {}
    explicit ECStdioFile(FILE* pOpenStream) : m_pStream(pOpenStream) {}

    FILE* m_pStream = nullptr;

    virtual LPTSTR ReadString(LPTSTR lpsz, UINT nMax);
    virtual BOOL ReadString(ECString& rString);
    virtual void WriteString(LPCTSTR lpsz);

private:
    bool IsTextMode() const;
};

class ECMemFile : public ECFile
{
    EDECLARE_DYNAMIC(ECMemFile)
public:
    ECMemFile() = default;
    explicit ECMemFile(UINT  ) {}
    ECMemFile(BYTE* lpBuffer, UINT nBufferSize, UINT   = 0)
        : m_buffer(lpBuffer, lpBuffer + nBufferSize) {}
    UINT Read(void* lpBuf, UINT nCount) override;
    void Write(const void* lpBuf, UINT nCount) override;
    ULONGLONG Seek(LONGLONG lOff, UINT nFrom) override;
    ULONGLONG GetLength() const override { return m_buffer.size(); }
    void SetLength(ULONGLONG dwNewLen) override { m_buffer.resize(static_cast<size_t>(dwNewLen)); }
    ULONGLONG GetPosition() const override { return m_pos; }
    BYTE* Detach();
    void Attach(BYTE* lpBuffer, UINT nBufferSize, UINT nGrowBytes = 0);
    virtual void GrowFile(ULONGLONG dwNewLen);

protected:
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

class ECFileFind : public ECObject
{
    EDECLARE_DYNAMIC(ECFileFind)
public:
    virtual BOOL FindFile(LPCTSTR pstrName = nullptr, DWORD dwUnused = 0);
    virtual BOOL FindNextFile();
    void Close() { m_it = std::filesystem::directory_iterator(); m_pending.reset(); }
    BOOL IsDirectory() const;
    virtual BOOL IsDots() const;
    BOOL IsSystem() const;
    BOOL IsHidden() const;
    BOOL IsReadOnly() const;
    virtual ECString GetFileName() const;
    virtual ECString GetFilePath() const;
    ULONGLONG GetLength() const;
    virtual ECString GetRoot() const;
    virtual BOOL GetLastWriteTime(ECTime& refTime) const;
    virtual BOOL GetCreationTime(ECTime& refTime) const;
    virtual BOOL GetLastAccessTime(ECTime& refTime) const;
    virtual BOOL GetLastWriteTime(FILETIME* pTimeStamp) const;
    virtual BOOL GetCreationTime(FILETIME* pTimeStamp) const;
    virtual BOOL GetLastAccessTime(FILETIME* pTimeStamp) const;
    BOOL IsTemporary() const;
    BOOL IsArchived() const;
    BOOL IsCompressed() const;

private:
    bool AdvanceToNextMatch();

    std::filesystem::path m_dir;
    std::basic_string<TCHAR> m_root;
    std::basic_string<TCHAR> m_pattern;
    std::filesystem::directory_iterator m_it;
    std::optional<std::filesystem::directory_entry> m_pending;
    std::filesystem::directory_entry m_current;
};

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
    ECArchive& operator<<(DWORD dw);
#endif
    ECArchive& operator<<(float f);
    ECArchive& operator<<(double d);
    ECArchive& operator<<(ULONGLONG dwdw);
    ECArchive& operator<<(const ECString& str);

private:
    ECFile* m_pFile;
    UINT m_nMode;
};

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

#include "eatltime.h"

struct ECFileStatus
{
    ECTime m_ctime;
    ECTime m_mtime;
    ECTime m_atime;
    ULONGLONG m_size = 0;
    BYTE m_attribute = 0;
    BYTE m_padding = 0;
    TCHAR m_szFullName[_MAX_PATH] = {};
};
