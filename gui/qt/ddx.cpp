// gui/qt/ddx.cpp — Dialog Data Exchange bodies for the Qt driver.
//
// The DDX_* functions and CDataExchange are declaration-only in the frozen
// interface (afxdd_.h / afxwin.h); their behaviour is a driver mechanism and
// lives here. Each DDX_ routine resolves its control id to the bound QWidget
// (via the dialog's GetDlgItem) and moves data in the direction CDataExchange
// requests: m_bSaveAndValidate FALSE loads the control from the member, TRUE
// saves the member from the control -- exactly the MFC contract.
#include "afxwin.h"
#include "driver_internal.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QString>
#include <QVariant>
#include <QWidget>

namespace {

QWidget* ctrlWidget(CDataExchange* pDX, int nIDC)
{
    if (!pDX || !pDX->m_pDlgWnd) return nullptr;
    CWnd* c = pDX->m_pDlgWnd->GetDlgItem(nIDC);
    return c ? smfc_qt::WidgetOf(c) : nullptr;
}

QString getText(QWidget* w)
{
    const QVariant v = w->property("text");
    return v.isValid() ? v.toString() : w->windowTitle();
}

void setText(QWidget* w, const QString& s)
{
    if (!w->setProperty("text", s))
        w->setWindowTitle(s);
}

template <class T>
void ddxInt(CDataExchange* pDX, int nIDC, T& value)
{
    QWidget* w = ctrlWidget(pDX, nIDC);
    if (!w) return;
    if (pDX->m_bSaveAndValidate)
        value = static_cast<T>(getText(w).toLongLong());
    else
        setText(w, QString::number(static_cast<qlonglong>(value)));
}

template <class T>
void ddxFloat(CDataExchange* pDX, int nIDC, T& value)
{
    QWidget* w = ctrlWidget(pDX, nIDC);
    if (!w) return;
    if (pDX->m_bSaveAndValidate)
        value = static_cast<T>(getText(w).toDouble());
    else
        setText(w, QString::number(static_cast<double>(value)));
}

} // namespace

// --- CDataExchange ---------------------------------------------------------
HWND CDataExchange::PrepareCtrl(int nIDC)
{
    if (!m_pDlgWnd) return nullptr;
    CWnd* c = m_pDlgWnd->GetDlgItem(nIDC);
    return c ? c->GetSafeHwnd() : nullptr;
}

HWND CDataExchange::PrepareEditCtrl(int nIDC) { return PrepareCtrl(nIDC); }

void CDataExchange::Fail()
{
    // Real MFC throws a CUserException here to unwind the DDX pass after a
    // failed validation. The driver has no exception-based validation flow
    // yet, so this is a no-op; DDV routines simply do not abort.
}

// --- DDX_Control -----------------------------------------------------------
void AFXAPI DDX_Control(CDataExchange* pDX, int nIDC, CWnd& rControl)
{
    if (!pDX) return;
    // Bind the member control object to the already-built dialog item, so the
    // app can call rControl.<method>() and GetDlgItem(nIDC) returns it.
    smfc_qt::BindDlgControl(pDX->m_pDlgWnd, nIDC, rControl);
}

// --- DDX_Text --------------------------------------------------------------
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, CString& value)
{
    QWidget* w = ctrlWidget(pDX, nIDC);
    if (!w) return;
    if (pDX->m_bSaveAndValidate)
        value = getText(w).toStdWString().c_str();
    else
        setText(w, QString::fromWCharArray(value.GetString()));
}

void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, unsigned char& v) { ddxInt(pDX, nIDC, v); }
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, short& v)         { ddxInt(pDX, nIDC, v); }
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, int& v)           { ddxInt(pDX, nIDC, v); }
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, UINT& v)          { ddxInt(pDX, nIDC, v); }
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, long& v)          { ddxInt(pDX, nIDC, v); }
#ifdef _WIN32
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, DWORD& v)         { ddxInt(pDX, nIDC, v); }
#endif
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, float& v)         { ddxFloat(pDX, nIDC, v); }
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, double& v)        { ddxFloat(pDX, nIDC, v); }

// --- DDX_Check / DDX_CBIndex ----------------------------------------------
void AFXAPI DDX_Check(CDataExchange* pDX, int nIDC, int& value)
{
    auto* b = qobject_cast<QAbstractButton*>(ctrlWidget(pDX, nIDC));
    if (!b) return;
    if (pDX->m_bSaveAndValidate)
        value = b->isChecked() ? 1 /*BST_CHECKED*/ : 0 /*BST_UNCHECKED*/;
    else
        b->setChecked(value != 0);
}

void AFXAPI DDX_CBIndex(CDataExchange* pDX, int nIDC, int& index)
{
    auto* cb = qobject_cast<QComboBox*>(ctrlWidget(pDX, nIDC));
    if (!cb) return;
    if (pDX->m_bSaveAndValidate)
        index = cb->currentIndex();
    else
        cb->setCurrentIndex(index);
}

// --- DDX_Radio -------------------------------------------------------------
// nIDC is the FIRST control of a radio-button group; `value` is the 0-based
// index of the checked button within the group (-1 == none). Load (FALSE)
// checks the value-th button; save (TRUE) reports which one is checked.
void AFXAPI DDX_Radio(CDataExchange* pDX, int nIDC, int& value)
{
    if (!pDX || !pDX->m_pDlgWnd) return;
    const std::vector<int> group = smfc_qt::RadioGroup(pDX->m_pDlgWnd, nIDC);
    if (group.empty()) return;

    if (pDX->m_bSaveAndValidate) {
        value = -1;
        for (int i = 0; i < static_cast<int>(group.size()); ++i) {
            auto* b = qobject_cast<QAbstractButton*>(ctrlWidget(pDX, group[i]));
            if (b && b->isChecked()) { value = i; break; }
        }
    } else {
        for (int i = 0; i < static_cast<int>(group.size()); ++i) {
            auto* b = qobject_cast<QAbstractButton*>(ctrlWidget(pDX, group[i]));
            if (b) b->setChecked(i == value);
        }
    }
}

// --- DDV_* (Dialog Data Validation) ----------------------------------------
// MFC runs DDV only in the save direction, right after the matching DDX_Text,
// and aborts the exchange via pDX->Fail() when the value is out of range. The
// driver's Fail() is a no-op for now (no CUserException unwinding), so an
// out-of-range value is reported to Fail() but not yet rejected.
void AFXAPI DDV_MinMaxInt(CDataExchange* pDX, int value, int minVal, int maxVal)
{
    if (pDX && pDX->m_bSaveAndValidate && (value < minVal || value > maxVal))
        pDX->Fail();
}

void AFXAPI DDV_MinMaxFloat(CDataExchange* pDX, float value, float minVal, float maxVal)
{
    if (pDX && pDX->m_bSaveAndValidate && (value < minVal || value > maxVal))
        pDX->Fail();
}
