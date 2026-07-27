// gui/qt/afxwinmain.cpp — the framework's entry point.
//
// This translation unit holds AfxWinMain and NOTHING else. main() is in its
// own file (gui/qt/main.cpp) on purpose: a static library's object is only
// pulled in to resolve a symbol something actually referenced, so keeping the
// two apart is what lets a program supply its own main and still CALL
// AfxWinMain - which is exactly what qt_app_test does. Put them together and
// referencing AfxWinMain drags main in with it, and the link fails on a
// duplicate symbol. Real MFC splits WinMain from AfxWinMain the same way.
//
// The MFC startup sequence, and what each step means here:
//
//   1. the application object is already constructed - it is a file-scope
//      global in the application (`CemuleApp theApp;`), so it exists before
//      main runs and AfxGetApp() already answers;
//   2. InitInstance() - the application builds its main window;
//   3. Run() - the message pump, until the application ends;
//   4. the value Run returned is the process exit code.
//
// If InitInstance returns FALSE the application declined to start, and MFC
// skips the pump but still runs ExitInstance. That path is reproduced exactly,
// because it is how an application reports "another instance is already
// running" - which eMule does.
#include "afxwin.h"
#include "driver_internal.h"

int AFXAPI AfxWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPTSTR lpCmdLine, int nCmdShow)
{
    CWinApp* pApp = AfxGetApp();
    if (pApp == nullptr)
        return -1;   // no application object was ever constructed

    // The toolkit has to exist before InitInstance runs: that is where an
    // application builds its main window, and no widget can be constructed
    // without it. Real MFC's equivalent is AfxWinInit, called from WinMain for
    // the same reason - the framework is initialised before application code
    // gets control, never lazily underneath it.
    if (smfc_qt::EnsureQApplication() == nullptr)
        return -1;

    pApp->m_hInstance = hInstance;
    pApp->m_hPrevInstance = hPrevInstance;
    pApp->m_lpCmdLine = lpCmdLine;
    pApp->m_nCmdShow = nCmdShow;

    if (!pApp->InitInstance()) {
        // MFC still tears down: an application that refused to start has
        // usually allocated something in InitInstance before deciding.
        return pApp->ExitInstance();
    }

    return pApp->Run();   // returns ExitInstance()'s value
}
