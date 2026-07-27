// gui/qt/driver_internal.h — Qt-driver internal helpers shared across the
// driver's translation units (cwnd.cpp, ddx.cpp, controls.cpp). NOT part of
// the frozen MFC interface: these are library-internal mechanisms, so they
// live in the driver's own namespace with our own shapes/names.
#pragma once
#include "afxwin.h"

#include <vector>

class QWidget;

namespace smfc_qt {

// The QWidget currently bound to a CWnd (its m_hWnd), or nullptr.
QWidget* WidgetOf(const CWnd* w);

// The radio-button group that DDX_Radio(pDX, nIDC, ...) addresses: the control
// ids, in template order, of the buttons belonging to nIDC's group. The group
// starts at nIDC and runs until the next control carrying WS_GROUP (Win32's
// group semantics), mirroring how real MFC's DDX_Radio walks GetNextDlgGroupItem.
// Resolved from the dialog's .rc template; empty if dlg has no template.
std::vector<int> RadioGroup(CWnd* dlg, int nIDC);

// DDX_Control binding: rebind control id `nIDC` in dialog `dlg` to the typed
// control object `rControl`. Detaches the builder's placeholder wrapper for
// that id, attaches rControl to the same QWidget, and makes
// dlg->GetDlgItem(nIDC) return rControl afterwards. No-op if the id is unknown.
void BindDlgControl(CWnd* dlg, int nIDC, CWnd& rControl);

} // namespace smfc_qt
