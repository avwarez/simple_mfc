// win32_types.h -- GENERATED, do not edit by hand.
//
// Struct layouts, enums and typedefs lifted verbatim from the preprocessed
// mingw-w64 headers, so field order and field types cannot drift from the SDK.
// Regenerate with tools/win32_oracle/mktypes.py.
//
// Three properties make this safe here:
//   * The preprocessor expands macros but NOT typedefs, so fields come out as
//     `LONG lfHeight`, and afx.h already owns LONG at the correct 32-bit
//     width. Emitting the underlying `long` would be 8 bytes on LP64 and 4 on
//     Windows.
//   * Where the SDK does spell a base type outright -- GUID's
//     `unsigned long Data1` -- the generator narrows it to `int`, which is
//     32-bit on both. Otherwise every field after it would shift.
//   * The oracle is preprocessed with UNICODE defined, matching how eMule
//     builds, so LPLOGFONT resolves to LOGFONTW and not LOGFONTA.
//
// Only what eMule failed on is emitted, plus what it transitively needs.
// Names owned by afx.h / atltypes.h / atlcomcli.h, and names that belong to
// glibc on this platform, are deliberately absent: single owner per symbol.

#pragma once

// GUID's owner is atlcomcli.h -- it is a COM type, and that header is
// reachable without afxwin.h, so it cannot be the one to borrow. Several
// structures here (NOTIFYICONDATAW.guidItem) have a GUID field, so pull the
// owner in rather than emitting a second definition. atlcomcli.h includes only
// atldef.h, so there is no cycle back into this file.
#include <atlcomcli.h>

