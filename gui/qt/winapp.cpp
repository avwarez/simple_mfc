// gui/qt/winapp.cpp — CWinApp's toolkit-bound half: the message pump.
//
// Real MFC's CWinThread::Run is an explicit loop: drain the queue with
// PumpMessage, and when nothing is pending call OnIdle repeatedly until it
// says it has no more work, then block. Qt owns its loop (QApplication::exec),
// so Run cannot BE that loop - it drives it, and reproduces the two observable
// behaviours an application depends on:
//
//   - OnIdle is called with a count that rises while nothing happens and
//     resets when the user does something, and calling stops as soon as it
//     returns FALSE;
//   - the value Run returns is ExitInstance()'s, called once the loop ends.
//
// eMule overrides both OnIdle and IsIdleMessage, so both have to be reached
// through the real virtual calls rather than approximated.
#include "afxwin.h"
#include "driver_internal.h"

#include <QApplication>
#include <QEvent>
#include <QObject>
#include <QTimer>

namespace {

// The process argv, as main() received it. QApplication keeps a reference to
// argc/argv for its whole life, so these must outlive it - hence file scope
// rather than locals. Defaults to a synthetic one-argument line for a program
// that embeds the framework without going through our main() (the tests).
int    g_argc = 1;
char   g_arg0[] = "simple_mfc";
char*  g_argvDefault[] = { g_arg0, nullptr };
char** g_argv = g_argvDefault;

// Which Qt events correspond to a Win32 window message at all, and to which.
// The distinction matters: MFC's pump only ever sees window messages, whereas
// Qt's event stream also carries purely internal traffic (layout requests,
// deferred deletes, polish). Feeding those to the idle logic as if they were
// messages would reset the idle count forever and the sequence would never
// advance. So anything without a real Win32 counterpart returns 0 = "not a
// window message", and is ignored rather than counted as activity.
//
// The ones that DO map are handed to IsIdleMessage, which is the hook whose
// whole purpose is letting an application declare that some of them - a mouse
// move, a paint, a timer - are not user activity. eMule overrides it.
UINT win32MessageFor(QEvent::Type t)
{
    switch (t) {
        case QEvent::MouseMove:              return 0x0200;   // WM_MOUSEMOVE
        case QEvent::NonClientAreaMouseMove: return 0x00A0;   // WM_NCMOUSEMOVE
        case QEvent::MouseButtonPress:       return 0x0201;   // WM_LBUTTONDOWN
        case QEvent::MouseButtonDblClick:    return 0x0203;   // WM_LBUTTONDBLCLK
        case QEvent::MouseButtonRelease:     return 0x0202;   // WM_LBUTTONUP
        case QEvent::Wheel:                  return 0x020A;   // WM_MOUSEWHEEL
        case QEvent::KeyPress:               return 0x0100;   // WM_KEYDOWN
        case QEvent::KeyRelease:             return 0x0101;   // WM_KEYUP
        case QEvent::Paint:                  return 0x000F;   // WM_PAINT
        case QEvent::UpdateRequest:          return 0x000F;   // WM_PAINT
        case QEvent::Timer:                  return 0x0113;   // WM_TIMER
        default:                             return 0x0000;   // not a message
    }
}

// Watches the application's event stream and keeps the idle state in sync with
// it, which is the job MFC's pump does inline between PumpMessage calls.
class IdleDriver : public QObject
{
public:
    IdleDriver(CWinApp* app, QObject* parent)
        : QObject(parent), m_app(app)
    {
        qApp->installEventFilter(this);
        // A zero-interval timer fires when Qt has nothing else to do, which is
        // the same condition MFC's "no message pending" test detects.
        m_timer.setInterval(0);
        m_timer.setSingleShot(false);
        QObject::connect(&m_timer, &QTimer::timeout, this, [this] { onIdleTick(); });
        m_timer.start();
    }

protected:
    bool eventFilter(QObject* obj, QEvent* e) override
    {
        MSG msg{};
        msg.message = win32MessageFor(e->type());
        if (msg.message != 0 && m_app->IsIdleMessage(&msg)) {
            // User activity: the idle sequence starts again from zero, exactly
            // as MFC resets lIdleCount when PumpMessage saw a real message.
            m_count = 0;
            if (!m_timer.isActive()) m_timer.start();
        }
        return QObject::eventFilter(obj, e);
    }

private:
    void onIdleTick()
    {
        // OnIdle returning FALSE means "no more idle work": stop calling until
        // the next activity wakes the sequence up again. Without this the timer
        // would spin the CPU for as long as the application is running.
        if (!m_app->OnIdle(m_count++))
            m_timer.stop();
    }

    CWinApp* m_app;
    QTimer   m_timer;
    LONG     m_count = 0;
};

} // namespace

namespace smfc_qt {

void SetProcessArgs(int argc, char** argv)
{
    if (argc > 0 && argv != nullptr) { g_argc = argc; g_argv = argv; }
}

QApplication* EnsureQApplication()
{
    if (qApp != nullptr)
        return qobject_cast<QApplication*>(qApp);
    // Deliberately leaked: it must outlive every widget, and the framework has
    // no point after Run at which destroying it would be safe.
    return new QApplication(g_argc, g_argv);
}

} // namespace smfc_qt

int CWinApp::Run()
{
    // Already created by AfxWinMain before InitInstance ran; this only fetches
    // it (and covers a program that reached Run by another route).
    QApplication* app = smfc_qt::EnsureQApplication();
    if (!app)
        return ExitInstance();

    // The application ends when its last window closes. In MFC that is the main
    // window posting WM_QUIT through its OnClose/PostNcDestroy; Qt's own rule
    // reaches the same point, and is made explicit here rather than relied on.
    app->setQuitOnLastWindowClosed(true);

    IdleDriver idle(this, app);
    QApplication::exec();

    // Real MFC's CWinThread::Run returns ExitInstance()'s value; AfxWinMain
    // passes it straight out as the process exit code.
    return ExitInstance();
}
