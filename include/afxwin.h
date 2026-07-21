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
typedef long (*AFX_THREADPROC)(void*);

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
#else
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
#endif
struct tagMEASUREITEMSTRUCT;
using LPMEASUREITEMSTRUCT = tagMEASUREITEMSTRUCT*;
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
// CCmdTarget — base of CWinThread for command routing (header afxwin.h)
// ---------------------------------------------------------------------
class CCmdTarget : public CObject {};

// ---------------------------------------------------------------------
// CWinThread (header afxwin.h, hierarchy CObject -> CCmdTarget -> CWinThread)
// ---------------------------------------------------------------------
class CWinThread : public CCmdTarget
{
public:
    BOOL m_bAutoDelete;
    void* m_hThread;
    DWORD m_nThreadID;
    CWnd* m_pMainWnd;   // real MFC public data members
    CWnd* m_pActiveWnd;

    CWinThread();
    BOOL CreateThread(DWORD dwCreateFlags = 0, UINT nStackSize = 0,
                       SECURITY_ATTRIBUTES* lpSecurityAttrs = nullptr);
    DWORD ResumeThread();
    DWORD SuspendThread();
    BOOL SetThreadPriority(int nPriority);
    int GetThreadPriority();
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    virtual int Run();
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

    UINT GetProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault);
    BOOL WriteProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nValue);
    CString GetProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault = nullptr);
    virtual BOOL OnIdle(LONG lCount);
};

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
    CBrush();
    CBrush(COLORREF crColor);
    CBrush(int nIndex, COLORREF crColor);
    explicit CBrush(CBitmap* pBitmap);

    BOOL CreateSolidBrush(COLORREF crColor);
    BOOL CreateHatchBrush(int nIndex, COLORREF crColor);
    BOOL CreatePatternBrush(CBitmap* pBitmap);
    BOOL CreateDIBPatternBrush(HGLOBAL hPackedDIB, UINT nUsage);
    BOOL CreateDIBPatternBrush(const void* lpPackedDIB, UINT nUsage);
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
    BOOL CreatePalette(LPLOGPALETTE lpLogPalette);
};

// CBitmap (header afxwin.h, deriva da CGdiObject)
class CBitmap : public CGdiObject
{
public:
    BOOL CreateCompatibleBitmap(CDC* pDC, int nWidth, int nHeight);
    int GetBitmap(struct tagBITMAP* pBitMap);
    BOOL LoadBitmap(LPCTSTR lpszResourceName);
    BOOL LoadBitmap(UINT nIDResource);
    CSize GetBitmapDimension() const;
};

#ifndef _WIN32
struct tagLOGFONT;
using LOGFONT = tagLOGFONT;
#endif

// CFont (header afxwin.h, deriva da CGdiObject)
class CFont : public CGdiObject
{
public:
    BOOL CreateFontIndirect(const LOGFONT* lpLogFont);
    int GetLogFont(LOGFONT* pLogFont);
    BOOL CreateFont(int nHeight, int nWidth, int nEscapement, int nOrientation, int nWeight,
                     BYTE bItalic, BYTE bUnderline, BYTE cStrikeOut, BYTE nCharSet,
                     BYTE nOutPrecision, BYTE nClipPrecision, BYTE nQuality,
                     BYTE nPitchAndFamily, LPCTSTR lpszFacename);
    BOOL CreatePointFont(int nPointSize, LPCTSTR lpszFaceName, CDC* pDC = nullptr);
};

// CDC (header afxwin.h, hierarchy CObject -> CDC)
class CDC : public CObject
{
public:
    static CDC* FromHandle(HDC hDC);

    CGdiObject* SelectObject(CGdiObject* pObject);
    CDC* SelectObject(CFont* pFont);
    CDC* SelectObject(CBrush* pBrush);
    CDC* SelectObject(CPen* pPen);
    CDC* SelectObject(CPalette* pPalette);
    CDC* SelectObject(CBitmap* pBitmap);
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
    BOOL GetTextMetrics(struct tagTEXTMETRIC* lpMetrics);
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

class CWnd : public CCmdTarget
{
public:
    BOOL EnableWindow(BOOL bEnable = TRUE);
    BOOL ShowWindow(int nCmdShow);
    void GetWindowRect(LPRECT lpRect) const;
    virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle,
                         const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = nullptr);
    LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
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
    BOOL SetWindowPos(CWnd* pWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags);
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
    CDC* GetDC();
    int ReleaseDC(CDC* pDC);
    BOOL RedrawWindow(LPCRECT lpRectUpdate = nullptr, CRgn* prgnUpdate = nullptr,
                       UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
    CWnd* SetCapture();
    void InvalidateRect(LPCRECT lpRect, BOOL bErase = TRUE);

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
    virtual int OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
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
};

class CDialog : public CWnd
{
public:
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

class CStatic : public CWnd
{
public:
    HBITMAP SetBitmap(HBITMAP hBitmap);
    HBITMAP GetBitmap() const;
};

class CEdit : public CWnd
{
public:
    void SetSel(DWORD dwSelection, BOOL bNoScroll = FALSE);
    void SetSel(int nStartChar, int nEndChar, BOOL bNoScroll = FALSE);
    void LimitText(int nChars = 0);
    DWORD GetSel() const;
    void GetSel(int& nStartChar, int& nEndChar) const;
    void ReplaceSel(LPCTSTR lpszNewText, BOOL bCanUndo = FALSE);
};

class CListBox : public CWnd
{
public:
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
};

class CComboBox : public CWnd
{
public:
    int GetCount() const;
    int GetCurSel() const;
    int SetCurSel(int nSelect);
    int AddString(LPCTSTR lpszString);
    DWORD_PTR GetItemData(int nIndex) const;
    int SetItemData(int nIndex, DWORD_PTR dwItemData);
    void ResetContent();
    int GetLBText(int nIndex, LPTSTR lpszText) const;
    void GetLBText(int nIndex, CString& rString) const;
    int DeleteString(UINT nIndex);
    int SelectString(int nStartAfter, LPCTSTR lpszString) const;
};

class CButton : public CWnd
{
public:
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
    BOOL AppendMenu(UINT nFlags, UINT_PTR nIDNewItem = 0, LPCTSTR lpszNewItem = nullptr);
    BOOL AppendMenu(UINT nFlags, UINT_PTR nIDNewItem, const CBitmap* pBmp);
    UINT EnableMenuItem(UINT nIDEnableItem, UINT nEnable);
    BOOL DestroyMenu();
    HMENU Detach();
    BOOL Attach(HMENU hMenu);
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
// ---------------------------------------------------------------------
// afx_msg is real MFC's marker keyword on every message-handler declaration
// (`afx_msg void OnPaint();`). It expands to nothing -- but it MUST be defined,
// otherwise `afx_msg void Foo();` parses as two adjacent declarations and every
// handler line becomes a C2144 syntax error. eMule uses it 816 times, so its
// absence alone was cascading through 150+ files.
#define afx_msg
#define DECLARE_MESSAGE_MAP()
#define BEGIN_MESSAGE_MAP(theClass, baseClass)
#define END_MESSAGE_MAP()

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
void* AfxGetInstanceHandle();
void* AfxGetResourceHandle();
CWnd* AfxGetMainWnd();
LPCTSTR AfxGetAppName();
int AfxMessageBox(LPCTSTR lpszText, UINT nType = 0, UINT nIDHelp = 0);
// Real MFC provides BOTH char-widths (afx.h): eMule's kademlia/Tag.h passes a
// narrow LPCSTR, other call sites the wide LPCTSTR, so declare both overloads.
BOOL AfxIsValidString(LPCTSTR lpsz, int nLength = -1);
BOOL AfxIsValidString(const char* lpsz, int nLength = -1);
BOOL AfxIsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite = TRUE);

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
};
