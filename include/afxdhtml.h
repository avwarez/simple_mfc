// afxdhtml.h — reference STUB (declarations only, no implementation).
// CDHtmlDialog (a dialog hosting a DHTML/IE control). The original scan
// found zero *instance* calls (".Method("/"->Method(") on it, but the
// FRONTEND/GDI blind-spot pass (see ../../mfc_scan_srchybrid.md
// addendum) found genuine qualified super-calls in MiniMule.h/.cpp
// (CMiniMule : public CDHtmlDialog): OnBeforeNavigate/OnNavigateComplete/
// OnDocumentComplete are CDHtmlDialog-specific virtual event handlers,
// overridden there and super-called for the default (no-op) behavior,
// e.g. "CDHtmlDialog::OnDocumentComplete(pDisp, pszUrl);". The rest of
// the super-calls found (OnClose/OnDestroy/PostNcDestroy/OnTimer/
// OnNcLButtonDblClk/DoDataExchange/OnInitDialog) resolve via inherited
// CWnd/CDialog declarations (see afxwin.h), no CDHtmlDialog-specific
// declaration needed for those.
#pragma once
#include "afxwin.h"

struct IDispatch; // real OLE Automation type; only used here as a pointer
using LPDISPATCH = IDispatch*;

// ---------------------------------------------------------------------
// CDHtmlDialog (header afxdhtml.h, hierarchy
// CObject -> CCmdTarget -> CWnd -> CDialog -> CDHtmlDialog)
// ---------------------------------------------------------------------
class CDHtmlDialog : public CDialog
{
public:
    virtual void OnBeforeNavigate(LPDISPATCH pDisp, LPCTSTR pszUrl);
    virtual void OnNavigateComplete(LPDISPATCH pDisp, LPCTSTR pszUrl);
    virtual void OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR pszUrl);
};
