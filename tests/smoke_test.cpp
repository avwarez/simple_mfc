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

    CWnd* w = nullptr; (void)w;
    CDC* dc = nullptr; (void)dc;
    CRect* r = nullptr; (void)r;
    CPropertySheet* sheet = nullptr; (void)sheet;
    CTreeCtrl* tree = nullptr; (void)tree;
    CImageList* iml = nullptr; (void)iml;
    COleDropTarget* dropTarget = nullptr; (void)dropTarget;
    CDHtmlDialog* dhtml = nullptr; (void)dhtml;
    CAsyncSocket* sock = nullptr; (void)sock;

    std::printf("simple_mfc smoke test: ALL OK\n");
    return 0;
}
