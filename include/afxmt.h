// afxmt.h — NATIVE implementation (standard C++17 library only).
// Multithread synchronization on top of <mutex>/<condition_variable>.
//
// Known limitation: in real MFC, CMutex supports "named" mutexes shared
// across processes (a Win32 kernel object) via lpszName — this is
// inherently operating-system specific and cannot be expressed with the
// standard C++ library alone. Here lpszName is accepted but ignored:
// CMutex only works as an in-process mutex.
#pragma once

#include "afx.h"

#include <condition_variable>
#include <mutex>

// ---------------------------------------------------------------------
// CSyncObject — abstract base for synchronization objects.
// ---------------------------------------------------------------------
class CSyncObject : public CObject
{
    DECLARE_DYNAMIC(CSyncObject)
public:
    virtual BOOL Lock(DWORD dwTimeout = 0xFFFFFFFF) = 0;
    virtual BOOL Unlock() = 0;
};

// ---------------------------------------------------------------------
// CCriticalSection — on top of std::recursive_mutex (reentrant, like the
// real Win32 critical section).
// ---------------------------------------------------------------------
class CCriticalSection : public CSyncObject
{
    DECLARE_DYNAMIC(CCriticalSection)
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
class CEvent : public CSyncObject
{
    DECLARE_DYNAMIC(CEvent)
public:
    explicit CEvent(BOOL bInitiallyOwn = FALSE, BOOL bManualReset = FALSE,
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
class CMutex : public CSyncObject
{
    DECLARE_DYNAMIC(CMutex)
public:
    explicit CMutex(BOOL bInitiallyOwn = FALSE, LPCTSTR lpszName = nullptr, void* lpsaAttribute = nullptr);

    BOOL Lock(DWORD dwTimeout = 0xFFFFFFFF) override;
    BOOL Unlock() override { m_mutex.unlock(); return TRUE; }

private:
    std::recursive_timed_mutex m_mutex;
};

// ---------------------------------------------------------------------
// CSingleLock — "manual" RAII (explicit Lock/Unlock, not in the
// constructor) on a CSyncObject, same as real MFC.
// ---------------------------------------------------------------------
class CSingleLock
{
public:
    explicit CSingleLock(CSyncObject* pObject, BOOL bInitialLock = FALSE) : m_pObject(pObject)
    {
        if (bInitialLock) Lock();
    }
    ~CSingleLock() { if (m_locked) Unlock(); }

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
    CSyncObject* m_pObject;
    BOOL m_locked = FALSE;
};
