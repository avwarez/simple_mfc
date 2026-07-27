// gui/qt/controls.cpp — typed control-class bodies for the Qt driver.
//
// The control classes (CButton/CEdit/CComboBox/CListBox/...) are declared in
// the frozen interface but their bodies are a driver mechanism, so they live
// here. Each method operates on the QWidget the control's CWnd is bound to
// (via DDX_Control or the dialog builder). Only the commonly used surface is
// implemented so far; more is added as eMule dialogs need it.
#include "afxwin.h"
#include "driver_internal.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QString>

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
