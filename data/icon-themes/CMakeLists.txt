# SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# Install zrythm dark icon theme
# install(DIRECTORY "zrythm-dark"
  # DESTINATION "${themesdir}/icons")

return()

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
