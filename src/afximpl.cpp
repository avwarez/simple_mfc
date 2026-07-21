// afximpl.cpp — AfxGetModuleThreadState/CTraceCategory. Both trivial:
// a thread-local static instance and a no-op constructor, pure standard
// C++.
#include "afximpl.h"

// Real MFC's per-thread module state genuinely lives in thread-local
// storage (retrieved via TLS); thread_local is the direct, portable
// equivalent.
AFX_MODULE_THREAD_STATE* AFXAPI AfxGetModuleThreadState()
{
    static thread_local AFX_MODULE_THREAD_STATE state{nullptr, nullptr, nullptr};
    return &state;
}

// TRACE expands to __noop here (see afx.h), so nothing ever reads a
// trace category's value -- it only has to exist as a complete,
// constructible type for the category globals below and for call sites
// like "TRACE(traceAppMsg, 0, ...)" to parse.
CTraceCategory::CTraceCategory(UINT /*nCategory*/) noexcept {}

CTraceCategory traceAppMsg;
CTraceCategory traceWinMsg;
CTraceCategory traceCmdRouting;
CTraceCategory traceDumpContext;
