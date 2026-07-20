// afxole.h — reference STUB (declarations only, no implementation).
// MFC OLE/drag-and-drop support. Subset actually used by eMule/
// srchybrid: only COleDropTarget (see ../../mfc_scan_srchybrid.md).
// As in real MFC, it includes afxwin.h.
#pragma once
#include "afxwin.h"

using DROPEFFECT = DWORD;
class COleDataObject; // real header afxole.h too; not otherwise used here beyond a pointer parameter

// ---------------------------------------------------------------------
// COleDropTarget (header afxole.h, deriva da CCmdTarget)
// ---------------------------------------------------------------------
class COleDropTarget : public CCmdTarget
{
public:
    BOOL Register(CWnd* pWnd);
    virtual void Revoke();
    virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
    virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
};
