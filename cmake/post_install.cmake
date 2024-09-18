# SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if(NOT DEFINED ENV{DESTDIR})
  set(prefix "${CMAKE_INSTALL_PREFIX}")
  set(datadir "${prefix}/share")
  set(schemadir "${datadir}/glib-2.0/schemas")
  set(fontsdir "${datadir}/fonts/zrythm")
  set(desktop_db_dir "${datadir}/applications")
  set(mime_dir "${datadir}/mime")
  set(doc_dir "${datadir}/doc/zrythm")

  message(STATUS "Compiling gsettings schemas...")
  execute_process(COMMAND glib-compile-schemas ${schemadir})

  find_program(GTK_UPDATE_ICON_CACHE_EXECUTABLE NAMES gtk-update-icon-cache)
  if(GTK_UPDATE_ICON_CACHE_EXECUTABLE)
    message(STATUS "Updating icon cache...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E touch "${datadir}/icons/hicolor")
    execute_process(COMMAND ${GTK_UPDATE_ICON_CACHE_EXECUTABLE})
  endif()

  find_program(UPDATE_MIME_DATABASE_EXECUTABLE NAMES update-mime-database)
  if(UPDATE_MIME_DATABASE_EXECUTABLE)
    message(STATUS "Updating MIME database...")
    execute_process(COMMAND ${UPDATE_MIME_DATABASE_EXECUTABLE} "${mime_dir}")
  endif()

  find_program(UPDATE_DESKTOP_DATABASE_EXECUTABLE NAMES update-desktop-database)
  if(UPDATE_DESKTOP_DATABASE_EXECUTABLE)
    message(STATUS "Updating desktop database...")
    if(NOT EXISTS "${desktop_db_dir}")
      file(MAKE_DIRECTORY "${desktop_db_dir}")
    endif()
    execute_process(COMMAND ${UPDATE_DESKTOP_DATABASE_EXECUTABLE} -q "${desktop_db_dir}")
  endif()

  find_program(UPDATE_GDK_PIXBUF_LOADERS_EXECUTABLE NAMES update-gdk-pixbuf-loaders)
  if(UPDATE_GDK_PIXBUF_LOADERS_EXECUTABLE)
    execute_process(COMMAND ${UPDATE_GDK_PIXBUF_LOADERS_EXECUTABLE})
  endif()
endif()
