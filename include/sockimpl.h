// sockimpl.h — reference STUB (declarations only, no implementation).
// Another of MFC's internal headers (it ships with MFC's sources, not in
// the include directory; eMule's CMakeLists.txt puts that directory on
// the include path). eMule includes it for one reason, stated in its own
// comment at Emule.cpp:23 -- "for *m_pfnSockTerm()" -- to work around a
// bug in AfxSocketTerm:
//
//   _AFX_SOCK_STATE *pState = _afxSockState.GetData();
//   if (pState->m_pfnSockTerm != NULL) { ...; pState->m_pfnSockTerm = NULL; }
//   ...
//   pState->m_pfnSockTerm = &AfxSocketTerm;
//
// so only the state struct, its accessor and that one member are needed.
#pragma once
#include "afxsock.h"

// MFC's per-module socket state. Real MFC has more in here (the socket
// handle maps); only the termination hook is reachable from application
// code, and it is the only thing eMule touches.
struct _AFX_SOCK_STATE
{
    void(AFX_CDECL* m_pfnSockTerm)();
};

// Real MFC declares the state through its THREAD_LOCAL/PROCESS_LOCAL
// machinery, which exposes exactly this accessor.
struct _AFX_SOCK_STATE_HOLDER
{
    _AFX_SOCK_STATE* GetData();
};
extern _AFX_SOCK_STATE_HOLDER _afxSockState;
