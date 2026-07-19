# compare.cmake — runs the two conformance probes and diffs their output
# byte-for-byte. Usage:
#   cmake -DSIMPLE_EXE=<path> -DREAL_EXE=<path> -P compare.cmake
cmake_policy(SET CMP0007 NEW) # list() should not silently drop empty elements

if (NOT DEFINED SIMPLE_EXE OR NOT DEFINED REAL_EXE)
    message(FATAL_ERROR "SIMPLE_EXE and REAL_EXE must both be defined")
endif()

execute_process(
    COMMAND "${SIMPLE_EXE}"
    OUTPUT_VARIABLE simple_out
    ERROR_VARIABLE simple_err
    RESULT_VARIABLE simple_rc
)
message(STATUS "simple_mfc_probe: result='${simple_rc}' stderr='${simple_err}'")
if (NOT simple_rc EQUAL 0)
    message(FATAL_ERROR "simple_mfc_probe exited with code ${simple_rc}:\nstdout:\n${simple_out}\nstderr:\n${simple_err}")
endif()

execute_process(
    COMMAND "${REAL_EXE}"
    OUTPUT_VARIABLE real_out
    ERROR_VARIABLE real_err
    RESULT_VARIABLE real_rc
)
message(STATUS "real_mfc_probe: result='${real_rc}' stderr='${real_err}'")
if (NOT real_rc EQUAL 0)
    message(FATAL_ERROR "real_mfc_probe exited with code ${real_rc}:\nstdout:\n${real_out}\nstderr:\n${real_err}")
endif()

if (simple_out STREQUAL real_out)
    message(STATUS "OK — simple_mfc output matches real MFC output byte-for-byte:")
    message(STATUS "${simple_out}")
    return()
endif()

# Outputs differ: report the first mismatching line for a readable diff
# instead of dumping both full outputs. Strip a trailing newline first so
# it doesn't turn into a spurious empty trailing list element.
string(REGEX REPLACE "\n+$" "" simple_trimmed "${simple_out}")
string(REGEX REPLACE "\n+$" "" real_trimmed "${real_out}")
string(REPLACE "\n" ";" simple_lines "${simple_trimmed}")
string(REPLACE "\n" ";" real_lines "${real_trimmed}")
list(LENGTH simple_lines n_simple)
list(LENGTH real_lines n_real)

set(n_max ${n_simple})
if (n_real GREATER n_max)
    set(n_max ${n_real})
endif()
if (n_max GREATER 0)
    math(EXPR n_max "${n_max} - 1")
    foreach(i RANGE ${n_max})
        set(sl "<no line>")
        set(rl "<no line>")
        if (i LESS n_simple)
            list(GET simple_lines ${i} sl)
        endif()
        if (i LESS n_real)
            list(GET real_lines ${i} rl)
        endif()
        if (NOT sl STREQUAL rl)
            message(FATAL_ERROR "Conformance mismatch at line ${i}:\n  simple_mfc: ${sl}\n  real MFC:   ${rl}")
        endif()
    endforeach()
endif()

message(FATAL_ERROR "Outputs differ (simple_mfc: ${n_simple} lines, real MFC: ${n_real} lines) but no single differing line was found — likely a trailing-line count mismatch.")
