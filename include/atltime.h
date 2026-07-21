// atltime.h — NATIVE implementation (standard C++17 library only).
// CTime/CTimeSpan on top of <chrono>/<ctime>, no Windows dependency
// (no SYSTEMTIME/FILETIME).
#pragma once

#include "afx.h"

#include <chrono>
#include <ctime>

// __time64_t is an MSVC/Windows CRT-specific typedef; here we redefine it
// ourselves as a standard alias (no dependency on the Windows runtime).
using __time64_t = long long;

class CTimeSpan
{
public:
    CTimeSpan() noexcept : m_span(0) {}
    explicit CTimeSpan(long long timeSpan) noexcept : m_span(timeSpan) {}
    CTimeSpan(long lDays, int nHours, int nMins, int nSecs) noexcept
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
    CString Format(const wchar_t* pszFormat) const;

    CTimeSpan operator+(const CTimeSpan& o) const { return CTimeSpan(m_span + o.m_span); }
    CTimeSpan operator-(const CTimeSpan& o) const { return CTimeSpan(m_span - o.m_span); }

private:
    long long m_span;
};

// GetCurrentTime is also a real winuser.h macro (expands to GetTickCount())
// -- undefined here for the same reason afx.h undefines FindNextFile
// above: keeps the member's true name instead of a silent rewrite to a
// method CTime doesn't have, consistently for every later call site too.
#undef GetCurrentTime
class CTime
{
public:
    CTime() noexcept : m_time(0) {}
    explicit CTime(__time64_t time) noexcept : m_time(time) {}
    CTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST = -1);

    static CTime GetCurrentTime() noexcept { return CTime(static_cast<__time64_t>(std::time(nullptr))); }

    CString Format(const wchar_t* pszFormat) const;

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

    CTimeSpan operator-(const CTime& o) const { return CTimeSpan(m_time - o.m_time); }
    CTime operator+(const CTimeSpan& s) const { return CTime(m_time + s.GetTotalSeconds()); }
    bool operator<(const CTime& o) const noexcept { return m_time < o.m_time; }
    bool operator==(const CTime& o) const noexcept { return m_time == o.m_time; }

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
