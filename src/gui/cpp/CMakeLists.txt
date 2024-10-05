# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_lib
  PRIVATE
    zrythm_application.h
    zrythm_application.cpp
)

add_subdirectory(glue)
add_subdirectory(backend)
add_subdirectory(gtk_widgets) # TODO delete eventually