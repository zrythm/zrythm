# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# Computes translation completion percentages for each language in
# language_mappings by counting finished vs unfinished <translation>
# elements in the .ts files.
#
# Sets ${out_var} to a list of "code|name|percent" entries.

function(compute_translation_stats out_var)
  set(entries)
  foreach(lang_pair ${language_mappings})
    string(REPLACE "/" ";" lang_list ${lang_pair})
    list(GET lang_list 0 lang_code)
    list(GET lang_list 1 lang_name)

    if(lang_code STREQUAL "en")
      # source language is always complete
      set(percent 100)
    else()
      file(READ "${CMAKE_SOURCE_DIR}/i18n/zrythm_${lang_code}.ts" ts_content)
      string(REGEX MATCHALL "<translation" all_translations "${ts_content}")
      string(REGEX MATCHALL "type=\"unfinished\"" unfinished_translations "${ts_content}")
      list(LENGTH all_translations total_count)
      list(LENGTH unfinished_translations unfinished_count)
      math(EXPR finished_count "${total_count} - ${unfinished_count}")
      if(total_count GREATER 0)
        math(EXPR percent "${finished_count} * 100 / ${total_count}")
      else()
        set(percent 0)
      endif()
    endif()

    list(APPEND entries "${lang_code}|${lang_name}|${percent}")
  endforeach()
  set(${out_var} "${entries}" PARENT_SCOPE)
endfunction()
