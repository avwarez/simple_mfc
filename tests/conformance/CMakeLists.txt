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
set(CMAKE_MFC_FLAG 1)
add_executable(real_mfc_probe cases.cpp)
target_compile_definitions(real_mfc_probe PRIVATE SIMPLE_MFC_USE_REAL_MFC UNICODE _UNICODE)
unset(CMAKE_MFC_FLAG)

add_test(
    NAME conformance_vs_real_mfc
    COMMAND ${CMAKE_COMMAND}
            "-DSIMPLE_EXE=$<TARGET_FILE:simple_mfc_probe>"
            "-DREAL_EXE=$<TARGET_FILE:real_mfc_probe>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/compare.cmake"
)
