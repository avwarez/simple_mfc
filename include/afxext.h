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
// No concrete method beyond the class itself (consistent with the
// "hierarchy only" skeleton already used for the other CWnd subclasses
// in afxwin.h).
// ---------------------------------------------------------------------
class CDialogBar : public CControlBar {};
