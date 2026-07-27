// gui/qt/smfc_qt.h — Qt-driver seam helpers (public to the driver, not part
// of the frozen MFC interface). These translate Qt signals/events into the
// Windows-message vocabulary the eMule message maps expect.
#pragma once
#include "afxwin.h"

class QAbstractButton;

namespace smfc_qt {

// Route a Qt button's clicked() signal to its owner CWnd as a
// WM_COMMAND / BN_CLICKED notification -- i.e. straight into the eMule
// message map (ON_BN_CLICKED(id, handler)). This is the driver owning the
// Qt->WM_* translation, so eMule code just sees its handler get called.
void WireButton(CWnd* pOwner, QAbstractButton* pButton, UINT nID);

} // namespace smfc_qt
