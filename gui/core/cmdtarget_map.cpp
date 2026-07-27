// gui/core/cmdtarget_map.cpp — the CCmdTarget end of the message-map
// machinery: the ROOT message map every chain terminates at, and the
// command/control-notification dispatcher OnCmdMsg. This is implementation
// (Layer 2b), NOT interface: it consumes the (MFC-faithful) AFX_MSGMAP_ENTRY
// data that afxmsg_.h's ON_* macros emit, and the AfxSig tags from afxwin.h.
//
// Scope for Milestone 1: the CCmdTarget-level command path (ON_COMMAND /
// ON_COMMAND_RANGE / ON_CONTROL, i.e. the "button/menu click" route:
// WM_COMMAND -> OnCommand -> OnCmdMsg -> handler). WM_/WM_NOTIFY window-message
// dispatch (CWnd::OnWndMsg) and reflection are added with the CWnd dispatcher.
#include "afxwin.h"

// The root message map. Real MFC roots every chain at CCmdTarget; its base
// link is null, which is what terminates the walk in OnCmdMsg below.
const AFX_MSGMAP* PASCAL CCmdTarget::GetThisMessageMap()
{
    static const AFX_MSGMAP_ENTRY _entries[] =
    {
        { 0, 0, 0, 0, AfxSig_end, (AFX_PMSG)0 }
    };
    static const AFX_MSGMAP theMap = { nullptr, &_entries[0] };
    return &theMap;
}

const AFX_MSGMAP* CCmdTarget::GetMessageMap() const
{
    return GetThisMessageMap();
}

BOOL CCmdTarget::OnCmdMsg(UINT nID, int nCode, void* /*pExtra*/,
                          AFX_CMDHANDLERINFO* pHandlerInfo)
{
    // Walk this object's message-map chain (derived map -> ... -> CCmdTarget's
    // root, whose base link is null) looking for a WM_COMMAND entry whose
    // notification code matches nCode and whose id range [nID..nLastID]
    // covers nID; then invoke the handler, using the AfxSig tag to recover
    // the true member-function signature (real MFC's own union technique).
    union MsgFn
    {
        AFX_PMSG pfn;
        void (CCmdTarget::*pfn_v)();
        BOOL (CCmdTarget::*pfn_b)();
        void (CCmdTarget::*pfn_vw)(UINT);
        BOOL (CCmdTarget::*pfn_bw)(UINT);
    };

    for (const AFX_MSGMAP* pMap = GetMessageMap(); pMap != nullptr;
         pMap = (pMap->pfnGetBaseMap != nullptr) ? (*pMap->pfnGetBaseMap)() : nullptr)
    {
        for (const AFX_MSGMAP_ENTRY* e = pMap->lpEntries; e->nSig != AfxSig_end; ++e)
        {
            if (e->nMessage != WM_COMMAND)          continue;
            if (static_cast<int>(e->nCode) != nCode) continue;
            if (nID < e->nID || nID > e->nLastID)    continue;

            // pHandlerInfo != NULL means "is there a handler?" (routing query),
            // not "run it" -- report the match without invoking, as real MFC does.
            if (pHandlerInfo != nullptr)
                return TRUE;

            MsgFn u;
            u.pfn = e->pfn;
            switch (e->nSig)
            {
            case AfxSigCmd_v:     (this->*u.pfn_v)();        return TRUE;
            case AfxSigCmd_b:     return (this->*u.pfn_b)();
            case AfxSigCmd_RANGE: (this->*u.pfn_vw)(nID);    return TRUE;
            case AfxSigCmd_EX:    return (this->*u.pfn_bw)(nID);
            default:              break; // signatures handled by the CWnd dispatcher
            }
        }
    }
    return FALSE;
}
