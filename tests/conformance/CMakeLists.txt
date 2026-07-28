# Conformance tests: the exact same test cases (cases.cpp, shared verbatim)
# run against simple_mfc's native implementation and against the real
# Visual Studio MFC, then compared byte-for-byte by compare.cmake. Any
# behavioral or result difference between the two shows up as a failing
# CTest case.
#
# Requires MSVC and the "MFC and ATL" Visual Studio component — see
# ../../.github/workflows/msvc.yml for how CI installs it. Not available
# on any other compiler/platform, since real MFC only exists on Windows.
if (NOT MSVC)
    message(FATAL_ERROR "Conformance tests require MSVC plus the real MFC libraries.")
endif()

# --- simple_mfc probe: our native implementation ---------------------------
add_executable(simple_mfc_probe cases.cpp)
target_compile_definitions(simple_mfc_probe PRIVATE SIMPLE_MFC_USE_NATIVE UNICODE _UNICODE)
target_link_libraries(simple_mfc_probe PRIVATE simple_mfc::simple_mfc)

# --- real_mfc probe: the actual Visual Studio MFC, statically linked so no
# redistributable DLLs are needed on the CI runner. CMAKE_MFC_FLAG is a
# directory-scoped variable read by the Visual Studio generator at
# add_executable() time, so it must be set immediately before this target
# and cleared right after, to avoid affecting simple_mfc_probe above/below.
# Statically-linked MFC (CMAKE_MFC_FLAG 1) requires the static CRT
# (/MT[d]): real MFC's own afx.h hard-errors ("requires MFC shared dll
# version") if it sees the default dynamic CRT (/MD[d]) instead. This only
# affects real_mfc_probe — simple_mfc_probe keeps CMake's default dynamic
# CRT, matching how the simple_mfc library itself is built, since the two
# executables share no binary linkage.
set(CMAKE_MFC_FLAG 1)
add_executable(real_mfc_probe cases.cpp)
set_target_properties(real_mfc_probe PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
target_compile_definitions(real_mfc_probe PRIVATE SIMPLE_MFC_USE_REAL_MFC UNICODE _UNICODE)

# --- real_mfc GDI probe ----------------------------------------------------
# gdi_cases.cpp has no simple_mfc counterpart HERE: our CDC is implemented in
# gui/qt, so the only build that has one is the Qt build, which does not exist
# on Windows. This target's whole job is therefore to RECORD what real MFC's
# GDI does, for the Qt build to be compared against later. There is no
# side-by-side comparison to run on this machine.
add_executable(real_mfc_gdi_probe gdi_cases.cpp)
set_target_properties(real_mfc_gdi_probe PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
target_compile_definitions(real_mfc_gdi_probe PRIVATE SIMPLE_MFC_USE_REAL_MFC UNICODE _UNICODE)
unset(CMAKE_MFC_FLAG)

add_test(
    NAME conformance_vs_real_mfc
    COMMAND ${CMAKE_COMMAND}
            "-DSIMPLE_EXE=$<TARGET_FILE:simple_mfc_probe>"
            "-DREAL_EXE=$<TARGET_FILE:real_mfc_probe>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/compare.cmake"
)
# Outer bound on the whole compare step. compare.cmake already gives each
# probe its own 300s ceiling, so reaching this one means the driving script
# itself is stuck, not a probe. Either way the job fails in minutes instead
# of holding a CI runner for hours.
set_tests_properties(conformance_vs_real_mfc PROPERTIES TIMEOUT 900)

# --- The golden recording stays accurate -----------------------------------
# golden/real_mfc_win32.txt is real MFC's output, committed so that builds on
# platforms where real MFC does not exist (i.e. the POSIX build we ship) can
# be compared against it. A recording is only worth as much as its currency:
# this re-derives it from real MFC on every Windows run and fails if the two
# have drifted, so the file can never quietly become fiction.
#
# The same platform_dependent.txt exclusions apply here, because the runner's
# own temp directory is not guaranteed stable between runs either — and
# running the list against both sides is what keeps it honest.
set(GOLDEN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/golden/real_mfc_win32.txt")
if (EXISTS "${GOLDEN_FILE}")
    add_test(
        NAME golden_recording_is_current
        COMMAND ${CMAKE_COMMAND}
                "-DPROBE_EXE=$<TARGET_FILE:real_mfc_probe>"
                "-DGOLDEN_FILE=${GOLDEN_FILE}"
                "-DEXCLUDE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/golden/platform_dependent.txt"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/golden.cmake"
    )
    set_tests_properties(golden_recording_is_current PROPERTIES TIMEOUT 900)
else()
    message(STATUS
        "golden_recording_is_current: skipped, no recording yet — the CI job "
        "uploads real_mfc_probe's output as the 'real-mfc-golden' artifact; "
        "commit it to golden/real_mfc_win32.txt to enable POSIX conformance.")
endif()

# Same currency check for the GDI recording.
set(GDI_GOLDEN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/golden/real_mfc_gdi_win32.txt")
if (EXISTS "${GDI_GOLDEN_FILE}")
    add_test(
        NAME gdi_golden_recording_is_current
        COMMAND ${CMAKE_COMMAND}
                "-DPROBE_EXE=$<TARGET_FILE:real_mfc_gdi_probe>"
                "-DGOLDEN_FILE=${GDI_GOLDEN_FILE}"
                "-DEXCLUDE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/golden/gdi_platform_dependent.txt"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/golden.cmake"
    )
    set_tests_properties(gdi_golden_recording_is_current PROPERTIES TIMEOUT 900)
else()
    message(STATUS
        "gdi_golden_recording_is_current: skipped, no recording yet — the CI "
        "job uploads real_mfc_gdi_probe's output as the 'real-mfc-gdi-golden' "
        "artifact; commit it to golden/real_mfc_gdi_win32.txt.")
endif()
