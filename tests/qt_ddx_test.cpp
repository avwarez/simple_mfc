// qt_ddx_test.cpp — Dialog Data Exchange + typed controls on the Qt driver.
//
// A CDialog built from the .rc IR (IDD_SAMPLE) declares member controls and a
// DoDataExchange that binds them (DDX_Control) and transfers values
// (DDX_Text/DDX_Check/DDX_CBIndex). This checks the full MFC data flow:
//   - OnInitDialog -> UpdateData(FALSE) binds controls and LOADS them from the
//     dialog members;
//   - the typed controls (CEdit/CButton/CComboBox) drive their QWidgets;
//   - UpdateData(TRUE) SAVES the members back from the controls.
// Headless (QT_QPA_PLATFORM=offscreen); Create() is modeless so no event loop.
#include "afxwin.h"

#include <QApplication>
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
#define IDD_SAMPLE        1000
#define IDC_NAME_EDIT     1001
#define IDC_ENABLE_CHECK  1003
#define IDC_MODE_COMBO    1007

class DdxDlg : public CDialog
{
public:
    DdxDlg() : CDialog(IDD_SAMPLE) {}

    // Bound controls + exchanged members.
    CEdit     m_edit;
    CButton   m_check;
    CComboBox m_combo;
    CString m_name;
    int     m_checked = 0;
    int     m_modeIndex = 0;

    void DoDataExchange(CDataExchange* pDX) override
    {
        DDX_Control(pDX, IDC_NAME_EDIT, m_edit);
        DDX_Control(pDX, IDC_ENABLE_CHECK, m_check);
        DDX_Control(pDX, IDC_MODE_COMBO, m_combo);
        DDX_Text(pDX, IDC_NAME_EDIT, m_name);
        DDX_Check(pDX, IDC_ENABLE_CHECK, m_checked);
        DDX_CBIndex(pDX, IDC_MODE_COMBO, m_modeIndex);
    }
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    DdxDlg dlg;
    dlg.m_name = _T("initial");
    dlg.m_checked = 1;

    // Create -> OnInitDialog -> UpdateData(FALSE): controls bound + loaded.
    CHECK(dlg.Create(IDD_SAMPLE) == TRUE);

    // DDX_Control bound the member controls to real QWidgets.
    CHECK(dlg.m_edit.GetSafeHwnd() != nullptr);
    CHECK(dlg.m_check.GetSafeHwnd() != nullptr);
    CHECK(dlg.m_combo.GetSafeHwnd() != nullptr);
    // GetDlgItem now returns the typed control object itself.
    CHECK(dlg.GetDlgItem(IDC_NAME_EDIT) == &dlg.m_edit);

    // Values were LOADED from the members into the controls.
    CString shown;
    dlg.m_edit.GetWindowText(shown);
    CHECK(shown == _T("initial"));
    CHECK(dlg.m_check.GetCheck() == 1);

    // Drive the controls as a user would, through the typed API.
    dlg.m_edit.SetWindowText(_T("edited"));
    dlg.m_check.SetCheck(0);
    CHECK(dlg.m_combo.GetCount() == 0);
    dlg.m_combo.AddString(_T("Alpha"));
    dlg.m_combo.AddString(_T("Beta"));
    dlg.m_combo.AddString(_T("Gamma"));
    CHECK(dlg.m_combo.GetCount() == 3);
    dlg.m_combo.SetCurSel(2);
    CString item;
    dlg.m_combo.GetLBText(2, item);
    CHECK(item == _T("Gamma"));

    // UpdateData(TRUE): members SAVED back from the controls.
    dlg.UpdateData(TRUE);
    CHECK(dlg.m_name == _T("edited"));
    CHECK(dlg.m_checked == 0);
    CHECK(dlg.m_modeIndex == 2);

    dlg.DestroyWindow();

    if (g_failures == 0)
        std::printf("qt_ddx_test: DDX_Control/Text/Check/CBIndex + typed "
                    "controls load & save OK\n");
    return g_failures == 0 ? 0 : 1;
}
