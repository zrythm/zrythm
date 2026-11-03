# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# Merge duplicate workspace entries in the .qmlls.build.ini.part file that
# Qt writes at the end of configuration.
#
# Qt writes one workspace entry per QML module, using the module target's
# source directory as the key. qmlls keeps only the last entry for a given
# source path, so multiple QML modules declared in the same directory (e.g.
# Zrythm and ZrythmApp in src/gui) would lose all but the last module's
# import paths and resource files. Collapse such entries into one entry
# holding the union of their import paths and resource files.
#
# The parsed format (numbered `N\field="value"` lines with a trailing
# `size=N`) is the Qt-internal one written by Qt 6.12, the minimum version
# this project requires. Any unrecognized line aborts the merge and leaves
# the file untouched, so a future Qt format change degrades to a no-op
# instead of losing data.

function(zrythm_merge_qmlls_build_ini_part)
  set(part_file "${CMAKE_BINARY_DIR}/.qt/.qmlls.build.ini.part")
  if(NOT EXISTS "${part_file}")
    return()
  endif()

  # Qt joins list values with QDir::listSeparator(): ';' on Windows, ':'
  # elsewhere.
  if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(sep ";")
  else()
    set(sep ":")
  endif()

  file(READ "${part_file}" content)
  string(REPLACE "\n" ";" lines "${content}")

  # Collect the fields of each workspace entry, keyed by its array index.
  # Any line that is not a known field or the trailing size counter means
  # the format changed: leave the file untouched.
  set(indexes "")
  foreach(line IN LISTS lines)
    if(line STREQUAL "" OR line MATCHES "^size=[0-9]+$")
      continue()
    endif()
    if(NOT line MATCHES "^([0-9]+)\\\\(sourcePath|importPaths|resourceFiles)=(.*)$")
      message(WARNING
        "Unrecognized line in ${part_file}: \"${line}\". Leaving the file "
        "untouched; update MergeQmllsBuildIni.cmake for the new format.")
      return()
    endif()
    set(index "${CMAKE_MATCH_1}")
    set(field "${CMAKE_MATCH_2}")
    set(value "${CMAKE_MATCH_3}")
    string(REGEX REPLACE "^\"(.*)\"$" "\\1" value "${value}")
    if(NOT index IN_LIST indexes)
      list(APPEND indexes "${index}")
    endif()
    set("entry_${index}_${field}" "${value}")
  endforeach()

  if(NOT indexes)
    message(WARNING
      "No workspace entries found in ${part_file}. Leaving the file "
      "untouched.")
    return()
  endif()

  foreach(index IN LISTS indexes)
    if("${entry_${index}_sourcePath}" STREQUAL "")
      message(WARNING
        "Workspace entry ${index} in ${part_file} has no sourcePath. "
        "Leaving the file untouched.")
      return()
    endif()
  endforeach()

  # Nothing to merge (and nothing to rewrite) when every entry already has a
  # distinct source path.
  set(all_source_paths "")
  foreach(index IN LISTS indexes)
    list(APPEND all_source_paths "${entry_${index}_sourcePath}")
  endforeach()
  set(distinct_source_paths "${all_source_paths}")
  list(REMOVE_DUPLICATES distinct_source_paths)
  list(LENGTH all_source_paths total_entries)
  list(LENGTH distinct_source_paths distinct_entries)
  if(total_entries EQUAL distinct_entries)
    return()
  endif()

  # Merge entries that share a sourcePath, keeping discovery order and
  # deduplicating individual list elements.
  set(source_paths "")
  set(entry_count 0)
  foreach(index IN LISTS indexes)
    set(source_path "${entry_${index}_sourcePath}")
    if(NOT source_path IN_LIST source_paths)
      list(APPEND source_paths "${source_path}")
      math(EXPR entry_count "${entry_count} + 1")
      set("merged_${entry_count}_sourcePath" "${source_path}")
    endif()

    set(fields importPaths resourceFiles)
    foreach(field IN LISTS fields)
      string(REPLACE "${sep}" ";" elements "${entry_${index}_${field}}")
      foreach(element IN LISTS elements)
        if(element STREQUAL "")
          continue()
        endif()
        set(seen_list "seen_${field}_${entry_count}")
        if(NOT element IN_LIST ${seen_list})
          list(APPEND "${seen_list}" "${element}")
        endif()
      endforeach()
    endforeach()
  endforeach()

  # Serialize the merged entries.
  set(new_content "")
  foreach(n RANGE 1 "${entry_count}")
    list(JOIN "seen_importPaths_${n}" "${sep}" import_paths)
    list(JOIN "seen_resourceFiles_${n}" "${sep}" resource_files)
    string(APPEND new_content
      "${n}\\sourcePath=\"${merged_${n}_sourcePath}\"\n"
      "${n}\\importPaths=\"${import_paths}\"\n"
      "${n}\\resourceFiles=\"${resource_files}\"\n")
  endforeach()
  string(APPEND new_content "size=${entry_count}\n")

  if(NOT content STREQUAL new_content)
    file(WRITE "${part_file}" "${new_content}")
    message(STATUS
      "Merged duplicate .qmlls.build.ini workspace entries into "
      "${entry_count} entries")

    # The final .qmlls.build.ini is assembled from this file at build time,
    # but its build rule has no dependency on this file, so it is only
    # regenerated when missing. Drop a copy that does not contain the
    # merged entries yet so the next build recreates it.
    set(final_file "${CMAKE_BINARY_DIR}/.qt/.qmlls.build.ini")
    if(EXISTS "${final_file}")
      file(READ "${final_file}" final_content)
      string(FIND "${final_content}" "${new_content}" merged_pos)
      if(merged_pos EQUAL -1)
        file(REMOVE "${final_file}")
        message(STATUS
          "Removed stale ${final_file}; it will be regenerated on the next "
          "build")
      endif()
    endif()
  endif()
endfunction()
