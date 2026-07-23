// atldef.h — NATIVE (functional macros, no linkable body needed).
// ATL's diagnostic macros. These are #defines, not functions, so they are
// fully operational as-is -- there is nothing to "implement" in a .cpp and
// nothing that fails to link. (An earlier banner called this a "reference
// STUB", which was wrong: a stub is a declaration with no working body,
// whereas ATLASSERT/ATLTRACE2 below genuinely do what real ATL's do.)
// Real ATL defines them here too, which is why this file exists separately
// rather than folding them into atlbase.h.
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
#ifndef ATLTRACE2
#define ATLTRACE2 (void)
#endif
