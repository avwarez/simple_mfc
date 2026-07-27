// qt_demo_test.cpp — renders the demo application's window and checks that
// what reached the screen is what its MFC code asked for.
//
// It links examples/demo/demo.cpp unchanged: the same translation unit the
// smfc_demo executable is built from, application object and all. The only
// difference is that this file supplies main() and drives the startup by hand
// instead of entering the pump, so the window can be captured and inspected
// rather than waited on. Headless (QT_QPA_PLATFORM=offscreen).
#include "afxwin.h"
#include "driver_internal.h"
#include "resource.h"

#include <QColor>
#include <QImage>
#include <QProgressBar>
#include <QSlider>
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

static bool nearColor(const QColor& c, int r, int g, int b, int tol = 24)
{
    return qAbs(c.red() - r) <= tol && qAbs(c.green() - g) <= tol
        && qAbs(c.blue() - b) <= tol;
}

int main(int argc, char** argv)
{
    smfc_qt::SetProcessArgs(argc, argv);
    CHECK(smfc_qt::EnsureQApplication() != nullptr);

    // The application object registered itself at file scope, in demo.cpp.
    CWinApp* pApp = AfxGetApp();
    CHECK(pApp != nullptr);
    if (!pApp) { std::printf("no application object\n"); return 1; }
    CHECK(CString(AfxGetAppName()) == _T("simple_mfc demo"));

    CHECK(pApp->InitInstance() == TRUE);
    CWnd* pMain = AfxGetMainWnd();
    CHECK(pMain != nullptr);
    if (!pMain) { std::printf("no main window\n"); return 1; }

    QWidget* w = smfc_qt::WidgetOf(pMain);
    CHECK(w != nullptr);
    if (!w) { std::printf("main window not bound to a widget\n"); return 1; }
    CHECK(w->width() > 200 && w->height() > 150);   // built from the template

    QImage shot(w->size(), QImage::Format_ARGB32);
    shot.fill(Qt::black);
    w->render(&shot);
    shot.save(QString::fromUtf8("smfc_demo.png"));   // artefact, for eyeballing

    // 1) OnPaint's header band: RGB(238,242,248) across the top of the client
    //    area, drawn with a real CDC through the WM_PAINT route.
    CHECK(nearColor(shot.pixelColor(w->width() / 2, 6), 238, 242, 248));

    // 2) The owner-draw list painted its rows: DrawItem fills a green bar whose
    //    width comes from the row's item data. Nothing switched owner-draw on -
    //    the template's LVS_OWNERDRAWFIXED did.
    int greens = 0;
    for (int y = 0; y < shot.height(); ++y)
        for (int x = 0; x < shot.width(); ++x)
            if (nearColor(shot.pixelColor(x, y), 120, 190, 120, 30)) ++greens;
    CHECK(greens > 500);

    // 3) DDX moved the member into the edit control before the window showed.
    CWnd* pEdit = pMain->GetDlgItem(IDC_NAME_EDIT);
    CHECK(pEdit != nullptr);
    if (pEdit) {
        CString s;
        pEdit->GetWindowText(s);
        CHECK(s == _T("world"));
    }

    // 4) The controls the template asked for all exist and are addressable.
    CHECK(pMain->GetDlgItem(IDC_GREET_BUTTON) != nullptr);
    CHECK(pMain->GetDlgItem(IDC_ENABLE_CHECK) != nullptr);
    CHECK(pMain->GetDlgItem(IDC_TASK_LIST) != nullptr);
    CHECK(pMain->GetDlgItem(IDC_LEVEL_SLIDER) != nullptr);
    CHECK(pMain->GetDlgItem(IDC_PROGRESS) != nullptr);

    // 5) The template's control styles reached the widgets. A trackbar is
    //    horizontal unless TBS_VERT says otherwise (TBS_HORZ is zero, so it
    //    cannot be read as a set bit) - and Qt's QSlider defaults to vertical,
    //    which is exactly the trap this guards.
    auto* slider = qobject_cast<QSlider*>(
        smfc_qt::WidgetOf(pMain->GetDlgItem(IDC_LEVEL_SLIDER)));
    CHECK(slider != nullptr);
    if (slider) {
        CHECK(slider->orientation() == Qt::Horizontal);
        CHECK(slider->value() == 65);          // CSliderCtrl::SetPos
        CHECK(slider->minimum() == 0 && slider->maximum() == 100);
    }

    // A Win32 progress bar never shows text; Qt's shows a percentage unless
    // told not to.
    auto* bar = qobject_cast<QProgressBar*>(
        smfc_qt::WidgetOf(pMain->GetDlgItem(IDC_PROGRESS)));
    CHECK(bar != nullptr);
    if (bar) {
        CHECK(bar->orientation() == Qt::Horizontal);
        CHECK(bar->isTextVisible() == false);
        CHECK(bar->value() == 65);             // CProgressCtrl::SetPos
    }

    CHECK(pApp->ExitInstance() == 0);

    if (g_failures == 0)
        std::printf("qt_demo_test: the demo application's window rendered "
                    "(OnPaint + owner-draw rows + DDX) -> smfc_demo.png\n");
    return g_failures == 0 ? 0 : 1;
}
