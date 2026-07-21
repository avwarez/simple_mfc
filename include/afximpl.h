// afximpl.h — reference STUB (declarations only, no implementation).
// In real MFC this is an INTERNAL implementation header: it ships with
// MFC's sources (VC/Tools/MSVC/<ver>/ATLMFC/src/mfc), not in the include
// directory, and applications are not supposed to include it. eMule's
// Stdafx.h includes it anyway, and eMule's own CMakeLists.txt puts that
// source directory on the include path to make it resolve
// (srchybrid/CMakeLists.txt: ATLMFC_SRC_MFC).
//
// It was an empty pass-through here until a control run against real
// MFC showed which symbols eMule genuinely takes from it: the two
// MFC-private tooltip flags, the pre-IE4 TOOLINFO layout, and the trace
// category. All three are declared below.
#pragma once
#include "afxwin.h"

// MFC's own tooltip flags, private to the framework: they extend
// commctrl.h's TTF_* set and are not part of the Windows SDK.
// CMuleStatusBarCtrl passes them when it fills a TOOLINFO by hand.
#ifndef TTF_ALWAYSTIP
#define TTF_ALWAYSTIP 0x02
#endif
#ifndef TTF_NOTBUTTON
#define TTF_NOTBUTTON 0x80
#endif

// The pre-IE4 TOOLINFO layout. MFC keeps it to tell the two apart by
// size, which is exactly what eMule does:
//   if (iHit == -1 && pTI != NULL && pTI->cbSize >= sizeof(AFX_OLDTOOLINFO))
#ifndef _WIN32
using HWND = void*;
using UINT_PTR = unsigned long long;
#endif
struct AFX_OLDTOOLINFO
{
    UINT cbSize;
    UINT uFlags;
    HWND hwnd;
    UINT_PTR uId;
    RECT rect;
    HINSTANCE hinst;
    LPTSTR lpszText;
};

// The trace categories MFC declares for the category form of TRACE
// ("TRACE(traceAppMsg, 0, ...)"). TRACE itself expands to __noop here,
// but __noop still parses its arguments, so the category has to exist.
struct CTraceCategory
{
    explicit CTraceCategory(UINT nCategory = 0) noexcept;
};
extern CTraceCategory traceAppMsg;
extern CTraceCategory traceWinMsg;
extern CTraceCategory traceCmdRouting;
extern CTraceCategory traceDumpContext;
