// afxwin.h — reference STUB (declarations only, no implementation).
// Transitively includes afx.h (as in real MFC). Contains the
// window/thread/application classes, the core GDI classes, and the
// message-map macros, with signatures verified against the official
// Microsoft Learn documentation for the subset of methods actually used
// by eMule/srchybrid (see ../../mfc_scan_srchybrid.md).
#pragma once
// Real MFC's afxwin.h defines this include guard, and downstream headers
// (e.g. eMule's Emule.h) #error out with "include 'stdafx.h' before ..."
// unless it is set -- it is their proxy for "the MFC core headers are in".
#ifndef __AFXWIN_H__
#define __AFXWIN_H__
#endif
#include "afx.h"
#include "atltypes.h"
#include <cstddef> // offsetof, used by METHOD_PROLOGUE below

// ---------------------------------------------------------------------
// afx_msg is real MFC's marker keyword on every message-handler declaration
// (`afx_msg void OnPaint();`). It expands to nothing -- but it MUST be defined,
// otherwise `afx_msg void Foo();` parses as two adjacent declarations and every
// handler line becomes a C2144 syntax error. eMule uses it 816 times, so its
// absence alone was cascading through 150+ files.
#define afx_msg
// Real MFC makes the template collections (CArray/CList/CMap and the
// CTypedPtr* wrappers) and the concrete string maps available transitively
// through the standard <afxwin.h> include chain; eMule/srchybrid relies on
// that (its Stdafx.h uses CArray/CTypedPtrList without a direct
// <afxtempl.h> include), so pull them in here too.
#include "afxtempl.h"

class CWnd;
class CDC;
class CMenu;
class CBitmap;
class CRgn;
class CPalette;
class CCreateContext;
class CDataExchange;
// Real MFC's signature: UINT __cdecl f(LPVOID). Both halves matter --
// eMule's thread functions return UINT and are declared AFX_CDECL, so a
// `long (*)(void*)` rejected every one of them at AfxBeginThread.
typedef UINT(AFX_CDECL* AFX_THREADPROC)(void*);

// ---------------------------------------------------------------------
// Win32 primitive handle/type stand-ins (normally from windef.h/winnt.h
// /winuser.h). On a real Windows/MSVC target these come from the real
// <windows.h> instead of being redefined here: discovered (2026-07-20,
// compiling real eMule/srchybrid against this header on windows-latest)
// that eMule also includes real Win32 headers directly (<winsock2.h>
// etc., for non-MFC networking), which pull in the actual HWND & friends
// (windef.h), SECURITY_ATTRIBUTES/CREATESTRUCT/HELPINFO/TOOLINFO
// (winbase.h/winuser.h) — several of these are typedef-NAMES for a
// differently-tagged real struct (e.g. real SECURITY_ATTRIBUTES aliases
// `struct _SECURITY_ATTRIBUTES`, not a struct literally tagged
// SECURITY_ATTRIBUTES), so our bare forward-declarations collided with
// them (C2371 "redefinition; different basic types"). On non-Windows
// targets (this project's main portability point) none of these headers
// exist, so we still provide our own stand-ins there.
// ---------------------------------------------------------------------
#ifdef _WIN32
#include <windows.h>
#include <commctrl.h> // TOOLINFO is a commctrl.h type, not windows.h
#include <shlwapi.h>  // PathFindExtension/PathAddBackslash & co, which
                      // eMule calls unqualified expecting MFC to have
                      // pulled them in (real MFC's headers do)
#include <shlobj.h>   // CSIDL_* shell folder ids
#include <shobjidl.h> // ITaskbarList3: CEMuleDlg holds one as a CComPtr
#include <uxtheme.h>  // OpenThemeData & co, used by eMule's skinned controls
#include <vssym32.h>  // the theme part/state ids (TABP_*, BP_*, CBS_*, TMT_*)
                      // member, so all 81 TUs that see EmuleDlg.h need the
                      // interface declared. Shell COM, not MFC -- eMule
                      // never includes it itself either, it inherits it
                      // from the MFC headers exactly like this.
#else
// __stdcall on Windows, where it is part of the callback's type; nothing
// to express off it.
#define CALLBACK
using HWND = void*;
using HINSTANCE = void*;
using HDC = void*;
using HICON = void*;
using HCURSOR = void*;
using HBRUSH = void*;
using HFONT = void*;
using HMENU = void*;
using HGDIOBJ = void*;
using HPALETTE = void*;
using HPEN = void*;
using HBITMAP = void*;
using HRGN = void*;
using HGLOBAL = void*;
using LPVOID = void*;
using BYTE = unsigned char;
using COLORREF = unsigned long;
using UINT_PTR = std::uintptr_t;
using DWORD_PTR = std::uintptr_t;
using INT_PTR = long long; // matches afxcoll.h's INT_PTR (identical redefinition is legal if both headers are included together)
using LONG_PTR = std::intptr_t;
using WPARAM = UINT_PTR;
using LPARAM = LONG_PTR;
using LRESULT = LONG_PTR;
struct SECURITY_ATTRIBUTES;
struct tagCREATESTRUCT;
using CREATESTRUCT = tagCREATESTRUCT;
using LPCREATESTRUCT = CREATESTRUCT*;
struct HELPINFO;
struct TOOLINFO;
struct WINDOWPLACEMENT;
struct NCCALCSIZE_PARAMS;
struct tagMENUINFO;
using MENUINFO = tagMENUINFO;
using LPMENUINFO = MENUINFO*;
using LPCMENUINFO = const MENUINFO*;
struct MENUITEMINFOW;
using MENUITEMINFO = MENUITEMINFOW;
using LPMENUITEMINFO = MENUITEMINFO*;
struct SCROLLINFO;
struct tagTEXTMETRIC;
using TEXTMETRIC = tagTEXTMETRIC;
// HIMAGELIST/IMAGEINFO back CImageList (below, its real MFC home is this
// header). On _WIN32 they come from <commctrl.h> included above; here are
// the portable stand-ins (void* handle, incomplete struct used by pointer).
using HIMAGELIST = void*;
struct IMAGEINFO;
#endif

// ---------------------------------------------------------------------
// Additional Win32 primitive stand-ins (FRONTEND/GDI blind-spot pass,
// see ../../mfc_scan_srchybrid.md addendum): needed only by the CWnd
// message-handler declarations below, which real code reaches through
// qualified super-calls (e.g. CDialog::DoDataExchange(),
// CWnd::OnDestroy()) invisible to a plain ".Method("/"->Method(" scan.
// All incomplete/forward-declared: only ever used by pointer here.
// Unlike the block above, these do NOT collide on a real Windows target
// (their real counterparts share the exact same tag name, e.g. real
// windows.h also has a struct literally tagged "tagMSG" — forward-
// declaring the same tag twice, later completed by the real definition,
// is legal C++, not a redefinition), so no #ifdef _WIN32 needed here.
// ---------------------------------------------------------------------
#ifndef _WIN32
struct tagMSG;
using MSG = tagMSG;
using LPMSG = MSG*;

// Win32 window-message and control-notification constants. On a real
// Windows target these come from <winuser.h> (pulled in by <windows.h>
// above); this block is the POSIX half of the same Win32 PLATFORM shim as
// the HWND/WPARAM/MSG stand-ins right above -- it is NOT part of the MFC
// interface (real MFC does not define these; Windows does). The numeric
// values are the actual Win32 ones because the message-map machinery
// (afxmsg_.h) and eMule's own WM_USER-relative custom messages assume the
// standard bases.
#define WM_NULL              0x0000
#define WM_CREATE            0x0001
#define WM_DESTROY           0x0002
#define WM_MOVE              0x0003
#define WM_SIZE              0x0005
#define WM_ACTIVATE          0x0006
#define WM_SETFOCUS          0x0007
#define WM_KILLFOCUS         0x0008
#define WM_ENABLE            0x000A
#define WM_SETREDRAW         0x000B
#define WM_SETTEXT           0x000C
#define WM_GETTEXT           0x000D
#define WM_GETTEXTLENGTH     0x000E
#define WM_PAINT             0x000F
#define WM_CLOSE             0x0010
#define WM_QUERYENDSESSION   0x0011
#define WM_QUIT              0x0012
#define WM_QUERYOPEN         0x0013
#define WM_ERASEBKGND        0x0014
#define WM_SYSCOLORCHANGE    0x0015
#define WM_ENDSESSION        0x0016
#define WM_SHOWWINDOW        0x0018
#define WM_WININICHANGE      0x001A
#define WM_SETTINGCHANGE     WM_WININICHANGE
#define WM_ACTIVATEAPP       0x001C
#define WM_CANCELMODE        0x001F
#define WM_SETCURSOR         0x0020
#define WM_MOUSEACTIVATE     0x0021
#define WM_GETMINMAXINFO     0x0024
#define WM_DRAWITEM          0x002B
#define WM_MEASUREITEM       0x002C
#define WM_DELETEITEM        0x002D
#define WM_SETFONT           0x0030
#define WM_GETFONT           0x0031
#define WM_QUERYDRAGICON     0x0037
#define WM_COMPAREITEM       0x0039
#define WM_NOTIFY            0x004E
#define WM_HELP              0x0053
#define WM_CONTEXTMENU       0x007B
#define WM_NCCREATE          0x0081
#define WM_NCDESTROY         0x0082
#define WM_NCCALCSIZE        0x0083
#define WM_NCHITTEST         0x0084
#define WM_NCPAINT           0x0085
#define WM_NCACTIVATE        0x0086
#define WM_GETDLGCODE        0x0087
#define WM_NCMOUSEMOVE       0x00A0
#define WM_NCLBUTTONDOWN     0x00A1
#define WM_NCLBUTTONUP       0x00A2
#define WM_NCLBUTTONDBLCLK   0x00A3
#define WM_NCRBUTTONDOWN     0x00A4
#define WM_KEYDOWN           0x0100
#define WM_KEYUP             0x0101
#define WM_CHAR              0x0102
#define WM_SYSKEYDOWN        0x0104
#define WM_SYSKEYUP          0x0105
#define WM_COMMAND           0x0111
#define WM_SYSCOMMAND        0x0112
#define WM_TIMER             0x0113
#define WM_HSCROLL           0x0114
#define WM_VSCROLL           0x0115
#define WM_INITMENU          0x0116
#define WM_INITMENUPOPUP     0x0117
#define WM_MENUSELECT        0x011F
#define WM_MENUCHAR          0x0120
#define WM_CTLCOLORMSGBOX    0x0132
#define WM_CTLCOLOREDIT      0x0133
#define WM_CTLCOLORLISTBOX   0x0134
#define WM_CTLCOLORBTN       0x0135
#define WM_CTLCOLORDLG       0x0136
#define WM_CTLCOLORSCROLLBAR 0x0137
#define WM_CTLCOLORSTATIC    0x0138
#define WM_MOUSEMOVE         0x0200
#define WM_LBUTTONDOWN       0x0201
#define WM_LBUTTONUP         0x0202
#define WM_LBUTTONDBLCLK     0x0203
#define WM_RBUTTONDOWN       0x0204
#define WM_RBUTTONUP         0x0205
#define WM_RBUTTONDBLCLK     0x0206
#define WM_MBUTTONDOWN       0x0207
#define WM_MBUTTONUP         0x0208
#define WM_MOUSEWHEEL        0x020A
#define WM_CAPTURECHANGED    0x0215
#define WM_DEVICECHANGE      0x0219
#define WM_QUERYNEWPALETTE   0x030F
#define WM_PALETTECHANGED    0x0311
#define WM_USER              0x0400
#define WM_APP               0x8000

// Control notification codes (nCode of an ON_CONTROL entry, HIWORD of
// WM_COMMAND's wParam).
#define BN_CLICKED           0
#define BN_DOUBLECLICKED     5
#define EN_SETFOCUS          0x0100
#define EN_KILLFOCUS         0x0200
#define EN_CHANGE            0x0300
#define EN_UPDATE            0x0400
#define CBN_SELCHANGE        1
#define CBN_DBLCLK           2
#define CBN_SETFOCUS         3
#define CBN_KILLFOCUS        4
#define CBN_EDITCHANGE       5
#define CBN_SELENDOK         9
#define STN_CLICKED          0
#define STN_DBLCLK           1

// A few kernel32 constants the threading layer needs (real target: from
// <windows.h>). Same POSIX-shim rationale as the WM_* block above.
#define CREATE_SUSPENDED        0x00000004
#define INFINITE                0xFFFFFFFF
#define THREAD_PRIORITY_IDLE            (-15)
#define THREAD_PRIORITY_LOWEST         (-2)
#define THREAD_PRIORITY_BELOW_NORMAL   (-1)
#define THREAD_PRIORITY_NORMAL         0
#define THREAD_PRIORITY_ABOVE_NORMAL   1
#define THREAD_PRIORITY_HIGHEST        2
#define THREAD_PRIORITY_TIME_CRITICAL  15
#endif
// The owner-draw callback structures. Bare-tag forward declarations, so
// they merge with the real winuser.h definitions rather than conflicting
// (same rule as tagMSG above).
struct tagMEASUREITEMSTRUCT;
using LPMEASUREITEMSTRUCT = tagMEASUREITEMSTRUCT*;
struct tagDRAWITEMSTRUCT;
using LPDRAWITEMSTRUCT = tagDRAWITEMSTRUCT*;
class CScrollBar; // real header afxwin.h too; only used here as a pointer parameter

// Real MFC's CWnd-derived classes intentionally hide the base Create()
// overload set with their own Create() (different signature per class):
// that's the actual API shape, not a mistake, but it trips
// -Woverloaded-virtual/C4266. Suppressed for this declaration-only
// hierarchy (afxwin.h/afxext.h/afxdlgs.h/afxcmn.h).
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4266)
#endif

// ---------------------------------------------------------------------
// Message-map data structures (header afxwin.h in real MFC too).
//
// These used to be absent, because every ON_* macro and every
// BEGIN/END_MESSAGE_MAP expanded to nothing: a message map declares
// handlers, and this library implements no dispatch, so an empty
// expansion looked sufficient. It is not. eMule writes message-map
// entries BY HAND -- its own _ON_WM_THEMECHANGED() (ButtonsTabCtrl.cpp,
// ClosableTabCtrl.cpp, DialogMinTrayBtn.cpp) expands to a braced
// AFX_MSGMAP_ENTRY aggregate ending in a comma:
//
//   { _WM_THEMECHANGED, 0, 0, 0, AfxSig_l, (AFX_PMSG)(AFX_PMSGW)
//     (static_cast<LRESULT (AFX_MSG_CALL CWnd::*)(void)>(_OnThemeChanged)) },
//
// which is only legal inside the array real BEGIN_MESSAGE_MAP opens. With
// no-op macros it landed at file scope and produced C2447/C2059. So the
// macros below reproduce real MFC's expansion faithfully -- that, and not
// a lookalike, is what makes eMule's hand-written entries compile, and it
// is also why every ON_* entry macro can keep expanding to nothing: they
// simply contribute no element to the array.
//
// Note the entry macros are the ONLY thing that stays empty. The
// demarcation macros now generate the two functions real MFC generates,
// so a class that writes BEGIN_MESSAGE_MAP without DECLARE_MESSAGE_MAP
// no longer compiles here either -- exactly as under real MFC.
// ---------------------------------------------------------------------
#ifndef _WIN32
// __stdcall on Windows (windef.h), where it is part of the function's
// type; nothing to express off it. Same treatment as CALLBACK above.
#define PASCAL
#endif
// Real MFC's marker on message handlers' calling convention: empty on
// every current target, kept because eMule spells it out in its own
// entry macros (static_cast<LRESULT (AFX_MSG_CALL CWnd::*)(void)>).
#define AFX_MSG_CALL

// MFC's consolidated CTLCOLOR pseudo-message. Win16's single WM_CTLCOLOR
// was split by Win32 into WM_CTLCOLORBTN/EDIT/... so it is NOT in winuser.h
// anymore; real MFC keeps the old id (0x0019) alive in afxwin.h and its
// ON_WM_CTLCOLOR()/ON_WM_CTLCOLOR_REFLECT() entries route through it. Define
// it here for the same reason, on every target (guarded in case an SDK
// still provides it).
#ifndef WM_CTLCOLOR
#define WM_CTLCOLOR 0x0019
#endif

struct AFX_MSGMAP_ENTRY;

struct AFX_MSGMAP
{
    const AFX_MSGMAP* (PASCAL* pfnGetBaseMap)();
    const AFX_MSGMAP_ENTRY* lpEntries;
};

// The pointer-to-member types an entry stores its handler as. Handlers
// have every possible signature, so MFC casts them all to one type; the
// CWnd form exists because a cast has to go through the class the
// handler is actually declared in before being flattened to AFX_PMSG.
class CCmdTarget; // defined just below; named here by the handler types
typedef void (AFX_MSG_CALL CCmdTarget::*AFX_PMSG)(void);
typedef void (AFX_MSG_CALL CWnd::*AFX_PMSGW)(void);

struct AFX_MSGMAP_ENTRY
{
    UINT nMessage;   // the Windows message
    UINT nCode;      // control notification code, or WM_NOTIFY code
    UINT nID;        // control id (0 for a plain window message)
    UINT nLastID;    // the end of the id range, for the _RANGE entries
    UINT_PTR nSig;   // which handler signature pfn really has
    AFX_PMSG pfn;    // the handler itself
};

// The handler-signature tags. Each names one distinct handler prototype;
// the gui/core dispatcher switches on the tag to cast the entry's pfn back
// to that prototype and marshal wParam/lParam into its arguments. This is
// a faithful MIRROR of real MFC's AfxSig enum (afxmsg_.h): identical
// enumerator names, order and ordinals, so simple_mfc's ON_* macros expand
// exactly as real MFC's do and eMule's message maps -- including its own
// hand-written entries that name AfxSig_l -- compile unchanged. The whole
// enum is reproduced (not just the subset eMule references) so the ordinals
// stay identical to the real header; the trailing "Old"/alias block are
// convenience names equal to the primary ones, exactly as in real MFC.
// The naming scheme is real MFC's: return_wParam_lParam, with
// v=void, b=BOOL, i=int, u=UINT, up=UINT_PTR, l=LRESULT/LPARAM, w=WPARAM,
// W=CWnd*, D=CDC*, M=CMenu*, F=CFont*, h=HANDLE, C=HCURSOR, p=CPoint,
// s=LPTSTR, S=LPCTSTR.
enum AfxSig
{
    AfxSig_end = 0,     // [marks end of message map]

    AfxSig_b_D_v,               // BOOL (CDC*)
    AfxSig_b_b_v,               // BOOL (BOOL)
    AfxSig_b_u_v,               // BOOL (UINT)
    AfxSig_b_h_v,               // BOOL (HANDLE)
    AfxSig_b_W_uu,              // BOOL (CWnd*, UINT, UINT)
    AfxSig_b_W_COPYDATASTRUCT,  // BOOL (CWnd*, COPYDATASTRUCT*)
    AfxSig_b_v_HELPINFO,        // BOOL (LPHELPINFO);
    AfxSig_CTLCOLOR,            // HBRUSH (CDC*, CWnd*, UINT)
    AfxSig_CTLCOLOR_REFLECT,    // HBRUSH (CDC*, UINT)
    AfxSig_i_u_W_u,             // int (UINT, CWnd*, UINT)  // ?TOITEM
    AfxSig_i_uu_v,              // int (UINT, UINT)
    AfxSig_i_W_uu,              // int (CWnd*, UINT, UINT)
    AfxSig_i_v_s,               // int (LPTSTR)
    AfxSig_l_w_l,               // LRESULT (WPARAM, LPARAM)
    AfxSig_l_uu_M,              // LRESULT (UINT, UINT, CMenu*)
    AfxSig_v_b_h,               // void (BOOL, HANDLE)
    AfxSig_v_h_v,               // void (HANDLE)
    AfxSig_v_h_h,               // void (HANDLE, HANDLE)
    AfxSig_v_v_v,               // void ()
    AfxSig_v_u_v,               // void (UINT)
    AfxSig_v_up_v,              // void (UINT_PTR)
    AfxSig_v_u_u,               // void (UINT, UINT)
    AfxSig_v_uu_v,              // void (UINT, UINT)
    AfxSig_v_v_ii,              // void (int, int)
    AfxSig_v_u_uu,              // void (UINT, UINT, UINT)
    AfxSig_v_u_ii,              // void (UINT, int, int)
    AfxSig_v_u_W,               // void (UINT, CWnd*)
    AfxSig_i_u_v,               // int (UINT)
    AfxSig_u_u_v,               // UINT (UINT)
    AfxSig_b_v_v,               // BOOL ()
    AfxSig_v_w_l,               // void (WPARAM, LPARAM)
    AfxSig_MDIACTIVATE,         // void (BOOL, CWnd*, CWnd*)
    AfxSig_v_D_v,               // void (CDC*)
    AfxSig_v_M_v,               // void (CMenu*)
    AfxSig_v_M_ub,              // void (CMenu*, UINT, BOOL)
    AfxSig_v_W_v,               // void (CWnd*)
    AfxSig_v_v_W,               // void (CWnd*)
    AfxSig_v_W_uu,              // void (CWnd*, UINT, UINT)
    AfxSig_v_W_p,               // void (CWnd*, CPoint)
    AfxSig_v_W_h,               // void (CWnd*, HANDLE)
    AfxSig_C_v_v,               // HCURSOR ()
    AfxSig_ACTIVATE,            // void (UINT, CWnd*, BOOL)
    AfxSig_SCROLL,              // void (UINT, UINT, CWnd*)
    AfxSig_SCROLL_REFLECT,      // void (UINT, UINT)
    AfxSig_v_v_s,               // void (LPTSTR)
    AfxSig_v_u_cs,              // void (UINT, LPCTSTR)
    AfxSig_OWNERDRAW,           // void (int, LPTSTR) force return TRUE
    AfxSig_i_i_s,               // int (int, LPTSTR)
    AfxSig_u_v_p,               // UINT (CPoint)
    AfxSig_u_v_v,               // UINT ()
    AfxSig_v_b_NCCALCSIZEPARAMS,// void (BOOL, NCCALCSIZE_PARAMS*)
    AfxSig_v_v_WINDOWPOS,       // void (WINDOWPOS*)
    AfxSig_v_uu_M,              // void (UINT, UINT, HMENU)
    AfxSig_v_u_p,               // void (UINT, CPoint)
    AfxSig_SIZING,              // void (UINT, LPRECT)
    AfxSig_MOUSEWHEEL,          // BOOL (UINT, short, CPoint)
    AfxSig_MOUSEHWHEEL,         // void (UINT, short, CPoint)
    AfxSigCmd_v,                // void ()
    AfxSigCmd_b,                // BOOL ()
    AfxSigCmd_RANGE,            // void (UINT)
    AfxSigCmd_EX,               // BOOL (UINT)
    AfxSigNotify_v,             // void (NMHDR*, LRESULT*)
    AfxSigNotify_b,             // BOOL (NMHDR*, LRESULT*)
    AfxSigNotify_RANGE,         // void (UINT, NMHDR*, LRESULT*)
    AfxSigNotify_EX,            // BOOL (UINT, NMHDR*, LRESULT*)
    AfxSigCmdUI,                // void (CCmdUI*)
    AfxSigCmdUI_RANGE,          // void (CCmdUI*, UINT)
    AfxSigCmd_v_pv,             // void (void*)
    AfxSigCmd_b_pv,             // BOOL (void*)
    AfxSig_l,                   // LRESULT ()
    AfxSig_l_p,                 // LRESULT (CPOINT)
    AfxSig_u_W_u,               // UINT (CWnd*, UINT)
    AfxSig_v_u_M,               // void (UINT, CMenu* )
    AfxSig_u_u_M,               // UINT (UINT, CMenu* )
    AfxSig_u_v_MENUGETOBJECTINFO,// UINT (MENUGETOBJECTINFO*)
    AfxSig_v_M_u,               // void (CMenu*, UINT)
    AfxSig_v_u_LPMDINEXTMENU,   // void (UINT, LPMDINEXTMENU)
    AfxSig_APPCOMMAND,          // void (CWnd*, UINT, UINT, UINT)
    AfxSig_RAWINPUT,            // void (UINT, HRAWINPUT)
    AfxSig_u_u_u,               // UINT (UINT, UINT)
    AfxSig_MOUSE_XBUTTON,       // void (UINT, UINT, CPoint)
    AfxSig_MOUSE_NCXBUTTON,     // void (short, UINT, CPoint)
    AfxSig_INPUTLANGCHANGE,     // void (UINT, UINT)
    AfxSig_v_u_hkl,             // void (UINT, HKL)
    AfxSig_INPUTDEVICECHANGE,   // void (unsigned short, HANDLE)
    AfxSig_l_D_u,               // LRESULT (CDC*, UINT)
    AfxSig_i_v_S,               // int (LPCTSTR)
    AfxSig_v_F_b,               // void (CFont*, BOOL)
    AfxSig_h_v_v,               // HANDLE ()
    AfxSig_h_b_h,               // HANDLE (BOOL, HANDLE)
    AfxSig_b_v_ii,              // BOOL (int, int)
    AfxSig_h_h_h,               // HANDLE (HANDLE, HANDLE)
    AfxSig_MDINext,             // void (CWnd*, BOOL)
    AfxSig_u_u_l,               // UINT (UINT, LPARAM)

// Old
    AfxSig_bD = AfxSig_b_D_v,           // BOOL (CDC*)
    AfxSig_bb = AfxSig_b_b_v,           // BOOL (BOOL)
    AfxSig_bWww = AfxSig_b_W_uu,        // BOOL (CWnd*, UINT, UINT)
    AfxSig_hDWw = AfxSig_CTLCOLOR,      // HBRUSH (CDC*, CWnd*, UINT)
    AfxSig_hDw = AfxSig_CTLCOLOR_REFLECT,   // HBRUSH (CDC*, UINT)
    AfxSig_iwWw = AfxSig_i_u_W_u,       // int (UINT, CWnd*, UINT)
    AfxSig_iww = AfxSig_i_uu_v,         // int (UINT, UINT)
    AfxSig_iWww = AfxSig_i_W_uu,        // int (CWnd*, UINT, UINT)
    AfxSig_is = AfxSig_i_v_s,           // int (LPTSTR)
    AfxSig_lwl = AfxSig_l_w_l,          // LRESULT (WPARAM, LPARAM)
    AfxSig_lwwM = AfxSig_l_uu_M,        // LRESULT (UINT, UINT, CMenu*)
    AfxSig_vv = AfxSig_v_v_v,           // void (void)

    AfxSig_vw = AfxSig_v_u_v,           // void (UINT)
    AfxSig_vww = AfxSig_v_u_u,          // void (UINT, UINT)
    AfxSig_vww2 = AfxSig_v_uu_v,        // void (UINT, UINT) // both come from wParam
    AfxSig_vvii = AfxSig_v_v_ii,        // void (int, int) // wParam is ignored
    AfxSig_vwww = AfxSig_v_u_uu,        // void (UINT, UINT, UINT)
    AfxSig_vwii = AfxSig_v_u_ii,        // void (UINT, int, int)
    AfxSig_vwl = AfxSig_v_w_l,          // void (UINT, LPARAM)
    AfxSig_vbWW = AfxSig_MDIACTIVATE,   // void (BOOL, CWnd*, CWnd*)
    AfxSig_vD = AfxSig_v_D_v,           // void (CDC*)
    AfxSig_vM = AfxSig_v_M_v,           // void (CMenu*)
    AfxSig_vMwb = AfxSig_v_M_ub,        // void (CMenu*, UINT, BOOL)

    AfxSig_vW = AfxSig_v_W_v,           // void (CWnd*)
    AfxSig_vWww = AfxSig_v_W_uu,        // void (CWnd*, UINT, UINT)
    AfxSig_vWp = AfxSig_v_W_p,          // void (CWnd*, CPoint)
    AfxSig_vWh = AfxSig_v_W_h,          // void (CWnd*, HANDLE)
    AfxSig_vwW = AfxSig_v_u_W,          // void (UINT, CWnd*)
    AfxSig_vwWb = AfxSig_ACTIVATE,      // void (UINT, CWnd*, BOOL)
    AfxSig_vwwW = AfxSig_SCROLL,        // void (UINT, UINT, CWnd*)
    AfxSig_vwwx = AfxSig_SCROLL_REFLECT,    // void (UINT, UINT)
    AfxSig_vs = AfxSig_v_v_s,           // void (LPTSTR)
    AfxSig_vOWNER = AfxSig_OWNERDRAW,   // void (int, LPTSTR), force return TRUE
    AfxSig_iis = AfxSig_i_i_s,          // int (int, LPTSTR)
    AfxSig_wp = AfxSig_u_v_p,           // UINT (CPoint)
    AfxSig_wv = AfxSig_u_v_v,           // UINT (void)
    AfxSig_vPOS = AfxSig_v_v_WINDOWPOS, // void (WINDOWPOS*)
    AfxSig_vCALC = AfxSig_v_b_NCCALCSIZEPARAMS,     // void (BOOL, NCCALCSIZE_PARAMS*)
    AfxSig_vNMHDRpl = AfxSigNotify_v,   // void (NMHDR*, LRESULT*)
    AfxSig_bNMHDRpl = AfxSigNotify_b,   // BOOL (NMHDR*, LRESULT*)
    AfxSig_vwNMHDRpl = AfxSigNotify_RANGE,  // void (UINT, NMHDR*, LRESULT*)
    AfxSig_bwNMHDRpl = AfxSigNotify_EX, // BOOL (UINT, NMHDR*, LRESULT*)
    AfxSig_bHELPINFO = AfxSig_b_v_HELPINFO, // BOOL (HELPINFO*)
    AfxSig_vwSIZING = AfxSig_SIZING,    // void (UINT, LPRECT) -- return TRUE

    // signatures specific to CCmdTarget
    AfxSig_cmdui = AfxSigCmdUI,         // void (CCmdUI*)
    AfxSig_cmduiw = AfxSigCmdUI_RANGE,  // void (CCmdUI*, UINT)
    AfxSig_vpv = AfxSigCmd_v_pv,        // void (void*)
    AfxSig_bpv = AfxSigCmd_b_pv,        // BOOL (void*)

    // Other aliases (based on implementation)
    AfxSig_vwwh = AfxSig_v_uu_M,        // void (UINT, UINT, HMENU)
    AfxSig_vwp = AfxSig_v_u_p,          // void (UINT, CPoint)
    AfxSig_bw = AfxSig_b_u_v,           // BOOL (UINT)
    AfxSig_bh = AfxSig_b_h_v,           // BOOL (HANDLE)
    AfxSig_iw = AfxSig_i_u_v,           // int (UINT)
    AfxSig_ww = AfxSig_u_u_v,           // UINT (UINT)
    AfxSig_bv = AfxSig_b_v_v,           // BOOL (void)
    AfxSig_hv = AfxSig_C_v_v,           // HANDLE (void)
    AfxSig_vb = AfxSig_vw,              // void (BOOL)
    AfxSig_vbh = AfxSig_v_b_h,          // void (BOOL, HANDLE)
    AfxSig_vbw = AfxSig_vww,            // void (BOOL, UINT)
    AfxSig_vhh = AfxSig_v_h_h,          // void (HANDLE, HANDLE)
    AfxSig_vh = AfxSig_v_h_v,           // void (HANDLE)
    AfxSig_viSS = AfxSig_vwl,           // void (int, STYLESTRUCT*)
    AfxSig_bwl = AfxSig_lwl,
    AfxSig_vwMOVING = AfxSig_vwSIZING,  // void (UINT, LPRECT) -- return TRUE

    AfxSig_vW2 = AfxSig_v_v_W,          // void (CWnd*) (CWnd* comes from lParam)
    AfxSig_bWCDS = AfxSig_b_W_COPYDATASTRUCT,   // BOOL (CWnd*, COPYDATASTRUCT*)
    AfxSig_bwsp = AfxSig_MOUSEWHEEL,    // BOOL (UINT, short, CPoint)
    AfxSig_vws = AfxSig_v_u_cs
};

// What a class puts in its own body to own a message map. Real MFC's
// expansion ends with "protected:", so the handlers declared after it in
// a class body are protected -- which is what makes a derived class's
// qualified super-call (CTrayDialog::OnSysCommand(...)) legal. Expanding
// to nothing left them private and produced C2248.
// GetThisMessageMap is static: that is what lets END_MESSAGE_MAP below
// write "&TheBaseClass::GetThisMessageMap" for ANY base without every
// base having to redeclare it -- CCmdTarget declares it once and the
// whole hierarchy inherits it.
// The counterpart macros BEGIN_MESSAGE_MAP/END_MESSAGE_MAP are defined
// further down, next to afxmsg_.h's entry macros: only .cpp files expand
// them, so they need nothing declared here.
#define DECLARE_MESSAGE_MAP()                                    \
protected:                                                       \
    static const AFX_MSGMAP* PASCAL GetThisMessageMap();         \
    virtual const AFX_MSGMAP* GetMessageMap() const;

// ---------------------------------------------------------------------
// CCmdTarget — base of CWinThread for command routing (header afxwin.h)
// ---------------------------------------------------------------------
// The routing context OnCmdMsg reports back into; opaque to callers.
struct AFX_CMDHANDLERINFO;
#ifndef _WIN32
struct IUnknown;
using LPUNKNOWN = IUnknown*;
using LPVOID = void*;
#endif

class CCmdTarget : public CObject
{
public:
    // COM plumbing for the nested interface parts. "External" is the
    // reference count seen by clients; "Internal" is the object's own,
    // and they differ only when the object is aggregated.
    DWORD ExternalQueryInterface(const void* iid, LPVOID* ppvObj);
    DWORD ExternalAddRef();
    DWORD ExternalRelease();
    DWORD InternalQueryInterface(const void* iid, LPVOID* ppvObj);
    DWORD InternalAddRef();
    DWORD InternalRelease();
    // Hands out one of this object's interfaces without adding a
    // reference (eMule: "(IDataObject*)pbdo->GetInterface(&IID_IDataObject)").
    LPUNKNOWN GetInterface(const void* iid);
    LPUNKNOWN GetControllingUnknown();
    void EnableAutomation();
    void EnableAggregation();

    // Command routing, and the wait-cursor helpers real MFC puts here.
    virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
    void BeginWaitCursor();
    void EndWaitCursor();
    void RestoreWaitCursor();

    // Real MFC ends CCmdTarget's body with this too. Declaring it here
    // and nowhere else is deliberate: GetThisMessageMap is static, so
    // every class in the hierarchy inherits a usable
    // "TheBaseClass::GetThisMessageMap" for END_MESSAGE_MAP to point the
    // base-map link at, whatever class eMule names as its base.
    DECLARE_MESSAGE_MAP()
};

// ---------------------------------------------------------------------
// CWinThread (header afxwin.h, hierarchy CObject -> CCmdTarget -> CWinThread)
// ---------------------------------------------------------------------
class CWinThread : public CCmdTarget
{
public:
    // Not on the Learn CWinThread page (which lists only m_bAutoDelete,
    // m_hThread, m_nThreadID, m_pActiveWnd, m_pMainWnd), but real: eMule
    // clears it directly ("m_pThread->m_pThreadParams = NULL;").
    LPVOID m_pThreadParams = nullptr;

public:
    BOOL m_bAutoDelete;
    void* m_hThread;
    DWORD m_nThreadID;
    CWnd* m_pMainWnd;   // real MFC public data members
    CWnd* m_pActiveWnd;

    CWinThread();
    // The worker-thread form: eMule constructs one directly with its
    // thread procedure and parameter.
    CWinThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam);
    BOOL CreateThread(DWORD dwCreateFlags = 0, UINT nStackSize = 0,
                       SECURITY_ATTRIBUTES* lpSecurityAttrs = nullptr);
    DWORD ResumeThread();
    DWORD SuspendThread();
    BOOL SetThreadPriority(int nPriority);
    int GetThreadPriority();
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    virtual int Run();
    // How a thread with m_bAutoDelete cleared is disposed of; eMule's
    // CGDIThread/CPreviewThread override it.
    virtual void Delete();
    // Real MFC declares ~CWinThread (it closes the OS handle and frees the
    // thread's state). Needed here so the pimpl below is released.
    virtual ~CWinThread();

private:
    // Internal implementation state (the std::thread, the worker procedure,
    // and the create-suspended gate). Kept OUT of this frozen MFC-subset
    // interface: it is a simple_mfc implementation mechanism, not part of
    // the MFC contract, so it is private, opaque, and its name/shape are our
    // own (real MFC's equivalents -- m_pfnThreadProc, m_hThread's real HANDLE
    // -- are internal too). Defined in gui/core/winthread.cpp. Derived
    // classes never touch it; they use the public methods above.
    struct Impl;
    Impl* m_pImpl = nullptr;
};

// ---------------------------------------------------------------------
// CCommandLineInfo (header afxwin.h, derives from CObject) — the parsed
// command line CWinApp::ParseCommandLine fills in. eMule reads the two
// fields that matter for a shell "open this ed2k/magnet link" launch.
// ---------------------------------------------------------------------
class CCommandLineInfo : public CObject
{
public:
    CCommandLineInfo();
    // Called once per token by ParseCommandLine; overriding it is how an
    // application adds its own switches.
    virtual void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast);

    BOOL m_bShowSplash;
    BOOL m_bRunEmbedded;
    BOOL m_bRunAutomated;
    // Which document command the shell asked for. eMule compares against
    // FileOpen to tell "opened with a file/link" from a plain start.
    enum
    {
        FileNew,
        FileOpen,
        FilePrint,
        FilePrintTo,
        FileDDE,
        AppRegister,
        AppUnregister,
        FileNothing = -1
    } m_nShellCommand;
    CString m_strFileName;
    CString m_strPrinterName;
    CString m_strDriverName;
    CString m_strPortName;
};

// ---------------------------------------------------------------------
// CWinApp (header afxwin.h, derives from CWinThread)
// ---------------------------------------------------------------------
class CWinApp : public CWinThread
{
public:
    // Real MFC public data members (winmain sets them; app code reads them,
    // e.g. eMule's GetProfileFile() returns m_pszProfileName).
    HINSTANCE m_hInstance;
    HINSTANCE m_hPrevInstance;
    LPTSTR    m_lpCmdLine;
    int       m_nCmdShow;
    LPCTSTR   m_pszAppName;
    LPCTSTR   m_pszRegistryKey;
    LPCTSTR   m_pszExeName;
    LPCTSTR   m_pszHelpFilePath;
    LPCTSTR   m_pszProfileName;
    // The help context a message box last asked for, so that the app's
    // Help command can answer about that box instead of the app itself.
    // Public in real MFC (an implementation member, not on its
    // documented member list); eMule reads it in CemuleApp::OnHelp.
    DWORD m_dwPromptContext;

    // The name form of the constructor: eMule's app object forwards its
    // own name to it ("CemuleApp::CemuleApp(LPCTSTR lpszAppName)
    // : CWinApp(lpszAppName)"), and the default makes the no-argument
    // form real MFC also offers work.
    explicit CWinApp(LPCTSTR lpszAppName = nullptr);

    UINT GetProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault);
    BOOL WriteProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nValue);
    CString GetProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault = nullptr);
    virtual BOOL OnIdle(LONG lCount);
    // The predefined system cursors (IDC_SIZEWE etc.), as opposed to
    // LoadCursor's application resources.
    HICON LoadIcon(UINT nIDResource) const;
    HICON LoadIcon(LPCTSTR lpszResourceName) const;
    HCURSOR LoadCursor(UINT nIDResource) const;
    HCURSOR LoadCursor(LPCTSTR lpszResourceName) const;
    HCURSOR LoadStandardCursor(LPCTSTR lpszCursorName) const;
    HICON LoadStandardIcon(LPCTSTR lpszIconName) const;
    // Called from the message pump to decide whether a message counts as
    // user activity; eMule overrides it and super-calls this one.
    virtual BOOL IsIdleMessage(MSG* pMsg);

    // Help. EnableHtmlHelp switches the application from the old WinHelp
    // engine to HTML Help (eMule calls it in its constructor);
    // WinHelpInternal is the entry point the framework routes a help
    // request through once a path has been set, and is virtual because
    // EnableHtmlHelp works by overriding it.
    void EnableHtmlHelp();
#ifdef _WIN32
    virtual void WinHelpInternal(DWORD_PTR dwData, UINT nCmd = HELP_CONTEXT);
#else
    virtual void WinHelpInternal(DWORD_PTR dwData, UINT nCmd = 1);
#endif

    // Splits the process command line into the fields of a
    // CCommandLineInfo, calling its ParseParam for each token.
    void ParseCommandLine(CCommandLineInfo& rCmdInfo);
};

// Enables an ActiveX control container in a dialog-based application;
// eMule calls it before creating its browser-hosting dialogs.
void AFXAPI AfxEnableControlContainer(void* pOccManager = nullptr);
// Loads the RichEdit 4.1/5.0 window class (MSFTEDIT.DLL) so that a
// dialog template's RICHEDIT50W controls can be created. Returns FALSE
// if the library is missing, which is what eMule tests.
BOOL AFXAPI AfxInitRichEdit5();

// ---------------------------------------------------------------------
// GDI classes (header afxwin.h per Microsoft Learn — CImageList is the
// one exception, it lives in afxcmn.h, see there). Declared before CWnd
// because CWnd::GetDC/ReleaseDC and CMenu::AppendMenu reference them.
// ---------------------------------------------------------------------

// CGdiObject — base of CBitmap/CBrush/CFont/CPalette/CPen/CRgn (header
// afxwin.h, hierarchy CObject -> CGdiObject). DeleteObject/Attach/
// Detach/GetSafeHandle/GetObject are genuinely defined once here, not
// re-implemented per subclass (verified against Microsoft Learn).
class CGdiObject : public CObject
{
public:
    // Public in real MFC too, and eMule reads it directly as the "is this
    // object created?" test (`if (theApp.m_fontSymbol.m_hObject)`) and to
    // pass the raw handle to SendMessage(WM_SETFONT).
    HGDIOBJ m_hObject = nullptr;

    // Real MFC's GDI wrappers convert implicitly to their own handle type,
    // which is how eMule returns a CBrush where an HBRUSH is expected.
    operator HGDIOBJ() const { return m_hObject; }

    BOOL DeleteObject();
    BOOL Attach(HGDIOBJ hObject);
    HGDIOBJ Detach();
    HGDIOBJ GetSafeHandle() const;
    int GetObject(int nCount, LPVOID lpObject) const;
};

// LOGBRUSH/LOGFONT (like CREATESTRUCT/HELPINFO/TOOLINFO above) are
// typedef-names for a real, differently-tagged ANSI/Unicode-dispatched
// struct on Windows (e.g. real LOGFONT aliases tagLOGFONTW) -- guarded
// the same way, real <windows.h> already pulled in above provides them.
#ifndef _WIN32
struct tagLOGBRUSH;
using LOGBRUSH = tagLOGBRUSH;
#endif

// CPen (header afxwin.h, deriva da CGdiObject)
class CPen : public CGdiObject
{
public:
    operator HPEN() const { return (HPEN)m_hObject; }
    CPen();
    CPen(int nPenStyle, int nWidth, COLORREF crColor);
    CPen(int nPenStyle, int nWidth, const LOGBRUSH* pLogBrush, int nStyleCount = 0, const DWORD* lpStyle = nullptr);

    BOOL CreatePen(int nPenStyle, int nWidth, COLORREF crColor);
    BOOL CreatePen(int nPenStyle, int nWidth, const LOGBRUSH* pLogBrush, int nStyleCount = 0, const DWORD* lpStyle = nullptr);
};

// CBrush (header afxwin.h, deriva da CGdiObject)
class CBrush : public CGdiObject
{
public:
    operator HBRUSH() const { return (HBRUSH)m_hObject; }
    CBrush();
    CBrush(COLORREF crColor);
    CBrush(int nIndex, COLORREF crColor);
    explicit CBrush(CBitmap* pBitmap);

    BOOL CreateSolidBrush(COLORREF crColor);
    BOOL CreateHatchBrush(int nIndex, COLORREF crColor);
    BOOL CreatePatternBrush(CBitmap* pBitmap);
    BOOL CreateDIBPatternBrush(HGLOBAL hPackedDIB, UINT nUsage);
    BOOL CreateDIBPatternBrush(const void* lpPackedDIB, UINT nUsage);
    // The LOGBRUSH form, which is how eMule builds its 8x8 pattern brush
    // (Emule.cpp:1780) after creating the bitmap by hand below.
    BOOL CreateBrushIndirect(const LOGBRUSH* lpLogBrush);
};

struct tagLOGPALETTE;
using LOGPALETTE = tagLOGPALETTE;
using LPLOGPALETTE = LOGPALETTE*;

// CPalette (header afxwin.h, deriva da CGdiObject). Was previously only
// forward-declared (used as an incomplete pointer-only type in
// CDC::SelectObject/SelectPalette); given a real definition here because
// eMule/srchybrid genuinely instantiates and uses one (ColourPopup.cpp:
// m_Palette.CreatePalette(pLogPalette)/.DeleteObject()/pDC->SelectPalette(
// &m_Palette, FALSE), m_Palette declared as a plain CPalette member in
// ColourPopup.h) — found during the FRONTEND/GDI blind-spot pass, see
// ../../mfc_scan_srchybrid.md addendum. DeleteObject is inherited from
// CGdiObject, not redeclared here.
class CPalette : public CGdiObject
{
public:
    operator HPALETTE() const { return (HPALETTE)m_hObject; }
    BOOL CreatePalette(LPLOGPALETTE lpLogPalette);
};

// CBitmap (header afxwin.h, deriva da CGdiObject)
class CBitmap : public CGdiObject
{
public:
    operator HBITMAP() const { return (HBITMAP)m_hObject; }
    // The from-scratch form (no DC involved): eMule builds a 1bpp 8x8
    // pattern from a static WORD[8] with it (Emule.cpp:1775).
    BOOL CreateBitmap(int nWidth, int nHeight, UINT nPlanes, UINT nBitcount, const void* lpBits);
    BOOL CreateCompatibleBitmap(CDC* pDC, int nWidth, int nHeight);
    int GetBitmap(struct tagBITMAP* pBitMap);
    DWORD GetBitmapBits(DWORD dwCount, void* lpBits) const;
    DWORD SetBitmapBits(DWORD dwCount, const void* lpBits);
    BOOL LoadBitmap(LPCTSTR lpszResourceName);
    BOOL LoadBitmap(UINT nIDResource);
    CSize GetBitmapDimension() const;
};

// CRgn (header afxwin.h, deriva da CGdiObject). CCreditsThread holds one
// by value (`CRgn m_rgnScreen;`), so the forward declaration at the top of
// this header is not enough -- the class has to be complete.
class CRgn : public CGdiObject
{
public:
    operator HRGN() const { return (HRGN)m_hObject; }
    BOOL CreateRectRgn(int x1, int y1, int x2, int y2);
    BOOL CreateRectRgnIndirect(const RECT* lpRect);
    int CombineRgn(CRgn* pRgn1, CRgn* pRgn2, int nCombineMode);
};

#ifndef _WIN32
struct tagLOGFONT;
using LOGFONT = tagLOGFONT;
#endif

// CFont (header afxwin.h, deriva da CGdiObject)
class CFont : public CGdiObject
{
public:
    operator HFONT() const { return (HFONT)m_hObject; }
    BOOL CreateFontIndirect(const LOGFONT* lpLogFont);
    int GetLogFont(LOGFONT* pLogFont);
    BOOL CreateFont(int nHeight, int nWidth, int nEscapement, int nOrientation, int nWeight,
                     BYTE bItalic, BYTE bUnderline, BYTE cStrikeOut, BYTE nCharSet,
                     BYTE nOutPrecision, BYTE nClipPrecision, BYTE nQuality,
                     BYTE nPitchAndFamily, LPCTSTR lpszFacename);
    BOOL CreatePointFont(int nPointSize, LPCTSTR lpszFaceName, CDC* pDC = nullptr);
};

// ---------------------------------------------------------------------
// CCreateContext (header afxwin.h, no base class) — the bundle MFC passes
// around while creating a frame/view. eMule declares one on the stack and
// fills in m_pNewViewClass to host a CFormView pane, so the forward
// declaration at the top of this header is not enough.
// ---------------------------------------------------------------------
class CDocument;
class CDocTemplate;
class CView;
class CFrameWnd;

class CCreateContext
{
public:
    CRuntimeClass* m_pNewViewClass = nullptr;
    CDocument* m_pCurrentDoc = nullptr;
    CDocTemplate* m_pNewDocTemplate = nullptr;
    CView* m_pLastView = nullptr;
    CFrameWnd* m_pCurrentFrame = nullptr;
};

// CDC (header afxwin.h, hierarchy CObject -> CDC)
class CDC : public CObject
{
public:
    // The wrapped device contexts, public in real MFC. CMemDC copies them
    // straight across (`m_hDC = pDC->m_hDC;`) and clears them on release,
    // so both have to be assignable members rather than accessors.
    HDC m_hDC = nullptr;
    HDC m_hAttribDC = nullptr;
    // TRUE while the DC is a printer DC. Public in real MFC, and eMule's
    // list controls branch on it to skip screen-only drawing.
    BOOL m_bPrinting = FALSE;

    // Lets a CDC be handed to a raw Win32 call that wants an HDC, which
    // eMule does directly (`FillRect(*pDC, &rc, hBrush)`).
    operator HDC() const { return m_hDC; }

    static CDC* FromHandle(HDC hDC);

    // Each overload returns the *previously selected object of the same
    // kind*, not the DC -- that is what makes the idiomatic
    // `CBitmap *pOld = dc.SelectObject(&bmp); ... dc.SelectObject(pOld);`
    // restore pattern (CMemDC, CBarShader, ...) compile. Selecting a region
    // is the odd one out and returns a region-type code. CPalette goes
    // through SelectPalette in real MFC, so it has no overload here.
    CGdiObject* SelectObject(CGdiObject* pObject);
    // Not on the Learn reference page, which lists only the six
    // CGdiObject-typed forms, but real: eMule assigns this one's result
    // to an HGDIOBJ and passes a CBitmap *object* (converted through
    // CGdiObject::operator HGDIOBJ) as the argument.
    HGDIOBJ SelectObject(HGDIOBJ hObject);
    virtual CFont* SelectObject(CFont* pFont);
    CBrush* SelectObject(CBrush* pBrush);
    CPen* SelectObject(CPen* pPen);
    CBitmap* SelectObject(CBitmap* pBitmap);
    int SelectObject(CRgn* pRgn);
    BOOL Attach(HDC hDC);
    HDC Detach();
    COLORREF SetTextColor(COLORREF crColor);
    virtual int DrawText(LPCTSTR lpszString, int nCount, LPRECT lpRect, UINT nFormat);
    int DrawText(const CString& str, LPRECT lpRect, UINT nFormat);
    void FillSolidRect(LPCRECT lpRect, COLORREF clr);
    void FillSolidRect(int x, int y, int cx, int cy, COLORREF clr);
    BOOL LineTo(int x, int y);
    BOOL LineTo(POINT point);
    CPoint MoveTo(int x, int y);
    CPoint MoveTo(POINT point);
    COLORREF SetBkColor(COLORREF crColor);
    COLORREF GetBkColor() const;
    COLORREF GetTextColor() const;
    BOOL DrawFrameControl(LPRECT lpRect, UINT nType, UINT nState);
    int SetPolyFillMode(int nPolyFillMode);
    int GetPolyFillMode() const;
    BOOL DeleteDC();
    int GetMapMode() const;
    // The mapping-mode extents/origins CMemDC mirrors from the DC it wraps.
    CSize GetWindowExt() const;
    CSize GetViewportExt() const;
    CPoint GetWindowOrg() const;
    CPoint GetViewportOrg() const;
    virtual CSize SetWindowExt(int cx, int cy);
    CSize SetWindowExt(SIZE size);
    virtual CSize SetViewportExt(int cx, int cy);
    CSize SetViewportExt(SIZE size);
    virtual CPoint SetWindowOrg(int x, int y);
    CPoint SetWindowOrg(POINT point);
    virtual CPoint SetViewportOrg(int x, int y);
    CPoint SetViewportOrg(POINT point);
    virtual int SelectClipRgn(CRgn* pRgn);
    int SelectClipRgn(CRgn* pRgn, int nMode);
    int SetBkMode(int nBkMode);
    BOOL CreateCompatibleDC(CDC* pDC);
    HDC GetSafeHdc();
    BOOL BitBlt(int x, int y, int nWidth, int nHeight, CDC* pSrcDC, int xSrc, int ySrc, DWORD dwRop);
    CSize GetTextExtent(LPCTSTR lpszString, int nCount);
    CSize GetTextExtent(const CString& str);
    BOOL TextOut(int x, int y, LPCTSTR lpszString, int nCount);
    BOOL TextOut(int x, int y, const CString& str);
    BOOL DrawEdge(LPRECT lpRect, UINT nEdge, UINT nFlags);
    UINT SetTextAlign(UINT nFlags);
    int GetDeviceCaps(int nIndex);
    void FrameRect(LPCRECT lpRect, CBrush* pBrush);
    void DrawFocusRect(LPCRECT lpRect);
    int SetROP2(int nDrawMode);
    int ExcludeClipRect(int x1, int y1, int x2, int y2);
    int ExcludeClipRect(LPCRECT lpRect);
    BOOL Rectangle(int x1, int y1, int x2, int y2);
    BOOL Rectangle(LPCRECT lpRect);
    CPalette* SelectPalette(CPalette* pPalette, BOOL bForceBackground);
    // TEXTMETRIC, not `struct tagTEXTMETRIC`: under UNICODE the real name
    // resolves to tagTEXTMETRICW, so the bare tag named a different type
    // than the one eMule actually passes.
    BOOL GetTextMetrics(TEXTMETRIC* lpMetrics) const;
    int GetClipBox(LPRECT lpRect);
    // Both overloads below fixed/added during the FRONTEND/GDI blind-spot
    // pass (see ../../mfc_scan_srchybrid.md addendum): real eMule usage
    // (TrayMenuBtn.cpp:114, TrayMenuBtn.cpp:105, ListBoxST.cpp:252) passes
    // a CBrush* (e.g. "(CBrush*)NULL"), not an HBRUSH, as the trailing
    // parameter — the text overload's last-parameter type was wrong, and
    // the HICON overload was entirely missing (verified against
    // Microsoft Learn's CDC::DrawState page: the CBrush*-taking overloads
    // are the ones with no matching HBITMAP overload actually used here).
    BOOL DrawState(CPoint pt, CSize size, LPCTSTR lpszText, UINT nFlags, BOOL bPrefixText = TRUE, int nTextLen = 0, CBrush* pBrush = nullptr);
    BOOL DrawState(CPoint pt, CSize size, HICON hIcon, UINT nFlags, CBrush* pBrush = nullptr);
    UINT RealizePalette();
    BOOL Polygon(LPPOINT lpPoints, int nCount);
    int SetMapMode(int nMapMode);
    CGdiObject* SelectStockObject(int nIndex);
    COLORREF SetPixel(int x, int y, COLORREF crColor);
    COLORREF SetPixel(POINT point, COLORREF crColor);
    COLORREF GetPixel(int x, int y);
    COLORREF GetPixel(POINT point);
    long TabbedTextOut(int x, int y, LPCTSTR lpszString, int nCount, int nTabPositions, const int* lpnTabStopPositions, int nTabOrigin);
    BOOL DrawIcon(int x, int y, HICON hIcon);
    BOOL DrawIcon(POINT point, HICON hIcon);
    void DPtoLP(LPPOINT lpPoints, int nCount = 1);
    void DPtoLP(LPRECT lpRect);
    void DPtoLP(LPSIZE lpSize);
    int SaveDC();
    BOOL RestoreDC(int nSavedDC);
    void Draw3dRect(LPCRECT lpRect, COLORREF clrTopLeft, COLORREF clrBottomRight);
    void Draw3dRect(int x, int y, int cx, int cy, COLORREF clrTopLeft, COLORREF clrBottomRight);
    UINT GetTextAlign();
    CSize GetOutputTextExtent(LPCTSTR lpszString, int nCount);
    CSize GetOutputTextExtent(const CString& str);
    BOOL IsPrinting();
    void FillRect(LPCRECT lpRect, CBrush* pBrush);
    void LPtoDP(LPPOINT lpPoints, int nCount = 1);
    void LPtoDP(LPRECT lpRect);
    void LPtoDP(LPSIZE lpSize);
    BOOL SetPixelV(int x, int y, COLORREF crColor);
    BOOL SetPixelV(POINT point, COLORREF crColor);
    // Bounds accumulation, which eMule's CMemoryDC resets before drawing.
    UINT SetBoundsRect(LPCRECT lpRectBounds, UINT flags);
    UINT GetBoundsRect(LPRECT lpRectBounds, UINT flags);
    BOOL ScrollDC(int dx, int dy, LPCRECT lpRectScroll, LPCRECT lpRectClip,
                  CRgn* pRgnUpdate, LPRECT lpRectUpdate);
    // The CString-taking form, alongside the count-based one above.
    CSize TabbedTextOut(int x, int y, const CString& str, int nTabPositions,
                        int* lpnTabStopPositions, int nTabOrigin);
};

// Device-context helpers (header afxwin.h). Real MFC derives each from CDC
// and wires up/tears down the DC in ctor/dtor; eMule only ever constructs
// them from a CWnd* ("CPaintDC dc(this);") and uses them as a CDC, so a
// declaration-only CWnd* constructor is all that is needed here.
class CPaintDC : public CDC
{
public:
    explicit CPaintDC(CWnd* pWnd);
};

class CClientDC : public CDC
{
public:
    explicit CClientDC(CWnd* pWnd);
};

class CWindowDC : public CDC
{
public:
    explicit CWindowDC(CWnd* pWnd);
};

// ---------------------------------------------------------------------
// CWnd — base of all windows/controls (header afxwin.h). Methods below
// are only the ones actually called on CWnd*/CWnd& in eMule/srchybrid
// (not the full real-MFC surface) — see ../../mfc_scan_srchybrid.md.
// ---------------------------------------------------------------------
// These are real Win32 *macros* (winuser.h), not just type names: on
// _WIN32 <windows.h> was already #included above in this same file, so
// by the time the preprocessor reaches these lines the macro is already
// live and would silently rewrite our own declaration's token (e.g.
// "constexpr UINT RDW_INVALIDATE = ..." becomes "constexpr UINT 0x0001 =
// ..." -- a syntax error) unless guarded with #ifndef rather than
// #ifdef _WIN32 (the two aren't equivalent for macros).
#ifndef RDW_INVALIDATE
constexpr UINT RDW_INVALIDATE = 0x0001;
#endif
#ifndef RDW_ERASE
constexpr UINT RDW_ERASE = 0x0004;
#endif
#ifndef RDW_UPDATENOW
constexpr UINT RDW_UPDATENOW = 0x0100;
#endif

// See CWnd::GetNextWindow below: winuser.h makes this name a two-argument
// macro, which would rewrite every member call into an unparsable one.
#undef GetNextWindow

class CWnd : public CCmdTarget
{
public:
    BOOL EnableWindow(BOOL bEnable = TRUE);
    BOOL ShowWindow(int nCmdShow);
    void GetWindowRect(LPRECT lpRect) const;
    virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle,
                         const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = nullptr);
    // The window handle, a public member in real MFC. eMule reads it
    // constantly, both as a validity test and to hand the raw handle to
    // Win32 calls.
    HWND m_hWnd = nullptr;
    operator HWND() const { return m_hWnd; }

    UINT_PTR SetTimer(UINT_PTR nIDEvent, UINT nElapse,
                       void(CALLBACK* lpfnTimer)(HWND, UINT, UINT_PTR, DWORD) = nullptr);
    BOOL KillTimer(UINT_PTR nIDEvent);
    UINT IsDlgButtonChecked(int nIDButton) const;

    LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
    // Addresses a child control by id instead of by CWnd. eMule relies on
    // the trailing defaults (`SendDlgItemMessage(IDC_IP, EM_SETREADONLY,
    // TRUE)` passes no lParam).
    LRESULT SendDlgItemMessage(int nID, UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
    // The message being handled right now; static because it is thread
    // state, not window state.
    static const MSG* GetCurrentMessage();
    void SetWindowText(LPCTSTR lpszString);
    void MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint = TRUE);
    void MoveWindow(LPCRECT lpRect, BOOL bRepaint = TRUE);
    HWND Detach();
    BOOL Attach(HWND hWndNew);
    CWnd* SetFocus();
    void SetRedraw(BOOL bRedraw = TRUE);
    HWND GetSafeHwnd() const;
    void Invalidate(BOOL bErase = TRUE);
    BOOL ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);
    HICON SetIcon(HICON hIcon, BOOL bBigIcon);
    BOOL IsWindowVisible() const;
    virtual BOOL DestroyWindow();
    BOOL SetWindowPos(const CWnd* pWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags);
    void UpdateWindow();
    void ScreenToClient(LPPOINT lpPoint) const;
    void ScreenToClient(LPRECT lpRect) const;
    BOOL ModifyStyleEx(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);
    DWORD GetStyle() const;
    void SetDlgItemText(int nID, LPCTSTR lpszString);
    int GetWindowTextLength() const;
    void GetClientRect(LPRECT lpRect) const;
    BOOL PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
    void ClientToScreen(LPPOINT lpPoint) const;
    void ClientToScreen(LPRECT lpRect) const;
    void GetWindowText(CString& rString) const;
    int GetWindowText(LPTSTR lpszStringBuf, int nMaxCount) const;
    BOOL SetForegroundWindow();
    BOOL BringWindowToTop();
    BOOL IsWindowEnabled() const;
    CWnd* GetParent() const;
    CWnd* GetDlgItem(int nID) const;
    CMenu* GetMenu() const;
    BOOL SetMenu(CMenu* pMenu);
    CMenu* GetSystemMenu(BOOL bRevert) const;
    // The dialog-item helpers. Each one that is missing does not simply
    // fail to resolve: the unqualified call falls through to the global
    // Win32 function of the same name, which then rejects the arity
    // because it wants a leading HWND.
    int GetDlgCtrlID() const;
    // Window traversal and style queries. GetFocus/GetCapture are static
    // in real MFC and return CWnd*, not a raw HWND -- eMule assigns the
    // result straight to a CWnd* (`CWnd *pWndFocus = GetFocus();`).
    static CWnd* GetFocus();
    static CWnd* GetCapture();
    // Same shape as GetFocus/GetCapture, and missing it fails the same
    // silent way the note above describes: the unqualified call falls
    // through to the global Win32 ::FindWindowEx, which returns an HWND
    // where eMule assigns a CWnd* ("CWnd *pWnd = FindWindowEx(
    // GetSafeHwnd(), 0, _T("msctls_updown32"), 0);", TabCtrl.cpp:146).
    static CWnd* FindWindowEx(HWND hwndParent, HWND hwndChildAfter,
                              LPCTSTR lpszClass, LPCTSTR lpszWindow);
    CWnd* GetWindow(UINT nCmd) const;
    CWnd* ChildWindowFromPoint(POINT point) const;
    CWnd* ChildWindowFromPoint(POINT point, UINT nFlags) const;
    BOOL IsIconic() const;
    BOOL IsZoomed() const;
    BOOL FlashWindow(BOOL bInvert);
    BOOL IsChild(const CWnd* pWnd) const;
    CWnd* SetActiveWindow();
    CWnd* GetActiveWindow();
    void MapWindowPoints(CWnd* pwndTo, LPRECT lpRect) const;
    void MapWindowPoints(CWnd* pwndTo, LPPOINT lpPoint, UINT nCount) const;
    int SetWindowRgn(HRGN hRgn, BOOL bRedraw);
    int GetWindowRgn(HRGN hRgn) const;
    // Undocumented on the Learn CWnd page, but eMule calls it unqualified
    // from a dialog member, so MFC declares it somewhere in this chain.
    void PrepareForHelp();
    DWORD GetExStyle() const;
    BOOL GetScrollInfo(int nBar, SCROLLINFO* lpScrollInfo, UINT nMask = 0x17 /*SIF_ALL*/);
    BOOL SetScrollInfo(int nBar, SCROLLINFO* lpScrollInfo, BOOL bRedraw = TRUE);
    BOOL GetWindowPlacement(WINDOWPLACEMENT* lpwndpl) const;
    BOOL SetWindowPlacement(const WINDOWPLACEMENT* lpwndpl);
    BOOL EnableToolTips(BOOL bEnable = TRUE);
    // Routes the current message to the default window procedure.
    LRESULT Default();
    int GetDlgItemText(int nID, LPTSTR lpStr, int nMaxCount) const;
    int GetDlgItemText(int nID, CString& rString) const;
    void SetDlgItemInt(int nID, UINT nValue, BOOL bSigned = TRUE);
    UINT GetDlgItemInt(int nID, BOOL* lpTrans = nullptr, BOOL bSigned = TRUE) const;
    void CheckDlgButton(int nIDButton, UINT nCheck);
    void CheckRadioButton(int nIDFirstButton, int nIDLastButton, int nIDCheckButton);
    // Runs the DDX exchange in either direction.
    BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
    void SetFont(CFont* pFont, BOOL bRedraw = TRUE);
    CFont* GetFont() const;
    CDC* GetDC();
    int ReleaseDC(CDC* pDC);
    BOOL RedrawWindow(LPCRECT lpRectUpdate = nullptr, CRgn* prgnUpdate = nullptr,
                       UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
    CWnd* SetCapture();
    void InvalidateRect(LPCRECT lpRect, BOOL bErase = TRUE);

    // Subclassing: eMule's control classes attach themselves to a window
    // created from the dialog template rather than creating it themselves.
    BOOL SubclassWindow(HWND hWnd);
    BOOL SubclassDlgItem(UINT nID, CWnd* pParent);
    HWND UnsubclassWindow();

    // GetNextWindow is a real winuser.h *macro* taking (hWnd, wCmd), so an
    // unqualified `pWnd->GetNextWindow()` expands to `GetWindow(,)` and
    // fails to parse. Undefined here for the same reason afx.h undefines
    // FindNextFile: keep the member's true name at every later call site.
    CWnd* GetNextWindow(UINT nFlag = 2 /*GW_HWNDNEXT*/) const;
    CWnd* GetTopWindow() const;
    CWnd* GetLastActivePopup() const;
    CWnd* GetTopLevelParent() const;
    CWnd* GetTopLevelFrame() const;
    CWnd* GetTopLevelOwner() const;
    // The owner is the window notifications go to, which is not always the
    // parent (real MFC keeps the distinction; eMule relies on it for its
    // floating bars).
    CWnd* GetOwner() const;
    void SetOwner(CWnd* pOwnerWnd);
    CDC* GetWindowDC();
    void CenterWindow(CWnd* pAlternateOwner = nullptr);
    int GetClassName(LPTSTR lpszClassName, int nMaxCount) const;

    // -----------------------------------------------------------------
    // Everything below was added during the FRONTEND/GDI blind-spot pass
    // (see ../../mfc_scan_srchybrid.md addendum): a plain textual
    // ".Method("/"->Method(" scan cannot see (a) static methods, which
    // are only ever called as "CWnd::Method(...)", or (b) qualified
    // super-calls like "CWnd::OnDestroy()" made by a derived class's own
    // override to reach the base behavior — both are pervasive in
    // eMule/srchybrid. Signatures verified against Microsoft Learn's
    // CWnd Class reference page.
    // -----------------------------------------------------------------
    static CWnd* FromHandle(HWND hWnd);
    static CWnd* FromHandlePermanent(HWND hWnd);
    static CWnd* GetDesktopWindow();
    static CWnd* WindowFromPoint(POINT point);

    virtual BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName,
                           DWORD dwStyle, int x, int y, int nWidth, int nHeight,
                           HWND hWndParent, HMENU nIDorHMenu, LPVOID lpParam = nullptr);
    virtual BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName,
                           DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID,
                           LPVOID lpParam = nullptr);

    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual void PreSubclassWindow();
    virtual void PostNcDestroy();
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    virtual void DoDataExchange(CDataExchange* pDX);
    // INT_PTR, not int: eMule's CMuleStatusBarCtrl overrides this and a
    // narrower return type is not a legal override (C2555).
    virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
    virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
    virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
    virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

    // WM_* message handlers actually reached via a qualified super-call
    // somewhere in eMule/srchybrid (e.g. CDialog::OnInitDialog() calling
    // through to CWnd, CStatic::OnPaint(), CTreeCtrl::OnMouseWheel(), ...).
    // Real MFC declares essentially all of these directly on CWnd, which
    // is what makes such super-calls valid regardless of which derived
    // class in the hierarchy actually names them in the call.
    virtual void OnPaint();
    virtual void OnDestroy();
    virtual void OnClose();
    virtual int OnCreate(LPCREATESTRUCT lpCreateStruct);
    virtual void OnSysColorChange();
    virtual BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    virtual void OnContextMenu(CWnd* pWnd, CPoint point);
    virtual void OnTimer(UINT_PTR nIDEvent);
    virtual void OnMouseMove(UINT nFlags, CPoint point);
    virtual BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    virtual void OnLButtonUp(UINT nFlags, CPoint point);
    virtual void OnLButtonDown(UINT nFlags, CPoint point);
    virtual void OnLButtonDblClk(UINT nFlags, CPoint point);
    virtual void OnRButtonDown(UINT nFlags, CPoint point);
    virtual void OnMButtonUp(UINT nFlags, CPoint point);
    virtual void OnNcLButtonDblClk(UINT nHitTest, CPoint point);
    virtual void OnNcDestroy();
    virtual void OnSize(UINT nType, int cx, int cy);
    virtual HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    virtual void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    virtual void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    virtual BOOL OnEraseBkgnd(CDC* pDC);
    virtual void OnSetFocus(CWnd* pOldWnd);
    virtual void OnKillFocus(CWnd* pNewWnd);
    virtual void OnActivateApp(BOOL bActive, DWORD dwThreadID);
    virtual BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    virtual BOOL OnQueryNewPalette();
    virtual void OnPaletteChanged(CWnd* pFocusWnd);
    virtual void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    virtual void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    virtual void OnSysCommand(UINT nID, LPARAM lParam);
    virtual void OnShowWindow(BOOL bShow, UINT nStatus);
    virtual void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    virtual UINT OnGetDlgCode();
    virtual void OnCaptureChanged(CWnd* pWnd);
    virtual void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu);
    virtual void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    virtual void OnCancelMode();
    afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
    // afx_msg, not virtual: real MFC dispatches these through the message
    // map, and Learn declares them exactly this way.
    afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    virtual void OnNcPaint();
    virtual BOOL OnNcActivate(BOOL bActive);
    virtual LRESULT OnNcHitTest(CPoint point);
    virtual void OnNcLButtonDown(UINT nHitTest, CPoint point);
    virtual void OnNcRButtonDown(UINT nHitTest, CPoint point);
    virtual void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
    virtual LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);
    virtual BOOL OnQueryEndSession();
    virtual void OnEndSession(BOOL bEnding);
    virtual void OnMove(int x, int y);
    virtual void OnEnable(BOOL bEnable);
    virtual void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
};

class CDialog : public CWnd
{
public:
    // Every eMule dialog passes its IDD (and usually a parent) straight to
    // the base, e.g. `CAddSourceDlg::CAddSourceDlg(CWnd *pParent)
    // : CDialog(CAddSourceDlg::IDD, pParent)`. The template can be named by
    // resource id or by string, and the default constructor exists for the
    // dialogs created through DYNCREATE.
    CDialog();
    explicit CDialog(UINT nIDTemplate, CWnd* pParentWnd = nullptr);
    explicit CDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd = nullptr);
    virtual INT_PTR DoModal();
    void EndDialog(int nResult);
    virtual BOOL Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd = nullptr);
    virtual BOOL Create(UINT nIDTemplate, CWnd* pParentWnd = nullptr);
    virtual BOOL OnInitDialog();

protected:
    virtual void OnOK();
    virtual void OnCancel();
};

extern const CRect rectDefault;
#ifndef WS_OVERLAPPEDWINDOW
constexpr DWORD WS_OVERLAPPEDWINDOW = 0x00CF0000;
#endif

class CControlBar; // real header afxext.h; only a pointer parameter here

class CFrameWnd : public CWnd
{
public:
    virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName,
                         DWORD dwStyle = WS_OVERLAPPEDWINDOW, const RECT& rect = rectDefault,
                         CWnd* pParentWnd = nullptr, LPCTSTR lpszMenuName = nullptr,
                         DWORD dwExStyle = 0, CCreateContext* pContext = nullptr);
    virtual void RecalcLayout(BOOL bNotify = TRUE);
    // CControlBar lives in afxext.h (which includes us, not vice versa), so
    // forward-declared just above for this pointer parameter.
    void ShowControlBar(CControlBar* pBar, BOOL bShow, BOOL bDelay);
    CWnd* CreateView(CCreateContext* pContext, UINT nID = 0xE900 /*AFX_IDW_PANE_FIRST*/);

    // Docking. The frame is the dock site: it decides which edges accept
    // bars (EnableDocking), performs the docking (DockControlBar) and
    // persists the layout to the registry (Load/SaveBarState). Declared
    // here rather than in afxext.h because real MFC declares them on
    // CFrameWnd, whose definition lives in this header.
    void EnableDocking(DWORD dwDockStyle);
    void DockControlBar(CControlBar* pBar, UINT nDockBarID = 0, LPCRECT lpRect = nullptr);
    void FloatControlBar(CControlBar* pBar, CPoint point, DWORD dwStyle = 0x2000L /*CBRS_ALIGN_TOP*/);
    void LoadBarState(LPCTSTR lpszProfileName);
    void SaveBarState(LPCTSTR lpszProfileName) const;
    CControlBar* GetControlBar(UINT nID);
    virtual BOOL OnCreateClient(CREATESTRUCT* lpcs, CCreateContext* pContext);
};

// ---------------------------------------------------------------------
// CView — base of the document/view classes (header afxwin.h, hierarchy
// CObject -> CCmdTarget -> CWnd -> CView). eMule/srchybrid uses no
// document/view architecture at all: it only needs CView as the base of
// CFormView (afxext.h), which CTransferWnd/CSearchResultsWnd derive from
// to host a dialog template inside the main frame. So the document half
// (GetDocument/OnUpdate/OnDraw against CDocument) is deliberately absent
// -- every message handler those two override already comes from CWnd.
// ---------------------------------------------------------------------
class CView : public CWnd
{
public:
    virtual void OnInitialUpdate();
};

// CScrollView (header afxwin.h in real MFC, hierarchy
// CObject -> CCmdTarget -> CWnd -> CView -> CScrollView). Here purely to
// keep the inheritance chain faithful for CFormView (afxext.h): eMule
// calls none of its scrolling API, so none is declared.
class CScrollView : public CView
{
};

// ---------------------------------------------------------------------
// CScrollBar (header afxwin.h, deriva da CWnd). Reaches eMule as the
// third parameter of OnHScroll/OnVScroll, where it needs to be complete.
// ---------------------------------------------------------------------
class CScrollBar : public CWnd
{
public:
    int SetScrollPos(int nPos, BOOL bRedraw = TRUE);
    int GetScrollPos() const;
    void SetScrollRange(int nMinPos, int nMaxPos, BOOL bRedraw = TRUE);
    void GetScrollRange(int* lpMinPos, int* lpMaxPos) const;
    BOOL EnableScrollBar(UINT nArrowFlags = 0);
    void ShowScrollBar(BOOL bShow = TRUE);
};

class CStatic : public CWnd
{
public:
    virtual BOOL Create(LPCTSTR lpszText, DWORD dwStyle, const RECT& rect,
                         CWnd* pParentWnd, UINT nID = 0xffff);
    HBITMAP SetBitmap(HBITMAP hBitmap);
    HBITMAP GetBitmap() const;
    HICON SetIcon(HICON hIcon);
    HICON GetIcon() const;
};

class CEdit : public CWnd
{
public:
    // The edit control's formatting rectangle.
    void SetRect(LPCRECT lpRect);
    void SetRectNP(LPCRECT lpRect);
    void GetRect(LPRECT lpRect) const;
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    void SetSel(DWORD dwSelection, BOOL bNoScroll = FALSE);
    void SetSel(int nStartChar, int nEndChar, BOOL bNoScroll = FALSE);
    void LimitText(int nChars = 0);
    void SetLimitText(UINT nMax);
    UINT GetLimitText() const;
    BOOL ShowBalloonTip(LPCTSTR lpszTitle, LPCTSTR lpszText, int ttiIcon = 0);
    DWORD GetSel() const;
    void GetSel(int& nStartChar, int& nEndChar) const;
    void ReplaceSel(LPCTSTR lpszNewText, BOOL bCanUndo = FALSE);
};

class CListBox : public CWnd
{
public:
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    // Hit-testing by point; the high word of the result says whether the
    // point actually fell inside the item.
    UINT ItemFromPoint(POINT pt, BOOL& bOutside) const;
    int GetCount() const;
    int GetCurSel() const;
    int SetCurSel(int nSelect);
    int AddString(LPCTSTR lpszItem);
    DWORD_PTR GetItemData(int nIndex) const;
    int SetItemData(int nIndex, DWORD_PTR dwItemData);
    void ResetContent();
    int GetText(int nIndex, LPTSTR lpszBuffer) const;
    void GetText(int nIndex, CString& rString) const;
    int DeleteString(UINT nIndex);
    // Added during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum): ListBoxST.cpp calls these
    // exclusively as "CListBox::GetItemDataPtr(...)"/etc. (qualified
    // super-calls from CListBoxST : public CListBox), invisible to the
    // original ".Method("/"->Method(" scan, which had marked all three
    // as "0 occurrences". Signatures verified against Microsoft Learn.
    void* GetItemDataPtr(int nIndex) const;
    int SetItemDataPtr(int nIndex, void* pData);
    int InsertString(int nIndex, LPCTSTR lpszItem);
    int GetTopIndex() const;
    int SetTopIndex(int nIndex);
    int GetItemRect(int nIndex, LPRECT lpRect) const;
    int GetItemHeight(int nIndex) const;
    int SetItemHeight(int nIndex, UINT cyItemHeight);
};

class CComboBox : public CWnd
{
public:
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    int GetCount() const;
    int GetCurSel() const;
    int SetCurSel(int nSelect);
    int AddString(LPCTSTR lpszString);
    DWORD_PTR GetItemData(int nIndex) const;
    int SetItemData(int nIndex, DWORD_PTR dwItemData);
    // The same per-item slot as GetItemData, typed as a pointer.
    void* GetItemDataPtr(int nIndex) const;
    int SetItemDataPtr(int nIndex, void* pData);
    void SetHorizontalExtent(UINT nExtent);
    UINT GetHorizontalExtent() const;
    int SetDroppedWidth(UINT nWidth);
    int GetDroppedWidth() const;
    void ResetContent();
    int GetLBText(int nIndex, LPTSTR lpszText) const;
    void GetLBText(int nIndex, CString& rString) const;
    int DeleteString(UINT nIndex);
    int SelectString(int nStartAfter, LPCTSTR lpszString) const;
};

class CButton : public CWnd
{
public:
    virtual BOOL Create(LPCTSTR lpszCaption, DWORD dwStyle, const RECT& rect,
                         CWnd* pParentWnd, UINT nID);
    HICON SetIcon(HICON hIcon);
    UINT GetState() const;
    void SetState(BOOL bHighlight);
    int GetCheck() const;
    void SetCheck(int nCheck);
    HBITMAP SetBitmap(HBITMAP hBitmap);
    HBITMAP GetBitmap() const;
};

// CMenu (header afxwin.h, deriva da CObject — NOT CWnd/CCmdTarget)
class CMenu : public CObject
{
public:
    // eMule tests a menu for validity directly (`if (menu) ...`) and hands
    // it to raw Win32 calls, both of which real MFC supports through the
    // handle member and its implicit conversion.
    HMENU m_hMenu = nullptr;
    operator HMENU() const { return m_hMenu; }

    BOOL AppendMenu(UINT nFlags, UINT_PTR nIDNewItem = 0, LPCTSTR lpszNewItem = nullptr);
    BOOL AppendMenu(UINT nFlags, UINT_PTR nIDNewItem, const CBitmap* pBmp);
    UINT EnableMenuItem(UINT nIDEnableItem, UINT nEnable);
    BOOL DestroyMenu();
    HMENU Detach();
    BOOL Attach(HMENU hMenu);
    BOOL CreateMenu();
    BOOL CreatePopupMenu();
    BOOL TrackPopupMenu(UINT nFlags, int x, int y, CWnd* pWnd, LPCRECT lpRect = nullptr);
    UINT CheckMenuItem(UINT nIDCheckItem, UINT nCheck);
    BOOL SetDefaultItem(UINT uItem, BOOL fByPos = FALSE);
    BOOL RemoveMenu(UINT nPosition, UINT nFlags);
    UINT GetMenuItemCount() const;
    BOOL InsertMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem = 0, LPCTSTR lpszNewItem = nullptr);
    BOOL LoadMenu(LPCTSTR lpszResourceName);
    BOOL LoadMenu(UINT nIDResource);
    CMenu* GetSubMenu(int nPos) const;
    BOOL ModifyMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem = 0, LPCTSTR lpszNewItem = nullptr);
    // Radio-style check marks (only one item in the range ticked, with a
    // bullet instead of a check), and the MENUITEMINFO-based insert.
    BOOL CheckMenuRadioItem(UINT nIDFirst, UINT nIDLast, UINT nIDItem, UINT nFlags);
    BOOL InsertMenuItem(UINT uItem, LPMENUITEMINFO lpMenuItemInfo, BOOL fByPos = FALSE);
    BOOL GetMenuItemInfo(UINT uItem, LPMENUITEMINFO lpMenuItemInfo, BOOL fByPos = FALSE) const;
    BOOL SetMenuItemInfo(UINT uItem, LPMENUITEMINFO lpMenuItemInfo, BOOL fByPos = FALSE);
    UINT GetMenuItemID(int nPos) const;
    UINT GetMenuState(UINT nID, UINT nFlags) const;
    BOOL GetMenuInfo(LPMENUINFO lpcmi) const;
    BOOL SetMenuInfo(LPCMENUINFO lpcmi);

    // Added during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum): CTitledMenu (TitledMenu.h),
    // an owner-draw CMenu subclass, overrides MeasureItem and calls
    // "CMenu::MeasureItem(lpMIS)" (TitledMenu.cpp:132) for the default
    // behavior — a qualified super-call, invisible to the original
    // ".Method("/"->Method(" scan. DrawItem is also overridden there but
    // never super-called, so it is intentionally NOT added (same
    // "framework-invoked only, no super-call found" rule already applied
    // elsewhere in this document, e.g. CDialog::OnOK originally).
    virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
};

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif

class CDocument : public CCmdTarget
{
public:
    // Added during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum): EmuleDlg.h:321 overrides and
    // super-calls "CDocument::OnNewDocument()" — same pattern already
    // noted for CDocument in the BACKEND group ("probably an
    // architectural container without real application logic").
    virtual BOOL OnNewDocument();
};

// CControlBar and CDialogBar are NOT declared here: they really belong to
// the afxext.h header (per Microsoft Learn), see afxext.h.
// CPropertyPage and CPropertySheet are NOT declared here: they really
// belong to the afxdlgs.h header (per Microsoft Learn), see afxdlgs.h.
// CImageList/CTreeCtrl/CListCtrl/CRichEditCtrl/CTabCtrl/CToolBarCtrl/
// CStatusBarCtrl/CToolTipCtrl are NOT declared here: they belong to
// afxcmn.h. COleDropTarget belongs to afxole.h. CDHtmlDialog belongs to
// afxdhtml.h.

// ---------------------------------------------------------------------
// Message-map demarcation macros (per Microsoft Learn: header afxwin.h).
// The "entry" macros (ON_COMMAND, ON_MESSAGE, ON_CONTROL, ON_NOTIFY, ...)
// are NOT declared here: they really belong to the afxmsg_.h header,
// which afxwin.h includes below (as in real MFC: a single
// #include "afxwin.h" also exposes the ON_* macros).
// DECLARE_MESSAGE_MAP is defined much earlier in this header, right after
// the AFX_MSGMAP structures, because CCmdTarget itself uses it.
//
// PTM ("pointer to member") is real MFC's own name for the pair below,
// and it is not cosmetic. A hand-written entry forms a pointer to member
// WITHOUT the address-of operator:
//   static_cast<LRESULT (AFX_MSG_CALL CWnd::*)(void)>(_OnThemeChanged)
// which MSVC accepts only as an extension, and diagnoses as C4867 --
// "error C4867", not a warning, so it stops the compile. Real MFC wraps
// every message map in exactly this push/disable/pop, which is why the
// same eMule sources build against it; without the pair, ButtonsTabCtrl
// and ClosableTabCtrl still failed after the maps themselves were made
// real. 4640 is the second one real MFC silences here (the function-local
// static the map array is).
#ifdef _MSC_VER
#define PTM_WARNING_DISABLE __pragma(warning(push)) __pragma(warning(disable : 4867))
#define PTM_WARNING_RESTORE __pragma(warning(pop))
#define AFX_MSGMAP_WARNING_DISABLE __pragma(warning(push)) __pragma(warning(disable : 4640))
#define AFX_MSGMAP_WARNING_RESTORE __pragma(warning(pop))
#else
#define PTM_WARNING_DISABLE
#define PTM_WARNING_RESTORE
#define AFX_MSGMAP_WARNING_DISABLE
#define AFX_MSGMAP_WARNING_RESTORE
#endif
//
// BEGIN/END_MESSAGE_MAP open and close the entry array. They are
// reproduced from real MFC rather than stubbed because eMule writes
// entries by hand and they have to land inside that array -- see the
// long note next to AFX_MSGMAP above. The two typedefs are not
// incidental: ThisClass is what makes an unqualified handler name in a
// hand-written entry resolve (the array lives inside a member function
// of the class), and TheBaseClass is what END_MESSAGE_MAP links the
// base map through.
#define BEGIN_MESSAGE_MAP(theClass, baseClass)                             \
    PTM_WARNING_DISABLE                                                    \
    const AFX_MSGMAP* theClass::GetMessageMap() const                      \
        { return GetThisMessageMap(); }                                    \
    const AFX_MSGMAP* PASCAL theClass::GetThisMessageMap()                 \
    {                                                                      \
        typedef theClass ThisClass;                                        \
        typedef baseClass TheBaseClass;                                    \
        AFX_MSGMAP_WARNING_DISABLE                                         \
        static const AFX_MSGMAP_ENTRY _messageEntries[] =                  \
        {

// The template form, for a message map on a class template. eMule needs
// it for CDialogMinTrayBtn<BASE> (DialogMinTrayBtn.cpp:98,
// "BEGIN_TEMPLATE_MESSAGE_MAP(CDialogMinTrayBtn, BASE, BASE)"): the
// definitions have to be templates themselves, and ThisClass has to name
// the specialization, not the bare template.
#define BEGIN_TEMPLATE_MESSAGE_MAP(theClass, type_name, baseClass)         \
    PTM_WARNING_DISABLE                                                    \
    template <typename type_name>                                          \
    const AFX_MSGMAP* theClass<type_name>::GetMessageMap() const           \
        { return GetThisMessageMap(); }                                    \
    template <typename type_name>                                          \
    const AFX_MSGMAP* PASCAL theClass<type_name>::GetThisMessageMap()      \
    {                                                                      \
        typedef theClass<type_name> ThisClass;                             \
        typedef baseClass TheBaseClass;                                    \
        AFX_MSGMAP_WARNING_DISABLE                                         \
        static const AFX_MSGMAP_ENTRY _messageEntries[] =                  \
        {

// Closes both forms. The trailing element is the terminator real MFC's
// dispatcher scans for; it is also what keeps the array non-empty when
// every ON_* entry in between expanded to nothing.
#define END_MESSAGE_MAP()                                                  \
            { 0, 0, 0, 0, AfxSig_end, (AFX_PMSG)0 }                        \
        };                                                                 \
        AFX_MSGMAP_WARNING_RESTORE                                         \
        static const AFX_MSGMAP messageMap =                               \
            { &TheBaseClass::GetThisMessageMap, &_messageEntries[0] };     \
        return &messageMap;                                                \
    }                                                                      \
    PTM_WARNING_RESTORE

// ---------------------------------------------------------------------
// CImageList (header afxwin.h in real MFC, derives from CObject — NOT
// CWnd). Placed after the GDI classes it references (CBitmap/CDC) and the
// HIMAGELIST/IMAGEINFO stand-ins above.
// ---------------------------------------------------------------------
class CImageList : public CObject
{
public:
    // The wrapped handle, public in real MFC; eMule tests it directly
    // (`piml == NULL || piml->m_hImageList == NULL`).
    HIMAGELIST m_hImageList = nullptr;

    // Real MFC converts implicitly to the raw handle and wraps one back
    // up; eMule passes a CImageList straight to APIs taking HIMAGELIST.
    operator HIMAGELIST() const { return m_hImageList; }
    static CImageList* FromHandle(HIMAGELIST hImageList);

    BOOL DeleteImageList();
    int Add(CBitmap* pbmImage, CBitmap* pbmMask);
    int Add(CBitmap* pbmImage, COLORREF crMask);
    int Add(HICON hIcon);
    BOOL Create(int cx, int cy, UINT nFlags, int nInitial, int nGrow);
    BOOL Create(UINT nBitmapID, int cx, int nGrow, COLORREF crMask);
    BOOL Create(LPCTSTR lpszBitmapID, int cx, int nGrow, COLORREF crMask);
    BOOL Create(CImageList& imagelist1, int nImage1, CImageList& imagelist2, int nImage2, int dx, int dy);
    BOOL Create(CImageList* pImageList);
    BOOL Replace(int nImage, CBitmap* pbmImage, CBitmap* pbmMask);
    int Replace(int nImage, HICON hIcon);
    BOOL Draw(CDC* pDC, int nImage, POINT pt, UINT nStyle);
    BOOL SetOverlayImage(int nImage, int nOverlay);
    int GetImageCount() const;
    BOOL Remove(int nImage);
    BOOL Read(CArchive* pArchive);
    BOOL Write(CArchive* pArchive);
    COLORREF SetBkColor(COLORREF cr);
    COLORREF GetBkColor() const;
    HICON ExtractIcon(int nImage);
    HIMAGELIST GetSafeHandle() const;
    BOOL Attach(HIMAGELIST hImageList);
    HIMAGELIST Detach();

    // Added during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum): these are all *static*
    // methods, only ever called as "CImageList::Method(...)"
    // (SharedDirsTreeCtrl.cpp:1083-1140) — structurally invisible to the
    // original ".Method("/"->Method(" scan, which only sees instance
    // calls. BeginDrag/DragEnter were originally left out on the strength
    // of that same scan; the compile check found real call sites for both,
    // so the drag API is complete here.
    BOOL BeginDrag(int nImage, CPoint ptHotSpot);
    static BOOL DragEnter(CWnd* pWndLock, CPoint point);
    BOOL GetImageInfo(int nImage, IMAGEINFO* pImageInfo) const;
    BOOL DrawEx(CDC* pDC, int nImage, POINT pt, SIZE sz, COLORREF clrBk,
                COLORREF clrFg, UINT nStyle);
    static BOOL DragShowNolock(BOOL bShow);
    static BOOL DragMove(CPoint pt);
    static BOOL DragLeave(CWnd* pWndLock);
    static void EndDrag();
};

#include "afxmsg_.h"

// ---------------------------------------------------------------------
// Global Afx* functions (header afxwin.h)
// ---------------------------------------------------------------------
CWinThread* AfxBeginThread(AFX_THREADPROC pfnThreadProc, void* pParam,
                            int nPriority = 0 /*THREAD_PRIORITY_NORMAL*/,
                            UINT nStackSize = 0, DWORD dwCreateFlags = 0,
                            SECURITY_ATTRIBUTES* lpSecurityAttrs = nullptr);
CWinThread* AfxBeginThread(CRuntimeClass* pThreadClass,
                            int nPriority = 0, UINT nStackSize = 0,
                            DWORD dwCreateFlags = 0,
                            SECURITY_ATTRIBUTES* lpSecurityAttrs = nullptr);

CWinApp* AfxGetApp();
// Returns HINSTANCE in real MFC. As void* the result could not be passed
// to any Win32 call expecting a module handle (LoadAccelerators, ...)
// without an explicit cast eMule does not write.
HINSTANCE AfxGetInstanceHandle();
HINSTANCE AfxGetResourceHandle();
CWnd* AfxGetMainWnd();
LPCTSTR AfxGetAppName();
int AfxMessageBox(LPCTSTR lpszText, UINT nType = 0, UINT nIDHelp = 0);
// The resource-id form, which real MFC declares alongside the string one.
int AfxMessageBox(UINT nIDPrompt, UINT nType = 0, UINT nIDHelp = (UINT)-1);
// Real MFC provides BOTH char-widths (afx.h): eMule's kademlia/Tag.h passes a
// narrow LPCSTR, other call sites the wide LPCTSTR, so declare both overloads.
BOOL AfxIsValidString(LPCTSTR lpsz, int nLength = -1);
BOOL AfxIsValidString(const char* lpsz, int nLength = -1);
// Registers a window class on the fly, for windows that need a cursor or
// background brush of their own (eMule's CColourPopup, CDownloadQueue).
LPCTSTR AFXAPI AfxRegisterWndClass(UINT nClassStyle, HCURSOR hCursor = nullptr,
                                    HBRUSH hbrBackground = nullptr, HICON hIcon = nullptr);
// The module a resource id should be looked up in: real MFC searches the
// app, then any loaded language DLL, which is exactly why eMule (which
// ships localisation DLLs) calls this instead of AfxGetResourceHandle.
HINSTANCE AFXAPI AfxFindResourceHandle(LPCTSTR lpszName, LPCTSTR lpszType);
BOOL AFXAPI AfxHtmlHelp(HWND hWnd, LPCTSTR szHelpFilePath, UINT nCmd, DWORD_PTR dwData);
// The window procedure MFC installs on every window it owns; eMule
// compares against it to tell its own subclassed windows apart.
LRESULT CALLBACK AfxWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);

// One of the four special CWnd values SetWindowPos takes as
// pWndInsertAfter -- the only one eMule reaches. Real MFC declares them
// as const CWnd objects, not as HWND constants, so that `&wndTopMost`
// has the parameter's type.
extern const CWnd wndTopMost;

// WM_INITIALUPDATE is MFC's own private message (not a Win32 one), sent
// to a view once its frame is fully created.
#ifndef WM_INITIALUPDATE
#define WM_INITIALUPDATE (WM_USER + 0x0364)
#endif
// Base of the id range MFC maps to status-bar prompt strings.
#ifndef HID_BASE_PROMPT
#define HID_BASE_PROMPT 0x00030000UL
#endif

// ---------------------------------------------------------------------
// Nested-interface support for CCmdTarget (real MFC puts these macros in
// afxwin.h too). A class exposes each COM interface it implements as a
// nested XName object whose this-pointer is a known offset from the
// outer object's; METHOD_PROLOGUE recovers the outer object from it.
// eMule uses them for its IDataObject / IServiceProvider /
// IInternetSecurityManager / IDocHostUIHandler implementations.
// ---------------------------------------------------------------------
#define METHOD_PROLOGUE(theClass, localClass)                                  \
    theClass* pThis = ((theClass*)((BYTE*)this - offsetof(theClass, m_x##localClass)))

#define BEGIN_INTERFACE_PART(localClass, baseClass)                            \
    class X##localClass : public baseClass                                     \
    {                                                                          \
    public:                                                                    \
        STDMETHOD_(ULONG, AddRef)();                                           \
        STDMETHOD_(ULONG, Release)();                                          \
        STDMETHOD(QueryInterface)(REFIID iid, LPVOID* ppvObj);

#define END_INTERFACE_PART(localClass)                                         \
    } m_x##localClass;                                                         \
    friend class X##localClass;

#define BEGIN_INTERFACE_MAP(theClass, theBase)
#define INTERFACE_PART(theClass, iid, localClass)
#define END_INTERFACE_MAP()
#define DECLARE_INTERFACE_MAP()

// ---------------------------------------------------------------------
// CWaitCursor — shows the hourglass for as long as the object is alive;
// eMule declares one at the top of its slow operations and lets scope
// exit restore the cursor. No base class in real MFC either.
// ---------------------------------------------------------------------
class CWaitCursor
{
public:
    CWaitCursor();
    ~CWaitCursor();
    void Restore();
};

// ---------------------------------------------------------------------
// CDataExchange — object passed to DoDataExchange (header afxwin.h per
// Microsoft Learn; the DDX_*/DDV_* functions that use it stay in
// afxdd_.h, where real MFC puts them). No base class.
// ---------------------------------------------------------------------
class CDataExchange
{
public:
    CWnd* m_pDlgWnd;
    BOOL m_bSaveAndValidate;

    // Called by every DDX_ routine to resolve a control id to its HWND;
    // eMule's own DDX helpers (CColorButton) call it directly.
    HWND PrepareCtrl(int nIDC);
    HWND PrepareEditCtrl(int nIDC);
    void Fail();
};

// Real MFC's afxwin.h ends by pulling in the DDX_*/DDV_* routines, which
// is why applications only ever include afxwin.h. Placed last because
// afxdd_.h takes CDataExchange (declared just above) by pointer.
#include "afxdd_.h"
// Real MFC exposes the standard resource symbols through the same chain.
#include "afxres.h"
