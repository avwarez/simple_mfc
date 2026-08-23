# Conformance suite: one source file (cases.cpp) compiled twice — once
# against this branch's headers, once against the real MFC/ATL — and the
# two outputs compared case by case.
#
#   smfc_probe      always built (any compiler, any platform)
#   real_mfc_probe  MSVC + the "MFC and ATL" Visual Studio component only,
#                   because that is the only place real MFC exists
#
# The Windows comparison applies renamed_symbols.txt and nothing else:
# every other difference there is a real difference. The cross-platform one
# additionally applies platform_dependent.txt.
#
# The Windows job runs the comparison in BOTH configurations. Release is
# the one whose recording travels to the POSIX job; Debug exists because
# real MFC declares CObject::AssertValid/Dump and the whole of
# CDumpContext under #ifdef _DEBUG, so a Release build cannot reach them.
# The recording is therefore named per configuration -- one file per
# config, so the Debug run cannot overwrite the Release one.
#
# With both probes present the comparison is a CTest case. With only the
# native one, there is nothing on this machine to compare against, so the
# suite degrades to "the probe builds and runs to completion" — the shape
# CI uses on Linux before it fetches the Windows recording.
#
# See ../../.github/workflows/conformance.yml.

# --- the native probe ------------------------------------------------------
add_executable(smfc_probe cases.cpp)
target_compile_definitions(smfc_probe PRIVATE SIMPLE_MFC_USE_NATIVE UNICODE _UNICODE)
target_include_directories(smfc_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(smfc_probe PRIVATE simple_mfc::simple_mfc)

# --- the real-MFC probe ----------------------------------------------------
# Opt-in rather than auto-detected. "Is real MFC available?" cannot be
# answered by probing the default include path: the ATLMFC headers only join
# it once a target declares UseOfMfc, which is exactly what CMAKE_MFC_FLAG
# below does -- so a check_include_file_cxx(afx.h) would answer "no" on a
# machine that has MFC perfectly well installed. The CI job installs the
# component and then says so explicitly.
option(SIMPLE_MFC_BUILD_REAL_MFC_PROBE
       "Also build cases.cpp against the real Visual Studio MFC/ATL" OFF)

if (SIMPLE_MFC_BUILD_REAL_MFC_PROBE AND NOT MSVC)
    message(FATAL_ERROR
        "SIMPLE_MFC_BUILD_REAL_MFC_PROBE needs MSVC plus the \"MFC and ATL\" "
        "Visual Studio component: real MFC exists nowhere else.")
endif()

if (SIMPLE_MFC_BUILD_REAL_MFC_PROBE)
    # CMAKE_MFC_FLAG is directory-scoped and read at add_executable() time,
    # so it is set immediately before the target and cleared right after.
    # Statically linked MFC (flag 1) requires the static CRT: real MFC's own
    # afx.h hard-errors ("requires MFC shared dll version") if it sees the
    # default dynamic CRT instead. Static linkage also means the runner
    # needs no MFC redistributable DLLs.
    set(CMAKE_MFC_FLAG 1)
    add_executable(real_mfc_probe cases.cpp)
    set_target_properties(real_mfc_probe PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    target_compile_definitions(real_mfc_probe PRIVATE SIMPLE_MFC_USE_REAL_MFC UNICODE _UNICODE)
    unset(CMAKE_MFC_FLAG)
else()
    message(STATUS
        "conformance: building the native probe only. The side-by-side "
        "comparison against real MFC runs on the Windows CI job.")
endif()

# --- the comparison --------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter REQUIRED)

if (SIMPLE_MFC_BUILD_REAL_MFC_PROBE)
    add_test(
        NAME conformance_vs_real_mfc
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/compare.py"
                --native $<TARGET_FILE:smfc_probe>
                --reference $<TARGET_FILE:real_mfc_probe>
                --write-reference "${CMAKE_CURRENT_BINARY_DIR}/real_mfc_$<CONFIG>.txt"
                --exclude "${CMAKE_CURRENT_SOURCE_DIR}/renamed_symbols.txt"
    )
    # Outer bound on the whole step: compare.py already gives each probe its
    # own ceiling, so reaching this one means the driver itself is stuck.
    set_tests_properties(conformance_vs_real_mfc PROPERTIES TIMEOUT 900)
else()
    # No reference on this machine. Still worth a test: a probe that crashes
    # or stops emitting halfway is a bug wherever it is found, and compare.py
    # checks the run reached its #END marker.
    add_test(
        NAME conformance_probe_runs
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/compare.py"
                --native $<TARGET_FILE:smfc_probe>
    )
    set_tests_properties(conformance_probe_runs PROPERTIES TIMEOUT 900)
endif()

# --- the cross-platform comparison ----------------------------------------
# The Windows job records real MFC's output and publishes it; the POSIX job
# downloads that recording and compares this platform's native probe against
# it. Enabled by pointing SIMPLE_MFC_REFERENCE_FILE at the downloaded file.
if (SIMPLE_MFC_REFERENCE_FILE)
    add_test(
        NAME conformance_vs_recorded_real_mfc
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/compare.py"
                --native $<TARGET_FILE:smfc_probe>
                --reference-file "${SIMPLE_MFC_REFERENCE_FILE}"
                --exclude "${CMAKE_CURRENT_SOURCE_DIR}/renamed_symbols.txt"
                --exclude "${CMAKE_CURRENT_SOURCE_DIR}/platform_dependent.txt"
    )
    set_tests_properties(conformance_vs_recorded_real_mfc PROPERTIES TIMEOUT 900)
endif()
