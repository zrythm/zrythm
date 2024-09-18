# SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

set(theme_iconsdir "${themesdir}/icons")

# Install zrythm dark icon theme
install(DIRECTORY "zrythm-dark"
  DESTINATION "${theme_iconsdir}")

# Create symlinks/copies for track icons
if(WIN32)
  set(COPY_OR_LINK_CMD "cp")
else()
  set(COPY_OR_LINK_CMD "ln")
endif()

install(CODE "
  execute_process(
    COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/install_tracktype_icons.py
      ${CMAKE_INSTALL_FULL_DATADIR}/zrythm/themes/icons/zrythm-dark/scalable/tracktypes
      ${COPY_OR_LINK_CMD}
  )"
)

# Install app icon
install(FILES "zrythm-dark/scalable/apps/zrythm.svg"
  RENAME "org.zrythm.Zrythm.svg"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps")
