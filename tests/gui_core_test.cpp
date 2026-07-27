// gui_core_test.cpp — proves the Milestone 1 gui/core runtime END-TO-END
// (not just at compile time): that a real MFC-style message map actually
// DISPATCHES a command to its handler, and that AfxBeginThread actually runs
// a worker on a std::thread (including create-suspended/ResumeThread).
//
// It uses eMule's exact map form -- the bare-name pointer-to-member
// ON_COMMAND(ID, OnFoo) -- which real MFC accepts via an MSVC extension;
// GCC/Clang accept the same with -fms-extensions (set on this target in
// CMakeLists.txt), so the test exercises the identical construct.
#include "afxwin.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <cstdint>
#include <cstdio>

static int g_failures = 0;
#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::printf("FAIL: %s (line %d)\n", #cond, __LINE__);        \
            ++g_failures;                                                \
        }                                                                \
    } while (0)

// ---- message-map dispatch (the "click" route: WM_COMMAND -> OnCmdMsg) ----
static std::atomic<int> g_cmdValue{ 0 };

class TestTarget : public CCmdTarget
{
public:
    afx_msg void OnDoIt()         { g_cmdValue = 42; }
    afx_msg void OnRange(UINT id) { g_cmdValue = static_cast<int>(id); }
    DECLARE_MESSAGE_MAP()
};

#define ID_DOIT        100
#define ID_RANGE_FIRST 200
#define ID_RANGE_LAST  209

BEGIN_MESSAGE_MAP(TestTarget, CCmdTarget)
    ON_COMMAND(ID_DOIT, OnDoIt)
    ON_COMMAND_RANGE(ID_RANGE_FIRST, ID_RANGE_LAST, OnRange)
END_MESSAGE_MAP()

// ---- threading ----
static std::atomic<int> g_threadValue{ 0 };

UINT AFX_CDECL Worker(void* p)
{
    g_threadValue = static_cast<int>(reinterpret_cast<std::intptr_t>(p));
    return 0;
}

int main()
{
    // 1) The mirrored ON_* macros build a well-formed map AND OnCmdMsg walks
    //    it and invokes the right handler by AfxSig tag.
    {
        TestTarget t;
        CHECK(t.OnCmdMsg(ID_DOIT, CN_COMMAND, nullptr, nullptr) != FALSE);
        CHECK(g_cmdValue == 42);

        g_cmdValue = 0;
        CHECK(t.OnCmdMsg(205, CN_COMMAND, nullptr, nullptr) != FALSE); // in range -> OnRange(205)
        CHECK(g_cmdValue == 205);

        CHECK(t.OnCmdMsg(999, CN_COMMAND, nullptr, nullptr) == FALSE); // no handler

        // "is there a handler?" query (non-null pHandlerInfo) must report the
        // match WITHOUT invoking. AFX_CMDHANDLERINFO is opaque here, so pass a
        // valid non-null address (OnCmdMsg only tests non-null, never derefs).
        g_cmdValue = 0;
        int dummy = 0;
        AFX_CMDHANDLERINFO* pInfo = reinterpret_cast<AFX_CMDHANDLERINFO*>(&dummy);
        CHECK(t.OnCmdMsg(ID_DOIT, CN_COMMAND, nullptr, pInfo) != FALSE);
        CHECK(g_cmdValue == 0);
    }

    // 2) AfxBeginThread (proc form) actually runs the worker.
    {
        g_threadValue = 0;
        CWinThread* pThread = AfxBeginThread(Worker, reinterpret_cast<void*>(7));
        CHECK(pThread != nullptr);
        for (int i = 0; i < 500 && g_threadValue.load() == 0; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        CHECK(g_threadValue == 7);
        // pThread is auto-delete: it frees itself after Worker returns -- do
        // not touch it again.
    }

    // 3) CREATE_SUSPENDED: the worker must NOT run until ResumeThread().
    {
        g_threadValue = 0;
        CWinThread* pThread = AfxBeginThread(Worker, reinterpret_cast<void*>(9),
                                             THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
        CHECK(pThread != nullptr);
        pThread->m_bAutoDelete = FALSE; // keep it so we can drive/observe it
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        CHECK(g_threadValue == 0);      // still gated

        pThread->ResumeThread();
        for (int i = 0; i < 500 && g_threadValue.load() == 0; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        CHECK(g_threadValue == 9);

        std::this_thread::sleep_for(std::chrono::milliseconds(30)); // let it finish
        delete pThread;                 // not auto-delete -> we delete
    }

    if (g_failures == 0)
        std::printf("gui_core_test: all checks passed\n");
    return (g_failures == 0) ? 0 : 1;
}
