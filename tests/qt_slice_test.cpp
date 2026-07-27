// qt_slice_test.cpp — Milestone 1 VERTICAL SLICE: a real Qt button click,
// routed through a real eMule-style MFC message map, reaching its handler.
//
// This is the end-to-end proof that the seam works on a live toolkit:
//   QPushButton::clicked  (Qt signal)
//     -> smfc_qt::WireButton  (driver: Qt -> WM_COMMAND/BN_CLICKED)
//       -> CWnd::WindowProc -> CWnd::OnCommand
//         -> CCmdTarget::OnCmdMsg  (walks the mirrored AFX_MSGMAP)
//           -> SliceDlg::OnGo      (the eMule-style ON_BN_CLICKED handler)
//
// Runs headless (QT_QPA_PLATFORM=offscreen, set by CTest) so it works on a
// display-less CI/box. The dialog is hand-wired here (no .rc yet) on purpose:
// it isolates the Qt<->message-map seam; feeding a real eMule dialog comes
// with the .rc resource compiler (Phase 3).
#include "afxwin.h"
#include "smfc_qt.h"

#include <QApplication>
#include <QDialog>
#include <QPushButton>
#include <cstdio>

static int g_failures = 0;
#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::printf("FAIL: %s (line %d)\n", #cond, __LINE__);        \
            ++g_failures;                                                \
        }                                                                \
    } while (0)

#define IDC_GO 1001

static bool g_goFired = false;

class SliceDlg : public CDialog
{
public:
    SliceDlg() : CDialog() {}
    afx_msg void OnGo() { g_goFired = true; }
    DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(SliceDlg, CDialog)
    ON_BN_CLICKED(IDC_GO, OnGo)
END_MESSAGE_MAP()

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    SliceDlg dlg;
    QDialog qdlg;
    dlg.Attach(reinterpret_cast<HWND>(&qdlg));      // CWnd <-> QWidget binding

    QPushButton* pButton = new QPushButton(QStringLiteral("Go"), &qdlg);
    smfc_qt::WireButton(&dlg, pButton, IDC_GO);      // driver: click -> WM_COMMAND

    CHECK(!g_goFired);
    pButton->click();                                // programmatic Qt click
    CHECK(g_goFired);                                // handler reached via the map

    dlg.Detach();
    if (g_failures == 0)
        std::printf("qt_slice_test: Qt click routed through the message map to the handler\n");
    return (g_failures == 0) ? 0 : 1;
}
