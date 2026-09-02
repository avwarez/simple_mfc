#!/usr/bin/env python3
"""Which declared members does the conformance suite actually reach?

Scans include/*.h for the public and protected member functions of every
E-prefixed class, then looks for each one in the probes.

Two populations, counted apart, because they are detected differently:

  named methods     called by name. Detection uses the word-boundary form,
                    not `.Name(`: the cases -- like eMule -- call inherited
                    members unqualified, and a dot-anchored search misses
                    those. That exact false negative once hid a method eMule
                    needs (CAsyncSocket::ReceiveFrom).

  operators and     invoked by SYNTAX (a == b, s[0], (HANDLE)ev), so no
  conversions       name search can find them. They are listed, not scored:
                    reaching them is checked by reading the cases.

Exit code is 1 while any named method is unreached, so CI can gate on it.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
HEADERS = os.path.join(ROOT, "include")
PROBES = [
    os.path.join(ROOT, "tests", "conformance", "cases.cpp"),
    os.path.join(ROOT, "tests", "portability", "instantiate.cpp"),
    os.path.join(ROOT, "tests", "coexistence", "coexist.cpp"),
]

KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "static_assert",
            "else", "catch", "throw", "do", "new", "delete", "decltype", "noexcept"}


def strip(text):
    """Blank out comments and literals in ONE pass.

    Two passes cannot do it: a line comment regex run first eats the rest of
    any line holding a "http://..." literal, unbalancing every quote after
    it -- which silently blanked most of cases.cpp and reported a whole
    class as unreached.
    """
    out, i, n = [], 0, len(text)
    while i < n:
        two = text[i:i + 2]
        if two == "//":
            while i < n and text[i] != "\n":
                i += 1
        elif two == "/*":
            i += 2
            while i < n and text[i:i + 2] != "*/":
                i += 1
            i += 2
        elif text[i] in "\"'":
            quote = text[i]
            i += 1
            while i < n and text[i] != quote:
                i += 2 if text[i] == "\\" else 1
            i += 1
            out.append(quote * 2)
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


def member_name(statement):
    """The declared name in a member-function declaration, or None."""
    depth = 0
    for i, ch in enumerate(statement):
        if ch in "<[":
            depth += 1
        elif ch in ">]":
            depth -= 1
        elif ch == "(" and depth == 0:
            head = statement[:i].strip()
            if head.endswith("operator"):
                pass
            hit = re.search(r"(operator\s*(?:[^\w\s(]+|[A-Za-z_][\w:\s\*&]*))$|([A-Za-z_]\w*)$", head)
            if not hit:
                return None
            name = re.sub(r"\s+", " ", (hit.group(1) or hit.group(2)).strip())
            return name
    return None


def declared():
    out = {}
    for filename in sorted(os.listdir(HEADERS)):
        if not filename.endswith(".h"):
            continue
        text = strip(open(os.path.join(HEADERS, filename), encoding="utf-8").read())
        i, n = 0, len(text)
        stack = []          # (class name, access)
        buf = ""
        while i < n:
            ch = text[i]
            if ch == "{":
                head = re.search(r"(?:class|struct)\s+(E[A-Za-z_]\w*)[^;{]*$", buf)
                if head:
                    kind = "class" if re.search(r"\bclass\s+" + head.group(1) + r"\b", buf) else "struct"
                    stack.append([head.group(1), "private" if kind == "class" else "public"])
                    out.setdefault(head.group(1), {"named": set(), "operators": set()})
                    buf = ""
                    i += 1
                    continue
                if stack:                       # a function body: skip it whole
                    name = member_name(buf)
                    record(out, stack, buf, name)
                    depth = 0
                    while i < n:
                        if text[i] == "{":
                            depth += 1
                        elif text[i] == "}":
                            depth -= 1
                            if depth == 0:
                                break
                        i += 1
                    buf = ""
                    i += 1
                    continue
                depth = 0                        # a namespace or free function
                while i < n:
                    if text[i] == "{":
                        depth += 1
                    elif text[i] == "}":
                        depth -= 1
                        if depth == 0:
                            break
                    i += 1
                buf = ""
                i += 1
                continue
            if ch == "}":
                if stack:
                    stack.pop()
                buf = ""
                i += 1
                continue
            if ch == ";":
                if stack:
                    record(out, stack, buf, member_name(buf))
                buf = ""
                i += 1
                continue
            if ch == ":" and stack:
                label = re.search(r"\b(public|protected|private)\s*$", buf)
                if label:
                    stack[-1][1] = label.group(1)
                    buf = ""
                    i += 1
                    continue
            buf += ch
            i += 1
    return {k: v for k, v in out.items() if v["named"] or v["operators"]}


def record(out, stack, statement, name):
    if not name:
        return
    cls, access = stack[-1]
    if access not in ("public", "protected"):
        return
    if name in KEYWORDS or name == cls or name.startswith("~"):
        return
    if name.startswith("operator"):
        out[cls]["operators"].add(name)
    elif re.fullmatch(r"[A-Za-z_]\w*", name):
        out[cls]["named"].add(name)


def bases():
    """{class: base class} for the E-prefixed hierarchy, so an inherited
    call credits the class that DECLARES the member, not the derived one."""
    out = {}
    for filename in sorted(os.listdir(HEADERS)):
        if not filename.endswith(".h"):
            continue
        text = strip(open(os.path.join(HEADERS, filename), encoding="utf-8").read())
        for hit in re.finditer(r"(?:class|struct)\s+(E\w+)\s*:\s*(?:public|protected|private)?\s*(E\w+)", text):
            out[hit.group(1)] = hit.group(2)
    return out


def probe_text():
    text = ""
    for path in PROBES:
        if os.path.exists(path):
            text += strip(open(path, encoding="utf-8").read())
    return text


def calls(text, classes):
    """(class, method) pairs the probes make, plus names called unqualified.

    Variables are typed from their declaration, which is what makes the
    attribution possible at all; MFC spelling in the probes maps back to the
    E-prefixed declarations by dropping the E.
    """
    mfc = {c[1:]: c for c in classes}
    var_type = {}
    for hit in re.finditer(r"\b(C[A-Za-z_]\w*)\s*(?:<[^;{}()]*>)?\s+(\*\s*)?([A-Za-z_]\w*)\s*[;=({\[]", text):
        if hit.group(1) in mfc:
            var_type.setdefault(hit.group(3), set()).add(mfc[hit.group(1)])

    attributed, by_name = set(), set()
    for hit in re.finditer(r"\b([A-Za-z_]\w*)\s*(?:\.|->)\s*([A-Za-z_]\w*)\s*\(", text):
        candidates = var_type.get(hit.group(1))
        if candidates:
            for cls in candidates:
                attributed.add((cls, hit.group(2)))
        else:
            by_name.add(hit.group(2))
    for hit in re.finditer(r"\b(C[A-Za-z_]\w*)\s*::\s*([A-Za-z_]\w*)", text):
        if hit.group(1) in mfc:
            attributed.add((mfc[hit.group(1)], hit.group(2)))
    for hit in re.finditer(r"(?<![A-Za-z0-9_.>:])([A-Za-z_]\w*)\s*\(", text):
        by_name.add(hit.group(1))
    return attributed, by_name


def main():
    decl = declared()
    text = probe_text()
    base = bases()
    attributed, by_name = calls(text, set(decl))

    def declares(cls, method):
        seen = set()
        while cls and cls not in seen:
            seen.add(cls)
            if method in decl.get(cls, {"named": set()})["named"]:
                return cls
            cls = base.get(cls)
        return None

    reached = set()
    for cls, method in attributed:
        owner = declares(cls, method)
        if owner:
            reached.add((owner, method))

    total = exact = loose = 0
    gaps, ops = {}, 0
    for cls in sorted(decl):
        ops += len(decl[cls]["operators"])
        missing = []
        for method in sorted(decl[cls]["named"]):
            total += 1
            if (cls, method) in reached:
                exact += 1
            elif method in by_name:
                loose += 1
            else:
                missing.append(method)
        if missing:
            gaps[cls] = missing

    print("classi:                        %d" % len(decl))
    print("metodi con nome dichiarati:    %d  (piu' %d operatori/conversioni, invocati per sintassi)" % (total, ops))
    print("raggiunti, chiamata attribuita: %d  (%.1f%%)" % (exact, 100.0 * exact / total))
    print("raggiunti solo per nome:        %d" % loose)
    print("non raggiunti:                  %d" % sum(len(v) for v in gaps.values()))
    if gaps:
        print()
        for cls in sorted(gaps):
            print("  %-24s %s" % (cls, ", ".join(gaps[cls])))
    return 0 if not gaps else 1


if __name__ == "__main__":
    sys.exit(main())
