# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_lib
  PRIVATE
    alert_manager.h
    alert_manager.cpp
    plugin_descriptor.h
    plugin_descriptor_list.h
    project_manager.h
    project_manager.cpp
    zrythm_application.h
    zrythm_application.cpp
)

add_subdirectory(backend)
add_subdirectory(gtk_widgets) # TODO delete eventually