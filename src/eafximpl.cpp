#include "eafximpl.h"

EAFX_MODULE_THREAD_STATE* EAFXAPI EAfxGetModuleThreadState()
{
    static thread_local EAFX_MODULE_THREAD_STATE state{nullptr, nullptr, nullptr};
    return &state;
}

ECTraceCategory::ECTraceCategory(UINT  ) noexcept {}

ECTraceCategory EtraceAppMsg;
