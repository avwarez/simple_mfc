// afxdlgs.h — reference STUB (declarations only, no implementation).
// Standard MFC dialogs (property page/sheet and other predefined
// dialogs). As in real MFC, it includes afxwin.h.
#pragma once
#include "afxwin.h"

// ---------------------------------------------------------------------
// CPropertyPage — a page of a property sheet
// (header afxdlgs.h, hierarchy CObject -> CCmdTarget -> CWnd -> CDialog -> CPropertyPage).
// ---------------------------------------------------------------------
class CPropertyPage : public CDialog {};

// ---------------------------------------------------------------------
// CPropertySheet — container for property pages. Derives directly from
// CWnd, NOT from CDialog (header afxdlgs.h, hierarchy
// CObject -> CCmdTarget -> CWnd -> CPropertySheet).
// ---------------------------------------------------------------------
class CPropertySheet : public CWnd {};
