#include "afxmt.h"

IMPLEMENT_DYNAMIC(CSyncObject, CObject)
IMPLEMENT_DYNAMIC(CCriticalSection, CSyncObject)
IMPLEMENT_DYNAMIC(CEvent, CSyncObject)
IMPLEMENT_DYNAMIC(CMutex, CSyncObject)

BOOL CCriticalSection::Lock(DWORD dwTimeout)
{
    if (dwTimeout == 0xFFFFFFFF) { m_sect.lock(); return TRUE; }
    return m_sect.try_lock_for(std::chrono::milliseconds(dwTimeout)) ? TRUE : FALSE;
}

CEvent::CEvent(BOOL bInitiallyOwn, BOOL bManualReset, LPCTSTR /*lpszName*/, void* /*lpsaAttribute*/)
    : m_manualReset(bManualReset != FALSE), m_signaled(bInitiallyOwn != FALSE)
{
}

BOOL CEvent::Lock(DWORD dwTimeout)
{
    std::unique_lock<std::mutex> lk(m_mutex);
    auto ready = [this] { return m_signaled; };
    if (dwTimeout == 0xFFFFFFFF)
        m_cv.wait(lk, ready);
    else if (!m_cv.wait_for(lk, std::chrono::milliseconds(dwTimeout), ready))
        return FALSE;

    if (!m_manualReset) m_signaled = false; // auto-reset: consumed by this waiter
    return TRUE;
}

BOOL CEvent::SetEvent()
{
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_signaled = true;
    }
    if (m_manualReset) m_cv.notify_all();
    else m_cv.notify_one();
    return TRUE;
}

BOOL CEvent::PulseEvent()
{
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_signaled = true;
    }
    if (m_manualReset) m_cv.notify_all();
    else m_cv.notify_one();
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_signaled = false;
    }
    return TRUE;
}

BOOL CEvent::ResetEvent()
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_signaled = false;
    return TRUE;
}

CMutex::CMutex(BOOL bInitiallyOwn, LPCTSTR /*lpszName*/, void* /*lpsaAttribute*/)
{
    if (bInitiallyOwn) m_mutex.lock();
}

BOOL CMutex::Lock(DWORD dwTimeout)
{
    if (dwTimeout == 0xFFFFFFFF) { m_mutex.lock(); return TRUE; }
    return m_mutex.try_lock_for(std::chrono::milliseconds(dwTimeout)) ? TRUE : FALSE;
}
