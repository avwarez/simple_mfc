# win32_oracle

Generates `platform/win32_constants.h` and `platform/win32_types.h` from the
mingw-w64 SDK headers instead of hand-writing them.

## Why

Porting eMule to Linux left ~300 undefined Win32 symbols, and roughly 260 of
them were plain constants and struct layouts. Writing those by hand is 260
chances to get a value wrong in a way that compiles and then misbehaves
silently at run time. So they are not written: clang preprocesses a maintained
SDK with a Windows target, and the generators pick out the subset eMule
actually needs.

Three bugs found in the first hour, none of which a careful reading would have
caught:

* `LPLOGFONT` resolved to `LOGFONTA` (`CHAR lfFaceName[32]`) instead of
  `LOGFONTW` (`WCHAR`) — a different struct, half the size in that field. Fixed
  by preprocessing with `UNICODE` defined, as eMule builds.
* `GUID.Data1` is `unsigned long` in the SDK: 4 bytes on Windows, 8 on LP64.
  Every field after it would have shifted. The generator narrows bare `long`
  to `int`, leaving `long long` alone.
* `DLGTEMPLATE` lives inside `#pragma pack(push,2)`. 138 pack directives guard
  these headers; copying the fields without the pragma yields a struct that
  compiles and has the wrong size.

## Use

```sh
apt install mingw-w64-common          # headers only: no compiler, no runtime
./genoracle.sh /tmp/oracle            # -> macros.txt, preproc.i
cd /tmp/oracle
python3 .../mkconst.py --emit win32_constants.h --wanted wanted.txt
python3 .../mktypes.py $(cat wanted.txt) > body.h
cat .../types_preamble.h body.h > win32_types.h
```

`wanted.txt` is the accumulated list of names the compiler has asked for.
Accumulated, not regenerated: a translation unit stops at its first hard error,
so fixing a symbol lets clang get further and reveal symbols it never reached.
Driving the next round from the latest error log alone would drop everything
the previous round fixed, and the sweep would oscillate instead of converging.
Measured convergence on eMule's 244 sources: 278 → 486 → 552 → 578 → 588 → 628
→ 654 names, deltas shrinking throughout.

## What it will not do

Only declarations. The remaining gaps are behaviour — `GetTickCount`,
`SelectObject`, `PlaySound` — and behaviour has to be re-aimed at Linux by
hand, which is what `platform/win32_kernel.h`, `tchar.h` and `share.h` are for.
The split is deliberate: anything that can be copied from a maintained SDK is
copied, and anything that requires a decision is written down with the reason.

Names that `simple_mfc/include` already **defines** are skipped automatically —
harvested from the headers rather than listed here, because that list was wrong
three times in a row. A bare forward declaration (`struct NCCALCSIZE_PARAMS;`)
does not count as owning the name: it is a placeholder `platform/` is expected
to fill.
