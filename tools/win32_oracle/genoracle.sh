#!/usr/bin/env bash
# Produce the two oracle files the generators read.
#
# Nothing is transcribed from documentation: clang preprocesses the maintained
# mingw-w64 headers with a Windows target and reports what it computed. Install
# them with `apt install mingw-w64-common` (headers only -- no compiler, no
# runtime, and deliberately no Wine: linking Wine's user32/gdi32 would have
# Wine draw the controls on Linux while the toolkit draws them on Windows).
#
#   macros.txt  -- every #define, with its value  (feeds mkconst.py)
#   preproc.i   -- every declaration, macros expanded but typedefs intact,
#                  which is what keeps LONG spelled LONG  (feeds mktypes.py)
#
# UNICODE is defined because eMule is built that way, and it decides whether
# LPLOGFONT means LOGFONTW or LOGFONTA -- WCHAR or CHAR in the face-name field.
set -eu
SDK=${SDK:-/usr/share/mingw-w64/include}
OUT=${1:-.}

cat > "$OUT/oracle.cpp" <<'HEADERS'
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wininet.h>
#include <richedit.h>
#include <share.h>
#include <timeapi.h>
#include <mmsystem.h>
HEADERS

for mode in "-dM -o $OUT/macros.txt" "-P -o $OUT/preproc.i"; do
    # shellcheck disable=SC2086
    clang++ -x c++ --target=x86_64-w64-mingw32 -isystem "$SDK" \
        -DUNICODE -D_UNICODE -E -w $mode "$OUT/oracle.cpp"
done
wc -l "$OUT/macros.txt" "$OUT/preproc.i"
