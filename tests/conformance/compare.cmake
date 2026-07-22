# compare.cmake — runs the two conformance probes and compares their
# output case by case. Usage:
#   cmake -DSIMPLE_EXE=<path> -DREAL_EXE=<path> -P compare.cmake
#
# Each probe prints one record per checked value, as two tab-separated
# fields: "<case name>\t<value>" (see cases.cpp's Line()). Records are
# matched BY NAME, never by position, so a divergence that changes how
# many records a section emits is reported as the one difference it is
# instead of shifting — and thus falsely failing — every record after it.
cmake_policy(SET CMP0007 NEW) # list() should not silently drop empty elements

if (NOT DEFINED SIMPLE_EXE OR NOT DEFINED REAL_EXE)
    message(FATAL_ERROR "SIMPLE_EXE and REAL_EXE must both be defined")
endif()

# Neither probe may ever hold this script open indefinitely. Both are
# expected to finish in well under a second; the generous ceiling only has
# to be short enough that a stuck probe fails the job in minutes rather
# than running until the CI runner's own multi-hour limit kills it. (The
# probes themselves also disable every Windows modal dialog that could
# block them — see SilenceWindowsDialogs() in cases.cpp — so this is the
# backstop, not the first line of defence.)
set(PROBE_TIMEOUT_SECONDS 300)

# How many differing cases to print before truncating the report. High
# enough to see a whole section's worth of related failures at once,
# bounded so a total divergence cannot bury the summary that explains it.
set(MAX_REPORTED 60)

# ---------------------------------------------------------------------
# Runs one probe and returns its stdout, or fails with a diagnosis.
# ---------------------------------------------------------------------
function(run_probe label exe out_var)
    execute_process(
        COMMAND "${exe}"
        OUTPUT_VARIABLE probe_out
        ERROR_VARIABLE probe_err
        RESULT_VARIABLE probe_rc
        TIMEOUT ${PROBE_TIMEOUT_SECONDS}
    )
    # On timeout execute_process sets RESULT_VARIABLE to a message, not a
    # number, so this has to be a string comparison — "NOT probe_rc EQUAL 0"
    # would be comparing a non-numeric value.
    if (probe_rc STREQUAL "0")
        message(STATUS "${label}: ok (stderr='${probe_err}')")
        set(${out_var} "${probe_out}" PARENT_SCOPE)
        return()
    endif()

    if (NOT probe_rc MATCHES "^-?[0-9]+$")
        message(FATAL_ERROR
            "${label} did not finish within ${PROBE_TIMEOUT_SECONDS}s (${probe_rc}).\n"
            "Output captured before the timeout:\n${probe_out}\nstderr:\n${probe_err}")
    endif()
    message(FATAL_ERROR
        "${label} exited with code ${probe_rc}:\nstdout:\n${probe_out}\nstderr:\n${probe_err}")
endfunction()

# ---------------------------------------------------------------------
# Splits a probe's stdout into records and defines, for each case name,
# a variable "<prefix>_<sanitized name>" holding its value. Returns the
# list of case names, and fails on a malformed or duplicated record.
# ---------------------------------------------------------------------
function(parse_records label text prefix out_names)
    # Semicolons would otherwise be swallowed by CMake's own list syntax
    # once the text is split on newlines.
    string(REPLACE ";" "\\;" text "${text}")
    # The probes write "\n"; the Windows CRT turns that into "\r\n" on the
    # pipe. No record can legitimately contain a bare CR — cases.cpp's
    # Escape() renders a real carriage return as the two characters \r —
    # so dropping every CR is safe and leaves the values untouched.
    string(REPLACE "\r" "" text "${text}")
    string(REGEX REPLACE "\n+$" "" text "${text}")

    string(REPLACE "\n" ";" lines "${text}")
    set(names "")
    foreach(line IN LISTS lines)
        if (line STREQUAL "")
            continue()
        endif()
        string(FIND "${line}" "\t" tab_pos)
        if (tab_pos LESS 0)
            message(FATAL_ERROR "${label}: malformed record (no tab separator): '${line}'")
        endif()
        string(SUBSTRING "${line}" 0 ${tab_pos} name)
        math(EXPR value_start "${tab_pos} + 1")
        string(SUBSTRING "${line}" ${value_start} -1 value)

        # Case names are ASCII identifiers plus '.', '#' and '_'; map them
        # onto legal variable names. Any collision this could introduce is
        # caught by the duplicate check right below.
        string(REGEX REPLACE "[^A-Za-z0-9_]" "_" key "${name}")
        if (DEFINED ${prefix}_${key})
            message(FATAL_ERROR
                "${label}: duplicate case name '${name}'. Case names are the "
                "comparison key and must be unique, or one case silently "
                "hides another.")
        endif()
        # Set twice on purpose: PARENT_SCOPE alone publishes the value to
        # the caller but leaves it undefined *here*, which would make the
        # DEFINED check above never fire.
        set(${prefix}_${key} "${value}")
        set(${prefix}_${key} "${value}" PARENT_SCOPE)
        list(APPEND names "${name}")
    endforeach()
    set(${out_names} "${names}" PARENT_SCOPE)
endfunction()

run_probe("simple_mfc_probe" "${SIMPLE_EXE}" simple_out)
run_probe("real_mfc_probe" "${REAL_EXE}" real_out)

parse_records("simple_mfc_probe" "${simple_out}" SIMPLE simple_names)
parse_records("real_mfc_probe" "${real_out}" REAL real_names)

list(LENGTH simple_names n_simple)
list(LENGTH real_names n_real)

# ---------------------------------------------------------------------
# Compare, by name.
# ---------------------------------------------------------------------
set(mismatches "")
set(only_simple "")
set(only_real "")

foreach(name IN LISTS simple_names)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" key "${name}")
    if (NOT DEFINED REAL_${key})
        list(APPEND only_simple "${name}")
    elseif (NOT SIMPLE_${key} STREQUAL "${REAL_${key}}")
        list(APPEND mismatches "${name}")
    endif()
endforeach()

foreach(name IN LISTS real_names)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" key "${name}")
    if (NOT DEFINED SIMPLE_${key})
        list(APPEND only_real "${name}")
    endif()
endforeach()

list(LENGTH mismatches n_mismatch)
list(LENGTH only_simple n_only_simple)
list(LENGTH only_real n_only_real)

if (n_mismatch EQUAL 0 AND n_only_simple EQUAL 0 AND n_only_real EQUAL 0)
    message(STATUS "OK — ${n_simple} cases, every one identical between simple_mfc and real MFC.")
    return()
endif()

# ---------------------------------------------------------------------
# Report. The summary comes first so it survives any truncation below.
# ---------------------------------------------------------------------
set(report "")
set(shown 0)

foreach(name IN LISTS mismatches)
    if (shown GREATER_EQUAL MAX_REPORTED)
        break()
    endif()
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" key "${name}")
    string(APPEND report
        "  DIFF ${name}\n"
        "       simple_mfc: ${SIMPLE_${key}}\n"
        "       real MFC:   ${REAL_${key}}\n")
    math(EXPR shown "${shown} + 1")
endforeach()
if (n_mismatch GREATER shown)
    math(EXPR hidden "${n_mismatch} - ${shown}")
    string(APPEND report "  ... and ${hidden} more differing case(s)\n")
endif()

foreach(name IN LISTS only_simple)
    string(APPEND report "  ONLY IN simple_mfc (real MFC never emitted it): ${name}\n")
endforeach()
foreach(name IN LISTS only_real)
    string(APPEND report "  ONLY IN real MFC (simple_mfc never emitted it): ${name}\n")
endforeach()

message(FATAL_ERROR
    "Conformance mismatch: ${n_mismatch} differing case(s), "
    "${n_only_simple} only in simple_mfc, ${n_only_real} only in real MFC "
    "(simple_mfc emitted ${n_simple} cases, real MFC ${n_real}).\n${report}")
