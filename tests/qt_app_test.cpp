// qt_app_test.cpp — the application bootstrap: a CWinApp at file scope, the
// AfxWinMain startup sequence, the message pump, and the idle protocol.
//
// This is the test that says an application RUNS rather than that its pieces
// compile. It defines its own main and calls AfxWinMain directly, which is
// what the framework's own main() (gui/qt/afxwinmain.cpp) does - that
// translation unit holds nothing else precisely so a program with its own main
// never pulls it in and never collides over the symbol.
// Headless (QT_QPA_PLATFORM=offscreen).
#include "afxwin.h"
#include "driver_internal.h"
#include "winapp_internal.h"

#include <cstdio>
#include <string>
#include <vector>

static int g_failures = 0;
#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::printf("FAIL: %s (line %d)\n", #cond, __LINE__);        \
            ++g_failures;                                                \
        }                                                                \
    } while (0)

#define IDD_SAMPLE 1000

// eMule's own command-line handling shape: a CCommandLineInfo subclass whose
// ParseParam picks up the switches the application cares about.
class TestCmdLine : public CCommandLineInfo
{
public:
    BOOL    sawFlag = FALSE;
    CString bare;
    void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast) override
    {
        if (bFlag && CString(lpszParam) == _T("quiet")) sawFlag = TRUE;
        CCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);
        if (!bFlag) bare = lpszParam;
    }
};

class TestApp : public CWinApp
{
public:
    int  inits = 0, exits = 0, idles = 0;
    bool ranPumpAfterInit = false;

    TestApp() : CWinApp(_T("qt_app_test")) {}

    BOOL InitInstance() override
    {
        ++inits;
        // What an MFC application does here: build the main window and publish
        // it as m_pMainWnd, which is what AfxGetMainWnd() then answers with.
        auto* dlg = new CDialog(IDD_SAMPLE);
        if (!dlg->Create(IDD_SAMPLE)) return FALSE;
        dlg->ShowWindow(1 /*SW_SHOWNORMAL*/);
        m_pMainWnd = dlg;
        return TRUE;
    }

    // Called by the pump while nothing is happening, with a count that rises.
    // Returning FALSE means "no more idle work" and stops the sequence, so
    // this one keeps the count going a few rounds, then ends the application
    // by destroying its main window - the ordinary way an MFC program stops.
    BOOL OnIdle(LONG lCount) override
    {
        ++idles;
        ranPumpAfterInit = true;
        if (lCount < 2) return TRUE;
        if (m_pMainWnd) m_pMainWnd->DestroyWindow();
        return FALSE;
    }

    int ExitInstance() override { ++exits; return 42; }
};

// File scope, exactly as `CemuleApp theApp;` is: constructing it is the whole
// of the registration, and it happens before main runs.
TestApp theApp;

int main(int argc, char** argv)
{
    // 1) The application registered itself by merely existing.
    CHECK(AfxGetApp() == &theApp);
    CHECK(CString(AfxGetAppName()) == _T("qt_app_test"));
    CHECK(AfxGetInstanceHandle() != nullptr);
    CHECK(AfxGetResourceHandle() == AfxGetInstanceHandle());
    CHECK(AfxGetMainWnd() == nullptr);         // no main window yet
    CHECK(theApp.inits == 0);                  // InitInstance has NOT run

    // 2) The command line reaches ParseCommandLine, switch flags and all.
    smfc::SetCommandLine({ L"/quiet", L"ed2k://file" });
    TestCmdLine info;
    CHECK(info.m_nShellCommand == CCommandLineInfo::FileNew);
    theApp.ParseCommandLine(info);
    CHECK(info.sawFlag == TRUE);
    CHECK(info.bare == _T("ed2k://file"));
    // A bare argument is the document to open - how a shell link handoff
    // arrives, which is exactly what eMule reads it for.
    CHECK(info.m_nShellCommand == CCommandLineInfo::FileOpen);
    CHECK(info.m_strFileName == _T("ed2k://file"));

    smfc_qt::SetProcessArgs(argc, argv);

    // 3) The startup sequence. AfxWinMain runs InitInstance, hands control to
    //    Run, and returns what ExitInstance returned.
    const int rc = AfxWinMain(nullptr, nullptr, nullptr, 1);

    CHECK(theApp.inits == 1);
    CHECK(theApp.exits == 1);
    CHECK(rc == 42);                     // Run returned ExitInstance()'s value

    // 4) The pump actually ran, and the idle protocol did its job: OnIdle was
    //    called with a rising count and the loop ended when it said so.
    CHECK(theApp.ranPumpAfterInit);
    CHECK(theApp.idles >= 3);

    if (g_failures == 0)
        std::printf("qt_app_test: CWinApp bootstrap (AfxWinMain -> InitInstance "
                    "-> Run/idle -> ExitInstance) OK\n");
    return g_failures == 0 ? 0 : 1;
}
