// qt_dlgitem_test.cpp — CWnd's dialog-item and window-service helpers on the
// Qt driver: SetDlgItemText/Int, GetDlgItemText/Int, CheckDlgButton,
// IsDlgButtonChecked, CheckRadioButton, Screen/ClientToScreen, focus, parent,
// visibility and SetTimer/KillTimer.
//
// This is the busiest corner of the GUI API in the pilot project (eMule calls
// SetDlgItemText 729 times, more than any other method in the library) and it
// had no coverage at all until now.
//
// NOTE ON WHAT THESE ASSERTIONS ARE WORTH: unlike the CDC suite, these are NOT
// compared against real MFC -- there is no GUI golden recording yet, because
// recording one needs a CWinApp and a message loop on Windows. They encode the
// documented Win32 semantics (truncation and return value of GetDlgItemText,
// the failure contract of GetDlgItemInt, BST_* states), so they pin our
// behaviour and catch regressions, but a wrong *reading* of Win32 would not be
// caught here. Promoting them to a golden comparison is the natural next step.
//
// Runs headless (QT_QPA_PLATFORM=offscreen), modeless, no event loop needed
// except for the timer case, which pumps one explicitly.
#include "afxwin.h"

#include <QApplication>
#include <QEventLoop>
#include <QTimer>
#include <QWidget>
#include <cstdio>
#include <cstring>

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
#define IDC_ENABLE_CHECK 0x03EB
#define IDC_OPTION_A     1004
#define IDC_OPTION_B     1005

class SampleDlg : public CDialog
{
public:
    SampleDlg() : CDialog(IDD_SAMPLE) {}
    int timerHits = 0;
    UINT_PTR lastTimerId = 0;
    void OnTimer(UINT_PTR nIDEvent) override
    {
        ++timerHits;
        lastTimerId = nIDEvent;
    }
    DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(SampleDlg, CDialog)
END_MESSAGE_MAP()

// Pump the Qt event loop for `ms` milliseconds so queued timers can fire.
static void PumpFor(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    SampleDlg dlg;
    CHECK(dlg.Create(IDD_SAMPLE) == TRUE);

    // --- SetDlgItemText / GetDlgItemText -----------------------------------
    dlg.SetDlgItemText(IDC_NAME_EDIT, _T("hello world"));
    CString s;
    CHECK(dlg.GetDlgItemText(IDC_NAME_EDIT, s) == 11);
    CHECK(s == _T("hello world"));

    // The buffer overload returns the number of characters COPIED and always
    // terminates: with room for 4 characters + NUL it reports 4, not 11.
    TCHAR buf[5];
    CHECK(dlg.GetDlgItemText(IDC_NAME_EDIT, buf, 5) == 4);
    CHECK(CString(buf) == _T("hell"));
    CHECK(buf[4] == _T('\0'));

    // Degenerate buffers are refused rather than written to.
    CHECK(dlg.GetDlgItemText(IDC_NAME_EDIT, buf, 0) == 0);
    CHECK(dlg.GetDlgItemText(IDC_NAME_EDIT, nullptr, 5) == 0);

    // An unknown id yields empty text and length zero, never a crash.
    CHECK(dlg.GetDlgItemText(9999, s) == 0);
    CHECK(s.IsEmpty());
    dlg.SetDlgItemText(9999, _T("ignored"));   // no-op, must not crash

    // Empty text round-trips.
    dlg.SetDlgItemText(IDC_NAME_EDIT, _T(""));
    CHECK(dlg.GetDlgItemText(IDC_NAME_EDIT, s) == 0);

    // --- SetDlgItemInt / GetDlgItemInt -------------------------------------
    BOOL ok = FALSE;
    dlg.SetDlgItemInt(IDC_NAME_EDIT, 4711);
    CHECK(dlg.GetDlgItemText(IDC_NAME_EDIT, s) == 4);
    CHECK(s == _T("4711"));
    CHECK(dlg.GetDlgItemInt(IDC_NAME_EDIT, &ok) == 4711);
    CHECK(ok == TRUE);

    // Signed round-trip: -1 written signed reads back as -1.
    dlg.SetDlgItemInt(IDC_NAME_EDIT, static_cast<UINT>(-1), TRUE);
    CHECK(dlg.GetDlgItemText(IDC_NAME_EDIT, s) == 2);
    CHECK(s == _T("-1"));
    CHECK(static_cast<int>(dlg.GetDlgItemInt(IDC_NAME_EDIT, &ok, TRUE)) == -1);
    CHECK(ok == TRUE);

    // ...but read UNSIGNED, a minus sign is a failed translation: 0 + FALSE.
    ok = TRUE;
    CHECK(dlg.GetDlgItemInt(IDC_NAME_EDIT, &ok, FALSE) == 0);
    CHECK(ok == FALSE);

    // Unsigned round-trip past INT_MAX.
    dlg.SetDlgItemInt(IDC_NAME_EDIT, 3000000000u, FALSE);
    CHECK(dlg.GetDlgItemInt(IDC_NAME_EDIT, &ok, FALSE) == 3000000000u);
    CHECK(ok == TRUE);

    // Trailing garbage fails the whole translation (Win32 does not stop early
    // and keep the prefix).
    dlg.SetDlgItemText(IDC_NAME_EDIT, _T("12abc"));
    ok = TRUE;
    CHECK(dlg.GetDlgItemInt(IDC_NAME_EDIT, &ok) == 0);
    CHECK(ok == FALSE);

    // Leading blanks are skipped, not treated as garbage.
    dlg.SetDlgItemText(IDC_NAME_EDIT, _T("   42"));
    CHECK(dlg.GetDlgItemInt(IDC_NAME_EDIT, &ok) == 42);
    CHECK(ok == TRUE);

    // Empty text is a failure, and lpTrans is optional.
    dlg.SetDlgItemText(IDC_NAME_EDIT, _T(""));
    ok = TRUE;
    CHECK(dlg.GetDlgItemInt(IDC_NAME_EDIT, &ok) == 0);
    CHECK(ok == FALSE);
    CHECK(dlg.GetDlgItemInt(IDC_NAME_EDIT) == 0);   // null lpTrans accepted

    // --- CheckDlgButton / IsDlgButtonChecked -------------------------------
    CHECK(dlg.IsDlgButtonChecked(IDC_ENABLE_CHECK) == BST_UNCHECKED);
    dlg.CheckDlgButton(IDC_ENABLE_CHECK, BST_CHECKED);
    CHECK(dlg.IsDlgButtonChecked(IDC_ENABLE_CHECK) == BST_CHECKED);
    dlg.CheckDlgButton(IDC_ENABLE_CHECK, BST_INDETERMINATE);
    CHECK(dlg.IsDlgButtonChecked(IDC_ENABLE_CHECK) == BST_INDETERMINATE);
    dlg.CheckDlgButton(IDC_ENABLE_CHECK, BST_UNCHECKED);
    CHECK(dlg.IsDlgButtonChecked(IDC_ENABLE_CHECK) == BST_UNCHECKED);

    // Unknown ids: reading reports unchecked, writing is a no-op.
    CHECK(dlg.IsDlgButtonChecked(9999) == BST_UNCHECKED);
    dlg.CheckDlgButton(9999, BST_CHECKED);

    // --- CheckRadioButton ---------------------------------------------------
    dlg.CheckRadioButton(IDC_OPTION_A, IDC_OPTION_B, IDC_OPTION_B);
    CHECK(dlg.IsDlgButtonChecked(IDC_OPTION_B) == BST_CHECKED);
    CHECK(dlg.IsDlgButtonChecked(IDC_OPTION_A) == BST_UNCHECKED);
    dlg.CheckRadioButton(IDC_OPTION_A, IDC_OPTION_B, IDC_OPTION_A);
    CHECK(dlg.IsDlgButtonChecked(IDC_OPTION_A) == BST_CHECKED);
    CHECK(dlg.IsDlgButtonChecked(IDC_OPTION_B) == BST_UNCHECKED);

    // --- Coordinate mapping -------------------------------------------------
    // ClientToScreen then ScreenToClient must return the original point: that
    // round-trip is the property callers actually depend on.
    CWnd* edit = dlg.GetDlgItem(IDC_NAME_EDIT);
    CHECK(edit != nullptr);
    if (edit) {
        POINT p = { 7, 9 };
        edit->ClientToScreen(&p);
        edit->ScreenToClient(&p);
        CHECK(p.x == 7 && p.y == 9);

        RECT r = { 1, 2, 30, 40 };
        edit->ClientToScreen(&r);
        edit->ScreenToClient(&r);
        CHECK(r.left == 1 && r.top == 2 && r.right == 30 && r.bottom == 40);

        // A child's client origin in screen coordinates equals its window
        // rectangle's top-left -- the two APIs must agree with each other.
        POINT origin = { 0, 0 };
        edit->ClientToScreen(&origin);
        RECT wr;
        edit->GetWindowRect(&wr);
        CHECK(origin.x == wr.left && origin.y == wr.top);

        // Null arguments are ignored rather than dereferenced.
        edit->ClientToScreen(static_cast<LPPOINT>(nullptr));
        edit->ScreenToClient(static_cast<LPRECT>(nullptr));
    }

    // --- Parent / top-level -------------------------------------------------
    if (edit) {
        CHECK(edit->GetParent() == static_cast<CWnd*>(&dlg));
        CHECK(edit->GetTopLevelParent() == static_cast<CWnd*>(&dlg));
    }
    CHECK(dlg.GetParent() == nullptr);   // the dialog is top-level here

    // --- Visible / enabled --------------------------------------------------
    CHECK(dlg.IsWindowVisible() == TRUE);       // Create() shows it
    if (edit) {
        CHECK(edit->IsWindowEnabled() == TRUE);
        edit->EnableWindow(FALSE);
        CHECK(edit->IsWindowEnabled() == FALSE);
        edit->EnableWindow(TRUE);
        CHECK(edit->IsWindowEnabled() == TRUE);
    }

    // --- Focus --------------------------------------------------------------
    // Qt hands out keyboard focus only inside the ACTIVE window, so activate
    // the dialog first. On a real desktop the window manager does this when the
    // user brings the dialog up; headless there is nobody to do it, and without
    // it QApplication::focusWidget() stays null no matter what has focus.
    QApplication::setActiveWindow(reinterpret_cast<QWidget*>(dlg.GetSafeHwnd()));

    // SetFocus returns the PREVIOUSLY focused window (null if there was none),
    // which is the contract callers use to restore focus afterwards.
    if (edit) {
        CWnd* optA = dlg.GetDlgItem(IDC_OPTION_A);
        CHECK(optA != nullptr);
        edit->SetFocus();
        CHECK(CWnd::GetFocus() == edit);
        if (optA) {
            CWnd* prev = optA->SetFocus();
            CHECK(prev == edit);                 // the one focused a moment ago
            CHECK(CWnd::GetFocus() == optA);
        }
    }

    // --- Repaint surface ----------------------------------------------------
    // No observable result to assert headless; these must simply be safe to
    // call on a bound window and on an unbound one.
    dlg.Invalidate();
    RECT ir = { 0, 0, 10, 10 };
    dlg.InvalidateRect(&ir);
    dlg.InvalidateRect(nullptr);
    dlg.SetRedraw(FALSE);
    dlg.SetRedraw(TRUE);
    dlg.UpdateWindow();
    {
        CWnd unbound;
        unbound.Invalidate();
        unbound.UpdateWindow();
        unbound.SetRedraw(FALSE);
        CHECK(unbound.IsWindowVisible() == FALSE);
        CHECK(unbound.GetParent() == nullptr);
    }

    // --- Timers -------------------------------------------------------------
    // A window timer fires WM_TIMER into the message map, reaching the derived
    // OnTimer with the id the caller chose.
    CHECK(dlg.SetTimer(7, 10) == 7);
    PumpFor(120);
    CHECK(dlg.timerHits > 0);
    CHECK(dlg.lastTimerId == 7);

    // KillTimer stops it: after killing, the count must stop moving.
    CHECK(dlg.KillTimer(7) == TRUE);
    const int hitsAtKill = dlg.timerHits;
    PumpFor(80);
    CHECK(dlg.timerHits == hitsAtKill);

    // Killing an unknown/already-killed timer reports failure, not a crash.
    CHECK(dlg.KillTimer(7) == FALSE);
    CHECK(dlg.KillTimer(1234) == FALSE);

    // Re-arming the same id replaces the timer rather than adding a second one:
    // a duplicate would roughly double the hit rate.
    dlg.timerHits = 0;
    CHECK(dlg.SetTimer(9, 10) == 9);
    CHECK(dlg.SetTimer(9, 10) == 9);
    PumpFor(120);
    const int single = dlg.timerHits;
    CHECK(single > 0);
    CHECK(dlg.KillTimer(9) == TRUE);   // one kill is enough to silence it
    const int hitsAfter = dlg.timerHits;
    PumpFor(80);
    CHECK(dlg.timerHits == hitsAfter);

    // The TimerProc form is refused (0 == Win32's failure value) rather than
    // silently armed with a callback that would never run.
    CHECK(dlg.SetTimer(11, 10,
                       reinterpret_cast<void(CALLBACK*)(HWND, UINT, UINT_PTR, DWORD)>(&PumpFor)) == 0);

    dlg.DestroyWindow();

    if (g_failures == 0)
        std::printf("qt_dlgitem_test: dialog-item text/int, button state, "
                    "coordinate mapping, parent/state and timers OK\n");
    return g_failures == 0 ? 0 : 1;
}
