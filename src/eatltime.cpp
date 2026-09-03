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

ECString ECTime::Format(const TCHAR* pszFormat) const
{
    std::tm t = Tm();
    const std::wstring fmt = mfc_detail::WideToWide<wchar_t>(
        pszFormat, pszFormat ? std::char_traits<TCHAR>::length(pszFormat) : 0);
    wchar_t buf[256];
    size_t n = std::wcsftime(buf, sizeof(buf) / sizeof(buf[0]), fmt.c_str(), &t);
    if (n == 0) return ECString();
    const std::basic_string<TCHAR> text = mfc_detail::WideToWide<TCHAR>(buf, n);
    return ECString(text.c_str(), static_cast<int>(text.size()));
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

ECString ECTimeSpan::Format(const TCHAR* pszFormat) const
{
    const long long span = m_span;
    ECString result;
    for (const TCHAR* p = pszFormat; p && *p; ++p) {
        if (*p != _T('%')) {
            result += *p;
            continue;
        }
        char buf[32];
        switch (*++p) {
        case _T('D'):
            std::snprintf(buf, sizeof buf, "%lld", span / 86400);
            result += ECString(buf);
            break;
        case _T('H'):
            std::snprintf(buf, sizeof buf, "%02lld", (span / 3600) % 24);
            result += ECString(buf);
            break;
        case _T('M'):
            std::snprintf(buf, sizeof buf, "%02lld", (span / 60) % 60);
            result += ECString(buf);
            break;
        case _T('S'):
            std::snprintf(buf, sizeof buf, "%02lld", span % 60);
            result += ECString(buf);
            break;
        case _T('%'):
            result += _T('%');
            break;
        case _T('\0'):
            return result;
        default:
            result += _T('%');
            result += *p;
            break;
        }
    }
    return result;
}
