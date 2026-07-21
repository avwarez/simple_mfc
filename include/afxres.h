// afxres.h — reference STUB. The standard MFC/AppWizard resource symbols.
// eMule/srchybrid references a handful of them (ID_HELP from the shared
// property-page command handling, IDC_STATIC on unnamed controls) without
// defining them itself: they come from real MFC's own afxres.h, which its
// .rc files include. Values match the real header.
#pragma once
#include "afx.h"

// The "no identifier" control id. Real afxres.h defines it as -1.
#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

// Standard command ids (real MFC's afxres.h block starting at 0xE100).
#ifndef ID_HELP
#define ID_HELP 0xE146
#endif
// The property-sheet "Apply" button.
#ifndef ID_APPLY_NOW
#define ID_APPLY_NOW 0x3021
#endif
#ifndef ID_CONTEXT_HELP
#define ID_CONTEXT_HELP 0xE145
#endif
#ifndef ID_DEFAULT_HELP
#define ID_DEFAULT_HELP 0xE143
#endif
#ifndef ID_HELP_INDEX
#define ID_HELP_INDEX 0xE142
#endif

// The first id MFC reserves for a control bar (eMule passes AFX_IDW_REBAR
// when it creates the main rebar).
#ifndef AFX_IDW_REBAR
#define AFX_IDW_REBAR 0xE800
#endif

// The control-bar id range, and the four dock bars a frame creates at its
// edges -- eMule names AFX_IDW_DOCKBAR_TOP/LEFT when it re-docks its
// search-parameters and transfer bars.
#ifndef AFX_IDW_CONTROLBAR_FIRST
#define AFX_IDW_CONTROLBAR_FIRST 0xE800
#define AFX_IDW_CONTROLBAR_LAST  0xE8FF
#endif
#ifndef AFX_IDW_DOCKBAR_TOP
#define AFX_IDW_DOCKBAR_TOP    0xE81B
#define AFX_IDW_DOCKBAR_LEFT   0xE81C
#define AFX_IDW_DOCKBAR_RIGHT  0xE81D
#define AFX_IDW_DOCKBAR_BOTTOM 0xE81E
#define AFX_IDW_DOCKBAR_FLOAT  0xE81F
#endif

// The standard DDV_ failure prompts (real MFC's afxres.h 0xE100 block).
#ifndef AFX_IDP_PARSE_INT
#define AFX_IDP_PARSE_INT  0xE108
#define AFX_IDP_PARSE_REAL 0xE109
#define AFX_IDP_PARSE_UINT 0xE114
#endif
#ifndef AFX_IDP_FAILED_TO_LAUNCH_HELP
#define AFX_IDP_FAILED_TO_LAUNCH_HELP 0xE202
#endif
