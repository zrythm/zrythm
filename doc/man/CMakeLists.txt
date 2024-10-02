# SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if(NOT ZRYTHM_MANPAGE)
  return()
endif()

find_program(HELP2MAN_EXECUTABLE "help2man" REQUIRED)

configure_file(
  zrythm.h2m.in
  ${CMAKE_CURRENT_BINARY_DIR}/zrythm.h2m
  @ONLY)

set(target_manpage_path "${CMAKE_CURRENT_BINARY_DIR}/zrythm.1")

add_custom_command(
  OUTPUT ${target_manpage_path}
  COMMAND $<TARGET_FILE:zrythm> --version
  COMMAND $<TARGET_FILE:zrythm> --help
  COMMAND ${HELP2MAN_EXECUTABLE}
    -s 1 -N
    -i ${CMAKE_CURRENT_BINARY_DIR}/zrythm.h2m
    -o ${target_manpage_path}
    -n "Zrythm"
    --version-string="Zrythm ${ZRYTHM_VERSION}"
    $<TARGET_FILE:zrythm>
  DEPENDS zrythm ${CMAKE_CURRENT_BINARY_DIR}/zrythm.h2m
  COMMENT "Generating man page"
  VERBATIM)

add_custom_target(manpage ALL DEPENDS ${target_manpage_path})

install(FILES ${target_manpage_path}
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

# Generate completions
if(ZRYTHM_SHELL_COMPLETIONS)
  find_program(MANPAGE_COMPLETIONS_RUN_SH
    NAMES run.sh
    PATHS ${CMAKE_SOURCE_DIR}/ext/sh-manpage-completions
    NO_DEFAULT_PATH
    REQUIRED)

  set(GEN_COMPLETIONS_PY ${CMAKE_CURRENT_SOURCE_DIR}/gen_completions.py)

  if(0) # broken in CI
  # Fish completion
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zrythm.fish
    COMMAND ${Python3_EXECUTABLE} ${GEN_COMPLETIONS_PY}
      ${target_manpage_path} fish
      ${CMAKE_CURRENT_BINARY_DIR}/zrythm.fish
      ${MANPAGE_COMPLETIONS_RUN_SH}
    DEPENDS manpage ${GEN_COMPLETIONS_PY}
    COMMENT "Generating fish completion"
    VERBATIM)

  add_custom_target(fish-completion ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/zrythm.fish)

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/zrythm.fish
    DESTINATION ${CMAKE_INSTALL_DATADIR}/fish/vendor_completions.d)
  endif()

  # Bash completion
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zrythm
    COMMAND ${Python3_EXECUTABLE} ${GEN_COMPLETIONS_PY}
      ${target_manpage_path} bash
      ${CMAKE_CURRENT_BINARY_DIR}/zrythm
      ${MANPAGE_COMPLETIONS_RUN_SH}
    DEPENDS manpage ${GEN_COMPLETIONS_PY}
    COMMENT "Generating bash completion"
    VERBATIM)

  add_custom_target(bash-completion ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/zrythm)

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/zrythm
    DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions)

  # skip zsh completion for now - causes errors with old bash versions
endif()