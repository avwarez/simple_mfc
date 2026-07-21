// afxmsg_.h — reference STUB (declarations only, no implementation).
// Message-map "entry" macros (they map a message/command/notification to
// a member function). Per Microsoft Learn, the required header for
// ON_COMMAND/ON_MESSAGE/ON_REGISTERED_MESSAGE/ON_CONTROL is afxmsg_.h
// (the demarcation macros BEGIN/END/DECLARE_MESSAGE_MAP instead stay in
// afxwin.h, where real MFC puts them). Included by afxwin.h itself, as in
// real MFC where a single #include <afxwin.h> also exposes these macros.
#pragma once

#define ON_COMMAND(commandId, memberFxn)
#define ON_MESSAGE(message, memberFxn)              // handler: afx_msg LRESULT memberFxn(WPARAM, LPARAM);
#define ON_REGISTERED_MESSAGE(nMessageVariable, memberFxn)
#define ON_CONTROL(wNotifyCode, commandId, memberFxn)

// ON_NOTIFY has no dedicated "Requirements" page on Microsoft Learn (it is
// absent from the current message-map-macros-mfc table), but conceptually
// belongs to the same family as ON_CONTROL/ON_COMMAND (a mapping macro,
// not a demarcation one) — placed here by pattern, not as a direct
// citation of a primary source.
#define ON_NOTIFY(wNotifyCode, id, memberFxn)        // handler: afx_msg void memberFxn(NMHDR*, LRESULT*);

// Convenience wrappers around ON_CONTROL — not individually documented by
// Microsoft, inferred by pattern from the afxmsg_.h source.
#define ON_BN_CLICKED(id, memberFxn)      ON_CONTROL(/*BN_CLICKED*/ 0, id, memberFxn)
#define ON_EN_CHANGE(id, memberFxn)       ON_CONTROL(/*EN_CHANGE*/ 0, id, memberFxn)
#define ON_EN_KILLFOCUS(id, memberFxn)    ON_CONTROL(/*EN_KILLFOCUS*/ 0, id, memberFxn)
#define ON_EN_SETFOCUS(id, memberFxn)     ON_CONTROL(/*EN_SETFOCUS*/ 0, id, memberFxn)
#define ON_EN_UPDATE(id, memberFxn)       ON_CONTROL(/*EN_UPDATE*/ 0, id, memberFxn)
#define ON_CBN_SELCHANGE(id, memberFxn)   ON_CONTROL(/*CBN_SELCHANGE*/ 0, id, memberFxn)
#define ON_CBN_SELENDOK(id, memberFxn)    ON_CONTROL(/*CBN_SELENDOK*/ 0, id, memberFxn)
#define ON_STN_CLICKED(id, memberFxn)     ON_CONTROL(/*STN_CLICKED*/ 0, id, memberFxn)
#define ON_STN_DBLCLK(id, memberFxn)      ON_CONTROL(/*STN_DBLCLK*/ 0, id, memberFxn)

// Ranges: one handler for a contiguous block of command ids / notifications.
#define ON_COMMAND_RANGE(id, idLast, memberFxn)
#define ON_NOTIFY_EX(wNotifyCode, id, memberFxn)
#define ON_NOTIFY_EX_RANGE(wNotifyCode, id, idLast, memberFxn)

// Reflection: a control handles its own notification instead of letting it
// go to the parent, which is how eMule's control subclasses (CListCtrlX,
// CEditDelayed, ...) intercept messages meant for their owner. The _EX
// forms let the handler return a BOOL to say whether the parent should see
// the message too; unlike the macros above, the reflected forms take no id
// (the sender is the control itself).
#define ON_CONTROL_REFLECT(wNotifyCode, memberFxn)
#define ON_CONTROL_REFLECT_EX(wNotifyCode, memberFxn)
#define ON_NOTIFY_REFLECT(wNotifyCode, memberFxn)
#define ON_NOTIFY_REFLECT_EX(wNotifyCode, memberFxn)

// ON_UPDATE_COMMAND_UI is intentionally NOT declared here: it is not part
// of the real-world usage this subset targets. Note: Microsoft Learn
// documents it with Requirements: Header afxole.h, not afxmsg_.h.

// ---------------------------------------------------------------------
// ON_WM_* — standard Windows-message entries (each maps to a fixed
// handler name, hence no memberFxn parameter, unlike ON_COMMAND/
// ON_MESSAGE/ON_CONTROL above). Real MFC's macro bodies are empty in a
// declaration-only sense too (they only expand to a message-map table
// entry). Subset actually used by eMule/srchybrid, verified against
// Microsoft Learn (see ../../mfc_scan_srchybrid.md) — the 25 most-used
// variants carry the exact handler signature as a comment; the 26 minor
// variants (1-2 occurrences each in the scanned codebase) are declared
// without a captured signature.
// ---------------------------------------------------------------------
#define ON_WM_DESTROY()                // handler: afx_msg void OnDestroy();
#define ON_WM_SYSCOLORCHANGE()         // handler: afx_msg void OnSysColorChange();
#define ON_WM_HELPINFO()               // handler: afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
#define ON_WM_CONTEXTMENU()            // handler: afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
#define ON_WM_PAINT()                  // handler: afx_msg void OnPaint();
#define ON_WM_CREATE()                 // handler: afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
#define ON_WM_TIMER()                  // handler: afx_msg void OnTimer(UINT_PTR nIDEvent);
#define ON_WM_MOUSEMOVE()              // handler: afx_msg void OnMouseMove(UINT nFlags, CPoint point);
#define ON_WM_LBUTTONUP()              // handler: afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
#define ON_WM_SIZE()                   // handler: afx_msg void OnSize(UINT nType, int cx, int cy);
#define ON_WM_CTLCOLOR()               // handler: afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
#define ON_WM_KEYDOWN()                // handler: afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
#define ON_WM_SYSCOMMAND()             // handler: afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
#define ON_WM_SHOWWINDOW()             // handler: afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
#define ON_WM_SETTINGCHANGE()          // handler: afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
#define ON_WM_SETCURSOR()              // handler: afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
#define ON_WM_LBUTTONDOWN()            // handler: afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
#define ON_WM_HSCROLL()                // handler: afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
#define ON_WM_THEMECHANGED()           // handler: afx_msg void OnThemeChanged();
#define ON_WM_ERASEBKGND()             // handler: afx_msg BOOL OnEraseBkgnd(CDC* pDC);
#define ON_WM_SETFOCUS()               // handler: afx_msg void OnSetFocus(CWnd* pOldWnd);
#define ON_WM_CLOSE()                  // handler: afx_msg void OnClose();
#define ON_WM_RBUTTONDOWN()            // handler: afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
#define ON_WM_LBUTTONDBLCLK()          // handler: afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
#define ON_WM_KILLFOCUS()              // handler: afx_msg void OnKillFocus(CWnd* pNewWnd);

// Minor variants (1-2 occurrences each in eMule/srchybrid): no handler
// signature captured by the scan, declared for message-map completeness
// only. ON_WM_MEASUREITEM_REFLECT/ON_WM_CTLCOLOR_REFLECT are MFC
// "reflection" macros (no dedicated Microsoft Learn page, unlike the
// direct ON_WM_* ones above).
#define ON_WM_CAPTURECHANGED()
#define ON_WM_ACTIVATEAPP()
#define ON_WM_VSCROLL()
#define ON_WM_QUERYNEWPALETTE()
#define ON_WM_QUERYENDSESSION()
#define ON_WM_QUERYDRAGICON()
#define ON_WM_PALETTECHANGED()
#define ON_WM_NCRBUTTONDOWN()
#define ON_WM_NCPAINT()
#define ON_WM_NCLBUTTONDOWN()
#define ON_WM_NCHITTEST()
#define ON_WM_NCDESTROY()
#define ON_WM_NCACTIVATE()
#define ON_WM_MENUSELECT()
#define ON_WM_MENUCHAR()
#define ON_WM_MEASUREITEM()
#define ON_WM_MBUTTONUP()
#define ON_WM_INITMENUPOPUP()
#define ON_WM_GETDLGCODE()
#define ON_WM_ENDSESSION()
#define ON_WM_DRAWITEM()
#define ON_WM_DEVICECHANGE()
#define ON_WM_CHAR()
#define ON_WM_CANCELMODE()
#define ON_WM_MEASUREITEM_REFLECT()
#define ON_WM_CTLCOLOR_REFLECT()
