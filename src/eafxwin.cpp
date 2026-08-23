// afxwin.cpp — native implementation of CWinThread / AfxBeginThread over
// std::thread. On `main` this file is gui/core/winthread.cpp, part of the
// toolkit-agnostic GUI runtime; on the `backend` branch the threading half is
// all that remains of afxwin.h, so it comes back to src/ next to the other
// implementations. The body is unchanged apart from the two members the
// header no longer has (see its banner).
//
// This is implementation, NOT interface: the interface (afxwin.h) only
// declares the public methods; the thread state lives in the private
// CWinThread::Impl pimpl (an internal mechanism, not part of the MFC
// contract, so its shape/name are our own).
//
// Deliberate deviations, documented: SuspendThread after start is a no-op
// (eMule almost only uses create-suspended-then-ResumeThread, which IS
// honoured via a condition_variable gate); m_hThread holds an opaque
// non-null token rather than a real waitable OS HANDLE (a runnable port
// would map it to std::thread::native_handle()).
#include "eafxwin.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>

struct ECWinThread::Impl
{
    EAFX_THREADPROC pfn = nullptr;   // worker procedure (proc-form threads)
    void* param = nullptr;          // its argument (mirror of m_pThreadParams)
    int priority = 0;               // last SetThreadPriority value
    std::thread th;
    std::mutex m;
    std::condition_variable cv;
    bool resumed = false;           // the create-suspended gate
    // Win32's suspend count, which ResumeThread returns the PREVIOUS value
    // of. A thread created suspended starts at 1, so the first resume
    // reports 1 and every later one 0 -- the conformance suite compares
    // this number against real MFC's, which is a straight ::ResumeThread.
    unsigned long suspendCount = 0;
};

namespace {
std::atomic<unsigned long> g_threadIdCounter{ 1 };
} // namespace

ECWinThread::ECWinThread()
{
    m_pThreadParams = nullptr;
    m_bAutoDelete = TRUE;
    m_hThread = nullptr;
    m_nThreadID = 0;
    m_pImpl = new Impl();
}

ECWinThread::ECWinThread(EAFX_THREADPROC pfnThreadProc, LPVOID pParam)
    : ECWinThread()
{
    m_pImpl->pfn = pfnThreadProc;
    m_pImpl->param = pParam;
    m_pThreadParams = pParam;       // public member eMule reads/clears directly
}

ECWinThread::~ECWinThread()
{
    if (m_pImpl != nullptr)
    {
        // Never block a destructor on the worker; an auto-delete thread runs
        // this from within itself (detaching its own std::thread is legal,
        // and the OS thread finishes on its own afterwards).
        if (m_pImpl->th.joinable())
            m_pImpl->th.detach();
        delete m_pImpl;
        m_pImpl = nullptr;
    }
}

BOOL ECWinThread::CreateThread(DWORD dwCreateFlags, UINT /*nStackSize*/,
                              SECURITY_ATTRIBUTES* /*lpSecurityAttrs*/)
{
    if (m_pImpl == nullptr)
        m_pImpl = new Impl();

    const bool suspended = (dwCreateFlags & CREATE_SUSPENDED) != 0;
    m_pImpl->resumed = !suspended;
    m_pImpl->suspendCount = suspended ? 1u : 0u;
    m_nThreadID = g_threadIdCounter.fetch_add(1);
    m_hThread = reinterpret_cast<void*>(m_pImpl); // opaque, non-null token

    m_pImpl->th = std::thread([this]()
    {
        // Honour create-suspended: block until ResumeThread() opens the gate.
        {
            std::unique_lock<std::mutex> lk(m_pImpl->m);
            m_pImpl->cv.wait(lk, [this]() { return m_pImpl->resumed; });
        }
        // Snapshot everything BEFORE running: an auto-delete thread frees its
        // own CWinThread at the very end, after which no member (nor m_pImpl)
        // may be touched again.
        EAFX_THREADPROC pfn = m_pImpl->pfn;
        void* param = m_pImpl->param;
        const BOOL autoDelete = m_bAutoDelete;

        if (pfn != nullptr)
        {
            pfn(param);                 // worker-proc thread
        }
        else
        {
            if (InitInstance())         // CRuntimeClass (GUI/worker) thread
                Run();
            ExitInstance();
        }

        if (autoDelete)
            Delete();                   // default Delete() == "delete this"
    });
    return TRUE;
}

DWORD ECWinThread::ResumeThread()
{
    if (m_pImpl == nullptr)
        return 0;

    unsigned long previous;
    {
        std::lock_guard<std::mutex> lk(m_pImpl->m);
        previous = m_pImpl->suspendCount;
        if (m_pImpl->suspendCount > 0)
            --m_pImpl->suspendCount;
        m_pImpl->resumed = (m_pImpl->suspendCount == 0);
    }
    m_pImpl->cv.notify_all();
    // Win32 returns the suspend count as it was BEFORE this call: 1 for the
    // first resume of a thread created suspended, 0 for a thread that was
    // already running. Returning a flat 0 was this branch's own invention
    // and the conformance suite caught it.
    return previous;
}

DWORD ECWinThread::SuspendThread()
{
    // Documented no-op: arbitrary mid-run suspension has no portable
    // equivalent; the create-suspended path (the only one eMule relies on)
    // is handled by the gate in CreateThread/ResumeThread.
    return 0;
}

BOOL ECWinThread::SetThreadPriority(int nPriority)
{
    if (m_pImpl != nullptr)
        m_pImpl->priority = nPriority;
    return TRUE;
}

int ECWinThread::GetThreadPriority()
{
    return (m_pImpl != nullptr) ? m_pImpl->priority : 0;
}

BOOL ECWinThread::InitInstance() { return TRUE; }
int  ECWinThread::ExitInstance() { return 0; }
int  ECWinThread::Run()          { return 0; }
void ECWinThread::Delete()       { delete this; }

ECWinThread* EAfxBeginThread(EAFX_THREADPROC pfnThreadProc, void* pParam,
                           int nPriority, UINT nStackSize, DWORD dwCreateFlags,
                           SECURITY_ATTRIBUTES* lpSecurityAttrs)
{
    ECWinThread* pThread = new ECWinThread(pfnThreadProc, pParam);
    pThread->m_bAutoDelete = TRUE;
    // Real MFC always creates suspended, sets priority, then resumes unless
    // the caller asked for CREATE_SUSPENDED.
    if (!pThread->CreateThread(CREATE_SUSPENDED, nStackSize, lpSecurityAttrs))
    {
        delete pThread;
        return nullptr;
    }
    pThread->SetThreadPriority(nPriority);
    if ((dwCreateFlags & CREATE_SUSPENDED) == 0)
        pThread->ResumeThread();
    return pThread;
}

ECWinThread* EAfxBeginThread(ECRuntimeClass* pThreadClass, int nPriority,
                           UINT nStackSize, DWORD dwCreateFlags,
                           SECURITY_ATTRIBUTES* lpSecurityAttrs)
{
    if (pThreadClass == nullptr)
        return nullptr;
    ECObject* pObject = pThreadClass->CreateObject();
    ECWinThread* pThread = static_cast<ECWinThread*>(pObject);
    if (pThread == nullptr)
        return nullptr;
    pThread->m_bAutoDelete = TRUE;
    if (!pThread->CreateThread(CREATE_SUSPENDED, nStackSize, lpSecurityAttrs))
    {
        delete pThread;
        return nullptr;
    }
    pThread->SetThreadPriority(nPriority);
    if ((dwCreateFlags & CREATE_SUSPENDED) == 0)
        pThread->ResumeThread();
    return pThread;
}

// AfxGetApp/AfxGetMainWnd/AfxGetAppName are NOT here: they read CWinApp's
// application state, and CWinApp is a CWinThread that owns a main window --
// frontend, and on `main`, not on this branch.
