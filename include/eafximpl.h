// afximpl.h — NATIVE implementation (afximpl.cpp) for the two symbols
// that need bodies (AfxGetModuleThreadState, CTraceCategory's
// constructor); everything else here is macros/struct layout only.
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
// category.
//
// BACKEND BRANCH: the two frontend-only pieces this header used to carry --
// MFC's private tooltip flags (TTF_ALWAYSTIP/TTF_NOTBUTTON) and the pre-IE4
// AFX_OLDTOOLINFO layout, both consumed only by CMuleStatusBarCtrl -- were
// dropped with the rest of the GUI surface, and with them the include of
// afxwin.h/afxpriv.h. What is left (the socket thread state and the trace
// category) needs nothing beyond afxcoll.h.
#pragma once
#include "eafxcoll.h" // CMapPtrToPtr / CPtrList, the module thread state's members

// MFC's per-thread, per-module state (real MFC: afxstat_.h, which this
// header includes). eMule reaches it through a shim of its own --
// AsyncSocketEx.h:77-78 defines
//     #define _afxSockThreadState     AfxGetModuleThreadState()
//     #define _AFX_SOCK_THREAD_STATE  AFX_MODULE_THREAD_STATE
// -- to reproduce, for its own socket classes, the initialisation MFC
// does for CAsyncSocket (Emule.cpp:384-390). Only the three socket
// members it touches are declared; real MFC's struct is much larger.
struct EAFX_MODULE_THREAD_STATE
{
    ECMapPtrToPtr* m_pmapSocketHandle;
    ECMapPtrToPtr* m_pmapDeadSockets;
    ECPtrList* m_plistSocketNotifications;
};

EAFX_MODULE_THREAD_STATE* EAFXAPI EAfxGetModuleThreadState();

// The trace categories MFC declares for the category form of TRACE
// ("TRACE(traceAppMsg, 0, ...)"). TRACE itself expands to __noop here,
// but __noop still parses its arguments, so the category has to exist.
struct ECTraceCategory
{
    explicit ECTraceCategory(UINT nCategory = 0) noexcept;
};
extern ECTraceCategory EtraceAppMsg;
