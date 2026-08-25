#pragma once

#include "eafx.h"

#include <condition_variable>
#include <mutex>

class ECSyncObject : public ECObject
{
    EDECLARE_DYNAMIC(ECSyncObject)
public:
    virtual BOOL Lock(DWORD dwTimeout = 0xFFFFFFFF) = 0;
    virtual BOOL Unlock() = 0;
    HANDLE m_hObject = nullptr;
    operator HANDLE() const { return m_hObject; }
};

class ECCriticalSection : public ECSyncObject
{
    EDECLARE_DYNAMIC(ECCriticalSection)
public:
    BOOL Lock(DWORD dwTimeout = 0xFFFFFFFF) override;
    BOOL Unlock() override { m_sect.unlock(); return TRUE; }

    std::recursive_timed_mutex m_sect;
};

class ECEvent : public ECSyncObject
{
    EDECLARE_DYNAMIC(ECEvent)
public:
    explicit ECEvent(BOOL bInitiallyOwn = FALSE, BOOL bManualReset = FALSE,
                     LPCTSTR lpszName = nullptr, void* lpsaAttribute = nullptr);

    BOOL Lock(DWORD dwTimeout = 0xFFFFFFFF) override;
    BOOL Unlock() override { return TRUE; }
    BOOL SetEvent();
    BOOL ResetEvent();

private:
    bool m_manualReset;
    bool m_signaled;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

class ECMutex : public ECSyncObject
{
    EDECLARE_DYNAMIC(ECMutex)
public:
    explicit ECMutex(BOOL bInitiallyOwn = FALSE, LPCTSTR lpszName = nullptr, void* lpsaAttribute = nullptr);

    BOOL Lock(DWORD dwTimeout = 0xFFFFFFFF) override;
    BOOL Unlock() override { m_mutex.unlock(); return TRUE; }

private:
    std::recursive_timed_mutex m_mutex;
};

class ECSingleLock
{
public:
    explicit ECSingleLock(ECSyncObject* pObject, BOOL bInitialLock = FALSE) : m_pObject(pObject)
    {
        if (bInitialLock) Lock();
    }
    ~ECSingleLock() { if (m_locked) Unlock(); }

    BOOL Lock(DWORD dwTimeOut = 0xFFFFFFFF)
    {
        m_locked = m_pObject->Lock(dwTimeOut);
        return m_locked;
    }
    BOOL Unlock()
    {
        BOOL ok = m_pObject->Unlock();
        m_locked = FALSE;
        return ok;
    }
    BOOL Unlock(LONG  , LONG* lPrevCount = nullptr)
    {
        if (lPrevCount) *lPrevCount = 1;
        return Unlock();
    }
    BOOL IsLocked() const { return m_locked; }

private:
    ECSyncObject* m_pObject;
    BOOL m_locked = FALSE;
};
