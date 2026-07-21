// atlcomtime.h — reference STUB (declarations only, no implementation).
// The OLE automation date/time value types, shared by ATL and MFC (real
// MFC reaches them through afxdisp.h, which includes this header).
//
// eMule/srchybrid never names COleDateTime anywhere. It is here for one
// reason: overload resolution. CDateTimeCtrl::SetTime has three forms in
// real MFC -- const CTime*, LPSYSTEMTIME, and const COleDateTime& -- and
// eMule passes a SYSTEMTIME *by value*
// ("m_pDateTime->SetTime(pItemData->m_DateTime);", TreeOptionsCtrl.cpp:
// 1202). Neither pointer form matches; what makes it compile under real
// MFC is COleDateTime's converting constructor from a SYSTEMTIME. With
// the class absent, that overload could not exist and the call failed
// with C2665.
//
// Declaration-only on purpose: nothing in this library, and nothing in
// eMule, ever calls a member of it -- it only has to be a complete type
// with the right constructor for the conversion to be found.
#pragma once
#include "atltime.h" // as real atlcomtime.h does; __time64_t comes from there

#ifdef _WIN32
#include <windows.h> // SYSTEMTIME, FILETIME, DATE
#else
struct SYSTEMTIME;
struct FILETIME;
using DATE = double;
#endif

// ---------------------------------------------------------------------
// COleDateTimeSpan — a duration in days, as an automation DATE delta.
// ---------------------------------------------------------------------
class COleDateTimeSpan
{
public:
    COleDateTimeSpan() noexcept;
    COleDateTimeSpan(double dblSpanSrc) noexcept;
    COleDateTimeSpan(LONG lDays, int nHours, int nMins, int nSecs) noexcept;

    double m_span;
};

// ---------------------------------------------------------------------
// COleDateTime — an automation DATE (days since 30 December 1899, the
// fraction being the time of day). The converting constructors are the
// point of the class here; each is non-explicit in real MFC, which is
// what lets a SYSTEMTIME reach a "const COleDateTime&" parameter.
// ---------------------------------------------------------------------
class COleDateTime
{
public:
    COleDateTime() noexcept;
    COleDateTime(const COleDateTime& dateSrc) noexcept;
    COleDateTime(DATE dtSrc) noexcept;
    COleDateTime(__time64_t timeSrc) noexcept;
    COleDateTime(const SYSTEMTIME& systimeSrc) noexcept;
    COleDateTime(const FILETIME& filetimeSrc) noexcept;
    COleDateTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec) noexcept;

    DATE m_dt;
};
