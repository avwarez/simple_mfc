# golden.cmake — run ONE conformance probe and compare its output against a
# recorded golden file, which is the output real MFC produced on Windows.
#
# WHY THIS EXISTS
# ---------------
# compare.cmake runs the two probes side by side, but both of them are MSVC
# builds: it answers "does simple_mfc-on-MSVC match real-MFC-on-MSVC?".
# That leaves the build we actually ship — the POSIX one, which is what the
# Qt frontend runs on — compared against nothing at all.
#
# The gap is not theoretical. A CString::Format bug (MSVC and the C standard
# disagree on what %s means in a wide format string, so every wide string
# came out truncated to its first character) lived only in the POSIX code
# path, because on MSVC that path is #ifdef'd out. Format already had six
# conformance cases and they all passed, on both probes, while the shipped
# build was broken. No amount of extra cases on the Windows pair could have
# caught it; only comparing the POSIX build against real MFC can.
#
# Real MFC exists only on Windows, so the comparison is split in time: the
# Windows job records real MFC's output into golden/real_mfc_win32.txt (and
# re-checks that the recording is still accurate on every run), and this
# script replays that recording against the probe built anywhere else.
#
# PLATFORM-DEPENDENT CASES
# ------------------------
# A handful of cases legitimately differ between the two platforms because
# the VALUE ITSELF is a platform fact, not a behaviour: a temporary path is
# C:\Users\...\Temp\ on Windows and /tmp/ here. Those case names are listed
# in golden/platform_dependent.txt and are skipped — but skipped LOUDLY:
# a listed case that is missing from either side is an error, so the list
# cannot silently rot into a way of hiding real divergence.
cmake_minimum_required(VERSION 3.16)

if (NOT DEFINED PROBE_EXE OR NOT DEFINED GOLDEN_FILE OR NOT DEFINED EXCLUDE_FILE)
    message(FATAL_ERROR "PROBE_EXE, GOLDEN_FILE and EXCLUDE_FILE must all be defined")
endif()

set(MAX_REPORTED 60)

if (NOT EXISTS "${GOLDEN_FILE}")
    message(FATAL_ERROR
        "No golden file at ${GOLDEN_FILE}.\n"
        "It is produced by the 'conformance' job on windows-latest, which is "
        "the only place real MFC exists; download its real-mfc-golden artifact "
        "and commit it. See .github/workflows/msvc.yml.")
endif()

# ---------------------------------------------------------------------
# Run the probe. Same TZ pinning as the recording (see below).
# ---------------------------------------------------------------------
execute_process(
    COMMAND ${CMAKE_COMMAND} -E env TZ=UTC0 "${PROBE_EXE}"
    OUTPUT_VARIABLE probe_out
    ERROR_VARIABLE  probe_err
    RESULT_VARIABLE probe_rc
    TIMEOUT 300
)
if (NOT probe_rc EQUAL 0)
    message(FATAL_ERROR "probe failed (exit ${probe_rc}): ${probe_err}")
endif()

# ---------------------------------------------------------------------
# Parse "<name>\t<value>" records into name -> value.
# Mirrors compare.cmake: strip CR (the CRT turns \n into \r\n on the pipe
# on the recording side) and escape ';' so CMake list syntax cannot eat a
# value that happens to contain one.
# ---------------------------------------------------------------------
#
# The records are walked with string(FIND) over the raw text and NEVER
# turned into a CMake list. Splitting on "\n" and iterating the result
# looks equivalent and is not: the moment CMake reads a string as a list it
# applies list-escaping rules to the contents, and the values here include
# fuzzed Unicode. In practice that silently glued the last 20 records into
# one element, so those cases were parsed as a single malformed blob and
# never compared at all. Byte offsets are safe because the only positions
# ever cut at are ASCII \n and \t, which cannot occur inside a UTF-8
# multi-byte sequence.
function(parse_records label text out_names prefix)
    string(REPLACE "\r" "" text "${text}")
    set(names "")
    string(LENGTH "${text}" text_len)
    set(pos 0)
    while(pos LESS text_len)
        string(SUBSTRING "${text}" ${pos} -1 rest)
        string(FIND "${rest}" "\n" nl)
        if (nl EQUAL -1)
            set(line "${rest}")
            set(pos ${text_len})
        else()
            string(SUBSTRING "${rest}" 0 ${nl} line)
            math(EXPR pos "${pos} + ${nl} + 1")
        endif()
        if (line STREQUAL "")
            continue()
        endif()
        string(FIND "${line}" "\t" tab)
        if (tab EQUAL -1)
            message(FATAL_ERROR "${label}: malformed record (no tab): '${line}'")
        endif()
        string(SUBSTRING "${line}" 0 ${tab} name)
        math(EXPR vstart "${tab} + 1")
        string(SUBSTRING "${line}" ${vstart} -1 value)
        string(REGEX REPLACE "[^A-Za-z0-9_]" "_" key "${name}")
        if (DEFINED ${prefix}_${key})
            message(FATAL_ERROR "${label}: duplicate case name '${name}'")
        endif()
        # Set in BOTH scopes: PARENT_SCOPE alone publishes the value to the
        # caller but leaves it undefined here, which would make the DEFINED
        # check above never fire and let a duplicate name through silently.
        set(${prefix}_${key} "${value}")
        set(${prefix}_${key} "${value}" PARENT_SCOPE)
        list(APPEND names "${name}")
    endwhile()
    set(${out_names} "${names}" PARENT_SCOPE)
endfunction()

file(READ "${GOLDEN_FILE}" golden_text)

parse_records("probe"  "${probe_out}"  probe_names  PROBE)
parse_records("golden" "${golden_text}" golden_names GOLDEN)

# ---------------------------------------------------------------------
# The platform-dependent skip list.
# ---------------------------------------------------------------------
# file(READ) + split by hand, NOT file(STRINGS): file(STRINGS) extracts
# printable ASCII runs and treats any other byte as a separator, so a single
# non-ASCII character anywhere in a comment silently splits that comment into
# fragments, each of which then reads as a case name. It turned the prose in
# this list's own header into half a dozen phantom entries.
file(READ "${EXCLUDE_FILE}" exclude_text)
string(REPLACE "\r" "" exclude_text "${exclude_text}")
string(REPLACE ";" "\\;" exclude_text "${exclude_text}")
string(REPLACE "\n" ";" exclude_lines "${exclude_text}")
set(excluded "")
foreach(line IN LISTS exclude_lines)
    string(STRIP "${line}" line)
    if (line STREQUAL "" OR line MATCHES "^#")
        continue()
    endif()
    list(APPEND excluded "${line}")
endforeach()

# ---------------------------------------------------------------------
# Compare by name.
# ---------------------------------------------------------------------
set(mismatches "")
set(only_probe "")
set(only_golden "")
set(stale_excludes "")

foreach(name IN LISTS probe_names)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" key "${name}")
    if (NOT DEFINED GOLDEN_${key})
        list(APPEND only_probe "${name}")
    elseif (NOT "${PROBE_${key}}" STREQUAL "${GOLDEN_${key}}")
        if (NOT name IN_LIST excluded)
            list(APPEND mismatches "${name}")
        endif()
    endif()
endforeach()

foreach(name IN LISTS golden_names)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" key "${name}")
    if (NOT DEFINED PROBE_${key})
        list(APPEND only_golden "${name}")
    endif()
endforeach()

# An excluded case that no longer exists on one of the two sides means the
# skip list is describing something that is gone. Left unchecked, the list
# is the one place where a real divergence could hide forever.
foreach(name IN LISTS excluded)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" key "${name}")
    if (NOT DEFINED PROBE_${key} OR NOT DEFINED GOLDEN_${key})
        list(APPEND stale_excludes "${name}")
    endif()
endforeach()

list(LENGTH probe_names   n_probe)
list(LENGTH excluded      n_excluded)
list(LENGTH mismatches    n_mismatch)
list(LENGTH only_probe    n_only_probe)
list(LENGTH only_golden   n_only_golden)
list(LENGTH stale_excludes n_stale)

if (n_mismatch EQUAL 0 AND n_only_probe EQUAL 0 AND n_only_golden EQUAL 0
    AND n_stale EQUAL 0)
    math(EXPR n_compared "${n_probe} - ${n_excluded}")
    message(STATUS
        "OK — ${n_compared} cases identical to real MFC's recorded output "
        "(${n_excluded} platform-dependent case(s) skipped).")
    return()
endif()

set(report "")
set(shown 0)
foreach(name IN LISTS mismatches)
    if (shown GREATER_EQUAL MAX_REPORTED)
        break()
    endif()
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" key "${name}")
    string(APPEND report
        "  DIFF ${name}\n"
        "       this build: ${PROBE_${key}}\n"
        "       real MFC:   ${GOLDEN_${key}}\n")
    math(EXPR shown "${shown} + 1")
endforeach()
if (n_mismatch GREATER shown)
    math(EXPR hidden "${n_mismatch} - ${shown}")
    string(APPEND report "  ... and ${hidden} more differing case(s)\n")
endif()
foreach(name IN LISTS only_probe)
    string(APPEND report "  ONLY IN this build (not in the golden): ${name}\n")
endforeach()
foreach(name IN LISTS only_golden)
    string(APPEND report "  ONLY IN the golden (this build never emitted it): ${name}\n")
endforeach()
foreach(name IN LISTS stale_excludes)
    string(APPEND report
        "  STALE EXCLUSION (listed as platform-dependent, but absent): ${name}\n")
endforeach()

message(FATAL_ERROR
    "Conformance mismatch against real MFC's recorded output: "
    "${n_mismatch} differing, ${n_only_probe} only here, "
    "${n_only_golden} only in the golden, ${n_stale} stale exclusion(s).\n"
    "${report}")
