// afxmsg_.h — message-map "entry" macros (they map a message/command/
// notification to a member function). Per Microsoft Learn, the required
// header for ON_COMMAND/ON_MESSAGE/ON_REGISTERED_MESSAGE/ON_CONTROL is
// afxmsg_.h (the demarcation macros BEGIN/END/DECLARE_MESSAGE_MAP instead
// stay in afxwin.h, where real MFC puts them). Included by afxwin.h itself,
// as in real MFC where a single #include <afxwin.h> also exposes these.
//
// These macros are a faithful MIRROR of real MFC's afxmsg_.h expansions
// (subset: only the entry macros eMule/srchybrid actually uses). Each
// expands to an AFX_MSGMAP_ENTRY aggregate ending in a comma, exactly as
// real MFC does, so that:
//   * eMule's message maps route their handlers identically to real MFC;
//   * eMule's own HAND-WRITTEN entries (its _ON_WM_THEMECHANGED, which
//     spells out AfxSig_l and the (AFX_PMSG)(AFX_PMSGW)(static_cast<...>)
//     cast itself) land in the same array shape and compile unchanged;
//   * the whole thing keeps compiling under MSVC, where forming a
//     pointer-to-member without '&' (the bare-name handler form eMule
//     writes, "ON_COMMAND(ID, OnFoo)") is accepted only as an extension
//     and diagnosed as C4867 -- which BEGIN_MESSAGE_MAP's surrounding
//     PTM_WARNING_DISABLE (in afxwin.h) silences, again as in real MFC.
//
// The AfxSig_* signature tags named below are defined in the AfxSig enum in
// afxwin.h (a faithful mirror of the real one). The dispatcher that reads
// these tags lives in gui/core (implementation, not interface).
#pragma once

/////////////////////////////////////////////////////////////////////////////
// Command notifications for CCmdTarget notifications (real MFC values).
#define CN_COMMAND              0               // void ()
#define CN_UPDATE_COMMAND_UI    ((UINT)(-1))    // void (CCmdUI*)
#define CN_EVENT                ((UINT)(-2))    // OLE event
#define CN_OLECOMMAND           ((UINT)(-3))    // OLE document command
#define CN_OLE_UNREGISTER       ((UINT)(-4))    // OLE unregister
// > 0 are control notifications; < 0 are for MFC's own use.

// The offset added to a message id to route it back to the originating
// control ("message reflection"), as in real MFC.
#define WM_REFLECT_BASE         0xBC00

/////////////////////////////////////////////////////////////////////////////
// Commands and general control notifications.

#define ON_COMMAND(id, memberFxn) \
	{ WM_COMMAND, CN_COMMAND, (WORD)id, (WORD)id, AfxSigCmd_v, \
		static_cast<AFX_PMSG> (memberFxn) },
	// ON_COMMAND(id, OnBar) == ON_CONTROL(0, id, OnBar) == ON_BN_CLICKED(0, id, OnBar)

#define ON_COMMAND_RANGE(id, idLast, memberFxn) \
	{ WM_COMMAND, CN_COMMAND, (WORD)id, (WORD)idLast, AfxSigCmd_RANGE, \
		(AFX_PMSG) \
		(static_cast< void (AFX_MSG_CALL CCmdTarget::*)(UINT) > \
		(memberFxn)) },

// for general controls
#define ON_CONTROL(wNotifyCode, id, memberFxn) \
	{ WM_COMMAND, (WORD)wNotifyCode, (WORD)id, (WORD)id, AfxSigCmd_v, \
		(static_cast< AFX_PMSG > (memberFxn)) },

#define ON_CONTROL_RANGE(wNotifyCode, id, idLast, memberFxn) \
	{ WM_COMMAND, (WORD)wNotifyCode, (WORD)id, (WORD)idLast, AfxSigCmd_RANGE, \
		(AFX_PMSG) \
		(static_cast< void (AFX_MSG_CALL CCmdTarget::*)(UINT) > (memberFxn)) },

// WM_NOTIFY-based notifications.
#define ON_NOTIFY(wNotifyCode, id, memberFxn) \
	{ WM_NOTIFY, (WORD)(int)wNotifyCode, (WORD)id, (WORD)id, AfxSigNotify_v, \
		(AFX_PMSG) \
		(static_cast< void (AFX_MSG_CALL CCmdTarget::*)(NMHDR*, LRESULT*) > \
		(memberFxn)) },

#define ON_NOTIFY_RANGE(wNotifyCode, id, idLast, memberFxn) \
	{ WM_NOTIFY, (WORD)(int)wNotifyCode, (WORD)id, (WORD)idLast, AfxSigNotify_RANGE, \
		(AFX_PMSG) \
		(static_cast< void (AFX_MSG_CALL CCmdTarget::*)(UINT, NMHDR*, LRESULT*) > \
		(memberFxn)) },

#define ON_NOTIFY_EX(wNotifyCode, id, memberFxn) \
	{ WM_NOTIFY, (WORD)(int)wNotifyCode, (WORD)id, (WORD)id, AfxSigNotify_EX, \
		(AFX_PMSG) \
		(static_cast< BOOL (AFX_MSG_CALL CCmdTarget::*)(UINT, NMHDR*, LRESULT*) > \
		(memberFxn)) },

#define ON_NOTIFY_EX_RANGE(wNotifyCode, id, idLast, memberFxn) \
	{ WM_NOTIFY, (WORD)(int)wNotifyCode, (WORD)id, (WORD)idLast, AfxSigNotify_EX, \
		(AFX_PMSG) \
		(static_cast< BOOL (AFX_MSG_CALL CCmdTarget::*)(UINT, NMHDR*, LRESULT*) > \
		(memberFxn)) },

/////////////////////////////////////////////////////////////////////////////
// Reflection: a control handles its own notification instead of letting it
// go to the parent (how eMule's control subclasses intercept messages meant
// for their owner). The _EX forms let the handler return a BOOL to say
// whether the parent should still see the message; the reflected forms take
// no id (the sender is the control itself).

#define ON_CONTROL_REFLECT(wNotifyCode, memberFxn) \
	{ WM_COMMAND+WM_REFLECT_BASE, (WORD)wNotifyCode, 0, 0, AfxSigCmd_v, \
		(static_cast<AFX_PMSG> (memberFxn)) },

#define ON_CONTROL_REFLECT_EX(wNotifyCode, memberFxn) \
	{ WM_COMMAND+WM_REFLECT_BASE, (WORD)wNotifyCode, 0, 0, AfxSigCmd_b, \
		(AFX_PMSG) \
		(static_cast<BOOL (AFX_MSG_CALL CCmdTarget::*)(void)> (memberFxn)) },

#define ON_NOTIFY_REFLECT(wNotifyCode, memberFxn) \
	{ WM_NOTIFY+WM_REFLECT_BASE, (WORD)(int)wNotifyCode, 0, 0, AfxSigNotify_v, \
		(AFX_PMSG) \
		(static_cast<void (AFX_MSG_CALL CCmdTarget::*)(NMHDR*, LRESULT*) > \
		(memberFxn)) },

#define ON_NOTIFY_REFLECT_EX(wNotifyCode, memberFxn) \
	{ WM_NOTIFY+WM_REFLECT_BASE, (WORD)(int)wNotifyCode, 0, 0, AfxSigNotify_b, \
		(AFX_PMSG) \
		(static_cast<BOOL (AFX_MSG_CALL CCmdTarget::*)(NMHDR*, LRESULT*) > \
		(memberFxn)) },

// ON_UPDATE_COMMAND_UI is intentionally NOT provided: eMule does not use it,
// and Microsoft Learn documents it with Requirements: Header afxole.h.

/////////////////////////////////////////////////////////////////////////////
// User extensions: raw and registered window messages.

#define ON_MESSAGE(message, memberFxn) \
	{ message, 0, 0, 0, AfxSig_lwl, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< LRESULT (AFX_MSG_CALL CWnd::*)(WPARAM, LPARAM) > \
		(memberFxn)) },

#define ON_REGISTERED_MESSAGE(nMessageVariable, memberFxn) \
	{ 0xC000, 0, 0, 0, (UINT_PTR)(UINT*)(&nMessageVariable), \
		/*implied 'AfxSig_lwl'*/ \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< LRESULT (AFX_MSG_CALL CWnd::*)(WPARAM, LPARAM) > \
		(memberFxn)) },

/////////////////////////////////////////////////////////////////////////////
// Convenience wrappers around ON_CONTROL (the notification-code constants
// come from <winuser.h> on Windows, exactly as real MFC's afxmsg_.h assumes).

#define ON_STN_DBLCLK(id, memberFxn)      ON_CONTROL(STN_DBLCLK, id, memberFxn)

#define ON_EN_SETFOCUS(id, memberFxn)     ON_CONTROL(EN_SETFOCUS, id, memberFxn)
#define ON_EN_KILLFOCUS(id, memberFxn)    ON_CONTROL(EN_KILLFOCUS, id, memberFxn)
#define ON_EN_CHANGE(id, memberFxn)       ON_CONTROL(EN_CHANGE, id, memberFxn)
#define ON_EN_UPDATE(id, memberFxn)       ON_CONTROL(EN_UPDATE, id, memberFxn)

#define ON_BN_CLICKED(id, memberFxn)      ON_CONTROL(BN_CLICKED, id, memberFxn)

#define ON_CBN_SELCHANGE(id, memberFxn)   ON_CONTROL(CBN_SELCHANGE, id, memberFxn)
#define ON_CBN_SELENDOK(id, memberFxn)    ON_CONTROL(CBN_SELENDOK, id, memberFxn)

/////////////////////////////////////////////////////////////////////////////
// Standard Windows-message entries. Each maps to a fixed handler name (hence
// no memberFxn parameter). Subset used by eMule/srchybrid; expansions are
// the real MFC ones verbatim (message id, AfxSig tag, and the exact handler
// signature the cast goes through).

#define ON_WM_CREATE() \
	{ WM_CREATE, 0, 0, 0, AfxSig_is, \
		(AFX_PMSG) (AFX_PMSGW) \
		(static_cast< int (AFX_MSG_CALL CWnd::*)(LPCREATESTRUCT) > ( &ThisClass :: OnCreate)) },

#define ON_WM_DESTROY() \
	{ WM_DESTROY, 0, 0, 0, AfxSig_vv, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(void) > ( &ThisClass :: OnDestroy)) },

#define ON_WM_PAINT() \
	{ WM_PAINT, 0, 0, 0, AfxSig_vv, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(void) > ( &ThisClass :: OnPaint)) },

#define ON_WM_CLOSE() \
	{ WM_CLOSE, 0, 0, 0, AfxSig_vv, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(void) > ( &ThisClass :: OnClose)) },

#define ON_WM_SYSCOLORCHANGE() \
	{ WM_SYSCOLORCHANGE, 0, 0, 0, AfxSig_vv, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(void) > ( &ThisClass :: OnSysColorChange)) },

#define ON_WM_NCPAINT() \
	{ WM_NCPAINT, 0, 0, 0, AfxSig_vv, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(void) > ( &ThisClass :: OnNcPaint)) },

#define ON_WM_NCDESTROY() \
	{ WM_NCDESTROY, 0, 0, 0, AfxSig_vv, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(void) > ( &ThisClass :: OnNcDestroy)) },

#define ON_WM_CANCELMODE() \
	{ WM_CANCELMODE, 0, 0, 0, AfxSig_vv, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(void) > ( &ThisClass :: OnCancelMode)) },

#define ON_WM_HELPINFO() \
	{ WM_HELP, 0, 0, 0, AfxSig_bHELPINFO, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< BOOL (AFX_MSG_CALL CWnd::*)(HELPINFO*) > ( &ThisClass :: OnHelpInfo)) },

#define ON_WM_CONTEXTMENU() \
	{ WM_CONTEXTMENU, 0, 0, 0, AfxSig_vWp, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(CWnd*, CPoint) > ( &ThisClass :: OnContextMenu)) },

#define ON_WM_TIMER() \
	{ WM_TIMER, 0, 0, 0, AfxSig_v_up_v, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT_PTR) > ( &ThisClass :: OnTimer)) },

#define ON_WM_MOUSEMOVE() \
	{ WM_MOUSEMOVE, 0, 0, 0, AfxSig_vwp, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, CPoint) > ( &ThisClass :: OnMouseMove)) },

#define ON_WM_LBUTTONUP() \
	{ WM_LBUTTONUP, 0, 0, 0, AfxSig_vwp, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, CPoint) > ( &ThisClass :: OnLButtonUp)) },

#define ON_WM_LBUTTONDOWN() \
	{ WM_LBUTTONDOWN, 0, 0, 0, AfxSig_vwp, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, CPoint) > ( &ThisClass :: OnLButtonDown)) },

#define ON_WM_LBUTTONDBLCLK() \
	{ WM_LBUTTONDBLCLK, 0, 0, 0, AfxSig_vwp, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, CPoint) > ( &ThisClass :: OnLButtonDblClk)) },

#define ON_WM_RBUTTONDOWN() \
	{ WM_RBUTTONDOWN, 0, 0, 0, AfxSig_vwp, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, CPoint) > ( &ThisClass :: OnRButtonDown)) },

#define ON_WM_MBUTTONUP() \
	{ WM_MBUTTONUP, 0, 0, 0, AfxSig_vwp, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, CPoint) > ( &ThisClass :: OnMButtonUp)) },

#define ON_WM_NCLBUTTONDBLCLK() \
	{ WM_NCLBUTTONDBLCLK, 0, 0, 0, AfxSig_vwp, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, CPoint) > ( &ThisClass :: OnNcLButtonDblClk)) },

#define ON_WM_NCLBUTTONDOWN() \
	{ WM_NCLBUTTONDOWN, 0, 0, 0, AfxSig_vwp, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, CPoint) > ( &ThisClass :: OnNcLButtonDown)) },

#define ON_WM_NCRBUTTONDOWN() \
	{ WM_NCRBUTTONDOWN, 0, 0, 0, AfxSig_vwp, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, CPoint) > ( &ThisClass :: OnNcRButtonDown)) },

#define ON_WM_SIZE() \
	{ WM_SIZE, 0, 0, 0, AfxSig_vwii, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, int, int) > ( &ThisClass :: OnSize)) },

#define ON_WM_CTLCOLOR() \
	{ WM_CTLCOLOR, 0, 0, 0, AfxSig_hDWw, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< HBRUSH (AFX_MSG_CALL CWnd::*)(CDC*, CWnd*, UINT)>  ( &ThisClass :: OnCtlColor)) },

#define ON_WM_CTLCOLOR_REFLECT() \
	{ WM_CTLCOLOR+WM_REFLECT_BASE, 0, 0, 0, AfxSig_hDw, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< HBRUSH (AFX_MSG_CALL CWnd::*)(CDC*, UINT) > ( &ThisClass :: CtlColor)) },

#define ON_WM_KEYDOWN() \
	{ WM_KEYDOWN, 0, 0, 0, AfxSig_vwww, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, UINT, UINT) > ( &ThisClass :: OnKeyDown)) },

#define ON_WM_CHAR() \
	{ WM_CHAR, 0, 0, 0, AfxSig_vwww, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, UINT, UINT) > ( &ThisClass :: OnChar)) },

#define ON_WM_SYSCOMMAND() \
	{ WM_SYSCOMMAND, 0, 0, 0, AfxSig_vwl, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, LPARAM) > ( &ThisClass :: OnSysCommand)) },

#define ON_WM_SHOWWINDOW() \
	{ WM_SHOWWINDOW, 0, 0, 0, AfxSig_vbw, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(BOOL, UINT) > ( &ThisClass :: OnShowWindow)) },

#define ON_WM_SETTINGCHANGE() \
	{ WM_SETTINGCHANGE, 0, 0, 0, AfxSig_vws, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, LPCTSTR) > ( &ThisClass :: OnSettingChange)) },

#define ON_WM_SETCURSOR() \
	{ WM_SETCURSOR, 0, 0, 0, AfxSig_bWww, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< BOOL (AFX_MSG_CALL CWnd::*)(CWnd*, UINT, UINT) > ( &ThisClass :: OnSetCursor)) },

#define ON_WM_HSCROLL() \
	{ WM_HSCROLL, 0, 0, 0, AfxSig_vwwW, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, UINT, CScrollBar*) > ( &ThisClass :: OnHScroll)) },

#define ON_WM_VSCROLL() \
	{ WM_VSCROLL, 0, 0, 0, AfxSig_vwwW, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, UINT, CScrollBar*) > ( &ThisClass :: OnVScroll)) },

#define ON_WM_ERASEBKGND() \
	{ WM_ERASEBKGND, 0, 0, 0, AfxSig_bD, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< BOOL (AFX_MSG_CALL CWnd::*)(CDC*) > ( &ThisClass :: OnEraseBkgnd)) },

#define ON_WM_SETFOCUS() \
	{ WM_SETFOCUS, 0, 0, 0, AfxSig_vW, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(CWnd*) > ( &ThisClass :: OnSetFocus)) },

#define ON_WM_KILLFOCUS() \
	{ WM_KILLFOCUS, 0, 0, 0, AfxSig_vW, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(CWnd*) > ( &ThisClass :: OnKillFocus)) },

#define ON_WM_CAPTURECHANGED() \
	{ WM_CAPTURECHANGED, 0, 0, 0, AfxSig_vW2, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(CWnd*) > ( &ThisClass :: OnCaptureChanged)) },

#define ON_WM_ACTIVATEAPP() \
	{ WM_ACTIVATEAPP, 0, 0, 0, AfxSig_vww, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(BOOL, DWORD) > ( &ThisClass :: OnActivateApp)) },

#define ON_WM_MOUSEWHEEL() \
	{ WM_MOUSEWHEEL, 0, 0, 0, AfxSig_bwsp, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< BOOL (AFX_MSG_CALL CWnd::*)(UINT, short, CPoint) > ( &ThisClass :: OnMouseWheel)) },

#define ON_WM_QUERYNEWPALETTE() \
	{ WM_QUERYNEWPALETTE, 0, 0, 0, AfxSig_bv, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< BOOL (AFX_MSG_CALL CWnd::*)(void) > ( &ThisClass :: OnQueryNewPalette)) },

#define ON_WM_QUERYENDSESSION() \
	{ WM_QUERYENDSESSION, 0, 0, 0, AfxSig_bv, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< BOOL (AFX_MSG_CALL CWnd::*)(void) > ( &ThisClass :: OnQueryEndSession)) },

#define ON_WM_QUERYDRAGICON() \
	{ WM_QUERYDRAGICON, 0, 0, 0, AfxSig_hv, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< HCURSOR (AFX_MSG_CALL CWnd::*)(void) > ( &ThisClass :: OnQueryDragIcon)) },

#define ON_WM_NCHITTEST() \
	{ WM_NCHITTEST, 0, 0, 0, AfxSig_l_p, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< LRESULT (AFX_MSG_CALL CWnd::*)(CPoint) > (&ThisClass :: OnNcHitTest)) },

#define ON_WM_NCACTIVATE() \
	{ WM_NCACTIVATE, 0, 0, 0, AfxSig_bb, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< BOOL (AFX_MSG_CALL CWnd::*)(BOOL) > ( &ThisClass :: OnNcActivate)) },

#define ON_WM_MENUSELECT() \
	{ WM_MENUSELECT, 0, 0, 0, AfxSig_vwwh, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(UINT, UINT, HMENU) > ( &ThisClass :: OnMenuSelect)) },

#define ON_WM_MENUCHAR() \
	{ WM_MENUCHAR, 0, 0, 0, AfxSig_lwwM, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< LRESULT (AFX_MSG_CALL CWnd::*)(UINT, UINT, CMenu*) > ( &ThisClass :: OnMenuChar)) },

#define ON_WM_MEASUREITEM() \
	{ WM_MEASUREITEM, 0, 0, 0, AfxSig_vOWNER, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(int, LPMEASUREITEMSTRUCT) > ( &ThisClass :: OnMeasureItem)) },

#define ON_WM_MEASUREITEM_REFLECT() \
	{ WM_MEASUREITEM+WM_REFLECT_BASE, 0, 0, 0, AfxSig_vs, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(LPMEASUREITEMSTRUCT) > ( &ThisClass :: MeasureItem)) },

#define ON_WM_DRAWITEM() \
	{ WM_DRAWITEM, 0, 0, 0, AfxSig_vOWNER, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(int, LPDRAWITEMSTRUCT) > ( &ThisClass :: OnDrawItem)) },

#define ON_WM_INITMENUPOPUP() \
	{ WM_INITMENUPOPUP, 0, 0, 0, AfxSig_vMwb, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(CMenu*, UINT, BOOL) > ( &ThisClass :: OnInitMenuPopup)) },

#define ON_WM_GETDLGCODE() \
	{ WM_GETDLGCODE, 0, 0, 0, AfxSig_wv, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< UINT (AFX_MSG_CALL CWnd::*)(void) > ( &ThisClass :: OnGetDlgCode)) },

#define ON_WM_ENDSESSION() \
	{ WM_ENDSESSION, 0, 0, 0, AfxSig_vb, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(BOOL) > ( &ThisClass :: OnEndSession)) },

#define ON_WM_DEVICECHANGE() \
	{ WM_DEVICECHANGE, 0, 0, 0, AfxSig_bwl, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< BOOL (AFX_MSG_CALL CWnd::*)(UINT, DWORD_PTR) > ( &ThisClass :: OnDeviceChange)) },

#define ON_WM_PALETTECHANGED() \
	{ WM_PALETTECHANGED, 0, 0, 0, AfxSig_vW, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(CWnd*) > ( &ThisClass :: OnPaletteChanged)) },
