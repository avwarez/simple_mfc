// atlbase.h — the ATL umbrella header, as in real ATL: it declares almost
// nothing itself and pulls in the headers that do.
//
// Why simple_mfc carries ATL headers at all: ATL is a separate library
// that ships with MSVC whether or not MFC is used, so the project's rule
// has been to defer to the real one. That stopped working once these
// headers shadowed MFC's, because real ATL's atlstr.h then contributes
// its own ATL::CString while simple_mfc contributes ::CString, and the
// automatic "using namespace ATL" makes every unqualified use ambiguous
// (C2872). Shadowing the ATL surface eMule/srchybrid actually reaches
// removes the collision and keeps the compile check self-contained.
//
// Deliberately NOT provided, because eMule names none of them:
// CComModule, CComObject and the object-map machinery,
// CComCriticalSection, the conversion macros (USES_CONVERSION/W2A/...),
// CAtlList/CAtlArray/CAtlMap, and ATL's own string classes.
//
// ALSO NOT PROVIDED, and this one is a removal rather than an omission:
// the COM smart pointers and wrappers (CComPtr, CComQIPtr, CComPtrBase,
// CComBSTR, CComVariant — formerly atlcomcli.h), the registry wrapper
// (CRegKey, formerly declared right here) and the OLE automation date
// (COleDateTime, formerly atlcomtime.h). All three were declaration-only
// on this branch: 86 declared members and not one defined anywhere, so a
// call to any of them would not have linked, and the conformance suite
// could not compare a single one against real ATL. They are Win32 COM and
// registry surface, which is the opposite of what this branch exists to
// isolate. If a consumer turns out to need them, they come back as an
// implementation with tests, not as declarations.
#pragma once

#include "eatldef.h"      // ATLASSERT & co
#include "eatlsimpcoll.h" // CSimpleArray
#include "eatlconv.h"     // AtlUnicodeToUTF8
#include "eatlalloc.h"    // CTempBuffer
