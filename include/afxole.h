// afxole.h — reference STUB (declarations only, no implementation).
// MFC OLE/drag-and-drop support. Subset actually used by eMule/
// srchybrid: only COleDropTarget (see ../../mfc_scan_srchybrid.md).
// As in real MFC, it includes afxwin.h.
#pragma once
#include "afxwin.h"

// Real MFC's OLE headers bring the OLE COM interfaces along with them, and
// eMule/srchybrid relies on that: CEnBitmap declares `static IPicture*
// LoadFromBuffer(...)`, which needs the interface declared, not merely
// named. These are plain Windows SDK headers -- nothing to reimplement.
#ifdef _WIN32
#include <ocidl.h>  // IPicture, IPictureDisp
#include <olectl.h> // OleLoadPicture and the OLE control constants
#else
// Off Windows there is no COM at all; the data-transfer types below are
// named only so the declarations parse (same approach as afxwin.h's
// WINDOWPLACEMENT & co).
struct IDataObject;
using LPDATAOBJECT = IDataObject*;
using CLIPFORMAT = unsigned short;
struct FORMATETC;
using LPFORMATETC = FORMATETC*;
struct STGMEDIUM;
using LPSTGMEDIUM = STGMEDIUM*;
using HGLOBAL = void*;
#endif

using DROPEFFECT = DWORD;

// Initializes the OLE libraries for this thread (and installs MFC's
// message filter). Real MFC declares it in afxdisp.h, which real
// afxole.h includes -- eMule includes only afxole.h and still sees it,
// so it belongs on this side of the include graph.
BOOL AFXAPI AfxOleInit();

// ---------------------------------------------------------------------
// COleDataObject — wraps the IDataObject handed to a drop target. eMule
// asks it which formats the dragged data offers and pulls the ones it
// understands (CF_HDROP file lists, text URLs).
// ---------------------------------------------------------------------
class COleDataObject
{
public:
    COleDataObject();
    virtual ~COleDataObject();
    void Attach(LPDATAOBJECT lpDataObject, BOOL bAutoRelease = TRUE);
    LPDATAOBJECT Detach();
    void Release();
    BOOL IsDataAvailable(CLIPFORMAT cfFormat, LPFORMATETC lpFormatEtc = nullptr);
    HGLOBAL GetGlobalData(CLIPFORMAT cfFormat, LPFORMATETC lpFormatEtc = nullptr);
    CFile* GetFileData(CLIPFORMAT cfFormat, LPFORMATETC lpFormatEtc = nullptr);
    BOOL GetData(CLIPFORMAT cfFormat, LPSTGMEDIUM lpStgMedium, LPFORMATETC lpFormatEtc = nullptr);
    void BeginEnumFormats();
    BOOL GetNextFormat(LPFORMATETC lpFormatEtc);
    LPDATAOBJECT m_lpDataObject;
    // Hands out the wrapped interface without releasing ownership.
    LPDATAOBJECT GetIDataObject(BOOL bAddRef);
};

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

// ---------------------------------------------------------------------
// Event-sink maps: how a container declares handlers for the events an
// embedded ActiveX control fires (eMule sinks the browser control's
// BeforeNavigate2). Real MFC builds a dispatch table; the handlers are
// ordinary member declarations, so as with the message maps these expand
// to nothing -- see afxmsg_.h for the rationale.
// ---------------------------------------------------------------------
#define DECLARE_EVENTSINK_MAP()
#define BEGIN_EVENTSINK_MAP(theClass, baseClass)
#define END_EVENTSINK_MAP()
#define ON_EVENT(theClass, id, dispid, pfn, vtsParams)
