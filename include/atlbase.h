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
// Deliberately NOT provided, because eMule never names them: CComModule,
// CComObject and the object-map machinery, CComCriticalSection, the
// conversion macros (USES_CONVERSION/W2A/...), CRegKey, CAtlList/CAtlArray
// /CAtlMap, and ATL's own string classes.
#pragma once

#include "atldef.h"      // ATLASSERT & co
#include "atlcomcli.h"   // CComPtr, CComQIPtr, CComBSTR, CComVariant
#include "atlsimpcoll.h" // CSimpleArray
