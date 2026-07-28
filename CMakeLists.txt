cmake_minimum_required(VERSION 3.16)
project(simple_mfc LANGUAGES CXX)

# Standard C++17 library only (see README.md) — no GNU extensions
# required, to stay portable across GCC, Clang and MSVC.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Every source here is UTF-8 without a BOM. MSVC does not assume that: with
# neither a BOM nor /utf-8 it decodes the source in the machine's ANSI code
# page, so a non-ASCII character literal silently becomes a *different*,
# longer sequence of characters than the one written (on the CI runner's
# CP1252, L"é" turned into two characters) -- which is how a passing local
# GCC test became a failing Windows one. Directory-scoped on purpose, so it
# applies to the library, the smoke test and both conformance probes alike
# but is NOT exported to consumers: eMule's compile-check reproduces its own
# real build flags and must keep doing exactly that.
if (MSVC)
    add_compile_options(/utf-8)
endif()

# --- Library --------------------------------------------------------------
# afxtempl.h/atlalloc.h (CTempBuffer<>) are header-only (templates): no
# source to compile for CArray<>/CList<>/CMap<>/CTempBuffer<>. The
# remaining "declaration-only" headers (afxwin.h..., see README.md) still
# have no .cpp: they are still exposed via include/ but not linked here,
# because they are inherently Windows/GUI/COM surfaces, not implemented in
# portable C++. afxsock.cpp (CAsyncSocket) is the exception: sockets map
# cleanly onto the portable Berkeley-sockets API (WinSock2 / POSIX), so it
# is a real, linked implementation.
add_library(simple_mfc STATIC
    src/afx.cpp
    src/afxcoll.cpp
    src/afximpl.cpp
    src/afxinet.cpp
    src/afxmt.cpp
    src/afxsock.cpp
    src/atlconv.cpp
    src/atlenc.cpp
    src/atltime.cpp
    src/atltypes.cpp
)
add_library(simple_mfc::simple_mfc ALIAS simple_mfc)

target_include_directories(simple_mfc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# afxmt.cpp uses <thread>/<mutex>/<condition_variable>: on GCC/Clang this
# needs pthread linked explicitly; on MSVC thread support is already in
# the runtime and Threads::Threads adds nothing (no-op).
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(simple_mfc PUBLIC Threads::Threads)

# afxsock.cpp (CAsyncSocket) uses the platform sockets API. On Windows that
# is WinSock2 (ws2_32) plus the IP helper API (iphlpapi, pulled in by
# afxsock.h for eMule's Pinger.cpp); on POSIX the sockets API is in libc,
# nothing extra to link.
if (WIN32)
    target_link_libraries(simple_mfc PUBLIC ws2_32 iphlpapi)
endif()

# afx.cpp uses <filesystem> (CFileFind): on GCC < 9 and on some Clang/
# libstdc++ builds it still lives in a separate library that must be
# linked explicitly. On GCC >= 9, recent Clang with libc++, and MSVC it is
# already part of the standard library: nothing to add.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(simple_mfc PUBLIC stdc++fs)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_LIBRARIES_SAVE ${CMAKE_REQUIRED_LIBRARIES})
    check_cxx_source_compiles("#include <filesystem>\nint main(){ (void)std::filesystem::current_path(); }" SIMPLE_MFC_FS_NO_EXTRA_LIB)
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_SAVE})
    if (NOT SIMPLE_MFC_FS_NO_EXTRA_LIB)
        target_link_libraries(simple_mfc PUBLIC stdc++fs)
    endif()
endif()

if (MSVC)
    target_compile_options(simple_mfc PUBLIC /W4 /EHsc /permissive-)
    target_compile_definitions(simple_mfc PUBLIC
        _CRT_SECURE_NO_WARNINGS # std::localtime/mktime/vswprintf: MSVC flags these as "unsafe" by default
        NOMINMAX                # in anticipation of a future afxwin.h implementation on real Windows
        WIN32_LEAN_AND_MEAN
    )
else()
    target_compile_options(simple_mfc PUBLIC -Wall -Wextra -Wpedantic)
endif()

# --- Smoke test (optional) -------------------------------------------------
option(SIMPLE_MFC_BUILD_SMOKE_TEST "Build the smoke-test executable" ON)
if (SIMPLE_MFC_BUILD_SMOKE_TEST)
    add_executable(simple_mfc_smoke_test tests/smoke_test.cpp)
    target_link_libraries(simple_mfc_smoke_test PRIVATE simple_mfc::simple_mfc)

    enable_testing()
    add_test(NAME smoke_test COMMAND simple_mfc_smoke_test)
    # The test runs in milliseconds; the timeout exists so that a test that
    # somehow blocks (a Windows modal dialog with nobody to dismiss it was
    # the real case) fails in a minute instead of pinning a CI runner for
    # hours. See SilenceWindowsCrtDialogs() in tests/smoke_test.cpp.
    set_tests_properties(smoke_test PROPERTIES TIMEOUT 60)
endif()

# --- GUI runtime (toolkit-agnostic core), optional -------------------------
# gui/core is the std, toolkit-agnostic GUI/threading runtime (message-map
# DISPATCH + CWinThread/AfxBeginThread) that backs the frozen MFC-subset
# declarations in include/. It is NOT part of the compile-check (which stays
# header-only): building it is opt-in. The toolkit DRIVERS (gui/qt, gui/wx)
# link on top of this and are wired in when SIMPLE_MFC_GUI selects them.
set(SIMPLE_MFC_GUI "none" CACHE STRING "GUI backend to build: none | qt | wx")
set_property(CACHE SIMPLE_MFC_GUI PROPERTY STRINGS none qt wx)

option(SIMPLE_MFC_BUILD_GUI_CORE_TEST
       "Build the gui/core runtime test (message-map dispatch + threading)" OFF)
option(SIMPLE_MFC_BUILD_RC_TESTS
       "Build the .rc resource-compiler tests (std-only, no MFC interface)" OFF)

# The neutral dialog-IR registry (gui/core/dialog_ir.h). It is the run-time
# consumer of the .rc compiler's output and is looked up by the drivers, so it
# is needed whenever the GUI core, a driver, or the .rc tests are built. Kept
# a separate tiny target: it is pure std C++ (no MFC interface, no -fms), so
# the codegen test can link it without pulling in the whole GUI core.
if (NOT SIMPLE_MFC_GUI STREQUAL "none"
        OR SIMPLE_MFC_BUILD_GUI_CORE_TEST
        OR SIMPLE_MFC_BUILD_RC_TESTS)
    add_library(simple_mfc_dialog_ir STATIC gui/core/dialog_registry.cpp)
    add_library(simple_mfc::dialog_ir ALIAS simple_mfc_dialog_ir)
    target_include_directories(simple_mfc_dialog_ir PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/gui/core)
endif()

if (NOT SIMPLE_MFC_GUI STREQUAL "none" OR SIMPLE_MFC_BUILD_GUI_CORE_TEST)
    add_library(simple_mfc_gui_core STATIC
        gui/core/cmdtarget_map.cpp
        gui/core/winthread.cpp
        gui/core/winapp.cpp
    )
    add_library(simple_mfc::gui_core ALIAS simple_mfc_gui_core)
    target_link_libraries(simple_mfc_gui_core
        PUBLIC simple_mfc::simple_mfc simple_mfc::dialog_ir)
    # winapp_internal.h: the AfxWinMain -> CWinApp seam, needed by the driver's
    # entry point and by anything that drives the framework without our main().
    target_include_directories(simple_mfc_gui_core
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gui/core)
endif()

if (SIMPLE_MFC_BUILD_GUI_CORE_TEST)
    enable_testing()
    # eMule (and this test) write message-map handlers in the bare-name
    # pointer-to-member form, ON_COMMAND(ID, OnFoo); real MFC compiles it via
    # an MSVC extension. GCC accepts the same construct with -fms-extensions.
    #
    # CLANG DOES NOT, with any flag combination -- measured: -fms-extensions
    # alone still rejects it, and adding -fms-compatibility makes it worse by
    # changing unrelated semantics. An earlier version of this comment claimed
    # Clang worked here; it does not. So the targets that use the bare-name
    # form are GCC/MSVC only, and Clang still builds and tests everything
    # else, including the whole conformance surface.
    if (MSVC OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_executable(gui_core_test tests/gui_core_test.cpp)
        target_link_libraries(gui_core_test PRIVATE simple_mfc::gui_core)
        if (NOT MSVC)
            target_compile_options(gui_core_test PRIVATE -fms-extensions)
        endif()
        add_test(NAME gui_core_test COMMAND gui_core_test)
        set_tests_properties(gui_core_test PROPERTIES TIMEOUT 60)
    else()
        message(STATUS
            "gui_core_test: skipped, ${CMAKE_CXX_COMPILER_ID} does not support "
            "the bare-name pointer-to-member form eMule's message maps use")
    endif()
endif()

# --- .rc resource compiler (build-time codegen -> neutral dialog IR) --------
# simple_mfc's own mini rc.exe. At an APP's build it reads the app's .rc +
# resource.h and emits generated C++ that registers the neutral dialog IR
# (see gui/core/rc/ and the plan's problem B). std-only, no MFC interface, so
# it is a plain host tool. Built whenever a driver is selected (real apps need
# it) or the .rc tests are on.
if (NOT SIMPLE_MFC_GUI STREQUAL "none" OR SIMPLE_MFC_BUILD_RC_TESTS)
    add_library(simple_mfc_rc STATIC gui/core/rc/rc_compiler.cpp)
    target_include_directories(simple_mfc_rc PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/gui/core/rc
        ${CMAKE_CURRENT_SOURCE_DIR}/gui/core)

    add_executable(smfc_rc gui/core/rc/smfc_rc_main.cpp)
    target_link_libraries(smfc_rc PRIVATE simple_mfc_rc)

    # Build-time codegen of the shared test fixture. Declared once here (so its
    # OUTPUT is unique) and consumed by rc_codegen_test and, under
    # SIMPLE_MFC_GUI=qt, qt_dialog_test -- the very step an app's build performs
    # on its own .rc. Only wired up when a consumer test is actually built.
    if (SIMPLE_MFC_BUILD_RC_TESTS OR SIMPLE_MFC_GUI STREQUAL "qt")
        set(SMFC_GEN_TEST_DIALOGS
            ${CMAKE_CURRENT_BINARY_DIR}/generated_test_dialogs.cpp)
        add_custom_command(
            OUTPUT ${SMFC_GEN_TEST_DIALOGS}
            COMMAND smfc_rc
                --rc ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/test_dialog.rc
                --header ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/test_resource.h
                --out ${SMFC_GEN_TEST_DIALOGS}
            DEPENDS smfc_rc
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/test_dialog.rc
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/test_resource.h
            COMMENT "smfc_rc: compiling test_dialog.rc -> neutral dialog IR")
    endif()
endif()

if (SIMPLE_MFC_BUILD_RC_TESTS)
    enable_testing()

    # Stage 1: unit-test the parser/symbols/emitter directly.
    add_executable(rc_compiler_test tests/rc_compiler_test.cpp)
    target_link_libraries(rc_compiler_test PRIVATE simple_mfc_rc)
    target_compile_definitions(rc_compiler_test PRIVATE
        SMFC_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
    add_test(NAME rc_compiler_test COMMAND rc_compiler_test)
    set_tests_properties(rc_compiler_test PROPERTIES TIMEOUT 60)

    # Stage 2: compile the fixture's generated C++ (produced above) into the
    # test and query the registry at run time -- the full .rc -> generated
    # code -> in-memory template pipeline, end to end.
    add_executable(rc_codegen_test
        tests/rc_codegen_test.cpp ${SMFC_GEN_TEST_DIALOGS})
    target_link_libraries(rc_codegen_test PRIVATE simple_mfc::dialog_ir)
    add_test(NAME rc_codegen_test COMMAND rc_codegen_test)
    set_tests_properties(rc_codegen_test PROPERTIES TIMEOUT 60)
endif()

# --- Qt driver (SIMPLE_MFC_GUI=qt) -----------------------------------------
# The Qt backing of simple_mfc's CWnd/CDialog (Layer 3). Links on top of the
# toolkit-agnostic gui/core (built above whenever SIMPLE_MFC_GUI != none).
if (SIMPLE_MFC_GUI STREQUAL "qt")
    find_package(Qt5 COMPONENTS Widgets REQUIRED)

    add_library(simple_mfc_gui_qt STATIC
        gui/qt/cwnd.cpp
        gui/qt/smfc_qt.cpp
        gui/qt/ddx.cpp
        gui/qt/controls.cpp
        gui/qt/cdc.cpp
        gui/qt/imagelist.cpp
        gui/qt/listctrl.cpp
        gui/qt/winapp.cpp
        gui/qt/messagebox.cpp
        gui/qt/afxwinmain.cpp
        gui/qt/main.cpp
    )
    add_library(simple_mfc::gui_qt ALIAS simple_mfc_gui_qt)
    target_link_libraries(simple_mfc_gui_qt PUBLIC simple_mfc::gui_core Qt5::Widgets)
    target_include_directories(simple_mfc_gui_qt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gui/qt)

    # Vertical slice: a live Qt button click routed through a real eMule-style
    # message map to its handler. Headless via QT_QPA_PLATFORM=offscreen.
    enable_testing()
    add_executable(qt_slice_test tests/qt_slice_test.cpp)
    target_link_libraries(qt_slice_test PRIVATE simple_mfc::gui_qt)
    if (NOT MSVC)
        # eMule's bare-name pointer-to-member map form (ON_BN_CLICKED(id, OnGo))
        # compiles on GCC/Clang with the same MSVC extension real MFC relies on.
        target_compile_options(qt_slice_test PRIVATE -fms-extensions)
    endif()
    add_test(NAME qt_slice_test COMMAND qt_slice_test)
    set_tests_properties(qt_slice_test PROPERTIES
        TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    # Driver-consumes-IR: build a CDialog from the .rc compiler's generated
    # template (IDD_SAMPLE) and exercise GetDlgItem / text / click routing on
    # the real QWidgets. smfc_rc generated the template above at build time.
    add_executable(qt_dialog_test
        tests/qt_dialog_test.cpp ${SMFC_GEN_TEST_DIALOGS})
    target_link_libraries(qt_dialog_test PRIVATE simple_mfc::gui_qt)
    if (NOT MSVC)
        target_compile_options(qt_dialog_test PRIVATE -fms-extensions)
    endif()
    add_test(NAME qt_dialog_test COMMAND qt_dialog_test)
    set_tests_properties(qt_dialog_test PROPERTIES
        TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    # DDX + typed controls: build a CDialog with member controls and a
    # DoDataExchange, and check load/save through the QWidgets.
    add_executable(qt_ddx_test
        tests/qt_ddx_test.cpp ${SMFC_GEN_TEST_DIALOGS})
    target_link_libraries(qt_ddx_test PRIVATE simple_mfc::gui_qt)
    if (NOT MSVC)
        target_compile_options(qt_ddx_test PRIVATE -fms-extensions)
    endif()
    add_test(NAME qt_ddx_test COMMAND qt_ddx_test)
    set_tests_properties(qt_ddx_test PROPERTIES
        TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    # GDI-independent common controls: CSliderCtrl/CProgressCtrl/CSpinButtonCtrl
    # over native Qt widgets + DDX_Radio over a radio group, built from the
    # IDD_CONTROLS template in the shared fixture.
    add_executable(qt_controls_test
        tests/qt_controls_test.cpp ${SMFC_GEN_TEST_DIALOGS})
    target_link_libraries(qt_controls_test PRIVATE simple_mfc::gui_qt)
    if (NOT MSVC)
        target_compile_options(qt_controls_test PRIVATE -fms-extensions)
    endif()
    add_test(NAME qt_controls_test COMMAND qt_controls_test)
    set_tests_properties(qt_controls_test PROPERTIES
        TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    # GDI slice 1: CDC/CPaintDC drawing primitives over QPainter, verified by
    # reading back pixels from the driver's offscreen paint surface. Links the
    # generated IR so CDialog(IDD_SAMPLE) has a template to size its canvas.
    add_executable(qt_gdi_test tests/qt_gdi_test.cpp ${SMFC_GEN_TEST_DIALOGS})
    target_link_libraries(qt_gdi_test PRIVATE simple_mfc::gui_qt)
    if (NOT MSVC)
        target_compile_options(qt_gdi_test PRIVATE -fms-extensions)
    endif()
    add_test(NAME qt_gdi_test COMMAND qt_gdi_test)
    set_tests_properties(qt_gdi_test PROPERTIES
        TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    add_executable(qt_paint_test tests/qt_paint_test.cpp ${SMFC_GEN_TEST_DIALOGS})
    target_link_libraries(qt_paint_test PRIVATE simple_mfc::gui_qt)
    if (NOT MSVC)
        target_compile_options(qt_paint_test PRIVATE -fms-extensions)
    endif()
    add_test(NAME qt_paint_test COMMAND qt_paint_test)
    set_tests_properties(qt_paint_test PROPERTIES
        TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    add_executable(qt_ownerdraw_test tests/qt_ownerdraw_test.cpp ${SMFC_GEN_TEST_DIALOGS})
    target_link_libraries(qt_ownerdraw_test PRIVATE simple_mfc::gui_qt)
    if (NOT MSVC)
        target_compile_options(qt_ownerdraw_test PRIVATE -fms-extensions)
    endif()
    add_test(NAME qt_ownerdraw_test COMMAND qt_ownerdraw_test)
    set_tests_properties(qt_ownerdraw_test PROPERTIES
        TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    add_executable(qt_app_test tests/qt_app_test.cpp ${SMFC_GEN_TEST_DIALOGS})
    target_link_libraries(qt_app_test PRIVATE simple_mfc::gui_qt)
    if (NOT MSVC)
        target_compile_options(qt_app_test PRIVATE -fms-extensions)
    endif()
    add_test(NAME qt_app_test COMMAND qt_app_test)
    set_tests_properties(qt_app_test PROPERTIES
        TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

# --- Demo application ------------------------------------------------------
# A complete little MFC application (examples/demo) built the way a real one
# is: its .rc goes through smfc_rc, its source knows nothing about Qt, and it
# supplies no main() - the framework's own entry point is linked in from the
# library. This is the end-to-end proof that an MFC program links and runs.
if (SIMPLE_MFC_GUI STREQUAL "qt")
    set(SMFC_DEMO_DIALOGS ${CMAKE_CURRENT_BINARY_DIR}/generated_demo_dialogs.cpp)
    add_custom_command(
        OUTPUT ${SMFC_DEMO_DIALOGS}
        COMMAND smfc_rc
            --rc ${CMAKE_CURRENT_SOURCE_DIR}/examples/demo/demo.rc
            --header ${CMAKE_CURRENT_SOURCE_DIR}/examples/demo/resource.h
            --out ${SMFC_DEMO_DIALOGS}
        DEPENDS smfc_rc
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/demo/demo.rc
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/demo/resource.h
        COMMENT "smfc_rc: compiling demo.rc -> neutral dialog IR")

    add_executable(smfc_demo examples/demo/demo.cpp ${SMFC_DEMO_DIALOGS})
    target_link_libraries(smfc_demo PRIVATE simple_mfc::gui_qt)
    target_include_directories(smfc_demo PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/demo)
    # The bare-name message-map form (ON_BN_CLICKED(ID, OnFoo)) real MFC
    # compiles via an MSVC extension; GCC/Clang need this to accept it.
    if (NOT MSVC)
        target_compile_options(smfc_demo PRIVATE -fms-extensions)
    endif()

    # The same demo.cpp, linked into a test that supplies its own main and
    # drives the startup by hand, so the window can be rendered and inspected
    # instead of waited on.
    add_executable(qt_demo_test tests/qt_demo_test.cpp
        examples/demo/demo.cpp ${SMFC_DEMO_DIALOGS})
    target_link_libraries(qt_demo_test PRIVATE simple_mfc::gui_qt)
    target_include_directories(qt_demo_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/demo)
    if (NOT MSVC)
        target_compile_options(qt_demo_test PRIVATE -fms-extensions)
    endif()
    add_test(NAME qt_demo_test COMMAND qt_demo_test)
    set_tests_properties(qt_demo_test PROPERTIES
        TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

# --- Conformance tests vs. real MFC (optional, MSVC + ATL/MFC only) --------
# Compares simple_mfc's behavior/results against the actual Visual Studio
# MFC libraries, method by method, with identical inputs. Off by default:
# it requires the "MFC and ATL" VS component, which is not installed
# everywhere. See tests/conformance/ and .github/workflows/msvc.yml.
option(SIMPLE_MFC_BUILD_CONFORMANCE_TESTS "Build the real-MFC conformance test suite (requires MSVC + ATL/MFC)" OFF)
if (SIMPLE_MFC_BUILD_CONFORMANCE_TESTS)
    enable_testing()
    add_subdirectory(tests/conformance)
endif()

# --- Golden-file conformance (every platform, POSIX included) --------------
# The suite above compares two MSVC builds with each other, so the build we
# actually ship — the POSIX one the Qt frontend runs on — was never compared
# against real MFC at all. This runs the same cases.cpp here and diffs it
# against real MFC's RECORDED output, produced by the Windows job (the only
# place real MFC exists) and committed under tests/conformance/golden/.
#
# This is what catches a divergence that lives only in a non-MSVC code path.
# The CString::Format dialect bug was exactly that shape: six conformance
# cases covered Format, all of them passed on both MSVC probes, and the
# shipped build still truncated every wide string to one character.
#
# ON by default and silently skipped when the recording is absent, so a
# fresh checkout without the golden still configures.
option(SIMPLE_MFC_BUILD_GOLDEN_CONFORMANCE
       "Compare this build against real MFC's recorded output" ON)
set(SIMPLE_MFC_GOLDEN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/tests/conformance/golden/real_mfc_win32.txt")
if (SIMPLE_MFC_BUILD_GOLDEN_CONFORMANCE AND EXISTS "${SIMPLE_MFC_GOLDEN_FILE}")
    enable_testing()
    add_executable(simple_mfc_golden_probe tests/conformance/cases.cpp)
    target_compile_definitions(simple_mfc_golden_probe
        PRIVATE SIMPLE_MFC_USE_NATIVE UNICODE _UNICODE)
    target_link_libraries(simple_mfc_golden_probe PRIVATE simple_mfc::simple_mfc)
    find_package(Threads REQUIRED)
    target_link_libraries(simple_mfc_golden_probe PRIVATE Threads::Threads)

    add_test(
        NAME golden_conformance
        COMMAND ${CMAKE_COMMAND}
                "-DPROBE_EXE=$<TARGET_FILE:simple_mfc_golden_probe>"
                "-DGOLDEN_FILE=${SIMPLE_MFC_GOLDEN_FILE}"
                "-DEXCLUDE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/tests/conformance/golden/platform_dependent.txt"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/conformance/golden.cmake"
    )
    set_tests_properties(golden_conformance PROPERTIES TIMEOUT 600)
elseif (SIMPLE_MFC_BUILD_GOLDEN_CONFORMANCE)
    message(STATUS
        "golden_conformance: skipped, no recording at ${SIMPLE_MFC_GOLDEN_FILE} "
        "(produced by the conformance job in .github/workflows/msvc.yml)")
endif()

# --- Install (optional, for use as an external library) --------------------
include(GNUInstallDirs)
install(TARGETS simple_mfc
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
