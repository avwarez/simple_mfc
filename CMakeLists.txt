# 3.20 is where cxx_std_23 became a language the generators know how to
# request (/std:c++latest on MSVC, -std=c++23 elsewhere).
cmake_minimum_required(VERSION 3.20)
project(simple_mfc LANGUAGES CXX)

# C++23, standard library only — no GNU extensions required, to stay
# portable across GCC, Clang and MSVC.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Every source here is UTF-8 without a BOM. MSVC does not assume that: with
# neither a BOM nor /utf-8 it decodes the source in the machine's ANSI code
# page, so a non-ASCII character literal silently becomes a *different*,
# longer sequence of characters than the one written (on the CI runner's
# CP1252, L"é" turned into two characters) -- which is how a passing local
# GCC test became a failing Windows one. Directory-scoped on purpose, so it
# applies to every target here alike
# but is NOT exported to consumers: eMule's compile-check reproduces its own
# real build flags and must keep doing exactly that.
if (MSVC)
    add_compile_options(/utf-8)
endif()

# --- Library --------------------------------------------------------------
# BACKEND BRANCH: this tree carries only the non-frontend half of the MFC/ATL
# surface. The GUI headers, the gui/core and gui/qt runtimes, the .rc compiler
# and the demo are on `main`, not here. Besides the library itself, the only
# other target is the opt-in conformance suite at the bottom of this file.
#
# afxtempl.h/atlalloc.h/atlcoll.h/atlsimpcoll.h are header-only (templates):
# there is no source to compile for CArray<>/CList<>/CMap<>/CTempBuffer<>/
# CRBMap<>/CSimpleArray<>, and tests/portability instantiates them in full
# so their bodies are compiled somewhere regardless. Every remaining header
# has a .cpp here: the declaration-only ones (atlbase.h's CRegKey,
# atlcomcli.h's COM wrappers) were removed from this branch rather than left
# unimplemented. afxsock.cpp (CAsyncSocket) shows why sockets stayed: they
# map cleanly onto the portable Berkeley-sockets API (WinSock2 / POSIX), so
# it is a real, linked implementation.
add_library(simple_mfc STATIC
    src/eafx.cpp
    src/eafxcoll.cpp
    src/eafximpl.cpp
    src/eafxinet.cpp
    src/eafxmt.cpp
    src/eafxsock.cpp
    src/eafxwin.cpp
    src/eatlconv.cpp
    src/eatlenc.cpp
    src/eatltime.cpp
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

# CTime is 64-bit time (__time64_t), but every conversion it makes goes
# through the CRT's localtime/mktime -- so its real range is the range of
# std::time_t. MSVC has made that 64 bits since VS2005, on Win32 too. A
# 32-bit glibc target still defaults to a 32-bit one, which silently caps
# CTime at 2038-01-19: mktime returns -1 for anything later and eMule's
# stored timestamps stop round-tripping. glibc >= 2.34 offers the way out,
# and it is all-or-nothing across a program's translation units, hence
# PUBLIC -- a consumer compiled without it would see a different time_t in
# the same headers. Probed rather than assumed: on a C library that does
# not offer the switch (or already has a 64-bit time_t), nothing is added
# and the static_assert in tests/portability reports whether the result is
# adequate.
if (NOT WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_DEFINITIONS -D_FILE_OFFSET_BITS=64 -D_TIME_BITS=64)
    check_cxx_source_compiles(
        "#include <ctime>\nstatic_assert(sizeof(std::time_t) == 8, \"32-bit time_t\");\nint main() { return 0; }"
        SIMPLE_MFC_HAVE_TIME_BITS_64)
    unset(CMAKE_REQUIRED_DEFINITIONS)
    if (SIMPLE_MFC_HAVE_TIME_BITS_64)
        target_compile_definitions(simple_mfc PUBLIC _FILE_OFFSET_BITS=64 _TIME_BITS=64)
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

# --- Conformance suite ----------------------------------------------------
# tests/conformance compares this library's behaviour, method by method,
# against the real MFC/ATL that ships with Visual Studio. Opt-in: the
# native half builds anywhere, the real-MFC half only on MSVC with the
# "MFC and ATL" component installed. See .github/workflows/conformance.yml.
option(SIMPLE_MFC_BUILD_CONFORMANCE "Build the MFC/ATL conformance probes" OFF)
if (SIMPLE_MFC_BUILD_CONFORMANCE)
    enable_testing()
    add_subdirectory(tests/conformance)
endif()

# --- Portability suite ----------------------------------------------------
# tests/portability asks whether the library builds, links and runs on this
# platform and architecture at all: every header included on its own, every
# class template instantiated in full, the language standard and the Win32
# integer widths asserted. Opt-in like the conformance suite, and enabled by
# every job of .github/workflows/portability.yml.
option(SIMPLE_MFC_BUILD_PORTABILITY "Build the header/instantiation portability probes" OFF)
if (SIMPLE_MFC_BUILD_PORTABILITY)
    enable_testing()
    add_subdirectory(tests/portability)
endif()

# --- Install (optional, for use as an external library) --------------------
include(GNUInstallDirs)
install(TARGETS simple_mfc
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
