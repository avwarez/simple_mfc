// afximpl.cpp — AfxGetModuleThreadState/CTraceCategory. Both trivial:
// a thread-local static instance and a no-op constructor, pure standard
// C++.
#include "eafximpl.h"

// Real MFC's per-thread module state genuinely lives in thread-local
// storage (retrieved via TLS); thread_local is the direct, portable
// equivalent.
EAFX_MODULE_THREAD_STATE* EAFXAPI EAfxGetModuleThreadState()
{
    static thread_local EAFX_MODULE_THREAD_STATE state{nullptr, nullptr, nullptr};
    return &state;
}

// TRACE expands to __noop here (see afx.h), so nothing ever reads a
// trace category's value -- it only has to exist as a complete,
// constructible type for the category global below and for call sites
// like "TRACE(traceAppMsg, 0, ...)" to parse.
ECTraceCategory::ECTraceCategory(UINT /*nCategory*/) noexcept {}

ECTraceCategory EtraceAppMsg;
