// afxpriv.h — reference SUBSET (declarations only, no implementation).
// In real MFC this is a "private but semi-public" header declaring the
// framework's internal window messages and a number of helper classes.
// Applications are not meant to include it directly, and eMule/srchybrid
// does not; it reaches these symbols transitively, because its Stdafx.h
// includes <afximpl.h> and real MFC's afximpl.h pulls in afxpriv.h. This
// header is included the same way (see afximpl.h).
//
// Only the framework-private WINDOW MESSAGES are declared here — the subset
// eMule actually names in its message maps (WM_KICKIDLE, WM_IDLEUPDATECMDUI)
// plus the surrounding standard block, so the ids keep the exact values and
// spacing real MFC gives them. These are NOT Windows SDK messages (not in
// winuser.h); MFC allocates them in the 0x0360.. range. Values mirror real
// MFC's afxpriv.h.
#pragma once

#define WM_QUERYAFXWNDPROC   0x0360     // lResult = 1 if processed by AfxWndProc
#define WM_SIZEPARENT        0x0361     // lParam = &AFX_SIZEPARENTPARAMS
#define WM_SETMESSAGESTRING  0x0362     // wParam = nIDS or 0, lParam = lpszOther or 0
#define WM_IDLEUPDATECMDUI   0x0363     // wParam == bDisableIfNoHandler
#define WM_INITIALUPDATE     0x0364     // (params unused) - sent to children
#define WM_COMMANDHELP       0x0365     // lParam = dwHelpContext, if 0L use default
#define WM_HELPHITTEST       0x0366     // wParam = 0, lParam = window based coordinates
#define WM_EXITHELPMODE      0x0367     // (params unused)
#define WM_RECALCPARENT      0x0368     // force RecalcLayout on frame window
#define WM_SIZECHILD         0x0369     // lParam = &AFX_SIZECHILD struct (params unused)
#define WM_KICKIDLE          0x036A     // (params unused) causes idles to be sent
#define WM_QUERYCENTERWND    0x036B     // lParam = &rectCenter (return center window)
#define WM_DISABLEMODAL      0x036C     // (params unused) return 0 to prevent disabling
#define WM_FLOATSTATUS       0x036D     // wParam combination of FS_* flags
#define WM_ACTIVATETOPLEVEL  0x036E     // wParam = count of hWnds, lParam = &hWndsList
#define WM_QUERY3DCONTROLS   0x036F     // lResult != 0 if 3D controls wanted
#define WM_SOCKET_NOTIFY     0x0373
#define WM_SOCKET_DEAD       0x0374
#define WM_POPMESSAGESTRING  0x0375
#define WM_HELPPROMPTADDR    0x0376     // no longer used
#define WM_OCC_LOADFROMSTREAM        0x0376
#define WM_OCC_LOADFROMSTORAGE       0x0377
#define WM_OCC_INITNEW               0x0378
#define WM_OCC_LOADFROMSTREAM_EX     0x0379
#define WM_OCC_LOADFROMSTORAGE_EX    0x037A
#define WM_QUERYSAFEMODE     0x037B
