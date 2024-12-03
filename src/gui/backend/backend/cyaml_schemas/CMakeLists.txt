# SPDX-FileCopyrightText: © 2022-2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_gui_lib PRIVATE
  dsp/channel.cpp
  dsp/control_room.cpp
  dsp/engine.cpp
  dsp/fader.cpp
  dsp/sample_processor.cpp
  dsp/track.cpp
  dsp/tracklist.cpp
  gui/backend/tracklist_selections.cpp
  project.cpp
)
