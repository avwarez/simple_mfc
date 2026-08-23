#include "eafxwin.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>

struct ECWinThread::Impl
{
    EAFX_THREADPROC pfn = nullptr;
    void* param = nullptr;
    int priority = 0;
    std::thread th;
    std::mutex m;
    std::condition_variable cv;
    bool resumed = false;
    unsigned long suspendCount = 0;
};

namespace {
std::atomic<unsigned long> g_threadIdCounter{ 1 };
}

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
    m_pThreadParams = pParam;
}

ECWinThread::~ECWinThread()
{
    if (m_pImpl != nullptr)
    {
        if (m_pImpl->th.joinable())
            m_pImpl->th.detach();
        delete m_pImpl;
        m_pImpl = nullptr;
    }
}

BOOL ECWinThread::CreateThread(DWORD dwCreateFlags, UINT  ,
                              SECURITY_ATTRIBUTES*  )
{
    if (m_pImpl == nullptr)
        m_pImpl = new Impl();

    const bool suspended = (dwCreateFlags & CREATE_SUSPENDED) != 0;
    m_pImpl->resumed = !suspended;
    m_pImpl->suspendCount = suspended ? 1u : 0u;
    m_nThreadID = g_threadIdCounter.fetch_add(1);
    m_hThread = reinterpret_cast<void*>(m_pImpl);

    m_pImpl->th = std::thread([this]()
    {
        {
            std::unique_lock<std::mutex> lk(m_pImpl->m);
            m_pImpl->cv.wait(lk, [this]() { return m_pImpl->resumed; });
        }
        EAFX_THREADPROC pfn = m_pImpl->pfn;
        void* param = m_pImpl->param;
        const BOOL autoDelete = m_bAutoDelete;

        if (pfn != nullptr)
        {
            pfn(param);
        }
        else
        {
            if (InitInstance())
                Run();
            ExitInstance();
        }

        if (autoDelete)
            Delete();
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
    return previous;
}

DWORD ECWinThread::SuspendThread()
{
    if (m_pImpl == nullptr)
        return 0;

    unsigned long previous;
    {
        std::lock_guard<std::mutex> lk(m_pImpl->m);
        previous = m_pImpl->suspendCount;
        ++m_pImpl->suspendCount;
        m_pImpl->resumed = false;
    }
    return previous;
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

BOOL ECWinThread::InitInstance() { return FALSE; }
int  ECWinThread::ExitInstance() { return 0; }
int  ECWinThread::Run()          { return 0; }
void ECWinThread::Delete()
{
    if (m_bAutoDelete)
        delete this;
}

ECWinThread* EAfxBeginThread(EAFX_THREADPROC pfnThreadProc, void* pParam,
                           int nPriority, UINT nStackSize, DWORD dwCreateFlags,
                           SECURITY_ATTRIBUTES* lpSecurityAttrs)
{
    ECWinThread* pThread = new ECWinThread(pfnThreadProc, pParam);
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
