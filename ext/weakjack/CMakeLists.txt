# SPDX-FileCopyrightText: © 2020, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if(${HAVE_JACK})
  set(WEAKJACK_SOURCES weak_libjack.c)
  set(WEAKJACK_COMPILE_DEFS)

  if(USE_WEAK_JACK)
    list(APPEND WEAKJACK_COMPILE_DEFS USE_WEAK_JACK=1)
  endif()

  add_library(weakjack STATIC ${WEAKJACK_SOURCES})
  target_compile_options(weakjack INTERFACE ${WEAKJACK_COMPILE_DEFS})
  add_library(weakjack::weakjack ALIAS weakjack)
endif()
