# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_lib
  PRIVATE
    plugin_descriptor.h
    zrythm_application.h
    zrythm_application.cpp
)

add_subdirectory(backend)
add_subdirectory(gtk_widgets) # TODO delete eventually