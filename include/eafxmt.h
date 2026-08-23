// afxmt.h — NATIVE implementation (standard C++17 library only).
// Multithread synchronization on top of <mutex>/<condition_variable>.
//
// Known limitation: in real MFC, CMutex supports "named" mutexes shared
// across processes (a Win32 kernel object) via lpszName — this is
// inherently operating-system specific and cannot be expressed with the
// standard C++ library alone. Here lpszName is accepted but ignored:
// CMutex only works as an in-process mutex.
#pragma once

#include "eafx.h"

#include <condition_variable>
#include <mutex>

// ---------------------------------------------------------------------
// CSyncObject — abstract base for synchronization objects.
// ---------------------------------------------------------------------
class ECSyncObject : public ECObject
{
    EDECLARE_DYNAMIC(ECSyncObject)
public:
    virtual BOOL Lock(DWORD dwTimeout = 0xFFFFFFFF) = 0;
    virtual BOOL Unlock() = 0;
    // Real MFC exposes the underlying Win32 handle by implicit conversion
    // (used e.g. by eMule's UploadBandwidthThrottler::GetSocketAvailableEvent,
    // which returns a CEvent where a HANDLE is expected). This portable
    // implementation has no real OS handle, so m_hObject stays null -- the
    // operator exists to satisfy those call sites at compile time.
    HANDLE m_hObject = nullptr;
    operator HANDLE() const { return m_hObject; }
};

// ---------------------------------------------------------------------
// CCriticalSection — on top of std::recursive_mutex (reentrant, like the
// real Win32 critical section).
// ---------------------------------------------------------------------
class ECCriticalSection : public ECSyncObject
{
    EDECLARE_DYNAMIC(ECCriticalSection)
public:
    // Lock() with no arguments remains available thanks to the default of
    // CSyncObject::Lock(DWORD dwTimeout = 0xFFFFFFFF): no separate overload
    // is needed (in real MFC, CCriticalSection::Lock()/Lock(DWORD) are
    // instead two distinct overloads on top of a parameterless
    // CSyncObject::Lock() — unified here so it can go through the same
    // abstract polymorphic interface also used by CEvent/CMutex).
    BOOL Lock(DWORD dwTimeout = 0xFFFFFFFF) override;
    BOOL Unlock() override { m_sect.unlock(); return TRUE; }

    std::recursive_timed_mutex m_sect;
};

// ---------------------------------------------------------------------
// CEvent — on top of std::condition_variable + a flag, with
// manual-reset/auto-reset semantics equivalent to the Win32 ones.
// ---------------------------------------------------------------------
class ECEvent : public ECSyncObject
{
    EDECLARE_DYNAMIC(ECEvent)
public:
    explicit ECEvent(BOOL bInitiallyOwn = FALSE, BOOL bManualReset = FALSE,
                     LPCTSTR lpszName = nullptr, void* lpsaAttribute = nullptr);

    BOOL Lock(DWORD dwTimeout = 0xFFFFFFFF) override;
    BOOL Unlock() override { return TRUE; } // CEvent has no real "unlock" in real MFC
    BOOL SetEvent();
    BOOL PulseEvent();
    BOOL ResetEvent();

private:
    bool m_manualReset;
    bool m_signaled;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

// ---------------------------------------------------------------------
// CMutex — on top of std::recursive_mutex, in-process only (see the note
// at the top of this file for the "named" cross-process mutex limitation).
// ---------------------------------------------------------------------
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

// ---------------------------------------------------------------------
// CSingleLock — "manual" RAII (explicit Lock/Unlock, not in the
// constructor) on a CSyncObject, same as real MFC.
// ---------------------------------------------------------------------
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
    BOOL Unlock(LONG /*lCount*/, LONG* lPrevCount = nullptr)
    {
        if (lPrevCount) *lPrevCount = 1;
        return Unlock();
    }
    BOOL IsLocked() const { return m_locked; }

private:
    ECSyncObject* m_pObject;
    BOOL m_locked = FALSE;
};
