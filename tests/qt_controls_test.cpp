// qt_controls_test.cpp — GDI-independent common controls on the Qt driver.
//
// A CDialog built from the .rc IR (IDD_CONTROLS) binds member controls with
// DDX_Control and exercises the typed API of the controls that map cleanly onto
// native Qt widgets without any GDI:
//   - CSliderCtrl   over QSlider      (pos/range/page/tic-freq)
//   - CProgressCtrl over QProgressBar (pos/range/step/StepIt/OffsetPos)
//   - CSpinButtonCtrl over QSpinBox   (pos/range/base/buddy)
//   - DDX_Radio over a QRadioButton group (load a selection, save it back)
// Headless (QT_QPA_PLATFORM=offscreen); Create() is modeless so no event loop.
#include "afxwin.h"
#include "afxcmn.h"

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
#define IDD_CONTROLS  1200
#define IDC_SLIDER    1201
#define IDC_PROGRESS2 1202
#define IDC_SPIN      1203
#define IDC_RADIO1    1204
#define IDC_RADIO2    1205

class CtrlDlg : public CDialog
{
public:
    CtrlDlg() : CDialog(IDD_CONTROLS) {}

    CSliderCtrl     m_slider;
    CProgressCtrl   m_progress;
    CSpinButtonCtrl m_spin;
    CButton         m_radio1;
    CButton         m_radio2;
    int             m_radio = 0;   // 0-based index of the checked radio

    void DoDataExchange(CDataExchange* pDX) override
    {
        DDX_Control(pDX, IDC_SLIDER, m_slider);
        DDX_Control(pDX, IDC_PROGRESS2, m_progress);
        DDX_Control(pDX, IDC_SPIN, m_spin);
        DDX_Control(pDX, IDC_RADIO1, m_radio1);
        DDX_Control(pDX, IDC_RADIO2, m_radio2);
        DDX_Radio(pDX, IDC_RADIO1, m_radio);
    }
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    CtrlDlg dlg;
    dlg.m_radio = 1;   // load should select the SECOND radio

    CHECK(dlg.Create(IDD_CONTROLS) == TRUE);

    // Controls bound to real QWidgets, GetDlgItem returns the member objects.
    CHECK(dlg.m_slider.GetSafeHwnd() != nullptr);
    CHECK(dlg.m_progress.GetSafeHwnd() != nullptr);
    CHECK(dlg.m_spin.GetSafeHwnd() != nullptr);
    CHECK(dlg.GetDlgItem(IDC_SLIDER) == &dlg.m_slider);
    CHECK(dlg.GetDlgItem(IDC_SPIN) == &dlg.m_spin);

    // --- CSliderCtrl -------------------------------------------------------
    dlg.m_slider.SetRange(0, 100);
    int lo = -1, hi = -1;
    dlg.m_slider.GetRange(lo, hi);
    CHECK(lo == 0 && hi == 100);
    dlg.m_slider.SetPos(42);
    CHECK(dlg.m_slider.GetPos() == 42);
    dlg.m_slider.SetPageSize(20);
    CHECK(dlg.m_slider.GetPageSize() == 20);
    dlg.m_slider.SetTicFreq(10);
    CHECK(dlg.m_slider.GetNumTics() == 11);   // (100-0)/10 + 1

    // --- CProgressCtrl -----------------------------------------------------
    dlg.m_progress.SetRange(0, 50);
    int plo = -1, phi = -1;
    dlg.m_progress.GetRange(plo, phi);
    CHECK(plo == 0 && phi == 50);
    dlg.m_progress.SetPos(10);
    CHECK(dlg.m_progress.GetPos() == 10);
    dlg.m_progress.SetStep(5);
    dlg.m_progress.StepIt();
    CHECK(dlg.m_progress.GetPos() == 15);     // 10 + step(5)
    dlg.m_progress.OffsetPos(3);
    CHECK(dlg.m_progress.GetPos() == 18);

    // --- CSpinButtonCtrl ---------------------------------------------------
    dlg.m_spin.SetRange32(1, 9);
    int slo = -1, shi = -1;
    dlg.m_spin.GetRange32(slo, shi);
    CHECK(slo == 1 && shi == 9);
    dlg.m_spin.SetPos(7);
    CHECK(dlg.m_spin.GetPos() == 7);
    CHECK(dlg.m_spin.SetBase(16) == 10);      // previous base was decimal
    CHECK(dlg.m_spin.GetBase() == 16);
    CHECK(dlg.m_spin.SetBuddy(&dlg.m_slider) == nullptr);
    CHECK(dlg.m_spin.GetBuddy() == &dlg.m_slider);

    // --- DDX_Radio ---------------------------------------------------------
    // Loaded with m_radio == 1, so the second radio must be checked.
    CHECK(dlg.m_radio2.GetCheck() == 1);
    CHECK(dlg.m_radio1.GetCheck() == 0);
    // Move the selection to the first radio (auto-exclusive) and save it back.
    dlg.m_radio1.SetCheck(1);
    CHECK(dlg.m_radio2.GetCheck() == 0);      // Qt auto-exclusivity
    dlg.UpdateData(TRUE);
    CHECK(dlg.m_radio == 0);

    dlg.DestroyWindow();

    if (g_failures == 0)
        std::printf("qt_controls_test: CSliderCtrl/CProgressCtrl/"
                    "CSpinButtonCtrl + DDX_Radio OK\n");
    return g_failures == 0 ? 0 : 1;
}
