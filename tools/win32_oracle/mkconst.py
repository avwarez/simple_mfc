#!/usr/bin/env python3
"""Generate platform/ constants from the mingw-w64 headers, using clang as the
oracle rather than my memory.

The values are not transcribed: they are whatever a real compiler computes when
it preprocesses the maintained mingw-w64 headers with a Windows target. This
script only decides WHICH of those definitions we need -- the ones eMule
actually failed on -- and normalises the couple of mingw-isms that do not
survive a move to LP64.
"""
import re, sys, collections

MACROS = 'macros.txt'
ERRORS = 'errors.log'

# clang's spellings for "this name does not exist"
MISSING = [
    re.compile(r"use of undeclared identifier '([A-Za-z_]\w*)'"),
    re.compile(r"unknown type name '([A-Za-z_]\w*)'"),
    re.compile(r"no type named '([A-Za-z_]\w*)'"),
    re.compile(r"no member named '([A-Za-z_]\w*)'"),
    re.compile(r"unknown type name '([A-Za-z_]\w*)'; did you mean"),
]


def missing_symbols(path):
    counts = collections.Counter()
    with open(path, encoding='utf-8', errors='replace') as fh:
        for line in fh:
            for rx in MISSING:
                m = rx.search(line)
                if m:
                    counts[m.group(1)] += 1
                    break
    return counts


def macro_table(path):
    """name -> (params or None, body). Order is preserved: a macro may use one
    defined earlier, and we emit in the same order so that still holds."""
    table = collections.OrderedDict()
    rx = re.compile(r'^#define\s+([A-Za-z_]\w*)(\([^)]*\))?\s*(.*)$')
    with open(path, encoding='utf-8', errors='replace') as fh:
        for line in fh:
            m = rx.match(line.rstrip('\n'))
            if m:
                table[m.group(1)] = (m.group(2), m.group(3))
    return table


# mingw spells a Windows 32-bit long constant as __MSABI_LONG(x) -> x##l.
# On LP64 that 'l' would widen the constant to 64 bits, so strip the wrapper
# and let the value keep its natural (32-bit) type.
MSABI = re.compile(r'__MSABI_LONG\(\s*([^()]*?)\s*\)')


def normalise(body):
    return MSABI.sub(r'\1', body)


def dependencies(body, table):
    """Other macros this body references, so we can emit them too."""
    return [t for t in re.findall(r'[A-Za-z_]\w*', body) if t in table]


def main():
    # --wanted <file> feeds the accumulated list from loop.sh; without it we
    # look only at the latest sweep, which is right for a one-off measurement
    # but would drop everything an earlier round already fixed.
    if '--wanted' in sys.argv:
        names = open(sys.argv[sys.argv.index('--wanted') + 1]).read().split()
        counts = collections.Counter(names)
    else:
        counts = missing_symbols(ERRORS)
    table = macro_table(MACROS)

    wanted, order = set(), []

    def want(name, depth=0):
        if name in wanted or name not in table or depth > 64:
            return
        wanted.add(name)
        params, body = table[name]
        for dep in dependencies(body, table):
            want(dep, depth + 1)
        order.append(name)          # after its dependencies

    resolved = [s for s in counts if s in table]
    for s in sorted(resolved):
        want(s)

    unresolved = sorted(s for s in counts if s not in table)

    print(f'missing symbols in errors.log : {len(counts)}')
    print(f'  resolved as mingw macros    : {len(resolved)}')
    print(f'  emitted incl. dependencies  : {len(order)}')
    print(f'  NOT macros (types/structs)  : {len(unresolved)}')
    if '--list-unresolved' in sys.argv:
        for s in unresolved:
            print(f'    {s:40s} {counts[s]}')

    if '--emit' not in sys.argv:
        return

    with open(sys.argv[sys.argv.index('--emit') + 1], 'w') as out:
        out.write('''// win32_constants.h -- GENERATED, do not edit by hand.
//
// Every value here was computed by clang preprocessing the mingw-w64 headers
// with a Windows target; none of it was transcribed by hand. Regenerate with
// scratchpad/mkconst.py rather than patching a value in place.
//
// Only the constants eMule actually failed on are emitted, plus whatever they
// transitively reference -- this is not a copy of the SDK.
//
// Guarded individually because afx.h/atltypes.h own a few of these names and
// must keep owning them: single owner per symbol.

#pragma once

''')
        for name in order:
            params, body = table[name]
            body = normalise(body)
            out.write(f'#ifndef {name}\n#define {name}{params or ""}'
                      f'{(" " + body) if body else ""}\n#endif\n')
        out.write(f'\n// {len(order)} definitions\n')


if __name__ == '__main__':
    main()
