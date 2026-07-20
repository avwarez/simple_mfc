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
};

// ---------------------------------------------------------------------
// CDialogBar — modeless dialog hosted inside a control bar
// (header afxext.h, hierarchy CObject -> CCmdTarget -> CWnd -> CControlBar -> CDialogBar).
// Create hides CWnd::Create (same real-MFC pattern as CDialog/
// CFrameWnd/CPropertySheet), see the pragma note in afxwin.h.
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
