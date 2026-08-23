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

ECEvent::ECEvent(BOOL bInitiallyOwn, BOOL bManualReset, LPCTSTR /*lpszName*/, void* /*lpsaAttribute*/)
    : m_manualReset(bManualReset != FALSE), m_signaled(bInitiallyOwn != FALSE)
{
    // Real MFC's CEvent and CMutex ARE kernel objects, so m_hObject is a
    // live handle and every `if (event)` / `WaitForSingleObject(event)`
    // sees something non-null. This implementation has no kernel object,
    // but leaving the member null made the conversion answer the opposite
    // of what real MFC answers -- so it carries an opaque, unique,
    // non-null token instead. It is NOT waitable: code that needs a real
    // waitable handle needs a real port, and the token makes that a
    // deliberate decision rather than a silent null.
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

    if (!m_manualReset) m_signaled = false; // auto-reset: consumed by this waiter
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

BOOL ECEvent::PulseEvent()
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

BOOL ECEvent::ResetEvent()
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_signaled = false;
    return TRUE;
}

ECMutex::ECMutex(BOOL bInitiallyOwn, LPCTSTR /*lpszName*/, void* /*lpsaAttribute*/)
{
    m_hObject = static_cast<HANDLE>(this); // see the note in CEvent's constructor
    if (bInitiallyOwn) m_mutex.lock();
}

BOOL ECMutex::Lock(DWORD dwTimeout)
{
    if (dwTimeout == 0xFFFFFFFF) { m_mutex.lock(); return TRUE; }
    return m_mutex.try_lock_for(std::chrono::milliseconds(dwTimeout)) ? TRUE : FALSE;
}
