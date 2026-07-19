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
| `afxwin.h` | ✅ | — | **Stub only** — `CWnd` and the GUI hierarchy, `CWinThread`/`CWinApp`, message-map macros |
| `afxext.h` | ✅ | — | **Stub only** — `CControlBar`/`CDialogBar` |
| `afxdlgs.h` | ✅ | — | **Stub only** — `CPropertyPage`/`CPropertySheet` |
| `afxmsg_.h` | ✅ | — | **Stub only** — `ON_COMMAND`/`ON_MESSAGE`/`ON_CONTROL`/... macros |
| `afxdd_.h` | ✅ | — | **Stub only** — `DDX_*`/`DDV_*` (uses `CDataExchange`, declared in `afxwin.h`) |
| `afxsock.h` | ✅ | — | **Stub only** — `CAsyncSocket` |

The "stub only" headers **compile** (syntactically valid declarations,
verified together with the rest via `g++ -fsyntax-only`), but have no
`.cpp`: any program that includes them and tries to actually *use* those
types (not just declare a pointer to one) will fail to link with missing
symbols. They exist as a reference/interface awaiting a future
implementation (which would require a cross-platform GUI layer for
`afxwin.h`/`afxext.h`/`afxdlgs.h`/`afxdd_.h`, and system APIs — outside the
"standard C++ only" scope — for `afxsock.h`).

## Why some headers are not implemented

- **GUI (`afxwin.h`, `afxext.h`, `afxdlgs.h`, `afxdd_.h`, `afxmsg_.h`)** —
  `CWnd` and its subclasses, message maps, `DDX_*`: pure Win32 concepts
  (windows, messages), with no standard C++ equivalent without a dedicated
  cross-platform UI layer.
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

Verified: GCC 14 / Linux (clean build, zero warnings with
`-Wall -Wextra -Wpedantic`). `CMakeLists.txt` also includes the flags and
`#define`s needed for MSVC (`/EHsc`, `/permissive-`,
`_CRT_SECURE_NO_WARNINGS` for `std::localtime`/`vswprintf`...), but it has
not yet been built on a real Windows machine — if MSVC-specific errors
come up, please report them.

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

## RTTI

`CObject`/`DECLARE_DYNAMIC`/`IMPLEMENT_DYNAMIC`/`RUNTIME_CLASS`/`IsKindOf`
reproduce the *real* internal MFC mechanism: a chain of `CRuntimeClass`
walkable at runtime through a pointer to the base class — not the
compiler's `typeid`/`dynamic_cast`. A purely standard C++ construct, no
external dependency.
