// gui/core/winthread.cpp — native, toolkit-agnostic implementation of
// CWinThread / AfxBeginThread over std::thread (Milestone 1, step 0). This
// is implementation (Layer 1/2b), NOT interface: the interface (afxwin.h)
// only declares the public methods; the thread state lives in the private
// CWinThread::Impl pimpl (an internal mechanism, not part of the MFC
// contract, so its shape/name are our own).
//
// Deliberate deviations, documented: SuspendThread after start is a no-op
// (eMule almost only uses create-suspended-then-ResumeThread, which IS
// honoured via a condition_variable gate); m_hThread holds an opaque
// non-null token rather than a real waitable OS HANDLE (a runnable port
// would map it to std::thread::native_handle()).
#include "afxwin.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>

struct CWinThread::Impl
{
    AFX_THREADPROC pfn = nullptr;   // worker procedure (proc-form threads)
    void* param = nullptr;          // its argument (mirror of m_pThreadParams)
    int priority = 0;               // last SetThreadPriority value
    std::thread th;
    std::mutex m;
    std::condition_variable cv;
    bool resumed = false;           // the create-suspended gate
};

namespace {
std::atomic<unsigned long> g_threadIdCounter{ 1 };
CWinApp* g_pApp = nullptr;          // real MFC sets this in CWinApp's ctor;
                                    // left null until CWinApp is implemented.
} // namespace

CWinThread::CWinThread()
{
    m_pThreadParams = nullptr;
    m_bAutoDelete = TRUE;
    m_hThread = nullptr;
    m_nThreadID = 0;
    m_pMainWnd = nullptr;
    m_pActiveWnd = nullptr;
    m_pImpl = new Impl();
}

CWinThread::CWinThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam)
    : CWinThread()
{
    m_pImpl->pfn = pfnThreadProc;
    m_pImpl->param = pParam;
    m_pThreadParams = pParam;       // public member eMule reads/clears directly
}

CWinThread::~CWinThread()
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

BOOL CWinThread::CreateThread(DWORD dwCreateFlags, UINT /*nStackSize*/,
                              SECURITY_ATTRIBUTES* /*lpSecurityAttrs*/)
{
    if (m_pImpl == nullptr)
        m_pImpl = new Impl();

    const bool suspended = (dwCreateFlags & CREATE_SUSPENDED) != 0;
    m_pImpl->resumed = !suspended;
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
        AFX_THREADPROC pfn = m_pImpl->pfn;
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

DWORD CWinThread::ResumeThread()
{
    if (m_pImpl != nullptr)
    {
        {
            std::lock_guard<std::mutex> lk(m_pImpl->m);
            m_pImpl->resumed = true;
        }
        m_pImpl->cv.notify_all();
    }
    return 0;
}

DWORD CWinThread::SuspendThread()
{
    // Documented no-op: arbitrary mid-run suspension has no portable
    // equivalent; the create-suspended path (the only one eMule relies on)
    // is handled by the gate in CreateThread/ResumeThread.
    return 0;
}

BOOL CWinThread::SetThreadPriority(int nPriority)
{
    if (m_pImpl != nullptr)
        m_pImpl->priority = nPriority;
    return TRUE;
}

int CWinThread::GetThreadPriority()
{
    return (m_pImpl != nullptr) ? m_pImpl->priority : 0;
}

BOOL CWinThread::InitInstance() { return TRUE; }
int  CWinThread::ExitInstance() { return 0; }
int  CWinThread::Run()          { return 0; }
void CWinThread::Delete()       { delete this; }

CWinThread* AfxBeginThread(AFX_THREADPROC pfnThreadProc, void* pParam,
                           int nPriority, UINT nStackSize, DWORD dwCreateFlags,
                           SECURITY_ATTRIBUTES* lpSecurityAttrs)
{
    CWinThread* pThread = new CWinThread(pfnThreadProc, pParam);
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

CWinThread* AfxBeginThread(CRuntimeClass* pThreadClass, int nPriority,
                           UINT nStackSize, DWORD dwCreateFlags,
                           SECURITY_ATTRIBUTES* lpSecurityAttrs)
{
    if (pThreadClass == nullptr)
        return nullptr;
    CObject* pObject = pThreadClass->CreateObject();
    CWinThread* pThread = static_cast<CWinThread*>(pObject);
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

CWinApp* AfxGetApp() { return g_pApp; }
CWnd* AfxGetMainWnd() { return (g_pApp != nullptr) ? g_pApp->m_pMainWnd : nullptr; }
LPCTSTR AfxGetAppName() { return (g_pApp != nullptr) ? g_pApp->m_pszAppName : nullptr; }
