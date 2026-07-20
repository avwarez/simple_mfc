// afxext.h — reference STUB (declarations only, no implementation).
// "Extended" MFC controls (bars, etc.) beyond the base ones in afxwin.h.
// As in real MFC, it includes afxwin.h.
#pragma once
#include "afxwin.h"

// ---------------------------------------------------------------------
// CControlBar — abstract base for the control-bar classes (CStatusBar,
// CToolBar, CDialogBar, CReBar, COleResizeBar). Protected constructor:
// not directly instantiable (header afxext.h, hierarchy
// CObject -> CCmdTarget -> CWnd -> CControlBar).
// ---------------------------------------------------------------------
class CControlBar : public CWnd
{
protected:
    CControlBar();

public:
    // Added during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum): CToolbarWnd/CSearchParamsWnd
    // (both real CDialogBar subclasses) override CalcDynamicLayout and
    // super-call "CDialogBar::CalcDynamicLayout(...)" for the default
    // behavior (ToolbarWnd.cpp:268, SearchParamsWnd.cpp:228) — resolves
    // here via inheritance, since real MFC declares it on CControlBar,
    // not CDialogBar itself. This was also the investigation for whether
    // CDialogBar::Create is genuinely used (see the note in afxext.h
    // below): it is not, but this method is.
    virtual CSize CalcDynamicLayout(int nLength, DWORD dwMode);
};

// ---------------------------------------------------------------------
// CDialogBar — modeless dialog hosted inside a control bar
// (header afxext.h, hierarchy CObject -> CCmdTarget -> CWnd -> CControlBar -> CDialogBar).
// Create hides CWnd::Create (same real-MFC pattern as CDialog/
// CFrameWnd/CPropertySheet), see the pragma note in afxwin.h.
//
// FRONTEND/GDI blind-spot pass (see ../../mfc_scan_srchybrid.md
// addendum): the original scan's "Create: 103 occ." for CDialogBar was
// an unattributable aggregate shared with 7 other classes. Verified
// directly against CDialogBar's two real subclasses in eMule/srchybrid
// (CToolbarWnd in ToolbarWnd.h/.cpp, CSearchParamsWnd in
// SearchParamsWnd.h/.cpp): neither ever calls Create on a CDialogBar-
// typed object (their ".Create("/"->Create(" hits are all on unrelated
// CImageList members, e.g. "iml.Create(...)"). Decision: KEEP Create
// anyway (it is still real, documented MFC API — harmless to keep even
// unattributed) AND add the methods that ARE genuinely used: those two
// subclasses override and super-call CalcDynamicLayout (now declared on
// CControlBar, see above) plus DoDataExchange/OnSetCursor/OnSize/
// OnDestroy/OnSysColorChange/OnSysCommand/PreTranslateMessage — all of
// which are now available through CWnd (see the CWnd handler block
// added in afxwin.h), which CDialogBar inherits transitively.
// ---------------------------------------------------------------------
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4266)
#endif

class CDialogBar : public CControlBar
{
public:
    virtual BOOL Create(CWnd* pParentWnd, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID);
    virtual BOOL Create(CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID);
};

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif
