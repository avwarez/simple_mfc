#include "eafx.h"
#include "eatltime.h"

ECTime::ECTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int  )
{
    std::tm t{};
    t.tm_year = nYear - 1900;
    t.tm_mon = nMonth - 1;
    t.tm_mday = nDay;
    t.tm_hour = nHour;
    t.tm_min = nMin;
    t.tm_sec = nSec;
    t.tm_isdst = -1;
    m_time = static_cast<__time64_t>(std::mktime(&t));
}

ECString ECTime::Format(const wchar_t* pszFormat) const
{
    std::tm t = Tm();
    wchar_t buf[256];
    size_t n = std::wcsftime(buf, sizeof(buf) / sizeof(buf[0]), pszFormat, &t);
    return ECString(n > 0 ? buf : L"");
}

ECString ECTime::Format(const char* pszFormat) const
{
    ECStringW strFormat(pszFormat ? pszFormat : "");
    return Format(strFormat.GetString());
}

ECString ECTimeSpan::Format(const char* pszFormat) const
{
    ECStringW strFormat(pszFormat ? pszFormat : "");
    return Format(strFormat.GetString());
}

ECString ECTimeSpan::Format(const wchar_t* pszFormat) const
{
    long long span = m_span < 0 ? -m_span : m_span;
    ECString result;
    for (const wchar_t* p = pszFormat; p && *p; ++p) {
        if (*p != L'%') {
            result += *p;
            continue;
        }
        wchar_t buf[32];
        switch (*++p) {
        case L'D':
            std::swprintf(buf, 32, L"%lld", span / 86400);
            result += buf;
            break;
        case L'H':
            std::swprintf(buf, 32, L"%02lld", (span / 3600) % 24);
            result += buf;
            break;
        case L'M':
            std::swprintf(buf, 32, L"%02lld", (span / 60) % 60);
            result += buf;
            break;
        case L'S':
            std::swprintf(buf, 32, L"%02lld", span % 60);
            result += buf;
            break;
        case L'%':
            result += L'%';
            break;
        case L'\0':
            return result;
        default:
            result += L'%';
            result += *p;
            break;
        }
    }
    return result;
}
