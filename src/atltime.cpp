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
