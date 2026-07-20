// Minimal smoke test: verifies that the library compiles, links and works
// for the pieces that are actually implemented (afx.h, afxcoll.h,
// afxtempl.h, afxmt.h, atltime.h). Not an exhaustive test suite.
#include "afx.h"
#include "afxcoll.h"
#include "afxtempl.h"
#include "afxmt.h"
#include "atltime.h"

// "Declaration-only" headers (no .cpp): here we only check that they are
// includable and that the types/hierarchies compile, without actually
// using them.
#include "afxwin.h"
#include "afxext.h"
#include "afxdlgs.h"
#include "afxdd_.h"
#include "afxcmn.h"
#include "afxole.h"
#include "afxdhtml.h"
#include "afxsock.h"

#include <cassert>
#include <cstdio>
#include <sstream>

int main()
{
    CString s = L"  simple_mfc  ";
    s.Trim();
    assert(s == CString(L"simple_mfc"));

    CObList list;
    CObject a, b;
    list.AddTail(&a);
    list.AddTail(&b);
    assert(list.GetCount() == 2);

    CArray<int> arr;
    arr.Add(42);
    assert(arr[0] == 42);

    CCriticalSection cs;
    {
        CSingleLock lk(&cs, TRUE);
        assert(lk.IsLocked());
    }

    CTime t = CTime::GetCurrentTime();
    (void)t;

    // CObject::Dump / CDumpContext (found via the qualified-call blind-spot
    // rescan: CObject::Dump(dc) super-calls in eMule/srchybrid).
    std::wostringstream oss;
    CDumpContext dcTest(oss);
    CObject dumpObj;
    dumpObj.Dump(dcTest);
    assert(oss.str() == L"CObject");

    // CFileException::ThrowOsError (static factory, found the same way).
    try
    {
        CFileException::ThrowOsError(2 /*ERROR_FILE_NOT_FOUND*/, L"missing.txt");
        assert(false && "ThrowOsError must throw");
    }
    catch (CFileException* e)
    {
        assert(e->m_cause == CFileException::fileNotFound);
        assert(e->m_strFileName == CString(L"missing.txt"));
        e->Delete();
    }

    CWnd* w = nullptr; (void)w;
    CDC* dc = nullptr; (void)dc;
    CRect* r = nullptr; (void)r;
    CPropertySheet* sheet = nullptr; (void)sheet;
    CTreeCtrl* tree = nullptr; (void)tree;
    CImageList* iml = nullptr; (void)iml;
    COleDropTarget* dropTarget = nullptr; (void)dropTarget;
    CDHtmlDialog* dhtml = nullptr; (void)dhtml;
    CAsyncSocket* sock = nullptr; (void)sock;
    // CPalette went from an incomplete forward declaration to a real
    // class definition during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum) — checked here like the
    // other declaration-only GUI/GDI classes above.
    CPalette* pal = nullptr; (void)pal;

    std::printf("simple_mfc smoke test: ALL OK\n");
    return 0;
}
