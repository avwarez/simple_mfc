#include "smfc_qt.h"

#include <QAbstractButton>

namespace smfc_qt {

void WireButton(CWnd* pOwner, QAbstractButton* pButton, UINT nID)
{
    // clicked() -> WM_COMMAND with wParam = MAKEWPARAM(nID, BN_CLICKED),
    // lParam = the control handle. Delivered through the owner's WindowProc,
    // exactly as Windows would deliver a button click to the parent dialog.
    // A lambda with a context object needs no moc (only our own QObject
    // subclasses would).
    QObject::connect(pButton, &QAbstractButton::clicked, pButton,
        [pOwner, nID, pButton]()
        {
            const WPARAM wParam =
                static_cast<WPARAM>((nID & 0xFFFF) | (static_cast<UINT>(BN_CLICKED) << 16));
            pOwner->SendMessage(WM_COMMAND, wParam, reinterpret_cast<LPARAM>(pButton));
        });
}

} // namespace smfc_qt
