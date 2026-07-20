// afxdtctl.h — reference STUB (declarations only, no implementation).
// MFC support for the date/time-picker and month-calendar common
// controls. Discovered missing (2026-07-20): the original class census
// never included CDateTimeCtrl at all — it wasn't on the initial
// candidate list, not filtered out by a check. Confirmed genuine real
// usage in eMule/srchybrid: CTreeOptionsDateCtrl : public CDateTimeCtrl
// (TreeOptionsCtrl.h/.cpp) and plain member instances (PPgScheduler.h).
// CMonthCalCtrl (the header's other class) has zero real usage beyond a
// Stdafx.h comment — not declared here.
// As in real MFC, it includes afxwin.h.
#pragma once
#include "afxwin.h"
#include "atltime.h" // CTime, used by SetTime/GetTime below

struct SYSTEMTIME;
using LPSYSTEMTIME = SYSTEMTIME*;

// Real Win32 return-value constants for CDateTimeCtrl::GetTime (commctrl.h).
constexpr DWORD GDT_ERROR = static_cast<DWORD>(-1);
constexpr DWORD GDT_VALID = 0;
constexpr DWORD GDT_NONE = 1;

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4266)
#endif

// ---------------------------------------------------------------------
// CDateTimeCtrl (header afxdtctl.h, hierarchy
// CObject -> CCmdTarget -> CWnd -> CDateTimeCtrl)
// ---------------------------------------------------------------------
class CDateTimeCtrl : public CWnd
{
public:
    CDateTimeCtrl();
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    BOOL SetFormat(LPCTSTR pstrFormat);
    BOOL SetTime(const CTime* pTimeNew);
    DWORD GetTime(CTime& timeDest) const;
    DWORD GetTime(LPSYSTEMTIME pTimeDest) const;
};

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif
