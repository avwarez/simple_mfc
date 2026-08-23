# Coexistence suite: one translation unit holding BOTH header sets.
#
# The conformance suite compiles cases.cpp into two separate binaries, one
# per library, and compares what they print. That answers "do we behave like
# real MFC?" and cannot answer "can we be in the room with it?" -- which is
# the question eMule's migration actually asks, because it replaces one
# symbol occurrence at a time and therefore keeps both libraries in every
# file for the whole length of the migration.
#
#   coexist_mfc_only    real MFC alone -- the baseline recording
#   coexist_mfc_first   real MFC, then simple_mfc
#   coexist_smfc_first  simple_mfc, then real MFC
#
# Building the last two at all is the compile-level answer; running them and
# comparing their `mfc.` records against the baseline is the behavioural one
# (nonperturbation.py). Both include orders are built because a header that
# only works second is a header with an undocumented ordering requirement.
#
# MSVC with the "MFC and ATL" Visual Studio component only: real MFC exists
# nowhere else, so unlike the conformance suite there is no native-only
# degraded mode -- without real MFC there is no coexistence to test.
# See ../../.github/workflows/coexistence.yml.

if (NOT MSVC)
    message(FATAL_ERROR
        "SIMPLE_MFC_BUILD_COEXISTENCE needs MSVC plus the \"MFC and ATL\" "
        "Visual Studio component: real MFC exists nowhere else.")
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Real MFC links statically here (CMAKE_MFC_FLAG 1), which requires the
# static CRT. simple_mfc's own library target is built with whatever runtime
# the rest of the project uses, so its sources are compiled again here
# instead of linked -- one CRT for the whole translation unit, and the same
# flags for both halves of it.
set(SMFC_COEXIST_SOURCES)
foreach (source IN ITEMS eafx eafxcoll eafximpl eafxinet eafxmt eafxsock eafxwin
                         eatlconv eatlenc eatltime)
    list(APPEND SMFC_COEXIST_SOURCES "${PROJECT_SOURCE_DIR}/src/${source}.cpp")
endforeach()

add_library(smfc_coexist_objects OBJECT ${SMFC_COEXIST_SOURCES})
set_target_properties(smfc_coexist_objects PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_include_directories(smfc_coexist_objects PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_definitions(smfc_coexist_objects PUBLIC
    _CRT_SECURE_NO_WARNINGS NOMINMAX UNICODE _UNICODE)
target_compile_options(smfc_coexist_objects PUBLIC /EHsc)

# CMAKE_MFC_FLAG is directory-scoped and read at add_executable() time.
set(CMAKE_MFC_FLAG 1)

function(smfc_add_coexistence_probe name config with_simple_mfc)
    add_executable(${name} coexist.cpp)
    set_target_properties(${name} PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_definitions(${name} PRIVATE ${config} UNICODE _UNICODE
                               _CRT_SECURE_NO_WARNINGS NOMINMAX)
    target_compile_options(${name} PRIVATE /EHsc)
    if (with_simple_mfc)
        target_link_libraries(${name} PRIVATE smfc_coexist_objects ws2_32 iphlpapi)
    endif()
endfunction()

smfc_add_coexistence_probe(coexist_mfc_only   SMFC_COEXIST_MFC_ONLY   OFF)
smfc_add_coexistence_probe(coexist_mfc_first  SMFC_COEXIST_MFC_FIRST  ON)
smfc_add_coexistence_probe(coexist_smfc_first SMFC_COEXIST_SMFC_FIRST ON)

unset(CMAKE_MFC_FLAG)

# The baseline is recorded on the first comparison and published by the CI
# job: when a run goes red, what real MFC answered on its own is the first
# thing worth reading.
add_test(
    NAME coexistence_inert_mfc_first
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/nonperturbation.py"
            --baseline $<TARGET_FILE:coexist_mfc_only>
            --coexisting $<TARGET_FILE:coexist_mfc_first>
            --write-baseline "${CMAKE_CURRENT_BINARY_DIR}/mfc_only_$<CONFIG>.txt"
)
add_test(
    NAME coexistence_inert_smfc_first
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/nonperturbation.py"
            --baseline $<TARGET_FILE:coexist_mfc_only>
            --coexisting $<TARGET_FILE:coexist_smfc_first>
)
set_tests_properties(coexistence_inert_mfc_first coexistence_inert_smfc_first
                     PROPERTIES TIMEOUT 900)
