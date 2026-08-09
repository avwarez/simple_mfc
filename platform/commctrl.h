// commctrl.h -- POSIX stand-in for the Windows common-controls SDK header.
//
// PART OF THE Win32 PLATFORM SHIM, NOT OF THE MFC INTERFACE.
//
// Unlike winsock2.h/io.h, almost nothing here can "resolve to the native Linux
// symbol", because there is no native counterpart: these are the notification
// structures COMCTL32.DLL posts to a window, and off Windows the controls are
// drawn by the GUI toolkit instead. What survives the port is the DATA -- the
// shape of the notification an MFC message handler receives. Those handlers
// are real code that must keep compiling and keep reading the same fields, so
// the structures are reproduced with their exact SDK layout (taken from the
// SDK/MinGW-w64 headers, not invented) and the GUI driver fills them in.
//
// Field order and types matter here even off Windows: eMule casts LPARAM to
// these pointers and reads through them, so a wrong layout would compile and
// then misread at run time.
#pragma once

#ifdef _WIN32
#error "simple_mfc/platform is for non-Windows builds only."
#endif

// ---------------------------------------------------------------------
// The common header every WM_NOTIFY notification starts with. Every richer
// notification structure below begins with one of these, which is what makes
// the "cast to NMHDR*, switch on code, then cast to the real type" idiom in
// every OnNotify handler work.
// ---------------------------------------------------------------------
struct tagNMHDR
{
    HWND      hwndFrom;
    UINT_PTR  idFrom;
    UINT      code;
};
using NMHDR = tagNMHDR;
using LPNMHDR = NMHDR *;

// ---------------------------------------------------------------------
// Custom draw. eMule's skinned list/tree controls handle NM_CUSTOMDRAW to
// paint rows themselves.
// ---------------------------------------------------------------------
struct tagNMCUSTOMDRAW
{
    NMHDR     hdr;
    DWORD     dwDrawStage;
    HDC       hdc;
    RECT      rc;
    DWORD_PTR dwItemSpec;
    UINT      uItemState;
    LPARAM    lItemlParam;
};
using NMCUSTOMDRAW = tagNMCUSTOMDRAW;
using LPNMCUSTOMDRAW = NMCUSTOMDRAW *;

#define CDDS_PREPAINT           0x00000001
#define CDDS_POSTPAINT          0x00000002
#define CDDS_PREERASE           0x00000003
#define CDDS_POSTERASE          0x00000004
#define CDDS_ITEM               0x00010000
#define CDDS_ITEMPREPAINT       (CDDS_ITEM | CDDS_PREPAINT)
#define CDDS_ITEMPOSTPAINT      (CDDS_ITEM | CDDS_POSTPAINT)
#define CDDS_ITEMPREERASE       (CDDS_ITEM | CDDS_PREERASE)
#define CDDS_ITEMPOSTERASE      (CDDS_ITEM | CDDS_POSTERASE)
#define CDDS_SUBITEM            0x00020000

#define CDRF_DODEFAULT          0x00000000
#define CDRF_NEWFONT            0x00000002
#define CDRF_SKIPDEFAULT        0x00000004
#define CDRF_NOTIFYPOSTPAINT    0x00000010
#define CDRF_NOTIFYITEMDRAW     0x00000020
#define CDRF_NOTIFYSUBITEMDRAW  0x00000020
#define CDRF_NOTIFYPOSTERASE    0x00000040

#define CDIS_SELECTED           0x0001
#define CDIS_GRAYED             0x0002
#define CDIS_DISABLED           0x0004
#define CDIS_CHECKED            0x0008
#define CDIS_FOCUS              0x0010
#define CDIS_DEFAULT            0x0020
#define CDIS_HOT                0x0040

// ---------------------------------------------------------------------
// Generic WM_NOTIFY codes (the control-specific ones live with their
// controls in afxcmn.h, which is MFC's own header and not this one).
// ---------------------------------------------------------------------
#define NM_FIRST                (0U -  0U)
#define NM_OUTOFMEMORY          (NM_FIRST - 1)
#define NM_CLICK                (NM_FIRST - 2)
#define NM_DBLCLK               (NM_FIRST - 3)
#define NM_RETURN               (NM_FIRST - 4)
#define NM_RCLICK               (NM_FIRST - 5)
#define NM_RDBLCLK              (NM_FIRST - 6)
#define NM_SETFOCUS             (NM_FIRST - 7)
#define NM_KILLFOCUS            (NM_FIRST - 8)
#define NM_CUSTOMDRAW           (NM_FIRST - 12)
#define NM_HOVER                (NM_FIRST - 13)
#define NM_NCHITTEST            (NM_FIRST - 14)
#define NM_KEYDOWN              (NM_FIRST - 15)
#define NM_RELEASEDCAPTURE      (NM_FIRST - 16)
#define NM_SETCURSOR            (NM_FIRST - 17)
#define NM_CHAR                 (NM_FIRST - 18)
#define NM_TOOLTIPSCREATED      (NM_FIRST - 19)
