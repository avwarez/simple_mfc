// qt_dialog_test.cpp — the Qt driver CONSUMING the neutral dialog IR.
//
// The .rc compiler ran at build time on tests/fixtures/test_dialog.rc; its
// generated C++ (linked in) registered IDD_SAMPLE. Here a CDialog built from
// that template gets real QWidgets with the right IDC_ ids and geometry, so:
//   - GetDlgItem(IDC_*) returns the control's CWnd,
//   - SetWindowText/GetWindowText round-trip through the bound QWidget,
//   - a real button click still routes through the message map to its handler.
// This proves the full pipeline: .rc -> IR -> live, addressable Qt dialog.
//
// Runs headless (QT_QPA_PLATFORM=offscreen). Uses Create() (modeless) so no
// event loop is needed to drive it.
#include "afxwin.h"

#include <QApplication>
#include <QPushButton>
#include <QWidget>
#include <cstdio>

static int g_failures = 0;
#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::printf("FAIL: %s (line %d)\n", #cond, __LINE__);        \
            ++g_failures;                                                \
        }                                                                \
    } while (0)

// Mirror of tests/fixtures/test_resource.h ids used here.
#define IDD_SAMPLE       1000
#define IDC_NAME_EDIT    1001
#define IDC_GO_BUTTON    1002
#define IDC_FILE_LIST    1006
#define IDC_STATIC       (-1)

static bool g_goFired = false;

class SampleDlg : public CDialog
{
public:
    SampleDlg() : CDialog(IDD_SAMPLE) {}
    afx_msg void OnGo() { g_goFired = true; }
    DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(SampleDlg, CDialog)
    ON_BN_CLICKED(IDC_GO_BUTTON, OnGo)
END_MESSAGE_MAP()

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    SampleDlg dlg;
    CHECK(dlg.Create(IDD_SAMPLE) == TRUE);   // builds from the IR template

    // Controls are addressable by their IDC_ ids.
    CWnd* edit = dlg.GetDlgItem(IDC_NAME_EDIT);
    CWnd* go   = dlg.GetDlgItem(IDC_GO_BUTTON);
    CWnd* list = dlg.GetDlgItem(IDC_FILE_LIST);
    CHECK(edit != nullptr);
    CHECK(go != nullptr);
    CHECK(list != nullptr);
    CHECK(dlg.GetDlgItem(9999) == nullptr);      // unknown id
    CHECK(dlg.GetDlgItem(IDC_STATIC) == nullptr); // IDC_STATIC is unaddressable

    // Geometry came through the dialog-unit -> pixel conversion (non-empty).
    if (go) {
        QWidget* qgo = reinterpret_cast<QWidget*>(go->GetSafeHwnd());
        CHECK(qgo != nullptr);
        if (qgo) CHECK(qgo->width() > 0 && qgo->height() > 0);
    }

    // SetWindowText / GetWindowText round-trip through the bound QLineEdit.
    if (edit) {
        edit->SetWindowText(_T("hello"));
        CString s;
        edit->GetWindowText(s);
        CHECK(s == _T("hello"));
    }

    // A real Qt click on the Go button routes through the message map.
    if (go) {
        QPushButton* qgo =
            qobject_cast<QPushButton*>(reinterpret_cast<QWidget*>(go->GetSafeHwnd()));
        CHECK(qgo != nullptr);
        CHECK(!g_goFired);
        if (qgo) qgo->click();
        CHECK(g_goFired);
    }

    dlg.DestroyWindow();

    if (g_failures == 0)
        std::printf("qt_dialog_test: .rc IR -> live Qt dialog, GetDlgItem + "
                    "text + click routing OK\n");
    return g_failures == 0 ? 0 : 1;
}
