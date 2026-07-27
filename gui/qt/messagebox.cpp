// gui/qt/messagebox.cpp — AfxMessageBox.
//
// The application-modal prompt, mapped onto QMessageBox. What has to survive
// the translation is the calling convention, not the look: the nType flags
// select which buttons appear and which icon, and the return value is the
// standard command id of the button pressed (IDOK, IDYES, ...) - which is
// what application code compares against.
#include "afxwin.h"
#include "driver_internal.h"

#include <QMessageBox>
#include <QString>

namespace {

QMessageBox::StandardButtons buttonsFor(UINT nType)
{
    switch (nType & MB_TYPEMASK) {
        case MB_OKCANCEL:         return QMessageBox::Ok | QMessageBox::Cancel;
        case MB_ABORTRETRYIGNORE: return QMessageBox::Abort | QMessageBox::Retry | QMessageBox::Ignore;
        case MB_YESNOCANCEL:      return QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
        case MB_YESNO:            return QMessageBox::Yes | QMessageBox::No;
        case MB_RETRYCANCEL:      return QMessageBox::Retry | QMessageBox::Cancel;
        case MB_OK:
        default:                  return QMessageBox::Ok;
    }
}

QMessageBox::Icon iconFor(UINT nType)
{
    switch (nType & MB_ICONMASK) {
        case MB_ICONHAND:        return QMessageBox::Critical;
        case MB_ICONQUESTION:    return QMessageBox::Question;
        case MB_ICONEXCLAMATION: return QMessageBox::Warning;
        case MB_ICONASTERISK:    return QMessageBox::Information;
        default:                 return QMessageBox::NoIcon;
    }
}

int commandIdFor(QMessageBox::StandardButton b)
{
    switch (b) {
        case QMessageBox::Ok:     return IDOK;
        case QMessageBox::Cancel: return IDCANCEL;
        case QMessageBox::Abort:  return IDABORT;
        case QMessageBox::Retry:  return IDRETRY;
        case QMessageBox::Ignore: return IDIGNORE;
        case QMessageBox::Yes:    return IDYES;
        case QMessageBox::No:     return IDNO;
        default:                  return IDCANCEL;   // closed without choosing
    }
}

} // namespace

// nIDHelp is the help topic the box's Help button would open. There is no help
// engine on this platform (see CWinApp::WinHelpInternal), so no Help button is
// offered and the id is not used - dropping the request rather than faking it.
int AfxMessageBox(LPCTSTR lpszText, UINT nType, UINT /*nIDHelp*/)
{
    if (smfc_qt::EnsureQApplication() == nullptr)
        return IDCANCEL;

    QMessageBox box;
    box.setIcon(iconFor(nType));
    box.setStandardButtons(buttonsFor(nType));
    box.setText(lpszText ? QString::fromWCharArray(lpszText) : QString());
    // The title real MFC uses when the caller gives none: the application name.
    if (LPCTSTR name = AfxGetAppName())
        box.setWindowTitle(QString::fromWCharArray(name));

    if ((nType & MB_DEFBUTTON2) != 0) {
        const QList<QAbstractButton*> btns = box.buttons();
        if (btns.size() > 1)
            box.setDefaultButton(box.standardButton(btns[1]));
    }

    box.exec();
    return commandIdFor(box.standardButton(box.clickedButton()));
}

// The resource-id form. String resources are not carried yet (the .rc compiler
// emits dialog templates only), so there is no text to look up: rather than
// showing an empty box, this reports the same "dismissed" answer a caller
// would get from a box the user closed.
int AfxMessageBox(UINT /*nIDPrompt*/, UINT /*nType*/, UINT /*nIDHelp*/)
{
    return IDCANCEL;
}
