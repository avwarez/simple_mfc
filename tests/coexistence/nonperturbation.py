#!/usr/bin/env python3
"""Did simple_mfc's presence change what real MFC means in this TU?

Runs two builds of coexist.cpp -- the baseline (real MFC alone) and one
that also has simple_mfc's headers in it -- and compares the `mfc.` half of
their output. That half is the SAME source lines asking the same questions
of the same library, so anything but an exact match is simple_mfc reaching
out of its own headers: a macro it undefined, a name it took, a member that
ended up compiled under a different name.

Unlike the conformance comparison there is no "skipped" outcome here. A
record present on one side and absent on the other is a failure like any
other: both binaries run the same code over the same records.
"""

import argparse
import subprocess
import sys

PROBE_TIMEOUT_SECONDS = 300
PREFIX = "mfc."


def run(path):
    try:
        proc = subprocess.run([path], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              timeout=PROBE_TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired:
        sys.exit("FATAL: %s did not finish within %d s" % (path, PROBE_TIMEOUT_SECONDS))
    if proc.returncode != 0:
        sys.stderr.write(proc.stderr.decode("utf-8", "replace"))
        sys.exit("FATAL: %s exited with code %d" % (path, proc.returncode))
    return proc.stdout.decode("utf-8", "replace")


def parse(text, origin):
    records = {}
    for lineno, line in enumerate(text.splitlines(), 1):
        if not line:
            continue
        name, sep, value = line.partition("\t")
        if not sep:
            sys.exit("FATAL: %s line %d is not a <name>TAB<value> record: %r"
                     % (origin, lineno, line))
        if name in records:
            sys.exit("FATAL: %s emits %r twice" % (origin, name))
        records[name] = value
    if "#END" not in records:
        sys.exit("FATAL: %s never reached its #END marker (%d records)"
                 % (origin, len(records)))
    return {n: v for n, v in records.items() if n.startswith(PREFIX)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline", required=True,
                        help="the probe built against real MFC alone")
    parser.add_argument("--coexisting", required=True,
                        help="the probe that also has simple_mfc in the TU")
    parser.add_argument("--write-baseline",
                        help="save the baseline's output here")
    args = parser.parse_args()

    baseline_text = run(args.baseline)
    if args.write_baseline:
        with open(args.write_baseline, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(baseline_text)
    baseline = parse(baseline_text, "the baseline probe")
    coexisting = parse(run(args.coexisting), "the coexisting probe")

    if not baseline:
        sys.exit("FATAL: the baseline emitted no %s records at all" % PREFIX)

    different = sorted(n for n in baseline
                       if n in coexisting and baseline[n] != coexisting[n])
    only_baseline = sorted(set(baseline) - set(coexisting))
    only_coexisting = sorted(set(coexisting) - set(baseline))

    if different:
        print("PERTURBED (%d) -- real MFC answers differently with simple_mfc "
              "in the same translation unit:" % len(different))
        for name in different:
            print("    %s" % name)
            print("        real MFC alone      : %s" % baseline[name])
            print("        alongside simple_mfc: %s" % coexisting[name])
    for label, names in (("only in the baseline", only_baseline),
                         ("only alongside simple_mfc", only_coexisting)):
        if names:
            print("\nRECORDS %s (%d):" % (label, len(names)))
            for name in names:
                print("    %s" % name)

    failed = bool(different or only_baseline or only_coexisting)
    print("\n%d %s records compared, %d perturbed"
          % (len(baseline), PREFIX, len(different)))
    if failed:
        return 1
    print("INERT")
    return 0


if __name__ == "__main__":
    sys.exit(main())
