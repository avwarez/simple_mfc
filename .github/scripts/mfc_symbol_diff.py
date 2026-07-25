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
    """Every file under atlmfc_root with this basename (case-insensitive,
    since the real tree ships e.g. ATLComTime.h while simple_mfc mirrors it
    as atlcomtime.h and the walk comparison would otherwise miss it)."""
    hits = []
    target = basename.lower()
    for dirpath, _, filenames in os.walk(atlmfc_root):
        for fn in filenames:
            if fn.lower() == target:
                hits.append(os.path.join(dirpath, fn))
    return hits

def declaration_regex(sym, cat):
    """A regex that matches a DECLARATION of `sym` (not a mere usage), keyed
    by the category simple_mfc classified it as. Deliberately conservative:
    better to miss a declaration (-> reported as ONLY_USAGE/NOT_IN_ATLMFC,
    a visible, checkable verdict) than to accept a call site as a decl."""
    s = re.escape(sym)
    if cat == 'class/struct':
        return re.compile(r'\b(class|struct)\s+(\w+\s+)?' + s + r'\b')
    if cat == 'macro':
        return re.compile(r'#\s*define\s+' + s + r'\b')
    if cat in ('using-alias', 'typedef'):
        return re.compile(r'\busing\s+' + s + r'\s*=|\btypedef\b.*\b' + s + r'\s*;')
    if cat == 'enum-type':
        return re.compile(r'\benum\s+(class\s+|struct\s+)?' + s + r'\b')
    if cat == 'enum-value':
        # A declaration line inside an enum body: the name starts the line
        # and is followed by '=', ',', '}' or end-of-line -- not "x = SYM;".
        return re.compile(r'^\s*' + s + r'\s*(=[^=]|,|$|\})')
    if cat == 'function':
        # A prototype: name followed by '(' on a line that ends in ';'.
        return re.compile(r'\b' + s + r'\s*\(.*\)\s*(const\s*)?;\s*$')
    if cat == 'global':
        return re.compile(r'\b(extern|constexpr|static|const)\b.*\b' + s + r'\b')
    return re.compile(r'\b' + s + r'\b')

def analyze_placement(tree_files, real_content_cache, simple_files_syms):
    """For each simple_mfc (header, symbol), find the real .h header(s) that
    DECLARE it and compare basenames (case-insensitive). Returns rows of
    (simple_header, symbol, category, verdict, real_decl_headers, sample)."""
    headers = [tf for tf in tree_files if tf.lower().endswith('.h')]
    # raw text per header, for a cheap substring pre-filter
    raw_cache = {}
    for tf in headers:
        if tf not in real_content_cache:
            try:
                real_content_cache[tf] = open(tf, encoding='latin-1').read().splitlines()
            except Exception:
                real_content_cache[tf] = []
        raw_cache[tf] = '\n'.join(real_content_cache[tf])

    rows = []
    for fname, symbols in simple_files_syms:
        want = fname.lower()
        seen = set()
        for sym, cat in symbols:
            if (fname, sym) in seen:
                continue
            seen.add((fname, sym))
            rx = declaration_regex(sym, cat)
            decl_headers = {}   # basename_lower -> (path, line, text)
            for tf in headers:
                if sym not in raw_cache[tf]:
                    continue
                for i, l in enumerate(real_content_cache[tf]):
                    if rx.search(l):
                        bn = os.path.basename(tf)
                        if bn.lower() not in decl_headers:
                            decl_headers[bn.lower()] = (tf, i + 1, l.strip())
                        break
            if not decl_headers:
                rows.append((fname, sym, cat, 'NOT_DECLARED_IN_ATLMFC_HEADERS', '', ''))
                continue
            if want in decl_headers:
                tf, ln, txt = decl_headers[want]
                rows.append((fname, sym, cat, 'SAME_HEADER',
                             os.path.basename(tf), f'{ln}: {txt}'))
            else:
                names = sorted(os.path.basename(v[0]) for v in decl_headers.values())
                tf, ln, txt = next(iter(decl_headers.values()))
                rows.append((fname, sym, cat, 'DIFFERENT_HEADER',
                             ';'.join(names), f'{ln}: {txt}'))
    return rows

def build_tree_index(atlmfc_root):
    """All .h/.cpp files anywhere under atlmfc_root, for the NO_MATCH fallback."""
    files = []
    for dirpath, _, filenames in os.walk(atlmfc_root):
        for fn in filenames:
            if fn.endswith('.h') or fn.endswith('.cpp'):
                files.append(os.path.join(dirpath, fn))
    return files

def main():
    simple_inc, atlmfc_root, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
    placement_path = sys.argv[4] if len(sys.argv) > 4 else None
    results = []
    real_file_cache = {}
    real_content_cache = {}
    tree_files = None
    simple_files_syms = []

    for fname in sorted(os.listdir(simple_inc)):
        if not fname.endswith('.h'):
            continue
        fpath = os.path.join(simple_inc, fname)
        symbols = extract_symbols(fpath)
        if not symbols:
            continue
        simple_files_syms.append((fname, symbols))

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
                # Fallback: this symbol may canonically live in a different
                # real header than the one simple_mfc happened to put it in
                # (e.g. a Windows-SDK type simple_mfc must forward-declare
                # locally, or an MFC symbol whose real home is a different
                # file). Search the whole atlmfc tree for it before giving
                # up, and say where it actually turned up.
                if tree_files is None:
                    tree_files = build_tree_index(atlmfc_root)
                elsewhere = []
                for tf in tree_files:
                    if tf in real_paths:
                        continue
                    if tf not in real_content_cache:
                        try:
                            real_content_cache[tf] = open(tf, encoding='latin-1').read().splitlines()
                        except Exception:
                            real_content_cache[tf] = []
                    tlines = real_content_cache[tf]
                    tmatches = [(i + 1, l.strip()) for i, l in enumerate(tlines) if pattern.search(l)]
                    if tmatches:
                        elsewhere.append((tf, tmatches))
                        if len(elsewhere) >= 3:
                            break
                if elsewhere:
                    for tf, tmatches in elsewhere:
                        for ln, txt in tmatches[:3]:
                            results.append((fname, sym, cat, f'ELSEWHERE:{tf}', f'{ln}: {txt}'))
                else:
                    results.append((fname, sym, cat, 'NOT_FOUND_ANYWHERE_IN_ATLMFC', ''))

    with open(out_path, 'w', encoding='utf-8') as out:
        for fname, sym, cat, real_ref, detail in results:
            out.write(f'{fname}\t{sym}\t{cat}\t{real_ref}\t{detail}\n')

    print(f'Wrote {len(results)} rows to {out_path}')

    if placement_path:
        if tree_files is None:
            tree_files = build_tree_index(atlmfc_root)
        prows = analyze_placement(tree_files, real_content_cache, simple_files_syms)
        with open(placement_path, 'w', encoding='utf-8') as out:
            out.write('simple_mfc_header\tsymbol\tcategory\tverdict\treal_declaring_headers\tsample_decl_line\n')
            for r in prows:
                out.write('\t'.join(r) + '\n')
        diff = sum(1 for r in prows if r[3] == 'DIFFERENT_HEADER')
        same = sum(1 for r in prows if r[3] == 'SAME_HEADER')
        none = sum(1 for r in prows if r[3] == 'NOT_DECLARED_IN_ATLMFC_HEADERS')
        print(f'Placement: {len(prows)} rows -> SAME={same} DIFFERENT={diff} NOT_DECLARED={none} to {placement_path}')

if __name__ == '__main__':
    main()
