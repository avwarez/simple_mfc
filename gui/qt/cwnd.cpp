// gui/qt/cwnd.cpp — the CWnd/CDialog bodies for the Qt driver. This makes
// CWnd instantiable (all its virtuals get a definition, hence a vtable) and
// wires the message-routing path that turns a Windows message into a call
// into eMule's message map:
//     WindowProc -> (WM_COMMAND) OnCommand -> OnCmdMsg  [gui/core]
//                -> otherwise    OnWndMsg  (full WM_ switch: TODO)
//                -> DefWindowProc
// m_hWnd carries the owning QWidget* (cast to HWND); a reverse map recovers
// the CWnd* from a QWidget* for FromHandle/routing. Only the surface the
// Milestone-1 vertical slice needs is fleshed out; the rest are faithful
// no-op/DefWindowProc defaults (real MFC's defaults are the same shape).
#include "afxwin.h"

#include <QWidget>
#include <unordered_map>

namespace {
// QWidget* -> CWnd* so a Qt event/signal can find the C++ window object.
std::unordered_map<const void*, CWnd*>& HandleMap()
{
    static std::unordered_map<const void*, CWnd*> m;
    return m;
}
inline QWidget* AsQWidget(HWND h) { return reinterpret_cast<QWidget*>(h); }
} // namespace

// ---------------------------------------------------------------------------
// Message routing
// ---------------------------------------------------------------------------
LRESULT CWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_COMMAND)
    {
        if (OnCommand(wParam, lParam))
            return 0;
    }
    LRESULT result = 0;
    if (OnWndMsg(message, wParam, lParam, &result))
        return result;
    return DefWindowProc(message, wParam, lParam);
}

BOOL CWnd::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
{
    // WM_COMMAND packs the control id in the low word and the notification
    // code in the high word of wParam; route it as a command through the
    // (gui/core) CCmdTarget dispatcher.
    const UINT nID = static_cast<UINT>(wParam & 0xFFFF);
    const int  nCode = static_cast<int>((wParam >> 16) & 0xFFFF);
    return OnCmdMsg(nID, nCode, nullptr, nullptr);
}

// Full window-message (ON_WM_*) dispatch via the AfxSig window switch is added
// with the rest of the Qt driver; the command path above already covers the
// vertical slice.
BOOL CWnd::OnWndMsg(UINT, WPARAM, LPARAM, LRESULT*) { return FALSE; }
BOOL CWnd::OnChildNotify(UINT, WPARAM, LPARAM, LRESULT*) { return FALSE; }
BOOL CWnd::OnNotify(WPARAM, LPARAM, LRESULT*) { return FALSE; }
LRESULT CWnd::DefWindowProc(UINT, WPARAM, LPARAM) { return 0; }

// ---------------------------------------------------------------------------
// Minimal toolkit surface used by the slice / driver
// ---------------------------------------------------------------------------
LRESULT CWnd::SendMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    return WindowProc(message, wParam, lParam);
}

HWND CWnd::GetSafeHwnd() const { return (this != nullptr) ? m_hWnd : nullptr; }

BOOL CWnd::Attach(HWND hWndNew)
{
    m_hWnd = hWndNew;
    if (hWndNew != nullptr) HandleMap()[hWndNew] = this;
    return TRUE;
}

HWND CWnd::Detach()
{
    HWND h = m_hWnd;
    if (h != nullptr) HandleMap().erase(h);
    m_hWnd = nullptr;
    return h;
}

CWnd* CWnd::FromHandle(HWND hWnd)
{
    if (hWnd == nullptr) return nullptr;
    auto it = HandleMap().find(hWnd);
    return (it != HandleMap().end()) ? it->second : nullptr;
}

BOOL CWnd::ShowWindow(int nCmdShow)
{
    if (QWidget* w = AsQWidget(m_hWnd)) { w->setVisible(nCmdShow != 0 /*SW_HIDE*/); return TRUE; }
    return FALSE;
}

BOOL CWnd::DestroyWindow()
{
    if (QWidget* w = AsQWidget(m_hWnd))
    {
        HandleMap().erase(m_hWnd);
        m_hWnd = nullptr;
        w->deleteLater();
        return TRUE;
    }
    return FALSE;
}

CWnd* CWnd::GetDlgItem(int /*nID*/) const
{
    // Resolved through the id->widget map the .rc-driven dialog builder sets
    // up (Phase 3); not needed by the hand-wired vertical slice.
    return nullptr;
}

// Generic create surfaces: the concrete control/dialog builders (Phase 3)
// override/replace these; kept as linkable defaults so CWnd is instantiable.
BOOL CWnd::Create(LPCTSTR, LPCTSTR, DWORD, const RECT&, CWnd*, UINT, CCreateContext*) { return FALSE; }
BOOL CWnd::CreateEx(DWORD, LPCTSTR, LPCTSTR, DWORD, int, int, int, int, HWND, HMENU, LPVOID) { return FALSE; }
BOOL CWnd::CreateEx(DWORD, LPCTSTR, LPCTSTR, DWORD, const RECT&, CWnd*, UINT, LPVOID) { return FALSE; }

// ---------------------------------------------------------------------------
// Faithful no-op / default-return virtual handlers (real MFC's defaults have
// the same shape: do nothing, or defer to DefWindowProc). Overridden by the
// derived eMule classes; present here only so the vtable is complete.
// ---------------------------------------------------------------------------
BOOL CWnd::PreTranslateMessage(MSG*) { return FALSE; }
void CWnd::PreSubclassWindow() {}
void CWnd::PostNcDestroy() {}
void CWnd::DoDataExchange(CDataExchange*) {}
INT_PTR CWnd::OnToolHitTest(CPoint, TOOLINFO*) const { return -1; }

void   CWnd::OnPaint() {}
void   CWnd::OnDestroy() {}
void   CWnd::OnClose() {}
int    CWnd::OnCreate(LPCREATESTRUCT) { return 0; }
void   CWnd::OnSysColorChange() {}
BOOL   CWnd::OnHelpInfo(HELPINFO*) { return FALSE; }
void   CWnd::OnContextMenu(CWnd*, CPoint) {}
void   CWnd::OnTimer(UINT_PTR) {}
void   CWnd::OnMouseMove(UINT, CPoint) {}
BOOL   CWnd::OnMouseWheel(UINT, short, CPoint) { return FALSE; }
void   CWnd::OnLButtonUp(UINT, CPoint) {}
void   CWnd::OnLButtonDown(UINT, CPoint) {}
void   CWnd::OnLButtonDblClk(UINT, CPoint) {}
void   CWnd::OnRButtonDown(UINT, CPoint) {}
void   CWnd::OnMButtonUp(UINT, CPoint) {}
void   CWnd::OnNcLButtonDblClk(UINT, CPoint) {}
void   CWnd::OnNcDestroy() {}
void   CWnd::OnSize(UINT, int, int) {}
HBRUSH CWnd::OnCtlColor(CDC*, CWnd*, UINT) { return nullptr; }
void   CWnd::OnKeyDown(UINT, UINT, UINT) {}
void   CWnd::OnChar(UINT, UINT, UINT) {}
BOOL   CWnd::OnEraseBkgnd(CDC*) { return FALSE; }
void   CWnd::OnSetFocus(CWnd*) {}
void   CWnd::OnKillFocus(CWnd*) {}
void   CWnd::OnActivateApp(BOOL, DWORD) {}
BOOL   CWnd::OnSetCursor(CWnd*, UINT, UINT) { return FALSE; }
BOOL   CWnd::OnQueryNewPalette() { return FALSE; }
void   CWnd::OnPaletteChanged(CWnd*) {}
void   CWnd::OnHScroll(UINT, UINT, CScrollBar*) {}
void   CWnd::OnVScroll(UINT, UINT, CScrollBar*) {}
void   CWnd::OnSysCommand(UINT, LPARAM) {}
void   CWnd::OnShowWindow(BOOL, UINT) {}
void   CWnd::OnSettingChange(UINT, LPCTSTR) {}
UINT   CWnd::OnGetDlgCode() { return 0; }
void   CWnd::OnCaptureChanged(CWnd*) {}
void   CWnd::OnMenuSelect(UINT, UINT, HMENU) {}
void   CWnd::OnInitMenuPopup(CMenu*, UINT, BOOL) {}
void   CWnd::OnCancelMode() {}
void   CWnd::OnNcPaint() {}
BOOL   CWnd::OnNcActivate(BOOL) { return TRUE; }
LRESULT CWnd::OnNcHitTest(CPoint) { return 0; }
void   CWnd::OnNcLButtonDown(UINT, CPoint) {}
void   CWnd::OnNcRButtonDown(UINT, CPoint) {}
void   CWnd::OnNcCalcSize(BOOL, NCCALCSIZE_PARAMS*) {}
LRESULT CWnd::OnMenuChar(UINT, UINT, CMenu*) { return 0; }
BOOL   CWnd::OnQueryEndSession() { return TRUE; }
void   CWnd::OnEndSession(BOOL) {}
void   CWnd::OnMove(int, int) {}
void   CWnd::OnEnable(BOOL) {}
void   CWnd::OnActivate(UINT, CWnd*, BOOL) {}

// ---------------------------------------------------------------------------
// CDialog — minimal, enough to instantiate and to run the slice. The real
// .rc-driven builder (DoModal building the template's controls + DDX) is
// Phase 3.
// ---------------------------------------------------------------------------
CDialog::CDialog() {}
CDialog::CDialog(UINT /*nIDTemplate*/, CWnd* /*pParentWnd*/) {}
CDialog::CDialog(LPCTSTR /*lpszTemplateName*/, CWnd* /*pParentWnd*/) {}

INT_PTR CDialog::DoModal() { return -1; }               // 2 == IDCANCEL; real impl Phase 3
void    CDialog::EndDialog(int) {}
BOOL    CDialog::Create(LPCTSTR, CWnd*) { return FALSE; }
BOOL    CDialog::Create(UINT, CWnd*) { return FALSE; }
BOOL    CDialog::OnInitDialog() { return TRUE; }
void    CDialog::OnOK() {}
void    CDialog::OnCancel() {}
