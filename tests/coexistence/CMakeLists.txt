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
                         eatlconv eatlenc eatltime eatltypes)
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

# --- one TU per header pair, both orders -----------------------------------
# The umbrella probe above includes the whole surface at once, so a header
# that only works because another one was included first passes there and
# fails in the first consumer that includes it alone -- which is how eMule
# includes most of them. Each of this branch's headers is therefore also
# compiled next to its real-MFC counterpart on its own, once in each order.
# A static library: nothing here needs to link, the question is whether the
# two headers parse together.
# Separated by "|" rather than ";": foreach(... IN LISTS) flattens a
# semicolon inside an element, which would turn every pair into two
# unrelated iterations.
set(SMFC_HEADER_PAIRS
    "eafx.h|afx.h"
    "eafxcoll.h|afxcoll.h"
    "eafximpl.h|afx.h"
    "eafxinet.h|afxinet.h"
    "eafxmt.h|afxmt.h"
    "eafxsock.h|afxsock.h"
    "eafxtempl.h|afxtempl.h"
    "eafxwin.h|afxwin.h"
    "eatlalloc.h|atlalloc.h"
    "eatlbase.h|atlbase.h"
    "eatlcoll.h|atlcoll.h"
    "eatlconv.h|atlconv.h"
    "eatldef.h|atldef.h"
    "eatlenc.h|atlenc.h"
    "eatlsimpcoll.h|atlsimpcoll.h"
    "eatltime.h|atltime.h"
    "eatltypes.h|atltypes.h"
    "esockimpl.h|afxsock.h"
    "etchar.h|tchar.h"
)

set(SMFC_PAIR_TUS)
foreach (pair IN LISTS SMFC_HEADER_PAIRS)
    string(REPLACE "|" ";" split "${pair}")
    list(GET split 0 ours)
    list(GET split 1 theirs)
    get_filename_component(stem "${ours}" NAME_WE)
    foreach (order mfc_first smfc_first)
        set(generated "${CMAKE_CURRENT_BINARY_DIR}/pair_${stem}_${order}.cpp")
        if (order STREQUAL "mfc_first")
            set(includes "#include <${theirs}>\n#include \"${ours}\"\n")
        else()
            set(includes "#include \"${ours}\"\n#include <${theirs}>\n")
        endif()
        file(WRITE "${generated}"
            "// generated by tests/coexistence/CMakeLists.txt -- do not edit\n"
            "// ${ours} next to ${theirs}, ${order}.\n"
            "${includes}"
            "int smfc_pair_${stem}_${order}() { return 0; }\n")
        list(APPEND SMFC_PAIR_TUS "${generated}")
    endforeach()
endforeach()

add_library(smfc_coexist_headers STATIC ${SMFC_PAIR_TUS})
set_target_properties(smfc_coexist_headers PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_include_directories(smfc_coexist_headers PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_compile_definitions(smfc_coexist_headers PRIVATE
    UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS NOMINMAX)
target_compile_options(smfc_coexist_headers PRIVATE /EHsc)

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
