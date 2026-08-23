#!/usr/bin/env python3
"""Run the conformance probes and compare their output case by case.

Each probe prints one record per checked value, as two tab-separated
fields:

    <case name>\t<escaped value>

and finishes with a "#END\t<count>" marker. Records are matched BY NAME,
never by position, so a case that only one side emits is reported as
exactly that instead of shifting every record after it.

Three outcomes per case:

  DIFFERENT  both sides emitted it, with different values -- a
             conformance bug, and the reason this suite exists.
  MISSING    the reference (real MFC) emitted it and the native probe did
             not. This is a SKIP, not a failure: this branch carries only
             part of the MFC surface, and a method it does not implement
             has no case to run. Reported, never fatal.
  EXTRA      the native probe emitted a case the reference did not. This
             IS a failure: both probes compile the same cases.cpp, so it
             means the reference run stopped early or diverged in control
             flow.

Usage:
  compare.py --native PROBE                       # run only, check it completes
  compare.py --native PROBE --reference PROBE     # run both, compare
  compare.py --native PROBE --reference-file FILE # compare against a recording
"""

import argparse
import subprocess
import sys

PROBE_TIMEOUT_SECONDS = 300


def run_probe(path, save_to=None):
    """Run a probe and return its stdout. Never waits indefinitely.

    Whatever the probe managed to print is saved (if asked) BEFORE any
    failure is reported: a probe that died partway through is exactly when
    its partial output is worth reading.
    """
    try:
        proc = subprocess.run([path], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              timeout=PROBE_TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired as expired:
        if save_to and expired.stdout:
            save(save_to, expired.stdout.decode("utf-8", "replace"))
        sys.exit("FATAL: %s did not finish within %d s" % (path, PROBE_TIMEOUT_SECONDS))
    text = proc.stdout.decode("utf-8", "replace")
    if save_to:
        save(save_to, text)
    if proc.returncode != 0:
        sys.stderr.write(proc.stderr.decode("utf-8", "replace"))
        sys.exit("FATAL: %s exited with code %d" % (path, proc.returncode))
    return text


def save(path, text):
    with open(path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(text)


def parse(text, origin):
    """Parse probe output into an ordered {name: value} mapping."""
    records = {}
    order = []
    for lineno, line in enumerate(text.splitlines(), 1):
        if not line:
            continue
        name, sep, value = line.partition("\t")
        if not sep:
            sys.exit("FATAL: %s line %d is not a <name>TAB<value> record: %r"
                     % (origin, lineno, line))
        if name in records:
            sys.exit("FATAL: %s emits the case %r twice; case names are the key "
                     "this suite matches on and must be unique" % (origin, name))
        records[name] = value
        order.append(name)
    if "#END" not in records:
        sys.exit("FATAL: %s never reached its #END marker -- the run stopped "
                 "partway through (%d cases emitted)" % (origin, len(records)))
    if records["#END"] != str(len(records) - 1):
        sys.exit("FATAL: %s emitted %d cases but its #END marker says %s"
                 % (origin, len(records) - 1, records["#END"]))
    return records, order


def load_exclusions(paths):
    """Case names not to compare, from every --exclude file given."""
    names = set()
    for path in paths or []:
        with open(path, encoding="utf-8") as handle:
            for line in handle:
                line = line.strip()
                if line and not line.startswith("#"):
                    names.add(line)
    return names


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--native", required=True,
                        help="the simple_mfc probe executable")
    parser.add_argument("--reference",
                        help="the real-MFC probe executable, run side by side")
    parser.add_argument("--reference-file",
                        help="a recording of the real-MFC probe's output")
    parser.add_argument("--exclude", action="append",
                        help="file listing case names not to compare (repeatable)")
    parser.add_argument("--write-reference",
                        help="save the reference probe's output here")
    args = parser.parse_args()

    native_text = run_probe(args.native)
    native, native_order = parse(native_text, "the native probe")

    if args.reference:
        reference_text = run_probe(args.reference, args.write_reference)
        origin = "the real-MFC probe"
    elif args.reference_file:
        with open(args.reference_file, encoding="utf-8") as handle:
            reference_text = handle.read()
        origin = "the recording %s" % args.reference_file
    else:
        print("native probe: %d cases, run complete (no reference to compare "
              "against on this machine)" % (len(native) - 1))
        return 0

    reference, _ = parse(reference_text, origin)
    excluded = load_exclusions(args.exclude)

    different, missing, extra, compared = [], [], [], 0
    for name in native_order:
        if name == "#END":
            continue
        if name not in reference:
            extra.append(name)
        elif name in excluded:
            pass
        else:
            compared += 1
            if native[name] != reference[name]:
                different.append(name)
    for name in reference:
        if name != "#END" and name not in native:
            missing.append(name)

    # An exclusion naming a case that no longer exists on either side is a
    # stale claim about a divergence: fail rather than let the list outlive
    # what it describes.
    stale = sorted(name for name in excluded
                   if name not in native and name not in reference)

    if missing:
        print("SKIPPED -- in real MFC, not implemented on this branch (%d):" % len(missing))
        for name in sorted(missing):
            print("    %s" % name)
    if excluded:
        print("EXCLUDED -- platform facts, not behaviour (%d)" % len(excluded))

    failed = False
    if different:
        failed = True
        print("\nDIFFERENT (%d):" % len(different))
        for name in different:
            print("    %s" % name)
            print("        this branch: %s" % native[name])
            print("        real MFC   : %s" % reference[name])
    if extra:
        failed = True
        print("\nEXTRA -- emitted here but not by the reference (%d):" % len(extra))
        for name in extra:
            print("    %s" % name)
    if stale:
        failed = True
        print("\nSTALE EXCLUSIONS -- naming cases that no longer exist (%d):" % len(stale))
        for name in stale:
            print("    %s" % name)

    print("\n%d cases compared, %d different, %d skipped, %d excluded"
          % (compared, len(different), len(missing), len(excluded)))
    if failed:
        return 1
    print("CONFORMANT")
    return 0


if __name__ == "__main__":
    sys.exit(main())
