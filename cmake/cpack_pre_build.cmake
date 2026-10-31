# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: CC0-1.0

# Remove shell completions from AppImage — they conflict with AppRun
# detection (the bash completion file is named "zrythm", same as the
# binary) and are useless inside a portable image anyway.
# Also fix AppRun — the generator picks the wrong file named "zrythm"
# in the install tree (e.g., share/zrythm directory) instead of bin/zrythm.
if(CPACK_GENERATOR STREQUAL "AppImage")
  file(REMOVE_RECURSE "${CPACK_TEMPORARY_DIRECTORY}/share/bash-completion")
  file(REMOVE_RECURSE "${CPACK_TEMPORARY_DIRECTORY}/share/fish")
  file(WRITE "${CPACK_TEMPORARY_DIRECTORY}/AppRun"
    "#!/usr/bin/env bash\nset -e\nthis_dir=\"$(readlink -f \"$(dirname \"$0\")\")\"\nexec \"$this_dir/bin/zrythm\" \"$@\"\n")

  # Debug AppImages: strip DWARF from bundled third-party libraries to keep
  # the image under GitLab's artifact size cap. `bin/zrythm` is left untouched.
  # `--strip-debug` preserves `.symtab`, so third-party frames still resolve to
  # function names in backtraces.
  if(CPACK_BUILD_CONFIG STREQUAL "Debug")
    message(STATUS "cpack_pre_build: stripping DWARF from bundled libraries")
    find_program(STRIP_CMD NAMES strip REQUIRED)

    file(GLOB_RECURSE lib_files
      LIST_DIRECTORIES false
      "${CPACK_TEMPORARY_DIRECTORY}/lib/*.so*"
    )

    set(processed_reals "")
    set(total_before 0)
    set(total_after 0)
    foreach(lib ${lib_files})
      # Resolve symlinks (e.g. libQt6Core.so -> libQt6Core.so.6 ->
      # libQt6Core.so.6.12.0) so each real file is stripped exactly once.
      get_filename_component(real_lib "${lib}" REALPATH)
      if(real_lib IN_LIST processed_reals)
        continue()
      endif()
      list(APPEND processed_reals "${real_lib}")

      file(SIZE "${real_lib}" size_before)
      execute_process(
        COMMAND ${STRIP_CMD} --strip-debug "${real_lib}"
        RESULT_VARIABLE strip_result
        OUTPUT_QUIET ERROR_QUIET
      )
      if(NOT strip_result EQUAL 0)
        message(WARNING "cpack_pre_build: strip failed for ${real_lib} (${strip_result})")
        continue()
      endif()
      file(SIZE "${real_lib}" size_after)
      math(EXPR total_before "${total_before} + ${size_before}")
      math(EXPR total_after "${total_after} + ${size_after}")
    endforeach()

    math(EXPR mib_before "${total_before} / 1048576")
    math(EXPR mib_after "${total_after} / 1048576")
    message(STATUS "cpack_pre_build: bundled libraries ${mib_before} MiB -> ${mib_after} MiB")
  endif()
endif()
