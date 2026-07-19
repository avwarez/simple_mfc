cmake_minimum_required(VERSION 3.16)
project(simple_mfc LANGUAGES CXX)

# Standard C++17 library only (see README.md) — no GNU extensions
# required, to stay portable across GCC, Clang and MSVC.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Library --------------------------------------------------------------
# afxtempl.h is header-only (templates): no source to compile for
# CArray<>/CList<>/CMap<>. The "declaration-only" headers (afxwin.h,
# afxsock.h..., see README.md) have no .cpp: they are still exposed via
# include/ but not linked here, because they are not implemented yet.
add_library(simple_mfc STATIC
    src/afx.cpp
    src/afxcoll.cpp
    src/afxmt.cpp
    src/atltime.cpp
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
    add_executable(simple_mfc_smoke_test test/smoke_test.cpp)
    target_link_libraries(simple_mfc_smoke_test PRIVATE simple_mfc::simple_mfc)

    enable_testing()
    add_test(NAME smoke_test COMMAND simple_mfc_smoke_test)
endif()

# --- Install (optional, for use as an external library) --------------------
include(GNUInstallDirs)
install(TARGETS simple_mfc
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
