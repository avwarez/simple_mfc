#!/usr/bin/env python3
"""Runs on a windows-latest runner with the VS ATLMFC component installed.

For every symbol simple_mfc declares (class/struct, macro, using/typedef
alias, enum name, enum value, free function, global constexpr/extern),
finds the same-named real MFC/ATL header (searched anywhere under the
installed atlmfc tree, include/ and src/mfc/ alike) and reports the
matching declaration line(s) there, so a human/AI reviewer can diff
simple_mfc's version against the real one. Symbols with no match in the
corresponding real file are also reported (out of scope for "list only
the differences" is the reverse direction: real symbols absent from
simple_mfc -- those are not reported here).

Usage: python mfc_symbol_diff.py <simple_mfc_include_dir> <real_atlmfc_root> <output_file>
"""
import os
import re
import sys

def strip_comments(txt):
    txt = re.sub(r'/\*.*?\*/', '', txt, flags=re.S)
    txt = re.sub(r'//[^\n]*', '', txt)
    return txt

def extract_symbols(path):
    """Returns a list of (symbol, category) tuples declared in this file."""
    raw = open(path, encoding='latin-1').read()
    txt = strip_comments(raw)
    lines = txt.splitlines()
    out = []

    for line in lines:
        m = re.match(r'^\s*(class|struct)\s+([A-Za-z_][A-Za-z0-9_]*)\s*(:|;|\{|$)', line)
        if m:
            out.append((m.group(2), 'class/struct'))
            continue
        m = re.match(r'^\s*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)', line)
        if m:
            out.append((m.group(1), 'macro'))
            continue
        m = re.match(r'^\s*using\s+([A-Za-z_][A-Za-z0-9_]*)\s*=', line)
        if m:
            out.append((m.group(1), 'using-alias'))
            continue
        m = re.match(r'^\s*enum\s+(class\s+)?([A-Za-z_][A-Za-z0-9_]*)', line)
        if m:
            out.append((m.group(2), 'enum-type'))
            continue

    # classic typedefs (not macro-parameterized bodies -- skip continuation lines)
    for line in lines:
        if re.match(r'^\s*typedef\b', line) and not line.rstrip().endswith('\\'):
            m = re.search(r'([A-Za-z_][A-Za-z0-9_]*)\s*\)?\s*;', line)
            if m:
                out.append((m.group(1), 'typedef'))

    # enum value bodies
    for m in re.finditer(r'enum\s+(class\s+)?[A-Za-z_][A-Za-z0-9_]*\s*(:\s*\w+\s*)?\{([^}]*)\}', txt):
        body = m.group(3)
        for item in body.split(','):
            item = item.strip()
            if not item:
                continue
            nm = re.match(r'([A-Za-z_][A-Za-z0-9_]*)', item)
            if nm:
                out.append((nm.group(1), 'enum-value'))

    # global constexpr/extern
    for line in lines:
        m = re.match(r'^(constexpr|inline constexpr|static const|extern const|extern|const)\s+\S+\s+\**([A-Za-z_][A-Za-z0-9_]*)', line)
        if m:
            out.append((m.group(2), 'global'))

    # free functions: unindented, function-call-shaped, ending in ; on the same line
    for line in lines:
        if re.match(r'^(class|struct|enum|typedef|using|template|#|namespace)', line):
            continue
        m = re.match(r'^[A-Za-z_][A-Za-z0-9_:<>,\*&\s]*?([A-Za-z_][A-Za-z0-9_]*)\s*\([^;{]*\)[^;{]*;\s*$', line)
        if m:
            out.append((m.group(1), 'function'))

    return out

def find_real_files(atlmfc_root, basename):
    """Every file under atlmfc_root with this exact basename."""
    hits = []
    for dirpath, _, filenames in os.walk(atlmfc_root):
        if basename in filenames:
            hits.append(os.path.join(dirpath, basename))
    return hits

def main():
    simple_inc, atlmfc_root, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
    results = []
    real_file_cache = {}
    real_content_cache = {}

    for fname in sorted(os.listdir(simple_inc)):
        if not fname.endswith('.h'):
            continue
        fpath = os.path.join(simple_inc, fname)
        symbols = extract_symbols(fpath)
        if not symbols:
            continue

        if fname not in real_file_cache:
            real_file_cache[fname] = find_real_files(atlmfc_root, fname)
        real_paths = real_file_cache[fname]

        seen = set()
        for sym, cat in symbols:
            if (fname, sym) in seen:
                continue
            seen.add((fname, sym))

            if not real_paths:
                results.append((fname, sym, cat, 'REAL_FILE_NOT_FOUND', ''))
                continue

            pattern = re.compile(r'\b' + re.escape(sym) + r'\b')
            found_any = False
            for rp in real_paths:
                if rp not in real_content_cache:
                    try:
                        real_content_cache[rp] = open(rp, encoding='latin-1').read().splitlines()
                    except Exception as e:
                        real_content_cache[rp] = []
                rlines = real_content_cache[rp]
                matches = [(i + 1, l.strip()) for i, l in enumerate(rlines) if pattern.search(l)]
                if matches:
                    found_any = True
                    for ln, txt in matches[:6]:
                        results.append((fname, sym, cat, rp, f'{ln}: {txt}'))
                    if len(matches) > 6:
                        results.append((fname, sym, cat, rp, f'... ({len(matches)} total matches, showing first 6)'))
            if not found_any:
                results.append((fname, sym, cat, 'NO_MATCH_IN_REAL_FILE', ''))

    with open(out_path, 'w', encoding='utf-8') as out:
        for fname, sym, cat, real_ref, detail in results:
            out.write(f'{fname}\t{sym}\t{cat}\t{real_ref}\t{detail}\n')

    print(f'Wrote {len(results)} rows to {out_path}')

if __name__ == '__main__':
    main()
