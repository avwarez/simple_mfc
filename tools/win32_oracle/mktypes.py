#!/usr/bin/env python3
"""Extract struct/enum/typedef declarations from the preprocessed mingw-w64 SDK.

Companion to mkconst.py, which handles the #define half. Same principle: the
declarations are lifted verbatim from a maintained SDK rather than retyped, so
field order and field types cannot drift. Macros are already expanded in the
input, but TYPEDEFS are not -- which is what makes this safe on LP64: a field
comes out as `LONG lfHeight`, and afx.h already owns LONG with the correct
32-bit width. Emitting the raw `long` would have been 8 bytes here and 4 on
Windows.
"""
import re, sys, collections

PREPROC = 'preproc.i'

# Names simple_mfc already owns. Re-emitting these would either collide with a
# struct definition or, worse for the 64-bit ones, redeclare a typedef with a
# same-width-but-different type, which is ill-formed.
OWNED = {
    # afx.h
    'UINT', 'WORD', 'BYTE', 'DWORD', 'BOOL', 'LONG', 'LONGLONG', 'ULONGLONG',
    'HANDLE', 'UINT_PTR', 'INT_PTR', 'DWORD_PTR', 'ULONG_PTR', 'LPARAM',
    'WPARAM', 'LRESULT', 'HWND', 'HDC', 'HFONT', 'HBITMAP', 'HICON', 'HBRUSH',
    'HPEN', 'HMENU', 'HINSTANCE', 'HCURSOR', 'HRGN', 'HPALETTE', 'COLORREF',
    'TCHAR', 'LPTSTR', 'LPCTSTR', 'LPSTR', 'LPCSTR', 'LPWSTR', 'LPCWSTR',
    # atltypes.h
    'POINT', 'SIZE', 'RECT', 'tagPOINT', 'tagSIZE', 'tagRECT',
    # atlcomcli.h -- GUID included: it is a COM type and that header is
    # reachable without afxwin.h, so it cannot borrow platform's copy.
    'HRESULT', 'GUID', '_GUID', 'CLSID', 'IID', 'REFGUID', 'REFIID', 'REFCLSID',
    # glibc owns these on Linux. Emitting the SDK's spelling would be a typedef
    # redefinition -- and the long-narrowing pass makes it a redefinition with a
    # DIFFERENT type, since glibc's u_long really is 64-bit here. Where POSIX
    # already has the name, POSIX wins: that is the whole point of platform/.
    'u_long', 'u_short', 'u_char', 'u_int', 'timeval', 'timespec', 'fd_set',
    'socklen_t', 'in_addr', 'in6_addr', 'sockaddr', 'sockaddr_in',
    'sockaddr_in6', 'hostent', 'servent', 'protoent', 'addrinfo', 'size_t',
    'ssize_t', 'off_t', 'time_t', 'wchar_t', 'va_list', 'FILE',
}


PACK = re.compile(r'#\s*pragma\s+pack\s*\(\s*(push\s*,\s*(\d+)|pop|(\d+)|)\s*\)')


def pack_at(src):
    """offset -> struct packing in effect there.

    Windows structures are not all naturally aligned: mingw wraps DLGTEMPLATE in
    #pragma pack(push,2), and dropping that pragma while copying the fields
    produces a struct that compiles and has the wrong size. 138 pragmas guard
    the headers we read, so this is not a corner case. Replay the directives in
    order and remember what was in effect where.
    """
    events, stack, cur = [], [], None
    for m in PACK.finditer(src):
        if m.group(2):                       # push,N
            stack.append(cur); cur = int(m.group(2))
        elif m.group(1) == 'pop':
            cur = stack.pop() if stack else None
        elif m.group(3):                     # pack(N)
            cur = int(m.group(3))
        else:                                # pack() -- back to default
            cur = None
        events.append((m.end(), cur))
    return events


def pack_for(events, offset):
    value = None
    for end, cur in events:
        if end > offset:
            break
        value = cur
    return value


import os
INCLUDE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                           os.pardir, os.pardir, 'include')


def owned_by_include(directory):
    """Names simple_mfc's frozen interface already DEFINES.

    Hand-listing these does not scale -- HTREEITEM was the third collision found
    one sweep at a time. Harvest them instead, and note the distinction that
    makes this correct: a real definition (`using HTREEITEM = void*;`, or a
    struct with a body) is owned by include/ and platform must not restate it,
    while a bare forward declaration (`struct NCCALCSIZE_PARAMS;`) is a
    placeholder that platform is expected to fill in.
    """
    import glob, os
    names = set()
    for path in glob.glob(os.path.join(directory, '*.h')):
        text = open(path, encoding='utf-8', errors='replace').read()
        names.update(re.findall(r'\busing\s+([A-Za-z_]\w*)\s*=', text))
        names.update(re.findall(r'\b(?:struct|class|union)\s+([A-Za-z_]\w*)'
                                r'\s*(?:final\s*)?(?::[^;{]*)?\{', text))
        names.update(re.findall(r'\btypedef\s+[^;{}]+?\b([A-Za-z_]\w*)\s*;', text))
    return names


def read_decls(path):
    """name -> (kind, text, pack). Brace-matched so nested structs survive."""
    src = open(path, encoding='utf-8', errors='replace').read()
    decls = collections.OrderedDict()
    packs = pack_at(src)

    # typedef struct|union|enum [tag] { ... } NAME [, *PNAME]...;
    rx = re.compile(r'\btypedef\s+(struct|union|enum)\s+([A-Za-z_]\w*)?\s*\{')
    for m in rx.finditer(src):
        i = src.index('{', m.start())
        depth, j = 0, i
        while j < len(src):
            if src[j] == '{':
                depth += 1
            elif src[j] == '}':
                depth -= 1
                if depth == 0:
                    break
            j += 1
        end = src.find(';', j)
        if end < 0:
            continue
        text = src[m.start():end + 1]
        pack = pack_for(packs, m.start())
        names = re.findall(r'\*?\s*([A-Za-z_]\w*)', src[j + 1:end])
        for n in names:
            decls.setdefault(n, (m.group(1), text, pack))
        if m.group(2):
            decls.setdefault(m.group(2), (m.group(1), text, pack))

    # function-pointer typedef: typedef RET (<conv> *NAME)(args);
    # Matched before the plain form, whose [^;{}] body would swallow it.
    for m in re.finditer(
            # the prefix must tolerate nested parens: mingw spells the calling
            # convention as __attribute__((__stdcall__))
            r'\btypedef\s+[^;{}]*?\*\s*([A-Za-z_]\w*)\s*\)\s*\([^;{}]*\)\s*;',
            src):
        decls.setdefault(m.group(1), ('typedef', m.group(0), None))

    # plain typedef, possibly declaring several names at once:
    #   typedef unsigned int UINT32,*PUINT32;
    for m in re.finditer(r'\btypedef\s+([^;{}()]+?)\s*;', src):
        for n in re.findall(r'\*?\s*([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*(?:,|$)',
                            m.group(1)):
            decls.setdefault(n, ('typedef', m.group(0), None))

    return decls


# Calling conventions have no meaning on the targets we build for (ARM64 and
# x86-64 both have a single convention), and smfc_msvc_compat.h already defines
# the MSVC spellings as empty. Carrying mingw's attribute spelling through would
# only produce ignored-attribute noise.
CONV = re.compile(r'__attribute__\s*\(\(\s*__(?:stdcall|cdecl|fastcall|thiscall)__\s*\)\)\s*')
EXT = re.compile(r'\b(?:__extension__|__MINGW_EXTENSION|__forceinline)\s*')


# Windows' `long` is 32-bit; on LP64 Linux it is 64. Most SDK structs are
# written in typedefs (LONG, DWORD) that afx.h already owns at the right width,
# but some spell the base type outright -- GUID's `unsigned long Data1` is the
# one that matters, since a 4-byte field silently becoming 8 shifts every field
# after it and doubles the size of every CLSID. Narrow those back to `int`,
# which is 32-bit on both.
LONGLONG = re.compile(r'\b(unsigned\s+|signed\s+)?long\s+long\b')
LONG = re.compile(r'\b(unsigned\s+|signed\s+)?long\b')


def narrow_long(text):
    keep = []

    def stash(m):
        keep.append(m.group(0))
        return f'\0{len(keep) - 1}\0'

    text = LONGLONG.sub(stash, text)                       # long long is 64 on both
    text = LONG.sub(lambda m: (m.group(1) or '') + 'int', text)
    return re.sub(r'\0(\d+)\0', lambda m: keep[int(m.group(1))], text)


def clean(text):
    return narrow_long(EXT.sub('', CONV.sub('', text)))


def deps(text, decls):
    return [t for t in set(re.findall(r'[A-Za-z_]\w*', text))
            if t in decls and t not in OWNED]


def main():
    wanted = [a for a in sys.argv[1:] if not a.startswith('-')]
    decls = read_decls(PREPROC)
    OWNED.update(owned_by_include(INCLUDE_DIR))

    emitted, order = set(), []

    def want(name, depth=0):
        # Keyed on the DECLARATION, not the name: one struct is registered under
        # every name it declares (_PROPSHEETPAGEW, PROPSHEETPAGEW, LP...), and
        # keying on the name let the recursion re-enter the same struct through
        # a second name and append it ahead of the typedef it depends on.
        if name in OWNED or name not in decls or depth > 64:
            return
        kind, text, _pack = decls[name]
        if text in emitted:
            return
        emitted.add(text)
        for d in deps(text, decls):
            want(d, depth + 1)
        order.append(name)

    for n in wanted:
        want(n)

    missing = [n for n in wanted if n not in decls]
    sys.stderr.write(f'requested {len(wanted)}, emitted {len(order)} '
                     f'(with dependencies), not found {len(missing)}\n')
    for n in missing:
        sys.stderr.write(f'  not in SDK: {n}\n')

    # Forward-declare every struct tag first. The SDK has genuine dependency
    # cycles -- PROPSHEETPAGEW has a field of type LPFNPSPCALLBACKW, whose own
    # signature takes a `struct _PROPSHEETPAGEW *` -- and no emission order can
    # satisfy both. A pointer only needs an incomplete type, so declaring the
    # tags up front breaks every such cycle at once.
    tags = sorted({m for n in order
                   for m in re.findall(r'\btypedef\s+struct\s+([A-Za-z_]\w*)\s*\{',
                                       decls[n][1])})
    if tags:
        for t in tags:
            print(f'struct {t};')
        print()

    seen = set()
    for n in order:
        kind, text, pack = decls[n]
        if text in seen:
            continue
        seen.add(text)
        body = re.sub(r'\n\s*\n', '\n', clean(text).strip())
        if pack:
            print(f'#pragma pack(push, {pack})')
        print(body)
        if pack:
            print('#pragma pack(pop)')
        print()


if __name__ == '__main__':
    main()
