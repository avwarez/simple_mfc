// atltime.h — NATIVE implementation (standard C++17 library only).
// CTime/CTimeSpan on top of <chrono>/<ctime>, no Windows dependency
// (no SYSTEMTIME/FILETIME).
#pragma once

// This header does NOT include afx.h: CTime/CTimeSpan reference CString
// only as a by-value return type in declarations (the bodies are in
// atltime.cpp), so a forward declaration is enough -- and NOT pulling in
// afx.h is what lets afx.h own CFileStatus (which needs a complete CTime)
// while still #including this header at its bottom, with no cycle. This
// also mirrors real ATL, where atltime.h does not depend on MFC's afx.h.
// The default for CStringT's second parameter stays solely on the
// definition in afx.h, so here the alias spells the second argument out as
// void explicitly (CStringT<wchar_t> == CStringT<wchar_t, void>, so this
// names the very same type and is a legal identical redefinition of the
// alias afx.h also declares).
template <class BaseType, class> class ECStringT;
using ECStringW = ECStringT<wchar_t, void>;
using ECString = ECStringW;

#include <chrono>
#include <ctime>

// __time64_t is an MSVC/Windows CRT-specific typedef; here we redefine it
// ourselves as a standard alias (no dependency on the Windows runtime).
using __time64_t = long long;

class ECTimeSpan
{
public:
    ECTimeSpan() noexcept : m_span(0) {}
    ECTimeSpan(long long timeSpan) noexcept : m_span(timeSpan) {}
    ECTimeSpan(long lDays, int nHours, int nMins, int nSecs) noexcept
        : m_span(static_cast<long long>(lDays) * 86400 + nHours * 3600 + nMins * 60 + nSecs) {}

    long GetDays() const noexcept { return static_cast<long>(m_span / 86400); }
    long GetHours() const noexcept { return static_cast<long>((m_span / 3600) % 24); }
    long GetMinutes() const noexcept { return static_cast<long>((m_span / 60) % 60); }
    long GetSeconds() const noexcept { return static_cast<long>(m_span % 60); }
    long long GetTotalSeconds() const noexcept { return m_span; }
    long long GetTotalHours() const noexcept { return m_span / 3600; }
    long long GetTotalMinutes() const noexcept { return m_span / 60; }

    // Real MFC's CTimeSpan format specifiers: %D total days, %H hours,
    // %M minutes, %S seconds (each zero-padded to two digits except %D),
    // %% a literal percent. Deliberately not strftime: a span is a
    // duration, so %H can exceed 23 only via %D carrying the days.
    ECString Format(const wchar_t* pszFormat) const;
    // The narrow-literal form. Not on the Learn page (which lists only
    // Format(LPCTSTR)/Format(UINT)), but real: eMule passes a plain
    // "..." literal while building UNICODE, and the result is then used
    // where a CString is expected -- so the overload has to accept char
    // and still return a CString. Same reasoning as CTime::Format below.
    ECString Format(const char* pszFormat) const;

    ECTimeSpan operator+(const ECTimeSpan& o) const { return ECTimeSpan(m_span + o.m_span); }
    ECTimeSpan operator-(const ECTimeSpan& o) const { return ECTimeSpan(m_span - o.m_span); }
    // Real MFC's full relational set. eMule reaches it generically:
    // OtherFunctions.h's sgn<T> is instantiated with T=CTimeSpan (the
    // difference of two CTimes) and evaluates "T(0) < val", so the
    // comparison has to exist for a span, not just for a point in time.
    // By value for the same reason as CTime's below: it lets the literal
    // 0 convert through the non-explicit long long constructor.
    bool operator<(ECTimeSpan o) const noexcept { return m_span < o.m_span; }
    bool operator>(ECTimeSpan o) const noexcept { return m_span > o.m_span; }
    bool operator<=(ECTimeSpan o) const noexcept { return m_span <= o.m_span; }
    bool operator>=(ECTimeSpan o) const noexcept { return m_span >= o.m_span; }
    bool operator==(ECTimeSpan o) const noexcept { return m_span == o.m_span; }
    bool operator!=(ECTimeSpan o) const noexcept { return m_span != o.m_span; }

private:
    long long m_span;
};

// GetCurrentTime is a winuser.h macro (expands to GetTickCount()), so on
// Windows the member below is declared -- and every call to it compiled --
// under that substituted name. Left alone for the same reason afx.h leaves
// FindNextFile alone: real ATL's own CTime::GetCurrentTime goes through the
// identical substitution, and undefining the macro here would silence it
// for the whole rest of the translation unit.
class ECTime
{
public:
    ECTime() noexcept : m_time(0) {}
    ECTime(__time64_t time) noexcept : m_time(time) {}
    ECTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST = -1);

    static ECTime GetCurrentTime() noexcept { return ECTime(static_cast<__time64_t>(std::time(nullptr))); }

    ECString Format(const wchar_t* pszFormat) const;
    // The narrow-literal form. Microsoft Learn lists only
    // Format(LPCTSTR) and Format(UINT nFormatID), neither of which
    // accepts a char literal under UNICODE -- yet real eMule code does
    // exactly that and casts the result to LPCTSTR:
    //   TRACE("tNow = %s\n", (LPCTSTR)CTime(tNow).Format("%X"));
    // (FileDetailDialogInfo.cpp:166-167; TRACE is __noop in NDEBUG, but
    // __noop still parses and overload-resolves its arguments, see
    // afximpl.h). It compiles against real MFC, so the page is
    // incomplete here: the overload takes narrow characters and still
    // returns a CString, which is what makes the LPCTSTR cast legal.
    ECString Format(const char* pszFormat) const;

    // Fills the caller's struct with the broken-down local time and hands
    // it back, so it can be used inline (`safe_mktime(t.GetLocalTm(&tm))`).
    // Real MFC allows a null pointer and then returns a pointer to shared
    // per-thread storage; that variant has no thread-safe standard C++
    // equivalent, so passing a buffer is the supported form here.
    std::tm* GetLocalTm(std::tm* ptm) const noexcept
    {
        if (ptm == nullptr)
            return nullptr;
        *ptm = Tm();
        return ptm;
    }

    int GetYear() const noexcept { return Tm().tm_year + 1900; }
    int GetMonth() const noexcept { return Tm().tm_mon + 1; }
    int GetDay() const noexcept { return Tm().tm_mday; }
    int GetHour() const noexcept { return Tm().tm_hour; }
    int GetMinute() const noexcept { return Tm().tm_min; }
    int GetSecond() const noexcept { return Tm().tm_sec; }
    int GetDayOfWeek() const noexcept { return Tm().tm_wday + 1; } // 1=Sunday..7=Saturday, matching the original MFC convention
    __time64_t GetTime() const noexcept { return m_time; }

    ECTimeSpan operator-(const ECTime& o) const { return ECTimeSpan(m_time - o.m_time); }
    ECTime operator+(const ECTimeSpan& s) const { return ECTime(m_time + s.GetTotalSeconds()); }
    // By value, as real MFC declares them -- which also lets a plain 0
    // convert through the (non-explicit) __time64_t constructor above,
    // the form eMule uses ("if (lastSeenComplete == 0)").
    bool operator<(ECTime o) const noexcept { return m_time < o.m_time; }
    bool operator==(ECTime o) const noexcept { return m_time == o.m_time; }
    bool operator!=(ECTime o) const noexcept { return m_time != o.m_time; }
    bool operator>(ECTime o) const noexcept { return m_time > o.m_time; }
    bool operator<=(ECTime o) const noexcept { return m_time <= o.m_time; }
    bool operator>=(ECTime o) const noexcept { return m_time >= o.m_time; }

private:
    // std::localtime (standard C++, <ctime>) uses an internal static
    // buffer and is not thread-safe — unlike localtime_r/localtime_s,
    // which are POSIX/Windows extensions, not standard C++. We copy the
    // result into a local struct right away to limit the risk window; if
    // multiple threads call CTime methods concurrently, an external lock
    // is required (a known limitation, documented in ../README.md).
    std::tm Tm() const noexcept
    {
        std::time_t t = static_cast<std::time_t>(m_time);
        std::tm* p = std::localtime(&t);
        return p ? *p : std::tm{};
    }
    __time64_t m_time;
};

// CFileStatus lives in afx.h (its real MFC home), defined at the very
// bottom of afx.h once this header has made CTime complete. _MAX_PATH,
// which its m_szFullName needs, is provided here so it is in scope by
// then.
#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif
