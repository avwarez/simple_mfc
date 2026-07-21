#include "atltime.h"

CTime::CTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int /*nDST*/)
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

CString CTime::Format(const wchar_t* pszFormat) const
{
    std::tm t = Tm();
    wchar_t buf[256];
    size_t n = std::wcsftime(buf, sizeof(buf) / sizeof(buf[0]), pszFormat, &t);
    return CString(n > 0 ? buf : L"");
}

// The narrow-literal overloads: widen the format string and reuse the
// wide implementation, so the two can never drift apart. See the header
// for why they exist at all (a UNICODE build passing a "..." literal).
CString CTime::Format(const char* pszFormat) const
{
    CStringW strFormat(pszFormat ? pszFormat : "");
    return Format(strFormat.GetString());
}

CString CTimeSpan::Format(const char* pszFormat) const
{
    CStringW strFormat(pszFormat ? pszFormat : "");
    return Format(strFormat.GetString());
}

CString CTimeSpan::Format(const wchar_t* pszFormat) const
{
    // A span is a duration, not a point in time, so wcsftime is not
    // applicable: %H must stay within 0-23 with the overflow carried by
    // %D, and a negative span formats from its magnitude.
    long long span = m_span < 0 ? -m_span : m_span;
    CString result;
    for (const wchar_t* p = pszFormat; p && *p; ++p) {
        if (*p != L'%') {
            result += *p;
            continue;
        }
        wchar_t buf[32];
        switch (*++p) {
        case L'D':
            swprintf(buf, 32, L"%lld", span / 86400);
            result += buf;
            break;
        case L'H':
            swprintf(buf, 32, L"%02lld", (span / 3600) % 24);
            result += buf;
            break;
        case L'M':
            swprintf(buf, 32, L"%02lld", (span / 60) % 60);
            result += buf;
            break;
        case L'S':
            swprintf(buf, 32, L"%02lld", span % 60);
            result += buf;
            break;
        case L'%':
            result += L'%';
            break;
        case L'\0':
            return result; // trailing '%', nothing to interpret
        default:
            // Unknown specifier: real MFC leaves it as-is rather than
            // dropping it, which keeps a malformed format visible.
            result += L'%';
            result += *p;
            break;
        }
    }
    return result;
}
