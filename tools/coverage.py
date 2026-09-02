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


OPERATOR = re.compile(r"\boperator\s*(\[\s*\]|\(\s*\)|[^\s\w(]+|[\w:\s\*&]+?)\s*\(")


def member_name(statement):
    """The declared name in a member-function declaration, or None.

    Operators are matched first and by their own rule: the angle-bracket
    tracking used for the rest reads the `<` of `operator<` as the start of
    a template argument list and never finds the parameter list, which is
    how eight comparison operators on CTime/CTimeSpan went uncounted.
    """
    op = OPERATOR.search(statement)
    if op:
        return "operator" + re.sub(r"\s+", "", op.group(1))

    depth = 0
    for i, ch in enumerate(statement):
        if ch in "<[":
            depth += 1
        elif ch in ">]":
            depth -= 1
        elif ch == "(" and depth == 0:
            head = statement[:i].strip()
            hit = re.search(r"([A-Za-z_]\w*)$", head)
            return hit.group(1) if hit else None
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


def probe_text(raw=False):
    """The probe sources. `raw` keeps the string literals: the case names
    live in them, and stripping first left the operator scan nothing to
    read."""
    text = ""
    for path in PROBES:
        if os.path.exists(path):
            body = open(path, encoding="utf-8").read()
            text += body if raw else strip(body)
    return text


def balanced(text, i):
    """Index just past the parenthesis run starting at text[i] == '('."""
    depth = 0
    while i < len(text):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return -1


def calls(text, classes):
    """(class, method) pairs the probes make, plus names seen unattributed.

    Three shapes are attributed to a class; everything else falls back to the
    bare name and is reported apart, never counted as covered:

      var.Method(...)        var typed from its declaration. A name reused in
                             two tests with two types keeps BOTH candidates --
                             taking the last one seen silently mis-attributed
                             most container calls.
      CClass(...).Method()   call on a temporary. Missing this shape reported
                             CompareNoCase as never exercised while the suite
                             had been comparing it against MFC all along.
      CClass::Method(...)    static or qualified call.
    """
    mfc = {c[1:]: c for c in classes}
    var_type = {}
    for hit in re.finditer(r"\b(C[A-Za-z_]\w*)\s*(?:<[^;{}()]*>)?\s+(\*\s*)?([A-Za-z_]\w*)\s*[;=({\[]", text):
        if hit.group(1) in mfc:
            var_type.setdefault(hit.group(3), set()).add(mfc[hit.group(1)])

    attributed, by_name = set(), set()

    for hit in re.finditer(r"\b(C[A-Za-z_]\w*)\s*(?:<[^;{}()]*>)?\s*\(", text):
        if hit.group(1) not in mfc:
            continue
        close = balanced(text, hit.end() - 1)
        if close < 0:
            continue
        tail = re.match(r"\s*\.\s*([A-Za-z_]\w*)\s*\(", text[close:])
        if tail:
            attributed.add((mfc[hit.group(1)], tail.group(1)))

    for hit in re.finditer(r"\b([A-Za-z_]\w*)\s*(?:\.|->)\s*([A-Za-z_]\w*)\s*\(", text):
        candidates = var_type.get(hit.group(1))
        if candidates:
            for cls in candidates:
                attributed.add((cls, hit.group(2)))
        else:
            by_name.add(hit.group(2))

    for hit in re.finditer(r"[)\]]\s*(?:\.|->)\s*([A-Za-z_]\w*)\s*\(", text):
        by_name.add(hit.group(1))

    for hit in re.finditer(r"\b(C[A-Za-z_]\w*)\s*::\s*([A-Za-z_]\w*)", text):
        if hit.group(1) in mfc:
            attributed.add((mfc[hit.group(1)], hit.group(2)))

    for hit in re.finditer(r"(?<![A-Za-z0-9_.>:])([A-Za-z_]\w*)\s*\(", text):
        by_name.add(hit.group(1))

    return attributed, by_name


def aliases(classes):
    """{name used in the probes: E-prefixed class that declares the members}

    The probes say CString; the declaration is ECStringT, two `using` hops
    away. Without following them, no operator case could ever be matched to
    the class it exercises.
    """
    links = {}
    sources = [os.path.join(HEADERS, f) for f in os.listdir(HEADERS) if f.endswith(".h")]
    sources.append(os.path.join(ROOT, "tests", "conformance", "mfc_names.h"))
    for path in sources:
        if not os.path.exists(path):
            continue
        text = strip(open(path, encoding="utf-8").read())
        for hit in re.finditer(r"\busing\s+([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*)", text):
            links[hit.group(1)] = hit.group(2)
        for hit in re.finditer(r"^\s*#\s*define\s+([A-Za-z_]\w*)\s+([A-Za-z_]\w*)\s*$", text, re.M):
            links[hit.group(1)] = hit.group(2)

    out = {}
    for name in list(links) + list(classes):
        seen, cur = set(), name
        while cur in links and cur not in seen:
            seen.add(cur)
            cur = links[cur]
        if cur in classes:
            out[name] = cur
    return out


def operator_cases(text):
    """{(probe class name, token)} named by the case-name convention
    <Class>.operator<token>[.detail], read from the case-name literals."""
    out = set()
    for hit in re.finditer(r'"([A-Za-z_]\w*)\.operator([^"]*?)(?:\.[A-Za-z_0-9]+)*"', text):
        out.add((hit.group(1), re.sub(r"\s+", "", hit.group(2))))
    return out


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

    alias = aliases(set(decl))
    named_ops = operator_cases(probe_text(raw=True))
    reached_ops = set()
    for probe_name, token in named_ops:
        owner = alias.get(probe_name)
        if owner:
            reached_ops.add((owner, "operator" + token))

    total = exact = loose = 0
    gaps, ops, op_gaps, op_total = {}, 0, {}, 0
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
        missing_ops = []
        for op in sorted(decl[cls]["operators"]):
            op_total += 1
            owner = cls
            while owner and (owner, op) not in reached_ops:
                owner = base.get(owner)
            if not owner:
                missing_ops.append(op[8:] or "()")
        if missing_ops:
            op_gaps[cls] = missing_ops

    print("classi:                        %d" % len(decl))
    print("metodi con nome dichiarati:    %d  (piu' %d operatori/conversioni, invocati per sintassi)" % (total, ops))
    print("raggiunti, chiamata attribuita: %d  (%.1f%%)" % (exact, 100.0 * exact / total))
    print("raggiunti solo per nome:        %d" % loose)
    print("non raggiunti:                  %d" % sum(len(v) for v in gaps.values()))
    print("operatori con un caso proprio:  %d su %d" % (op_total - sum(len(v) for v in op_gaps.values()), op_total))
    if gaps:
        print("\nmetodi non raggiunti:")
        for cls in sorted(gaps):
            print("  %-24s %s" % (cls, ", ".join(gaps[cls])))
    if op_gaps:
        print("\noperatori senza un caso che li nomini:")
        for cls in sorted(op_gaps):
            print("  %-24s %s" % (cls, " ".join(op_gaps[cls])))
    return 0 if (gaps or op_gaps) else 1


if __name__ == "__main__":
    sys.exit(main())
