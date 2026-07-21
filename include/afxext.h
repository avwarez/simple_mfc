// afxext.h — reference STUB (declarations only, no implementation).
// "Extended" MFC controls (bars, etc.) beyond the base ones in afxwin.h.
// As in real MFC, it includes afxwin.h.
#pragma once
#include "afxwin.h"

// ---------------------------------------------------------------------
// Control-bar styles. Real MFC defines them in this header as macros, so
// #ifndef (not #ifdef _WIN32) is the right guard -- see afxwin.h's RDW_*
// for why the two are not interchangeable for a macro.
// The alignment/border bits are combined into the CBRS_TOP/BOTTOM/LEFT/
// RIGHT shorthands eMule actually passes to CDialogBar::Create.
// ---------------------------------------------------------------------
#ifndef CBRS_ALIGN_ANY
#define CBRS_ALIGN_LEFT     0x1000L
#define CBRS_ALIGN_TOP      0x2000L
#define CBRS_ALIGN_RIGHT    0x4000L
#define CBRS_ALIGN_BOTTOM   0x8000L
#define CBRS_ALIGN_ANY      0xF000L

#define CBRS_BORDER_LEFT    0x0100L
#define CBRS_BORDER_TOP     0x0200L
#define CBRS_BORDER_RIGHT   0x0400L
#define CBRS_BORDER_BOTTOM  0x0800L
#define CBRS_BORDER_ANY     0x0F00L

#define CBRS_FLOATING       0x0001L
#define CBRS_SIZE_FIXED     0x0002L
#define CBRS_SIZE_DYNAMIC   0x0004L
#define CBRS_HIDE_INPLACE   0x0008L
#define CBRS_TOOLTIPS       0x0010L
#define CBRS_FLYBY          0x0020L
#define CBRS_FLOAT_MULTI    0x0040L
#define CBRS_BORDER_3D      0x0080L
#define CBRS_GRIPPER        0x00400000L

#define CBRS_TOP            (CBRS_ALIGN_TOP|CBRS_BORDER_TOP|CBRS_BORDER_BOTTOM)
#define CBRS_BOTTOM         (CBRS_ALIGN_BOTTOM|CBRS_BORDER_TOP|CBRS_BORDER_BOTTOM)
#define CBRS_LEFT           (CBRS_ALIGN_LEFT|CBRS_BORDER_LEFT|CBRS_BORDER_RIGHT)
#define CBRS_RIGHT          (CBRS_ALIGN_RIGHT|CBRS_BORDER_LEFT|CBRS_BORDER_RIGHT)
#define CBRS_ORIENT_HORZ    (CBRS_ALIGN_TOP|CBRS_ALIGN_BOTTOM)
#define CBRS_ORIENT_VERT    (CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT)
#define CBRS_ORIENT_ANY     (CBRS_ORIENT_HORZ|CBRS_ORIENT_VERT)
#endif

// The dwMode bits CalcDynamicLayout receives (real MFC: afxpriv.h).
// eMule's two CDialogBar subclasses both override CalcDynamicLayout and
// test them to decide their own size.
#ifndef LM_STRETCH
#define LM_STRETCH   1
#define LM_HORZ      2
#define LM_MRUWIDTH  4
#define LM_HORZDOCK  8
#define LM_VERTDOCK  16
#define LM_LENGTHY   32
#define LM_COMMIT    64
#endif

// ---------------------------------------------------------------------
// CDockContext — the per-bar drag/dock state. eMule never constructs one;
// it only reads the id of the dock bar this control bar was last docked
// to, in order to put it back there.
// ---------------------------------------------------------------------
class CDockBar;

class CDockContext
{
public:
    UINT m_uMRUDockID;
    CRect m_rectMRUDockPos;
};

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

    // Docking state. Public in real MFC but not on its documented
    // member list (they are implementation members of afxext.h that
    // applications nonetheless reach); eMule reads m_pDockContext to
    // restore a floating bar to its last dock position.
    CDockContext* m_pDockContext;
    CFrameWnd* m_pDockSite;
    CDockBar* m_pDockBar;
    DWORD m_dwStyle;
    int m_cxLeftBorder, m_cxRightBorder, m_cyTopBorder, m_cyBottomBorder;

    // Which sides of the frame this bar may dock to; the frame's own
    // EnableDocking (CFrameWnd, afxwin.h) has to allow them as well.
    void EnableDocking(DWORD dwDockStyle);
    BOOL IsFloating() const;
    BOOL IsVisible() const;
    // The client rectangle minus this bar's borders.
    virtual void CalcInsideRect(CRect& rect, BOOL bHorz) const;
    CFrameWnd* GetDockingFrame() const;
    void SetBorders(int cxLeft = 0, int cyTop = 0, int cxRight = 0, int cyBottom = 0);
    void SetBorders(LPCRECT lpRect);
    CRect GetBorders() const;
    DWORD GetBarStyle();
    void SetBarStyle(DWORD dwStyle);
    virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
    // Defers a show/hide until the frame recalculates its layout.
    void DelayShow(BOOL bShow);
};

// ---------------------------------------------------------------------
// CDockBar — the strip along one edge of a frame that holds the control
// bars docked there. eMule reads it off a bar to find out where that bar
// currently sits, so it needs to be a complete type, not a forward
// declaration.
// ---------------------------------------------------------------------
class CControlBar;

class CDockBar : public CControlBar
{
public:
    int FindBar(CControlBar* pBar, int nPos = 0) const;
    CControlBar* GetDockedControlBar(int nPos) const;
    BOOL IsHorz() const;
    // Set when this dock bar is the one inside a mini floating frame
    // rather than one of the four fixed strips. eMule reads it off a
    // bar's m_pDockBar to tell a real close-button click from an ordinary
    // hide (ToolbarWnd.cpp:435).
    BOOL m_bFloating;
    CPtrArray m_arrBars;
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

protected:
    // The size of the dialog template, which the bar defaults to; both of
    // eMule's subclasses grow their own layout from it.
    CSize m_sizeDefault;
};

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif

// ---------------------------------------------------------------------
// CScrollView / CFormView (header afxext.h, hierarchy
// CObject -> CCmdTarget -> CWnd -> CView -> CScrollView -> CFormView).
//
// CFormView hosts a dialog template as a view; CTransferWnd and
// CSearchResultsWnd derive from it. Its constructors are protected in
// real MFC too -- it is only ever instantiated through a derived class,
// which is what IMPLEMENT_DYNCREATE needs.
//
// CScrollView is here purely to keep the inheritance chain faithful:
// eMule calls none of its scrolling API, so none is declared.
// ---------------------------------------------------------------------
class CScrollView : public CView
{
};

class CFormView : public CScrollView
{
protected:
    explicit CFormView(LPCTSTR lpszTemplateName);
    explicit CFormView(UINT nIDTemplate);
};
