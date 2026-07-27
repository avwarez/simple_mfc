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
#include "smfc_qt.h"
#include "driver_internal.h"
#include "dialog_ir.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFont>
#include <QFontMetrics>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSlider>
#include <QString>
#include <QTreeWidget>
#include <QVariant>
#include <QWidget>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
// QWidget* -> CWnd* so a Qt event/signal can find the C++ window object.
std::unordered_map<const void*, CWnd*>& HandleMap()
{
    static std::unordered_map<const void*, CWnd*> m;
    return m;
}
inline QWidget* AsQWidget(HWND h) { return reinterpret_cast<QWidget*>(h); }

// Driver-side per-window state. The frozen MFC interface (include/) carries no
// place to hang the built template id or the id->control map, and it must not
// (encapsulation policy: internal mechanisms stay driver-private with our own
// shapes). So it lives here, keyed by the owning CWnd*.
struct WndExtra {
    int templateId = 0;                       // CDialog's IDD, if any
    std::unordered_map<int, CWnd*> idToWnd;   // control id -> its CWnd wrapper
    std::vector<std::unique_ptr<CWnd>> childWnds; // owns the control wrappers
};

std::unordered_map<const CWnd*, WndExtra>& ExtraMap()
{
    static std::unordered_map<const CWnd*, WndExtra> m;
    return m;
}
WndExtra& Extra(const CWnd* w) { return ExtraMap()[w]; }
WndExtra* ExtraIfAny(const CWnd* w)
{
    auto it = ExtraMap().find(w);
    return it == ExtraMap().end() ? nullptr : &it->second;
}

QString ToQString(const std::string& s) { return QString::fromStdString(s); }
} // namespace

// Driver-internal helpers (declared in driver_internal.h). Defined here
// because the id->control map lives in this translation unit.
namespace smfc_qt {
QWidget* WidgetOf(const CWnd* w)
{
    return w ? AsQWidget(const_cast<CWnd*>(w)->GetSafeHwnd()) : nullptr;
}

void BindDlgControl(CWnd* dlg, int nIDC, CWnd& rControl)
{
    if (!dlg) return;
    WndExtra* ex = ExtraIfAny(dlg);
    if (!ex) return;
    auto it = ex->idToWnd.find(nIDC);
    if (it == ex->idToWnd.end() || !it->second) return;

    HWND h = it->second->GetSafeHwnd();
    if (!h) return;
    it->second->Detach();          // release the placeholder wrapper's binding
    rControl.Attach(h);            // the typed control now owns the HWND
    ex->idToWnd[nIDC] = &rControl; // GetDlgItem(nIDC) returns it from now on
}
} // namespace smfc_qt

namespace {

// Dialog units -> pixels (Win32 MapDialogRect): x*baseX/4, y*baseY/8, where
// the base units come from the dialog font metrics. Computed once per build.
struct BaseUnits { int x = 4; int y = 8; };
BaseUnits dialogBaseUnits(const smfc::DialogDesc& d)
{
    QFont f(ToQString(d.fontFace), d.fontSize > 0 ? d.fontSize : 8);
    QFontMetrics fm(f);
    BaseUnits b;
    b.x = fm.averageCharWidth();
    if (b.x <= 0) b.x = fm.horizontalAdvance(QLatin1Char('X'));
    if (b.x <= 0) b.x = 6;
    b.y = fm.height();
    if (b.y <= 0) b.y = 13;
    return b;
}
QRect duToPx(const smfc::ControlDesc& c, const BaseUnits& b)
{
    return QRect((c.x * b.x) / 4, (c.y * b.y) / 8,
                 (c.cx * b.x) / 4, (c.cy * b.y) / 8);
}

// Create the concrete Qt widget for a neutral control, set its caption, and
// (for buttons) report that it should be wired to the owner's message map.
QWidget* makeControlWidget(const smfc::ControlDesc& c, QWidget* parent,
                           bool& isButton)
{
    using smfc::ControlKind;
    const QString text = ToQString(c.text);
    isButton = false;
    switch (c.kind) {
        case ControlKind::DefButton:
        case ControlKind::Button: {
            auto* w = new QPushButton(text, parent);
            if (c.kind == ControlKind::DefButton) w->setDefault(true);
            isButton = true;
            return w;
        }
        case ControlKind::CheckBox: {
            auto* w = new QCheckBox(text, parent);
            isButton = true;
            return w;
        }
        case ControlKind::RadioButton: {
            auto* w = new QRadioButton(text, parent);
            isButton = true;
            return w;
        }
        case ControlKind::GroupBox:
            return new QGroupBox(text, parent);
        case ControlKind::Static:
        case ControlKind::StaticIcon:
            return new QLabel(text, parent);
        case ControlKind::Edit:
            return new QLineEdit(text, parent);
        case ControlKind::ListBox:
            return new QListWidget(parent);
        case ControlKind::ComboBox:
            return new QComboBox(parent);
        case ControlKind::ScrollBar:
            return new QScrollBar(parent);
        case ControlKind::Custom:
        default: {
            // Map the well-known Win32 common-control classes; anything else
            // becomes a bare placeholder QWidget so geometry/ids still work.
            const std::string& k = c.windowClass;
            if (k == "SysListView32")   return new QTreeWidget(parent);
            if (k == "SysTreeView32")   return new QTreeWidget(parent);
            if (k == "msctls_progress32") return new QProgressBar(parent);
            if (k == "msctls_trackbar32") return new QSlider(parent);
            return new QWidget(parent);
        }
    }
}
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
        // Release the built child-control wrappers and their handle-map
        // entries, then the window itself.
        if (WndExtra* ex = ExtraIfAny(this)) {
            for (auto& child : ex->childWnds)
                if (child) child->Detach();
            ExtraMap().erase(this);
        }
        HandleMap().erase(m_hWnd);
        m_hWnd = nullptr;
        w->deleteLater();       // Qt deletes the child widgets with the parent
        return TRUE;
    }
    return FALSE;
}

CWnd* CWnd::GetDlgItem(int nID) const
{
    // Resolved through the id->control map the .rc-driven dialog builder set
    // up (see buildDialogFromTemplate). Returns the control's CWnd wrapper,
    // exactly as real MFC's GetDlgItem returns a CWnd* for the child.
    if (const WndExtra* ex = ExtraIfAny(this)) {
        auto it = ex->idToWnd.find(nID);
        if (it != ex->idToWnd.end()) return it->second;
    }
    return nullptr;
}

// --- Control text / geometry / enable (operate on the bound QWidget) -------
void CWnd::SetWindowText(LPCTSTR lpszString)
{
    if (QWidget* w = AsQWidget(m_hWnd)) {
        const QString s = lpszString ? QString::fromWCharArray(lpszString)
                                     : QString();
        // QLabel/QAbstractButton/QLineEdit expose a "text" property; a plain
        // window (dialog) does not, so fall back to the window title.
        if (!w->setProperty("text", s))
            w->setWindowTitle(s);
    }
}

void CWnd::GetWindowText(CString& rString) const
{
    QString s;
    if (QWidget* w = AsQWidget(m_hWnd)) {
        const QVariant v = w->property("text");
        s = v.isValid() ? v.toString() : w->windowTitle();
    }
    rString = s.toStdWString().c_str();
}

void CWnd::MoveWindow(int x, int y, int nWidth, int nHeight, BOOL /*bRepaint*/)
{
    if (QWidget* w = AsQWidget(m_hWnd))
        w->setGeometry(x, y, nWidth, nHeight);
}

BOOL CWnd::EnableWindow(BOOL bEnable)
{
    if (QWidget* w = AsQWidget(m_hWnd)) {
        const BOOL wasDisabled = w->isEnabled() ? FALSE : TRUE;
        w->setEnabled(bEnable != FALSE);
        return wasDisabled;   // real MFC returns the previous *disabled* state
    }
    return FALSE;
}

// UpdateData drives Dialog Data Exchange: it builds a CDataExchange and hands
// it to the (virtual) DoDataExchange the derived dialog overrides. FALSE loads
// controls from members, TRUE saves members from controls -- exactly as MFC.
BOOL CWnd::UpdateData(BOOL bSaveAndValidate)
{
    CDataExchange dx;
    dx.m_pDlgWnd = this;
    dx.m_bSaveAndValidate = bSaveAndValidate;
    DoDataExchange(&dx);
    return TRUE;
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
// Dialog builder — turns a neutral dialog-IR template (from the .rc compiler)
// into a live QDialog with the template's controls, correct IDC_ ids and
// geometry, and buttons wired into the owner's message map. This is the Qt
// driver "consuming the IR": the same IR a future wx driver would consume.
// ---------------------------------------------------------------------------
namespace {
QDialog* buildDialogFromTemplate(CWnd* self, int idd)
{
    const smfc::DialogDesc* d = smfc::FindDialog(idd);
    if (!d)
        return nullptr;   // no .rc supplied this dialog

    auto* qd = new QDialog();
    qd->setWindowTitle(ToQString(d->caption));
    const BaseUnits b = dialogBaseUnits(*d);
    if (d->cx > 0 && d->cy > 0)
        qd->resize((d->cx * b.x) / 4, (d->cy * b.y) / 8);

    WndExtra& ex = Extra(self);
    ex.templateId = idd;
    ex.idToWnd.clear();
    ex.childWnds.clear();

    for (const auto& c : d->controls) {
        bool isButton = false;
        QWidget* cw = makeControlWidget(c, qd, isButton);
        cw->setGeometry(duToPx(c, b));

        // Wrap each control in a CWnd so GetDlgItem/DDX_Control get a CWnd*.
        auto wnd = std::make_unique<CWnd>();
        wnd->Attach(reinterpret_cast<HWND>(cw));
        if (c.id != 0 && c.id != -1)   // -1 == IDC_STATIC: unaddressable
            ex.idToWnd[c.id] = wnd.get();
        ex.childWnds.push_back(std::move(wnd));

        // Route button notifications through the owner's message map, so an
        // ON_BN_CLICKED(id, handler) in the derived dialog just fires.
        if (isButton && c.id != 0) {
            if (auto* qb = qobject_cast<QAbstractButton*>(cw))
                smfc_qt::WireButton(self, qb, static_cast<UINT>(c.id));
        }
    }

    self->Attach(reinterpret_cast<HWND>(qd));  // bind the dialog to its QWidget
    return qd;
}
} // namespace

// ---------------------------------------------------------------------------
// CDialog — now IR-driven. The IDD is captured at construction (driver-side,
// since the frozen interface has nowhere to store it) and used by Create/
// DoModal to build the template. DDX execution + typed control classes are
// the next step.
// ---------------------------------------------------------------------------
CDialog::CDialog() {}
CDialog::CDialog(UINT nIDTemplate, CWnd* /*pParentWnd*/)
{
    Extra(static_cast<CWnd*>(this)).templateId = static_cast<int>(nIDTemplate);
}
CDialog::CDialog(LPCTSTR /*lpszTemplateName*/, CWnd* /*pParentWnd*/) {}

INT_PTR CDialog::DoModal()
{
    const int idd = Extra(static_cast<CWnd*>(this)).templateId;
    QDialog* qd = buildDialogFromTemplate(this, idd);
    if (!qd)
        return -1;
    OnInitDialog();
    return static_cast<INT_PTR>(qd->exec());   // EndDialog -> QDialog::done
}

BOOL CDialog::Create(UINT nIDTemplate, CWnd* /*pParentWnd*/)
{
    // Modeless build: same template construction as DoModal, but non-blocking
    // (this is also what a headless test drives without an event loop).
    QDialog* qd = buildDialogFromTemplate(this, static_cast<int>(nIDTemplate));
    if (!qd)
        return FALSE;
    OnInitDialog();
    qd->show();
    return TRUE;
}

BOOL CDialog::Create(LPCTSTR, CWnd*) { return FALSE; }

void CDialog::EndDialog(int nResult)
{
    if (auto* qd = qobject_cast<QDialog*>(AsQWidget(m_hWnd)))
        qd->done(nResult);
}

BOOL CDialog::OnInitDialog()
{
    // Real MFC's CDialog::OnInitDialog loads the initial control state from the
    // dialog's members via DDX; a derived OnInitDialog reaches this through its
    // base call. (Controls were already built from the .rc template.)
    UpdateData(FALSE);
    return TRUE;
}
void CDialog::OnOK()     { EndDialog(1 /*IDOK*/); }
void CDialog::OnCancel() { EndDialog(2 /*IDCANCEL*/); }
