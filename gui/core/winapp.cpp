// gui/core/winapp.cpp — CWinApp's toolkit-independent half: the process-wide
// "current application" pointer, the app-global Afx accessors that read it, the
// command line, and the idle protocol.
//
// This lives in gui/core, not gui/qt, because none of it needs a toolkit: it is
// bookkeeping and parsing over pointers and strings. Only the message pump
// itself (CWinApp::Run) is toolkit-bound, and that is in the driver.
//
// Real MFC keeps the current app in its module state (AFX_MODULE_STATE), a
// per-module structure reached through AfxGetModuleState(). That machinery is
// not part of the interface we implement, so the pointer lives here instead,
// under our own name. The observable behaviour is what MFC's is: constructing
// the single global application object is what makes AfxGetApp() work, from
// that moment until the object dies.
#include "afxwin.h"

#include <cstring>
#include <string>
#include <vector>

namespace {

// The one application object. Real MFC asserts when a second CWinApp is
// constructed in a module; nothing here can report an assertion, so the first
// one wins and the second simply does not take over - which keeps AfxGetApp()
// answering about the object the process was actually started for.
CWinApp* g_pCurrentApp = nullptr;

// The command line, as AfxWinMain received it. Kept here so ParseCommandLine
// has something to parse in a process the framework started.
std::vector<std::wstring>& CmdLineArgs()
{
    static std::vector<std::wstring> v;
    return v;
}

} // namespace

namespace smfc {

// Set by the driver's AfxWinMain before InitInstance runs. Declared in
// gui/core/winapp_internal.h.
void SetCommandLine(const std::vector<std::wstring>& args) { CmdLineArgs() = args; }

} // namespace smfc

// ---------------------------------------------------------------------------
// CWinApp
// ---------------------------------------------------------------------------
CWinApp::CWinApp(LPCTSTR lpszAppName)
    : m_hInstance(nullptr)
    , m_hPrevInstance(nullptr)
    , m_lpCmdLine(nullptr)
    , m_nCmdShow(1 /*SW_SHOWNORMAL*/)
    , m_pszAppName(lpszAppName)
    , m_pszRegistryKey(nullptr)
    , m_pszExeName(lpszAppName)
    , m_pszHelpFilePath(nullptr)
    , m_pszProfileName(nullptr)
    , m_dwPromptContext(0)
{
    // Constructing the application object is what registers it, exactly as in
    // MFC: `CemuleApp theApp;` at file scope is the whole of eMule's
    // registration, and every AfxGetApp() in the program depends on it.
    if (g_pCurrentApp == nullptr)
        g_pCurrentApp = this;

    // The app object is its own thread object for the main thread: MFC's
    // AfxWinMain runs the app THROUGH CWinThread's interface, and eMule reads
    // m_pMainWnd off it.
    m_pMainWnd = nullptr;
    m_pActiveWnd = nullptr;
}

CWinApp::~CWinApp()
{
    if (g_pCurrentApp == this)
        g_pCurrentApp = nullptr;
}

// Real MFC's CWinApp::OnIdle does the framework's own idle work at counts 0 and
// 1 (idle command-UI update, then temporary-map cleanup) and returns TRUE while
// it still has work queued, FALSE afterwards - which is the signal to the pump
// to stop calling it and block for the next message. Neither piece of framework
// work exists here yet (no command-UI update, and the temporary maps are freed
// by the destructors), so this reports "nothing more to do" straight away. An
// application that overrides OnIdle - as eMule does - still gets called and
// still controls the count, which is the part that matters.
BOOL CWinApp::OnIdle(LONG lCount)
{
    return lCount < 0 ? TRUE : FALSE;
}

// Whether a message counts as user activity, i.e. whether it should reset the
// idle count. Real MFC excludes the messages that arrive without the user doing
// anything: mouse moves that did not move, timers, and paints.
BOOL CWinApp::IsIdleMessage(MSG* pMsg)
{
    if (pMsg == nullptr)
        return TRUE;
    constexpr UINT kWmMouseMove = 0x0200;
    constexpr UINT kWmNcMouseMove = 0x00A0;
    constexpr UINT kWmPaint = 0x000F;
    constexpr UINT kWmTimer = 0x0113;
    const UINT m = pMsg->message;
    return !(m == kWmMouseMove || m == kWmNcMouseMove || m == kWmPaint || m == kWmTimer);
}

// Splits the command line into tokens and offers each to the CCommandLineInfo,
// flagging the ones introduced by '/' or '-' (Windows switch syntax, which is
// what eMule's own ParseParam override expects to be told about). The token is
// handed over WITHOUT its switch character, as real MFC does.
void CWinApp::ParseCommandLine(CCommandLineInfo& rCmdInfo)
{
    const std::vector<std::wstring>& args = CmdLineArgs();
    for (size_t i = 0; i < args.size(); ++i) {
        const std::wstring& a = args[i];
        if (a.empty())
            continue;
        const bool bFlag = (a[0] == L'/' || a[0] == L'-');
        const bool bLast = (i + 1 == args.size());
        rCmdInfo.ParseParam(bFlag ? a.c_str() + 1 : a.c_str(), bFlag ? TRUE : FALSE,
                            bLast ? TRUE : FALSE);
    }
}

// Help. Real MFC's EnableHtmlHelp swaps the engine by installing an HTML Help
// implementation of WinHelpInternal; the choice is remembered here, but neither
// engine exists on this platform - WinHelp is a Windows service and HTML Help a
// Windows component. Recording the request keeps the observable state right
// (eMule calls EnableHtmlHelp in its constructor) without pretending a help
// viewer will open. It is virtual because that is how MFC does the swap, and
// because it has to anchor CWinApp's vtable somewhere.
namespace {
bool g_bHtmlHelp = false;
}

void CWinApp::EnableHtmlHelp() { g_bHtmlHelp = true; }

void CWinApp::WinHelpInternal(DWORD_PTR /*dwData*/, UINT /*nCmd*/)
{
    // No help engine to route to; the request is dropped rather than faked.
}

// ---------------------------------------------------------------------------
// CCommandLineInfo — the defaults real MFC starts from, and the base
// ParseParam that turns a bare (non-switch) argument into "open this file".
// ---------------------------------------------------------------------------
CCommandLineInfo::CCommandLineInfo()
    : m_bShowSplash(TRUE)
    , m_bRunEmbedded(FALSE)
    , m_bRunAutomated(FALSE)
    , m_nShellCommand(FileNew)
{
}

void CCommandLineInfo::ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL /*bLast*/)
{
    if (lpszParam == nullptr)
        return;
    if (!bFlag) {
        // A plain argument is the document to open - how a shell "open with"
        // (and eMule's ed2k:// / magnet: link handoff) arrives.
        if (m_nShellCommand == FileNew) {
            m_nShellCommand = FileOpen;
            m_strFileName = lpszParam;
        }
        return;
    }
    // The switches real MFC understands itself. Anything else is the
    // application's business, and reaches it through its ParseParam override.
    if (std::wcscmp(lpszParam, L"Embedding") == 0) {
        m_bRunEmbedded = TRUE;
        m_bShowSplash = FALSE;
    } else if (std::wcscmp(lpszParam, L"Automation") == 0) {
        m_bRunAutomated = TRUE;
        m_bShowSplash = FALSE;
    }
}

// ---------------------------------------------------------------------------
// The app-global accessors. All of them are reads of the registered
// application object, which is why they are here and not in the driver.
// ---------------------------------------------------------------------------
CWinApp* AfxGetApp() { return g_pCurrentApp; }

LPCTSTR AfxGetAppName()
{
    return g_pCurrentApp ? g_pCurrentApp->m_pszAppName : nullptr;
}

CWnd* AfxGetMainWnd()
{
    // Real MFC returns the active window if the thread has one, and the main
    // window otherwise; the distinction only matters once modal dialogs track
    // activation, which they do not here yet.
    if (!g_pCurrentApp)
        return nullptr;
    return g_pCurrentApp->m_pActiveWnd ? g_pCurrentApp->m_pActiveWnd
                                       : g_pCurrentApp->m_pMainWnd;
}

// On Windows a module handle identifies the loaded image; there is no such
// thing here, so the application object's address serves as the process-wide
// unique, non-null token. Nothing in the covered call sites dereferences it -
// it is passed back to framework calls, which is what it stays valid for.
HINSTANCE AfxGetInstanceHandle()
{
    return g_pCurrentApp ? reinterpret_cast<HINSTANCE>(g_pCurrentApp) : nullptr;
}

HINSTANCE AfxGetResourceHandle()
{
    // Real MFC lets an application swap this to load resources from a
    // satellite DLL; with resources compiled in (see gui/core/rc/), there is
    // only ever the one module.
    return AfxGetInstanceHandle();
}
