#pragma once
#include "eafxcoll.h"

struct EAFX_MODULE_THREAD_STATE
{
    ECMapPtrToPtr* m_pmapSocketHandle;
    ECMapPtrToPtr* m_pmapDeadSockets;
    ECPtrList* m_plistSocketNotifications;
};

EAFX_MODULE_THREAD_STATE* EAFXAPI EAfxGetModuleThreadState();

struct ECTraceCategory
{
    explicit ECTraceCategory(UINT nCategory = 0) noexcept;
};
extern ECTraceCategory EtraceAppMsg;
