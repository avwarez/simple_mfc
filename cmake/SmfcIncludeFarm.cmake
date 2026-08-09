# SmfcIncludeFarm.cmake -- configure-time case-insensitive include farm.
#
# THE PROBLEM. Windows filesystems are case-insensitive, so MSVC resolves
# #include "StdAfx.h" against a file named Stdafx.h without complaint. Linux
# filesystems are case-sensitive and refuse. A Windows codebase of any age
# accumulates these mismatches silently -- eMule has 476 distinct include
# spellings across 313 headers, and the mismatches are spread over hundreds of
# translation units.
#
# WHAT NOT TO DO. Renaming the files (or rewriting the #include lines) in the
# ported source tree would work, but it edits hundreds of files for a reason
# that has nothing to do with the port, and it makes every future merge from
# upstream conflict. The mismatch is a property of the FILESYSTEM, so the fix
# belongs in the filesystem.
#
# WHAT THIS DOES. At configure time it builds a directory of symlinks -- one
# per include spelling that the sources actually use -- each pointing at the
# real header, found case-insensitively. That directory goes on the include
# path ahead of the real one, so every spelling resolves and not a single
# source file is touched.
#
# It is deliberately driven by the spellings that APPEAR IN THE SOURCE, not by
# a blanket lowercasing of every header: the farm then contains exactly the
# links that are needed, and a spelling that resolves to nothing shows up as a
# missing link rather than being hidden under a pile of generated aliases.
#
# NO-OP ON WINDOWS, by design: there the filesystem already does this, and
# creating symlinks would need elevated privileges.

# smfc_add_include_farm(<out-var>
#     FARM  <directory to create>
#     ROOTS <directory> [<directory> ...])
#
# Sets <out-var> to the farm directory (or to nothing on Windows, so callers
# can pass it straight to target_include_directories either way).
function(smfc_add_include_farm out_var)
    cmake_parse_arguments(FARM "" "FARM" "ROOTS" ${ARGN})

    if(WIN32)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    if(NOT FARM_FARM OR NOT FARM_ROOTS)
        message(FATAL_ERROR "smfc_add_include_farm: FARM and ROOTS are both required")
    endif()

    file(MAKE_DIRECTORY "${FARM_FARM}")

    # --- 1. Index every real header by its lowercased path relative to its
    #        root, so a spelling can be looked up case-insensitively.
    set(_index_keys "")
    foreach(_root IN LISTS FARM_ROOTS)
        file(GLOB_RECURSE _headers RELATIVE "${_root}"
             "${_root}/*.h" "${_root}/*.hpp" "${_root}/*.inl")
        foreach(_h IN LISTS _headers)
            string(TOLOWER "${_h}" _key)
            # First root wins on a collision, matching include-path order.
            if(NOT DEFINED _index_${_key})
                set(_index_${_key} "${_root}/${_h}")
                list(APPEND _index_keys "${_key}")
            endif()
        endforeach()
    endforeach()

    # --- 2. Collect every include spelling the sources actually use.
    set(_spellings "")
    foreach(_root IN LISTS FARM_ROOTS)
        file(GLOB_RECURSE _sources
             "${_root}/*.h" "${_root}/*.hpp" "${_root}/*.inl"
             "${_root}/*.c" "${_root}/*.cpp")
        foreach(_src IN LISTS _sources)
            file(STRINGS "${_src}" _lines REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"]")
            foreach(_line IN LISTS _lines)
                if(_line MATCHES "#[ \t]*include[ \t]*[<\"]([^>\"]+)[>\"]")
                    # Windows sources spell nested includes with backslashes.
                    string(REPLACE "\\" "/" _spelling "${CMAKE_MATCH_1}")
                    list(APPEND _spellings "${_spelling}")
                endif()
            endforeach()
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES _spellings)

    # --- 3. One symlink per spelling that resolves to a real header.
    #        A spelling that resolves to nothing is left alone on purpose: it
    #        is either a system header or a genuinely missing file, and both
    #        are the compiler's business to report, not this function's to
    #        paper over.
    set(_made 0)
    foreach(_spelling IN LISTS _spellings)
        string(TOLOWER "${_spelling}" _key)
        if(NOT DEFINED _index_${_key})
            continue()
        endif()
        set(_target "${_index_${_key}}")
        set(_link "${FARM_FARM}/${_spelling}")

        # Skip the link when the spelling already matches the real file
        # exactly: on a case-sensitive filesystem that include resolves on its
        # own, and a self-referential link would be pointless.
        if("${_link}" STREQUAL "${_target}")
            continue()
        endif()

        get_filename_component(_dir "${_link}" DIRECTORY)
        file(MAKE_DIRECTORY "${_dir}")
        if(NOT IS_SYMLINK "${_link}")
            file(CREATE_LINK "${_target}" "${_link}" SYMBOLIC)
            math(EXPR _made "${_made} + 1")
        endif()
    endforeach()

    list(LENGTH _spellings _n_spellings)
    list(LENGTH _index_keys _n_headers)
    message(STATUS
        "include farm: ${_made} new links in ${FARM_FARM} "
        "(${_n_spellings} spellings seen, ${_n_headers} headers indexed)")

    set(${out_var} "${FARM_FARM}" PARENT_SCOPE)
endfunction()
