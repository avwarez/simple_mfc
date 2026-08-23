#pragma once

template <class BaseType, class> class ECStringT;
using ECStringW = ECStringT<wchar_t, void>;
using ECString = ECStringW;

#include <chrono>
#include <ctime>

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

    ECString Format(const wchar_t* pszFormat) const;
    ECString Format(const char* pszFormat) const;

    ECTimeSpan operator+(const ECTimeSpan& o) const { return ECTimeSpan(m_span + o.m_span); }
    ECTimeSpan operator-(const ECTimeSpan& o) const { return ECTimeSpan(m_span - o.m_span); }
    bool operator<(ECTimeSpan o) const noexcept { return m_span < o.m_span; }
    bool operator>(ECTimeSpan o) const noexcept { return m_span > o.m_span; }
    bool operator<=(ECTimeSpan o) const noexcept { return m_span <= o.m_span; }
    bool operator>=(ECTimeSpan o) const noexcept { return m_span >= o.m_span; }
    bool operator==(ECTimeSpan o) const noexcept { return m_span == o.m_span; }
    bool operator!=(ECTimeSpan o) const noexcept { return m_span != o.m_span; }

private:
    long long m_span;
};

class ECTime
{
public:
    ECTime() noexcept : m_time(0) {}
    ECTime(__time64_t time) noexcept : m_time(time) {}
    ECTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST = -1);

    static ECTime GetCurrentTime() noexcept { return ECTime(static_cast<__time64_t>(std::time(nullptr))); }

    ECString Format(const wchar_t* pszFormat) const;
    ECString Format(const char* pszFormat) const;

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
    int GetDayOfWeek() const noexcept { return Tm().tm_wday + 1; }
    __time64_t GetTime() const noexcept { return m_time; }

    ECTimeSpan operator-(const ECTime& o) const { return ECTimeSpan(m_time - o.m_time); }
    ECTime operator+(const ECTimeSpan& s) const { return ECTime(m_time + s.GetTotalSeconds()); }
    bool operator<(ECTime o) const noexcept { return m_time < o.m_time; }
    bool operator==(ECTime o) const noexcept { return m_time == o.m_time; }
    bool operator!=(ECTime o) const noexcept { return m_time != o.m_time; }
    bool operator>(ECTime o) const noexcept { return m_time > o.m_time; }
    bool operator<=(ECTime o) const noexcept { return m_time <= o.m_time; }
    bool operator>=(ECTime o) const noexcept { return m_time >= o.m_time; }

private:
    std::tm Tm() const noexcept
    {
        std::time_t t = static_cast<std::time_t>(m_time);
        std::tm* p = std::localtime(&t);
        return p ? *p : std::tm{};
    }
    __time64_t m_time;
};

#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif
