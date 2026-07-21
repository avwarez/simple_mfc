// atldef.h — reference STUB (declarations only, no implementation).
// ATL's diagnostic macros. Real ATL defines them here, which is why this
// file exists separately rather than folding them into atlbase.h.
//
// ATL is a different library from MFC and normally ships with MSVC
// regardless. These headers exist because eMule/srchybrid reaches ATL
// directly (<atlimage.h>, <atlenc.h>, <atlcoll.h>) and, with simple_mfc's
// headers shadowing MFC's, the real ATL's own ::CString typedef then
// collides with ours. Shadowing the ATL surface eMule actually touches
// removes that collision and keeps the compile check self-contained.
#pragma once

// afx.h already defines these (eMule mixes ATL and MFC diagnostics), each
// behind its own #ifndef, so including both headers in either order is
// fine and the first one seen wins -- the same rule real ATL and MFC use
// between themselves.
#ifndef ATLASSERT
#define ATLASSERT(expr) ASSERT(expr)
#endif
#ifndef ATLVERIFY
#define ATLVERIFY(expr) VERIFY(expr)
#endif
#ifndef ATLTRACE
#define ATLTRACE (void)
#endif
#ifndef ATLTRACE2
#define ATLTRACE2 (void)
#endif
#ifndef ATLENSURE
#define ATLENSURE(expr) ATLASSERT(expr)
#endif
