#include "eafxmt.h"

EIMPLEMENT_DYNAMIC(ECSyncObject, ECObject)
EIMPLEMENT_DYNAMIC(ECCriticalSection, ECSyncObject)
EIMPLEMENT_DYNAMIC(ECEvent, ECSyncObject)
EIMPLEMENT_DYNAMIC(ECMutex, ECSyncObject)

BOOL ECCriticalSection::Lock(DWORD dwTimeout)
{
    if (dwTimeout == 0xFFFFFFFF) { m_sect.lock(); return TRUE; }
    return m_sect.try_lock_for(std::chrono::milliseconds(dwTimeout)) ? TRUE : FALSE;
}

ECEvent::ECEvent(BOOL bInitiallyOwn, BOOL bManualReset, LPCTSTR  , void*  )
    : m_manualReset(bManualReset != FALSE), m_signaled(bInitiallyOwn != FALSE)
{
    m_hObject = static_cast<HANDLE>(this);
}

BOOL ECEvent::Lock(DWORD dwTimeout)
{
    std::unique_lock<std::mutex> lk(m_mutex);
    auto ready = [this] { return m_signaled; };
    if (dwTimeout == 0xFFFFFFFF)
        m_cv.wait(lk, ready);
    else if (!m_cv.wait_for(lk, std::chrono::milliseconds(dwTimeout), ready))
        return FALSE;

    if (!m_manualReset) m_signaled = false;
    return TRUE;
}

BOOL ECEvent::SetEvent()
{
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_signaled = true;
    }
    if (m_manualReset) m_cv.notify_all();
    else m_cv.notify_one();
    return TRUE;
}

BOOL ECEvent::ResetEvent()
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_signaled = false;
    return TRUE;
}

ECMutex::ECMutex(BOOL bInitiallyOwn, LPCTSTR  , void*  )
{
    m_hObject = static_cast<HANDLE>(this);
    if (bInitiallyOwn) m_mutex.lock();
}

BOOL ECMutex::Lock(DWORD dwTimeout)
{
    if (dwTimeout == 0xFFFFFFFF) { m_mutex.lock(); return TRUE; }
    return m_mutex.try_lock_for(std::chrono::milliseconds(dwTimeout)) ? TRUE : FALSE;
}
