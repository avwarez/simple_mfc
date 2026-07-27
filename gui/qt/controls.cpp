// gui/qt/controls.cpp — typed control-class bodies for the Qt driver.
//
// The control classes (CButton/CEdit/CComboBox/CListBox/...) are declared in
// the frozen interface but their bodies are a driver mechanism, so they live
// here. Each method operates on the QWidget the control's CWnd is bound to
// (via DDX_Control or the dialog builder). Only the commonly used surface is
// implemented so far; more is added as eMule dialogs need it.
#include "afxwin.h"
#include "afxcmn.h"
#include "driver_internal.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPixmap>
#include <QProgressBar>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QVariant>

#include <unordered_map>

namespace {
template <class T>
T* as(const CWnd* self) { return qobject_cast<T*>(smfc_qt::WidgetOf(self)); }

QString fromT(LPCTSTR s) { return s ? QString::fromWCharArray(s) : QString(); }

// Copy a QString into a caller LPTSTR buffer (assumed large enough, as the
// Win32/MFC GetText contract requires); returns the character count.
int copyOut(const QString& s, LPTSTR buf)
{
    const std::wstring w = s.toStdWString();
    if (buf) {
        for (size_t i = 0; i < w.size(); ++i) buf[i] = w[i];
        buf[w.size()] = L'\0';
    }
    return static_cast<int>(w.size());
}
} // namespace

// Dynamic Create() for the typed controls. These are the classes' "key
// functions" (first declared virtual), so defining them here also anchors each
// class's vtable in this translation unit. eMule's controls come from the .rc
// template and are attached via DDX_Control, so run-time Create is a stub for
// now (returns FALSE == not created); real dynamic creation is added when a
// dialog needs a control it builds itself.
BOOL CEdit::Create(DWORD, const RECT&, CWnd*, UINT) { return FALSE; }
BOOL CButton::Create(LPCTSTR, DWORD, const RECT&, CWnd*, UINT) { return FALSE; }
BOOL CComboBox::Create(DWORD, const RECT&, CWnd*, UINT) { return FALSE; }
BOOL CListBox::Create(DWORD, const RECT&, CWnd*, UINT) { return FALSE; }
BOOL CStatic::Create(LPCTSTR, DWORD, const RECT&, CWnd*, UINT) { return FALSE; }
BOOL CProgressCtrl::Create(DWORD, const RECT&, CWnd*, UINT) { return FALSE; }
BOOL CSpinButtonCtrl::Create(DWORD, const RECT&, CWnd*, UINT) { return FALSE; }

// --- CButton ---------------------------------------------------------------
int CButton::GetCheck() const
{
    if (auto* b = as<QAbstractButton>(this)) return b->isChecked() ? 1 : 0;
    return 0;
}
void CButton::SetCheck(int nCheck)
{
    if (auto* b = as<QAbstractButton>(this)) b->setChecked(nCheck != 0);
}

// --- CComboBox -------------------------------------------------------------
int CComboBox::GetCount() const
{
    auto* cb = as<QComboBox>(this);
    return cb ? cb->count() : 0;
}
int CComboBox::GetCurSel() const
{
    auto* cb = as<QComboBox>(this);
    return cb ? cb->currentIndex() : -1;   // CB_ERR == -1
}
int CComboBox::SetCurSel(int nSelect)
{
    auto* cb = as<QComboBox>(this);
    if (!cb) return -1;
    cb->setCurrentIndex(nSelect);
    return cb->currentIndex();
}
int CComboBox::AddString(LPCTSTR lpszString)
{
    auto* cb = as<QComboBox>(this);
    if (!cb) return -1;
    cb->addItem(fromT(lpszString));
    return cb->count() - 1;
}
void CComboBox::ResetContent()
{
    if (auto* cb = as<QComboBox>(this)) cb->clear();
}
int CComboBox::GetLBText(int nIndex, LPTSTR lpszText) const
{
    auto* cb = as<QComboBox>(this);
    if (!cb || nIndex < 0 || nIndex >= cb->count()) return -1; // CB_ERR
    return copyOut(cb->itemText(nIndex), lpszText);
}
void CComboBox::GetLBText(int nIndex, CString& rString) const
{
    auto* cb = as<QComboBox>(this);
    rString = (cb && nIndex >= 0 && nIndex < cb->count())
                  ? cb->itemText(nIndex).toStdWString().c_str()
                  : L"";
}
int CComboBox::DeleteString(UINT nIndex)
{
    auto* cb = as<QComboBox>(this);
    if (!cb || static_cast<int>(nIndex) >= cb->count()) return -1;
    cb->removeItem(static_cast<int>(nIndex));
    return cb->count();
}

// --- CListBox --------------------------------------------------------------
int CListBox::GetCount() const
{
    auto* lb = as<QListWidget>(this);
    return lb ? lb->count() : 0;
}
int CListBox::GetCurSel() const
{
    auto* lb = as<QListWidget>(this);
    return lb ? lb->currentRow() : -1;   // LB_ERR == -1
}
int CListBox::SetCurSel(int nSelect)
{
    auto* lb = as<QListWidget>(this);
    if (!lb) return -1;
    lb->setCurrentRow(nSelect);
    return lb->currentRow();
}
int CListBox::AddString(LPCTSTR lpszItem)
{
    auto* lb = as<QListWidget>(this);
    if (!lb) return -1;
    lb->addItem(fromT(lpszItem));
    return lb->count() - 1;
}
void CListBox::ResetContent()
{
    if (auto* lb = as<QListWidget>(this)) lb->clear();
}
int CListBox::GetText(int nIndex, LPTSTR lpszBuffer) const
{
    auto* lb = as<QListWidget>(this);
    if (!lb || nIndex < 0 || nIndex >= lb->count()) return -1; // LB_ERR
    return copyOut(lb->item(nIndex)->text(), lpszBuffer);
}
void CListBox::GetText(int nIndex, CString& rString) const
{
    auto* lb = as<QListWidget>(this);
    rString = (lb && nIndex >= 0 && nIndex < lb->count())
                  ? lb->item(nIndex)->text().toStdWString().c_str()
                  : L"";
}
int CListBox::DeleteString(UINT nIndex)
{
    auto* lb = as<QListWidget>(this);
    if (!lb || static_cast<int>(nIndex) >= lb->count()) return -1;
    delete lb->takeItem(static_cast<int>(nIndex));
    return lb->count();
}

// --- CSliderCtrl (over QSlider) -------------------------------------------
// Individual tic positions (SetTic/ClearTics) have no QSlider equivalent —
// Qt draws tics at a fixed interval only — so those are approximated via the
// tick interval; value/range/page/line map 1:1.
void CSliderCtrl::SetPos(int nPos)          { if (auto* s = as<QSlider>(this)) s->setValue(nPos); }
int  CSliderCtrl::GetPos() const            { auto* s = as<QSlider>(this); return s ? s->value() : 0; }
void CSliderCtrl::SetRange(int nMin, int nMax, BOOL) { if (auto* s = as<QSlider>(this)) s->setRange(nMin, nMax); }
void CSliderCtrl::SetRangeMin(int nMin, BOOL) { if (auto* s = as<QSlider>(this)) s->setMinimum(nMin); }
void CSliderCtrl::SetRangeMax(int nMax, BOOL) { if (auto* s = as<QSlider>(this)) s->setMaximum(nMax); }
void CSliderCtrl::GetRange(int& nMin, int& nMax) const
{
    auto* s = as<QSlider>(this);
    nMin = s ? s->minimum() : 0;
    nMax = s ? s->maximum() : 0;
}
int  CSliderCtrl::SetPageSize(int nSize)
{
    auto* s = as<QSlider>(this);
    if (!s) return -1;
    const int prev = s->pageStep();
    s->setPageStep(nSize);
    return prev;
}
int  CSliderCtrl::GetPageSize() const       { auto* s = as<QSlider>(this); return s ? s->pageStep() : 0; }
void CSliderCtrl::SetTicFreq(int nFreq)      { if (auto* s = as<QSlider>(this)) s->setTickInterval(nFreq); }
BOOL CSliderCtrl::SetTic(int /*nTic*/)       { return TRUE; }   // no per-tic API in Qt
void CSliderCtrl::SetLineSize(int nSize)     { if (auto* s = as<QSlider>(this)) s->setSingleStep(nSize); }
void CSliderCtrl::ClearTics(BOOL)            { if (auto* s = as<QSlider>(this)) s->setTickInterval(0); }
int  CSliderCtrl::GetNumTics() const
{
    auto* s = as<QSlider>(this);
    if (!s) return 0;
    const int iv = s->tickInterval();
    if (iv <= 0 || s->maximum() <= s->minimum()) return 0;
    return (s->maximum() - s->minimum()) / iv + 1;
}

// --- CProgressCtrl (over QProgressBar) ------------------------------------
// The Win32 progress bar carries a "step" (default 10) that StepIt advances by;
// QProgressBar has no such notion, so it lives in a dynamic property.
namespace {
constexpr int kDefaultProgressStep = 10;
int progressStep(QProgressBar* pb)
{
    const QVariant v = pb->property("smfc_step");
    return v.isValid() ? v.toInt() : kDefaultProgressStep;
}
} // namespace

void CProgressCtrl::SetRange(short nLower, short nUpper) { if (auto* p = as<QProgressBar>(this)) p->setRange(nLower, nUpper); }
void CProgressCtrl::SetRange32(int nLower, int nUpper)   { if (auto* p = as<QProgressBar>(this)) p->setRange(nLower, nUpper); }
void CProgressCtrl::GetRange(int& nLower, int& nUpper)
{
    auto* p = as<QProgressBar>(this);
    nLower = p ? p->minimum() : 0;
    nUpper = p ? p->maximum() : 0;
}
int CProgressCtrl::SetPos(int nPos)
{
    auto* p = as<QProgressBar>(this);
    if (!p) return 0;
    const int prev = p->value();
    p->setValue(nPos);
    return prev;
}
int CProgressCtrl::GetPos() { auto* p = as<QProgressBar>(this); return p ? p->value() : 0; }
int CProgressCtrl::OffsetPos(int nPos)
{
    auto* p = as<QProgressBar>(this);
    if (!p) return 0;
    const int prev = p->value();
    p->setValue(prev + nPos);
    return prev;
}
int CProgressCtrl::SetStep(int nStep)
{
    auto* p = as<QProgressBar>(this);
    if (!p) return 0;
    const int prev = progressStep(p);
    p->setProperty("smfc_step", nStep);
    return prev;
}
int CProgressCtrl::StepIt()
{
    auto* p = as<QProgressBar>(this);
    if (!p) return 0;
    const int prev = p->value();
    p->setValue(prev + progressStep(p));
    return prev;
}
BOOL CProgressCtrl::SetMarquee(BOOL fMarqueeMode, int /*nInterval*/)
{
    auto* p = as<QProgressBar>(this);
    if (!p) return FALSE;
    // A 0..0 range makes QProgressBar a busy (marquee) indicator; restore a
    // determinate 0..100 range when marquee is turned off.
    if (fMarqueeMode) p->setRange(0, 0);
    else              p->setRange(0, 100);
    return TRUE;
}

// --- CSpinButtonCtrl (over QSpinBox) --------------------------------------
// A Win32 up-down is arrows-only with a separate buddy edit; QSpinBox fuses
// both. SetBuddy therefore just records the buddy CWnd* (no live visual link);
// value/range/base map onto QSpinBox directly.
int CSpinButtonCtrl::SetPos(int nPos)
{
    auto* s = as<QSpinBox>(this);
    if (!s) return 0;
    const int prev = s->value();
    s->setValue(nPos);
    return prev;
}
int  CSpinButtonCtrl::GetPos() const           { auto* s = as<QSpinBox>(this); return s ? s->value() : 0; }
void CSpinButtonCtrl::SetRange(short nLower, short nUpper) { if (auto* s = as<QSpinBox>(this)) s->setRange(nLower, nUpper); }
void CSpinButtonCtrl::SetRange32(int nLower, int nUpper)   { if (auto* s = as<QSpinBox>(this)) s->setRange(nLower, nUpper); }
DWORD CSpinButtonCtrl::GetRange() const
{
    // UDM_GETRANGE packs the upper limit in the low word, the lower in the high
    // word (Win32 contract).
    auto* s = as<QSpinBox>(this);
    if (!s) return 0;
    const WORD upper = static_cast<WORD>(s->maximum());
    const WORD lower = static_cast<WORD>(s->minimum());
    return static_cast<DWORD>(upper) | (static_cast<DWORD>(lower) << 16);
}
void CSpinButtonCtrl::GetRange32(int& lower, int& upper) const
{
    auto* s = as<QSpinBox>(this);
    lower = s ? s->minimum() : 0;
    upper = s ? s->maximum() : 0;
}
CWnd* CSpinButtonCtrl::SetBuddy(CWnd* pWndBuddy)
{
    auto* s = as<QSpinBox>(this);
    if (!s) return nullptr;
    CWnd* prev = reinterpret_cast<CWnd*>(
        static_cast<quintptr>(s->property("smfc_buddy").toULongLong()));
    s->setProperty("smfc_buddy",
                   static_cast<qulonglong>(reinterpret_cast<quintptr>(pWndBuddy)));
    return prev;
}
CWnd* CSpinButtonCtrl::GetBuddy() const
{
    auto* s = as<QSpinBox>(this);
    if (!s) return nullptr;
    return reinterpret_cast<CWnd*>(
        static_cast<quintptr>(s->property("smfc_buddy").toULongLong()));
}
UINT CSpinButtonCtrl::SetBase(int nBase)
{
    auto* s = as<QSpinBox>(this);
    if (!s) return 0;
    const UINT prev = static_cast<UINT>(s->displayIntegerBase());
    s->setDisplayIntegerBase(nBase);
    return prev;
}
UINT CSpinButtonCtrl::GetBase() const
{
    auto* s = as<QSpinBox>(this);
    return s ? static_cast<UINT>(s->displayIntegerBase()) : 0;
}

// --- CStatic image accessors ----------------------------------------------
// SetBitmap/SetIcon store the GDI handle and, when the control is bound to a
// QLabel, render the image into it (bitmap via the CBitmap's QImage, icon via
// the driver's icon registry). The handle is kept in a driver-side map keyed by
// the CStatic* — the round-trip source of truth Get{Bitmap,Icon} returns — so it
// works whether or not a widget is attached (real MFC stores it on the HWND).
namespace {
struct StaticImg { HBITMAP bmp = nullptr; HICON icon = nullptr; };
std::unordered_map<const CStatic*, StaticImg>& StaticImgs()
{
    static std::unordered_map<const CStatic*, StaticImg> m;
    return m;
}
// Show a QImage (or clear, if null) on the CStatic's bound QLabel, if any.
void showOnLabel(const CStatic* self, const QImage* img)
{
    if (auto* lbl = as<QLabel>(self))
        lbl->setPixmap(img ? QPixmap::fromImage(*img) : QPixmap());
}
} // namespace

HBITMAP CStatic::SetBitmap(HBITMAP hBitmap)
{
    StaticImg& si = StaticImgs()[this];
    const HBITMAP prev = si.bmp;
    si.bmp = hBitmap;
    si.icon = nullptr;
    showOnLabel(this, smfc_qt::BitmapImageFromHandle(hBitmap));
    return prev;
}
HBITMAP CStatic::GetBitmap() const
{
    auto it = StaticImgs().find(this);
    return it == StaticImgs().end() ? nullptr : it->second.bmp;
}
HICON CStatic::SetIcon(HICON hIcon)
{
    StaticImg& si = StaticImgs()[this];
    const HICON prev = si.icon;
    si.icon = hIcon;
    si.bmp = nullptr;
    showOnLabel(this, smfc_qt::IconImage(hIcon));
    return prev;
}
HICON CStatic::GetIcon() const
{
    auto it = StaticImgs().find(this);
    return it == StaticImgs().end() ? nullptr : it->second.icon;
}
