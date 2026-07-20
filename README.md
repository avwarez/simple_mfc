# simple_mfc

A subset of the MFC (Microsoft Foundation Classes) interface, useful for
projects migrating away from real MFC, organized as a normal C++
**include/src** project:

- **`include/`** — the declarations (interface). Some headers are
  **implemented** (they have a matching `.cpp` in `src/`, with real bodies
  built only on the standard C++17 library); others are still
  **declaration-only stubs** (no implementation, signatures verified
  against the official Microsoft Learn documentation) because they are
  inherently tied to the Win32 GUI or to system APIs — see the table below.
- **`src/`** — the implementation of the "real" headers only.

Every header lives **in exactly one place** (`include/`): there is no
separate "stub" copy and "native" copy.

## Status per header

| Header | `include/` | `src/` | Status |
|---|---|---|---|
| `afx.h` | ✅ | `afx.cpp` | **Implemented** — MFC-style RTTI, `CString`, `CException`/`CFileException`/`CMemoryException`, `CFile`/`CStdioFile`/`CMemFile`, `CFileFind` |
| `afxcoll.h` | ✅ | `afxcoll.cpp` | **Implemented** — `CObList`/`CPtrList`/`CStringList`/`CObArray`/`CPtrArray`/`CStringArray`/`CByteArray`/`CUIntArray` |
| `afxtempl.h` | ✅ | *(header-only)* | **Implemented** — `CArray<>`/`CList<>`/`CMap<>` |
| `afxmt.h` | ✅ | `afxmt.cpp` | **Implemented** — `CCriticalSection`/`CEvent`/`CMutex`/`CSingleLock` |
| `atltime.h` | ✅ | `atltime.cpp` | **Implemented** — `CTime`/`CTimeSpan` |
| `afxwin.h` | ✅ | — | **Stub only** — `CWnd`/`CDialog`/`CFrameWnd`/`CStatic`/`CEdit`/`CListBox`/`CComboBox`/`CButton`/`CMenu`, `CWinThread`/`CWinApp`, core GDI (`CGdiObject`/`CBitmap`/`CPen`/`CBrush`/`CFont`/`CDC`), message-map macros |
| `afxext.h` | ✅ | — | **Stub only** — `CControlBar`/`CDialogBar` |
| `afxdlgs.h` | ✅ | — | **Stub only** — `CPropertyPage`/`CPropertySheet` |
| `afxmsg_.h` | ✅ | — | **Stub only** — `ON_COMMAND`/`ON_MESSAGE`/`ON_CONTROL`/`ON_WM_*` (51 variants)/... macros |
| `afxdd_.h` | ✅ | — | **Stub only** — `DDX_*`/`DDV_*` (uses `CDataExchange`, declared in `afxwin.h`) |
| `afxcmn.h` | ✅ | — | **Stub only** — `CImageList`/`CTreeCtrl`/`CListCtrl`/`CRichEditCtrl`/`CTabCtrl`/`CToolBarCtrl`/`CStatusBarCtrl`/`CToolTipCtrl` |
| `afxole.h` | ✅ | — | **Stub only** — `COleDropTarget` |
| `afxdhtml.h` | ✅ | — | **Stub only** — `CDHtmlDialog` (hierarchy only, no methods used) |
| `afxdtctl.h` | ✅ | — | **Stub only** — `CDateTimeCtrl` |
| `atltypes.h` | ✅ | — | **Stub only** — `CPoint`/`CRect`/`CSize` (ATL/MFC "shared classes", not declared in `afxwin.h`) |
| `afxsock.h` | ✅ | — | **Stub only** — `CAsyncSocket` |

The "stub only" headers **compile** (syntactically valid declarations,
verified together with the rest via `g++ -fsyntax-only`), but have no
`.cpp`: any program that includes them and tries to actually *use* those
types (not just declare a pointer to one) will fail to link with missing
symbols. They exist as a reference/interface awaiting a future
implementation (which would require a cross-platform GUI layer for
`afxwin.h`/`afxext.h`/`afxdlgs.h`/`afxdd_.h`/`afxcmn.h`/`afxole.h`/
`afxdhtml.h`/`afxdtctl.h`/`atltypes.h`, and system APIs — outside the "standard C++
only" scope — for `afxsock.h`).

The GUI/GDI stubs only declare the **subset of classes and methods
actually used by a real MFC application** (eMule/srchybrid), not the
full real-MFC API surface — each signature was cross-referenced against
the official Microsoft Learn documentation. See `mfc_scan_srchybrid.md`
(sibling of this repo) for the full method-by-method scan (occurrence
counts, exact signatures, required headers) this subset was derived
from.

## Why some headers are not implemented

- **GUI (`afxwin.h`, `afxext.h`, `afxdlgs.h`, `afxdd_.h`, `afxmsg_.h`,
  `afxcmn.h`, `afxole.h`, `afxdhtml.h`, `afxdtctl.h`, `atltypes.h`)** — `CWnd` and its
  subclasses, common controls, GDI drawing, message maps, `DDX_*`: pure
  Win32 concepts (windows, messages, device contexts), with no standard
  C++ equivalent without a dedicated cross-platform UI layer.
- **Networking (`afxsock.h`)** — sockets are not "Windows-specific" in a
  strict sense (they also exist on POSIX), but they still require system
  APIs: they are not part of the C++ standard library (no Networking TS in
  C++17/20/23). Implementing them would violate the "standard C++
  constructs only" constraint.

## Build

### With CMake (recommended — GCC, Clang, MSVC)

```sh
cmake -S . -B build                 # on Windows with MSVC: add -G "Visual Studio 17 2022" (or the generator you prefer)
cmake --build build
ctest --test-dir build              # runs the smoke test
```

Produces a `simple_mfc` static library (target `simple_mfc::simple_mfc`)
plus the `simple_mfc_smoke_test` executable (disable with
`-DSIMPLE_MFC_BUILD_SMOKE_TEST=OFF`). `cmake --install build --prefix <dir>`
installs the headers + library for use as an external dependency.

Verified in CI on both GCC 14 / Linux and MSVC / Windows (`windows-latest`
GitHub Actions runner) — clean builds, zero warnings with
`-Wall -Wextra -Wpedantic` / `/W4`. See `.github/workflows/msvc.yml`.

### Manual (without CMake)

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -c src/afx.cpp -o afx.o
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -c src/afxcoll.cpp -o afxcoll.o
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -c src/afxmt.cpp -o afxmt.o
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -c src/atltime.cpp -o atltime.o
# link the .o files into your program, with -Iinclude and -pthread (for afxmt.cpp)
```

## Known limitations (native implementation)

- **`POSITION`** (in `afxcoll.h`/`afxtempl.h`) is an opaque pointer to a
  heap-allocated iterator (a "box"): if a `GetHeadPosition()` /
  `GetNext()` loop is interrupted before reaching `POSITION == nullptr`,
  that box is not freed. In full-loop usage (the near-universal pattern)
  there is no leak.
- **`CMutex`** does not support "named" mutexes shared across processes
  (`lpszName` is accepted but ignored): that is a Win32 kernel object, not
  expressible in pure standard C++. In-process only.
- **`CTime`** uses `std::localtime` (standard, but not thread-safe: it
  uses an internal static buffer) — `localtime_r`/`localtime_s` are
  POSIX/Windows extensions, not standard C++.
- **`CString::LoadString`** (PE `.rc` resources) is omitted: no standard
  C++ equivalent.
- **`CException::ReportError`** prints to `stderr` instead of a Win32
  `MessageBox` (an equivalent "headless" behavior).
- **`AfxThrowFileException`/`AfxThrowMemoryException`** throw by
  **pointer** (`throw new CFileException(...)`), not by value: this is
  the real MFC pattern, needed for compatibility with existing MFC-style
  code that catches with `catch (CFileException* e)` and then calls
  `e->Delete()`.
- **`AfxThrowInvalidArgException`/`AfxThrowNotSupportedException`/
  `AfxThrowResourceException`/`AfxThrowUserException`/
  `AfxThrowArchiveException`** are not implemented (nor declared): only
  `AfxThrowFileException` and `AfxThrowMemoryException` are provided.
- **`CMap::GetHashTableSize()`** is implemented (`bucket_count()`), while
  `PGetFirstAssoc`/`PGetNextAssoc`/`PLookup` are also implemented as
  temporary-view accessors over the underlying `std::unordered_map` node.

## Conformance test suite (simple_mfc vs. real MFC)

`tests/conformance/` runs the exact same call sequence — same class, same
method, same input — against simple_mfc's native implementation and
against the actual Visual Studio MFC libraries (statically linked), then
diffs the two runs' output byte-for-byte. It's the strongest verification
this project has: not "does it compile," but "does it behave and return
the same thing as real MFC."

It only runs on MSVC with the "MFC and ATL" Visual Studio component
installed (CI installs it explicitly — see
`.github/workflows/msvc.yml`, job `conformance`), since real MFC doesn't
exist anywhere else:

```sh
cmake -S . -B build -DSIMPLE_MFC_BUILD_CONFORMANCE_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

It covers RTTI/`IsKindOf`, `CString`, the `CFile` family, `CFileFind`,
every collection class (concrete and template), `CTime`/`CTimeSpan`, and
behavioral tests (not just return values) for the synchronization
primitives — e.g. that an auto-reset `CEvent` wakes exactly one waiter,
that a manual-reset one stays signaled until `ResetEvent()`, that N
threads incrementing a counter through a `CCriticalSection` never lose an
update.

### What building this suite found

Running the same code against real MFC surfaced a few genuine gaps,
fixed in the library itself (not just in the test):

- **`CFileException::m_strFileName`** was a `std::wstring` internally;
  real MFC's is a `CString`. Now matches.
- **`CStdioFile::ReadString`** was stripping `\r` unconditionally. Real
  MFC — when the file is opened without the `typeText` flag, as
  simple_mfc always does — only splits on `\n` and leaves a `\r` from a
  `"\r\n"` terminator as the last character of the line. Now matches.
- **`CMap<CString, ...>`** needs `ARG_KEY = LPCTSTR`, not
  `const CString&` — real MFC only ships a `HashKey(LPCTSTR)` overload,
  and `const CString&` is an exact match for the generic (`long`-casting)
  fallback template instead, which doesn't compile for a class type. This
  is also simply the standard, documented MFC idiom for CString-keyed
  maps.
- **`CStdioFile::ReadString(LPTSTR, UINT)`** (the buffer overload) was
  dropping the trailing `\n`. Real MFC's is `fgets`-like: it keeps the
  `\n` (and the preceding `\r`, since the file is opened as binary) as the
  last characters written into the buffer. The `CString&` overload strips
  the `\n` but keeps the `\r`. Now both match.
- **Array `Append` (`CObArray`/`CPtrArray`/`CArray<>`)** returned the new
  total size; real MFC returns the **index of the first appended element**
  (the old size). Now matches.
- **`CFile::Seek` with origin `current`** doubled the offset (it applied
  the relative offset to both the get and put pointer of the single shared
  file position). It now syncs the put pointer to the absolute position
  resolved by the get seek. `begin`/`end` were unaffected (absolute, so
  idempotent), which is why only current-origin seeks were wrong.
- **`CFileFind::GetRoot`** returned the filesystem root (`C:\`); real MFC
  returns the searched directory with the trailing separator kept
  (e.g. `C:\dir\`). Now matches.

And a few things that are **not** conformance bugs, just properties of
testing outside a running GUI app (documented inline in `cases.cpp` at
each point they matter):

- `CFileException`/`CMemoryException::GetErrorMessage()`'s exact *text*
  is never compared: real MFC builds it from MFC's own string resources
  plus the OS's localized `FormatMessage` output, while simple_mfc uses
  fixed English text.
- In a bare console harness with no `CWinApp` (by design — these are the
  non-GUI classes), real MFC's resource-string lookup itself fails:
  `CFileException::GetErrorMessage` still returns `TRUE` but with an
  empty message; `CMemoryException::GetErrorMessage` — whose message
  comes *only* from a resource string, no `FormatMessage` fallback —
  returns `FALSE` outright. Neither is compared for that reason.

### Known conformance gaps (deliberately not exercised yet)

These are real behavioral differences from MFC that the suite intentionally
does **not** exercise for now, because a faithful match is hard or
impossible in pure standard C++17. They are documented here so they are not
mistaken for "verified equivalent." **To be refined, near the end of the
project, with the best available solution:**

- **`CFileFind` dot entries (`.` and `..`).** `std::filesystem::directory_iterator`
  skips the `.`/`..` pseudo-entries, whereas real MFC's `FindFile`/`FindNextFile`
  (over the Win32 `FindFirstFile` API) enumerates them. A wildcard search
  such as `*` would therefore return two extra entries under real MFC, and
  `IsDots()` never observes a `TRUE` result under simple_mfc. The suite
  currently searches `*.txt` (which the dot entries never match) to avoid
  the divergence. A faithful fix would synthesize the two entries during
  enumeration.
- **Non-ASCII case conversion / case-insensitive compare.**
  `CString::MakeUpper`/`MakeLower`/`CompareNoCase` use the C runtime
  (`towupper`/`towlower`), which in practice only case-folds ASCII, while
  real MFC uses the Win32 `CharUpperBuff`/`CharLowerBuff` API with full
  Unicode case mapping. Results diverge for accented/non-ASCII characters.
  The suite currently uses ASCII-only inputs for these methods. A faithful
  fix would require a Unicode case-mapping table (outside the standard
  library).

## RTTI

`CObject`/`DECLARE_DYNAMIC`/`IMPLEMENT_DYNAMIC`/`RUNTIME_CLASS`/`IsKindOf`
reproduce the *real* internal MFC mechanism: a chain of `CRuntimeClass`
walkable at runtime through a pointer to the base class — not the
compiler's `typeid`/`dynamic_cast`. A purely standard C++ construct, no
external dependency.
