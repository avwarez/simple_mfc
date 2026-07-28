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
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFont>
#include <QFontMetrics>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QImage>
#include <QListWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QHeaderView>
#include <QTimer>
#include <QTreeWidget>
#include <QVariant>
#include <QWidget>

#include <cerrno>
#include <climits>
#include <cwchar>
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

// --- SetTimer/KillTimer registry -------------------------------------------
// A Win32 timer is owned by the window and identified by an id the caller
// chooses; re-arming an existing id replaces it rather than adding a second
// timer. There is nowhere in the frozen interface to hang a QTimer, so the
// timers live here, keyed by owner window and id.
std::unordered_map<const CWnd*, std::unordered_map<UINT_PTR, QTimer*>>& TimerMap()
{
    static std::unordered_map<const CWnd*, std::unordered_map<UINT_PTR, QTimer*>> m;
    return m;
}

// Stop and delete every timer owned by `w`. Called when the window goes away:
// a timer that outlived its window would keep firing WM_TIMER into freed memory.
void KillAllTimers(const CWnd* w)
{
    auto it = TimerMap().find(w);
    if (it == TimerMap().end()) return;
    for (auto& kv : it->second)
        if (kv.second) { kv.second->stop(); delete kv.second; }
    TimerMap().erase(it);
}
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

    // The typed object is only now able to act on its own style bits (the
    // placeholder CWnd could not): give the style-driven behaviours their
    // chance, which is how an LVS_OWNERDRAWFIXED list starts owner-drawing
    // without the app asking for it.
    ApplyStyleBehaviour(rControl);
}

std::vector<int> RadioGroup(CWnd* dlg, int nIDC)
{
    // WS_GROUP (0x00020000): the style bit that marks the first control of a
    // group. Real MFC's DDX_Radio starts at nIDC and stops at the next control
    // that carries it; we mirror that over the .rc template's control order.
    constexpr uint32_t kWsGroup = 0x00020000u;
    std::vector<int> ids;
    WndExtra* ex = ExtraIfAny(dlg);
    if (!ex) return ids;
    const smfc::DialogDesc* d = smfc::FindDialog(ex->templateId);
    if (!d) return ids;

    size_t start = d->controls.size();
    for (size_t i = 0; i < d->controls.size(); ++i)
        if (d->controls[i].id == nIDC) { start = i; break; }
    if (start == d->controls.size()) return ids;

    for (size_t i = start; i < d->controls.size(); ++i) {
        const smfc::ControlDesc& c = d->controls[i];
        if (i > start && (c.style & kWsGroup)) break;   // next group begins
        if (c.kind == smfc::ControlKind::RadioButton)
            ids.push_back(c.id);
    }
    return ids;
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
            if (k == "SysListView32") {
                auto* tree = new QTreeWidget(parent);
                // A list starts with no columns: InsertColumn defines them. Qt
                // gives a QTreeWidget one column labelled "1" and falls back to
                // that number whenever the header text is empty, so the header
                // stays hidden until there is something real to put in it.
                tree->header()->setVisible(false);
                tree->setRootIsDecorated(false);   // a list, not a tree
                return tree;
            }
            if (k == "SysTreeView32")   return new QTreeWidget(parent);
            if (k == "msctls_progress32") {
                // PBS_VERTICAL (0x04) fills bottom-to-top instead of
                // left-to-right. A Win32 progress bar shows no text, ever;
                // Qt's shows a percentage unless told otherwise.
                constexpr uint32_t kPbsVertical = 0x0004u;
                auto* bar = new QProgressBar(parent);
                bar->setOrientation((c.style & kPbsVertical) ? Qt::Vertical
                                                             : Qt::Horizontal);
                bar->setTextVisible(false);
                bar->setRange(0, 100);   // the Win32 default range
                bar->setValue(0);
                return bar;
            }
            if (k == "msctls_trackbar32") {
                // A trackbar is horizontal unless TBS_VERT (0x02) says
                // otherwise - TBS_HORZ is zero, so orientation cannot be read
                // as a set bit. Qt's QSlider defaults to VERTICAL, which is
                // why an untouched horizontal trackbar came out squashed into
                // its own width.
                constexpr uint32_t kTbsVert = 0x0002u;
                constexpr uint32_t kTbsAutoTicks = 0x0001u;
                constexpr uint32_t kTbsBoth = 0x0008u;
                constexpr uint32_t kTbsNoTicks = 0x0010u;
                const bool vertical = (c.style & kTbsVert) != 0;
                auto* sl = new QSlider(vertical ? Qt::Vertical : Qt::Horizontal,
                                       parent);
                if (c.style & kTbsNoTicks)
                    sl->setTickPosition(QSlider::NoTicks);
                else if (c.style & kTbsBoth)
                    sl->setTickPosition(QSlider::TicksBothSides);
                else if (c.style & kTbsAutoTicks)
                    sl->setTickPosition(vertical ? QSlider::TicksRight
                                                 : QSlider::TicksBelow);
                return sl;
            }
            if (k == "msctls_updown32")   return new QSpinBox(parent);
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
    if (message == WM_PAINT)
    {
        // ON_WM_PAINT's route: WM_PAINT -> OnPaint (virtual here, so a derived
        // class's OnPaint override is reached). The host widget's paintEvent
        // sends this, then blits the CDC surface OnPaint drew onto (see
        // SmfcPaintDialog). The base OnPaint is a no-op -> no surface -> the
        // widget keeps its default Qt appearance.
        OnPaint();
        return 0;
    }
    if (message == WM_TIMER)
    {
        // ON_WM_TIMER's route. The QTimer armed by SetTimer sends this with the
        // timer id in wParam, exactly as Win32 does, so a derived OnTimer
        // override is reached through the virtual.
        OnTimer(static_cast<UINT_PTR>(wParam));
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

// ---------------------------------------------------------------------------
// Window styles. On Windows these live in the HWND (GWL_STYLE); here they live
// on the QWidget, seeded from the .rc template at build time. Real behaviour,
// not a stub: ModifyStyle actually changes what the control does, because the
// style-driven behaviours are re-applied after every change.
// ---------------------------------------------------------------------------
DWORD CWnd::GetStyle() const
{
    QWidget* w = smfc_qt::WidgetOf(this);
    return w ? DWORD(w->property(smfc_qt::kStyleProp).toUInt()) : 0;
}

DWORD CWnd::GetExStyle() const
{
    QWidget* w = smfc_qt::WidgetOf(this);
    return w ? DWORD(w->property(smfc_qt::kExStyleProp).toUInt()) : 0;
}

namespace {
BOOL modifyStyleProp(CWnd* self, const char* prop, DWORD dwRemove, DWORD dwAdd)
{
    QWidget* w = smfc_qt::WidgetOf(self);
    if (!w) return FALSE;
    const DWORD before = DWORD(w->property(prop).toUInt());
    const DWORD after  = (before & ~dwRemove) | dwAdd;
    if (after == before) return TRUE;   // real MFC still reports success
    w->setProperty(prop, quint32(after));
    return TRUE;
}
} // namespace

BOOL CWnd::ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT /*nFlags*/)
{
    // nFlags carries SetWindowPos flags for the frame-change repaint; there is
    // no separate non-client frame to invalidate here, so it is not needed.
    const BOOL ok = modifyStyleProp(this, smfc_qt::kStyleProp, dwRemove, dwAdd);
    if (ok) smfc_qt::ApplyStyleBehaviour(*this);   // e.g. owner-draw on/off
    return ok;
}

BOOL CWnd::ModifyStyleEx(DWORD dwRemove, DWORD dwAdd, UINT /*nFlags*/)
{
    const BOOL ok = modifyStyleProp(this, smfc_qt::kExStyleProp, dwRemove, dwAdd);
    if (ok) smfc_qt::ApplyStyleBehaviour(*this);
    return ok;
}

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

CWnd::~CWnd()
{
    // Drop every driver-side entry keyed on this object or its handle. Without
    // this the maps outlive the object, and a later CWnd allocated at the same
    // address would inherit the dead one's child list and paint surface.
    //
    // The QWidget itself is deliberately NOT deleted here: Qt's parent-child
    // ownership already destroys child widgets with their dialog, and a bound
    // control (DDX_Control) does not own the widget it was attached to. A
    // window this object created is torn down by DestroyWindow, as in MFC.
    if (WndExtra* ex = ExtraIfAny(this)) {
        for (auto& child : ex->childWnds)
            if (child) child->Detach();
        ExtraMap().erase(this);
    }
    KillAllTimers(this);
    smfc_qt::ReleaseWndSurface(this);
    if (m_hWnd != nullptr) {
        // Only if the handle still maps back to us: a handle can have been
        // re-bound to another object (Detach/Attach), and that binding is live.
        auto it = HandleMap().find(m_hWnd);
        if (it != HandleMap().end() && it->second == this) HandleMap().erase(it);
        m_hWnd = nullptr;
    }
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
        KillAllTimers(this);                // a timer must not outlive its window
        smfc_qt::ReleaseWndSurface(this);   // free the CDC paint buffer, if any
        HandleMap().erase(m_hWnd);
        m_hWnd = nullptr;
        // close() before deleting: destroying a window has to be visible to the
        // message pump, because destroying the LAST one is how an MFC program
        // ends (the main window's destruction posts WM_QUIT). Deleting the
        // widget outright would take it away without that ever being announced,
        // and Run would sit waiting for an application that no longer has a UI.
        w->close();
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

// The client area in client coordinates: left/top are always zero, so this
// reports the size only, which is what drawing code needs it for.
void CWnd::GetClientRect(LPRECT lpRect) const
{
    if (lpRect == nullptr)
        return;
    lpRect->left = lpRect->top = lpRect->right = lpRect->bottom = 0;
    if (QWidget* w = smfc_qt::WidgetOf(this)) {
        lpRect->right = w->width();
        lpRect->bottom = w->height();
    }
}

// The window rectangle in SCREEN coordinates, which is what distinguishes it
// from GetClientRect.
void CWnd::GetWindowRect(LPRECT lpRect) const
{
    if (lpRect == nullptr)
        return;
    lpRect->left = lpRect->top = lpRect->right = lpRect->bottom = 0;
    if (QWidget* w = smfc_qt::WidgetOf(this)) {
        const QPoint tl = w->mapToGlobal(QPoint(0, 0));
        lpRect->left = tl.x();
        lpRect->top = tl.y();
        lpRect->right = tl.x() + w->width();
        lpRect->bottom = tl.y() + w->height();
    }
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

// ---------------------------------------------------------------------------
// Dialog-item helpers. Real MFC implements each of these as ::GetDlgItem(m_hWnd,
// nID) followed by the matching window message; here they route through our own
// GetDlgItem, which resolves the id through the map the dialog builder filled
// in. This is eMule's single busiest corner of the GUI API -- SetDlgItemText
// alone is called 729 times, more than any other method in the library.
// ---------------------------------------------------------------------------
void CWnd::SetDlgItemText(int nID, LPCTSTR lpszString)
{
    if (CWnd* c = GetDlgItem(nID))
        c->SetWindowText(lpszString);
}

int CWnd::GetDlgItemText(int nID, CString& rString) const
{
    rString.Empty();
    if (CWnd* c = GetDlgItem(nID))
        c->GetWindowText(rString);
    return rString.GetLength();
}

int CWnd::GetDlgItemText(int nID, LPTSTR lpStr, int nMaxCount) const
{
    // Win32 copies at most nMaxCount-1 characters plus the terminator, and
    // returns the number of characters actually COPIED -- not the length of the
    // control's text, which is why a caller cannot use the result to size a
    // buffer.
    if (lpStr == nullptr || nMaxCount <= 0)
        return 0;
    CString s;
    GetDlgItemText(nID, s);
    int n = s.GetLength();
    if (n > nMaxCount - 1)
        n = nMaxCount - 1;
    const LPCTSTR src = s.GetString();
    for (int i = 0; i < n; ++i)
        lpStr[i] = src[i];
    lpStr[n] = L'\0';
    return n;
}

void CWnd::SetDlgItemInt(int nID, UINT nValue, BOOL bSigned)
{
    const std::wstring s = bSigned
        ? std::to_wstring(static_cast<int>(nValue))
        : std::to_wstring(static_cast<unsigned>(nValue));
    SetDlgItemText(nID, s.c_str());
}

UINT CWnd::GetDlgItemInt(int nID, BOOL* lpTrans, BOOL bSigned) const
{
    // Win32: leading blanks are skipped, the first character that cannot be part
    // of the number ends the scan and makes the translation FAIL, and a failed
    // translation returns 0 with *lpTrans set FALSE. An unsigned request that
    // meets a minus sign fails too.
    if (lpTrans) *lpTrans = FALSE;
    CString s;
    GetDlgItemText(nID, s);
    const wchar_t* p = s.GetString();
    while (*p == L' ' || *p == L'\t') ++p;
    if (*p == L'\0')
        return 0;
    if (!bSigned && *p == L'-')
        return 0;

    errno = 0;
    wchar_t* end = nullptr;
    UINT result = 0;
    if (bSigned) {
        const long v = std::wcstol(p, &end, 10);
        if (errno == ERANGE || v < INT_MIN || v > INT_MAX) return 0;
        result = static_cast<UINT>(static_cast<int>(v));
    } else {
        const unsigned long v = std::wcstoul(p, &end, 10);
        if (errno == ERANGE || v > 0xFFFFFFFFul) return 0;
        result = static_cast<UINT>(v);
    }
    if (end == p || *end != L'\0')   // nothing consumed, or trailing garbage
        return 0;

    if (lpTrans) *lpTrans = TRUE;
    return result;
}

void CWnd::CheckDlgButton(int nIDButton, UINT nCheck)
{
    CWnd* c = GetDlgItem(nIDButton);
    if (!c) return;
    QWidget* w = smfc_qt::WidgetOf(c);
    // BST_INDETERMINATE is only meaningful for a tri-state check box; Win32
    // ignores it on anything else, and so does this.
    if (auto* cb = qobject_cast<QCheckBox*>(w)) {
        if (nCheck == BST_INDETERMINATE) {
            cb->setTristate(true);
            cb->setCheckState(Qt::PartiallyChecked);
            return;
        }
        cb->setCheckState(nCheck == BST_UNCHECKED ? Qt::Unchecked : Qt::Checked);
        return;
    }
    if (auto* b = qobject_cast<QAbstractButton*>(w))
        b->setChecked(nCheck != BST_UNCHECKED);
}

UINT CWnd::IsDlgButtonChecked(int nIDButton) const
{
    CWnd* c = GetDlgItem(nIDButton);
    if (!c) return BST_UNCHECKED;
    QWidget* w = smfc_qt::WidgetOf(c);
    if (auto* cb = qobject_cast<QCheckBox*>(w)) {
        switch (cb->checkState()) {
        case Qt::Checked:          return BST_CHECKED;
        case Qt::PartiallyChecked: return BST_INDETERMINATE;
        default:                   return BST_UNCHECKED;
        }
    }
    if (auto* b = qobject_cast<QAbstractButton*>(w))
        return b->isChecked() ? BST_CHECKED : BST_UNCHECKED;
    return BST_UNCHECKED;
}

void CWnd::CheckRadioButton(int nIDFirstButton, int nIDLastButton, int nIDCheckButton)
{
    // Win32 walks the id range and leaves exactly one button checked. Qt's radio
    // buttons in a shared parent are auto-exclusive, so setting the chosen one
    // would be enough -- but the range may hold check boxes or span groups, and
    // clearing explicitly is what Win32 guarantees.
    for (int id = nIDFirstButton; id <= nIDLastButton; ++id)
        CheckDlgButton(id, id == nIDCheckButton ? BST_CHECKED : BST_UNCHECKED);
}

// --- Coordinate mapping ----------------------------------------------------
void CWnd::ScreenToClient(LPPOINT lpPoint) const
{
    if (lpPoint == nullptr) return;
    if (QWidget* w = smfc_qt::WidgetOf(this)) {
        const QPoint p = w->mapFromGlobal(QPoint(lpPoint->x, lpPoint->y));
        lpPoint->x = p.x();
        lpPoint->y = p.y();
    }
}

void CWnd::ScreenToClient(LPRECT lpRect) const
{
    if (lpRect == nullptr) return;
    // Win32 maps both corners independently, which is what keeps the rectangle
    // right/bottom-exclusive through the conversion.
    POINT tl = { lpRect->left,  lpRect->top    };
    POINT br = { lpRect->right, lpRect->bottom };
    ScreenToClient(&tl);
    ScreenToClient(&br);
    lpRect->left = tl.x; lpRect->top    = tl.y;
    lpRect->right = br.x; lpRect->bottom = br.y;
}

void CWnd::ClientToScreen(LPPOINT lpPoint) const
{
    if (lpPoint == nullptr) return;
    if (QWidget* w = smfc_qt::WidgetOf(this)) {
        const QPoint p = w->mapToGlobal(QPoint(lpPoint->x, lpPoint->y));
        lpPoint->x = p.x();
        lpPoint->y = p.y();
    }
}

void CWnd::ClientToScreen(LPRECT lpRect) const
{
    if (lpRect == nullptr) return;
    POINT tl = { lpRect->left,  lpRect->top    };
    POINT br = { lpRect->right, lpRect->bottom };
    ClientToScreen(&tl);
    ClientToScreen(&br);
    lpRect->left = tl.x; lpRect->top    = tl.y;
    lpRect->right = br.x; lpRect->bottom = br.y;
}

// --- Repaint ---------------------------------------------------------------
// bErase (whether the background is wiped before WM_PAINT) has no Qt analogue:
// a Qt widget always repaints its background in paintEvent. Accepted and
// ignored, which matches what the caller observes -- a repainted window.
void CWnd::Invalidate(BOOL bErase) { InvalidateRect(nullptr, bErase); }

void CWnd::InvalidateRect(LPCRECT lpRect, BOOL /*bErase*/)
{
    QWidget* w = smfc_qt::WidgetOf(this);
    if (!w) return;
    if (lpRect)
        w->update(QRect(lpRect->left, lpRect->top,
                        lpRect->right - lpRect->left,
                        lpRect->bottom - lpRect->top));
    else
        w->update();   // whole client area
}

// Win32's UpdateWindow paints the pending invalid region IMMEDIATELY rather
// than posting WM_PAINT, which is the whole reason callers use it (progress
// feedback inside a long loop). QWidget::repaint has exactly that meaning;
// QWidget::update would not.
void CWnd::UpdateWindow()
{
    if (QWidget* w = smfc_qt::WidgetOf(this))
        w->repaint();
}

void CWnd::SetRedraw(BOOL bRedraw)
{
    if (QWidget* w = smfc_qt::WidgetOf(this))
        w->setUpdatesEnabled(bRedraw != FALSE);
}

// --- Focus / parent / state ------------------------------------------------
CWnd* CWnd::SetFocus()
{
    CWnd* prev = GetFocus();          // real MFC returns the previously focused
    if (QWidget* w = smfc_qt::WidgetOf(this))
        w->setFocus(Qt::OtherFocusReason);
    return prev;                      // window, or null if there was none
}

CWnd* CWnd::GetFocus()
{
    QWidget* f = QApplication::focusWidget();
    return f ? FromHandle(reinterpret_cast<HWND>(f)) : nullptr;
}

CWnd* CWnd::GetParent() const
{
    if (QWidget* w = smfc_qt::WidgetOf(this))
        if (QWidget* p = w->parentWidget())
            return FromHandle(reinterpret_cast<HWND>(p));
    return nullptr;
}

CWnd* CWnd::GetTopLevelParent() const
{
    if (QWidget* w = smfc_qt::WidgetOf(this))
        if (QWidget* t = w->window())     // Qt's own "the top-level ancestor"
            return FromHandle(reinterpret_cast<HWND>(t));
    return nullptr;
}

BOOL CWnd::IsWindowVisible() const
{
    QWidget* w = smfc_qt::WidgetOf(this);
    return (w && w->isVisible()) ? TRUE : FALSE;
}

BOOL CWnd::IsWindowEnabled() const
{
    QWidget* w = smfc_qt::WidgetOf(this);
    return (w && w->isEnabled()) ? TRUE : FALSE;
}

// --- Timers ----------------------------------------------------------------
UINT_PTR CWnd::SetTimer(UINT_PTR nIDEvent, UINT nElapse,
                        void(CALLBACK* lpfnTimer)(HWND, UINT, UINT_PTR, DWORD))
{
    // The TimerProc form is not supported: eMule always passes null and handles
    // WM_TIMER through its message map. Refusing loudly (returning 0, Win32's
    // failure value) beats silently arming a timer whose callback never runs.
    if (lpfnTimer != nullptr)
        return 0;

    QTimer*& t = TimerMap()[this][nIDEvent];
    if (t == nullptr) {
        t = new QTimer();
        // The lambda captures the owner and the id, so the timer sends the same
        // WM_TIMER Win32 would. KillAllTimers deletes it with the window, so the
        // captured `this` cannot outlive the object.
        QObject::connect(t, &QTimer::timeout, [this, nIDEvent] {
            SendMessage(WM_TIMER, static_cast<WPARAM>(nIDEvent), 0);
        });
    }
    t->start(static_cast<int>(nElapse));
    return nIDEvent;   // Win32 echoes the id back for a window timer
}

BOOL CWnd::KillTimer(UINT_PTR nIDEvent)
{
    auto wit = TimerMap().find(this);
    if (wit == TimerMap().end()) return FALSE;
    auto tit = wit->second.find(nIDEvent);
    if (tit == wit->second.end()) return FALSE;
    if (tit->second) { tit->second->stop(); delete tit->second; }
    wit->second.erase(tit);
    return TRUE;
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
// A dialog QWidget that routes Qt paint events into the MFC WM_PAINT path.
// paintEvent is the toolkit's "the OS wants this window redrawn" — the analog
// of WM_PAINT — so it dispatches WM_PAINT to the owning CWnd (reaching a derived
// OnPaint via the virtual), then blits whatever that OnPaint drew onto the
// window's offscreen CDC surface up to the real widget. This is what makes a
// CDialog/CWnd subclass that paints in OnPaint (via CPaintDC) actually show.
class SmfcPaintDialog : public QDialog {
public:
    using QDialog::QDialog;
protected:
    void paintEvent(QPaintEvent* e) override {
        QDialog::paintEvent(e);   // default background / frame first
        CWnd* w = CWnd::FromHandle(reinterpret_cast<HWND>(this));
        if (!w) return;
        w->SendMessage(WM_PAINT, 0, 0);   // -> OnPaint draws onto the CDC surface
        if (QImage* img = smfc_qt::WndSurfaceImage(w)) {
            QPainter p(this);
            p.drawImage(0, 0, *img);
        }
    }
};

QDialog* buildDialogFromTemplate(CWnd* self, int idd)
{
    const smfc::DialogDesc* d = smfc::FindDialog(idd);
    if (!d)
        return nullptr;   // no .rc supplied this dialog

    auto* qd = new SmfcPaintDialog();
    qd->setWindowTitle(ToQString(d->caption));
    const BaseUnits b = dialogBaseUnits(*d);
    if (d->cx > 0 && d->cy > 0)
        qd->resize((d->cx * b.x) / 4, (d->cy * b.y) / 8);

    WndExtra& ex = Extra(self);
    ex.templateId = idd;
    ex.idToWnd.clear();
    ex.childWnds.clear();

    qd->setProperty(smfc_qt::kStyleProp, quint32(d->style));
    qd->setProperty(smfc_qt::kExStyleProp, quint32(d->exStyle));

    for (const auto& c : d->controls) {
        bool isButton = false;
        QWidget* cw = makeControlWidget(c, qd, isButton);
        cw->setGeometry(duToPx(c, b));

        // Carry the template's style bits onto the widget: this is where
        // GetStyle()/ModifyStyle() read and write, and it is what lets a
        // style-driven behaviour (LVS_OWNERDRAWFIXED, ...) switch itself on
        // when a typed control object is bound - exactly as on Windows, where
        // the style lives in the HWND and the control consults it.
        cw->setProperty(smfc_qt::kStyleProp, quint32(c.style));
        cw->setProperty(smfc_qt::kExStyleProp, quint32(c.exStyle));

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
