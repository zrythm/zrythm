/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>

#include "guile/modules.h"

#include <gtk/gtk.h>

#include <libguile.h>
#include <pthread.h>

SCM position_type;
SCM track_type;
SCM tracklist_type;

/**
 * Inits the guile subsystem.
 */
int
guile_init (
  int     argc,
  char ** argv)
{
  g_message ("Initializing guile subsystem...");

  return 0;
}

/**
 * Defines all available modules to be used
 * by scripts.
 *
 * This must be called in guile mode.
 */
void
guile_define_modules (void)
{
  guile_actions_channel_send_action_define_module ();
  guile_actions_create_tracks_action_define_module ();
  guile_actions_port_connection_action_define_module ();
  guile_actions_undo_manager_define_module ();
  guile_audio_channel_define_module ();
  guile_audio_midi_note_define_module ();
  guile_audio_midi_region_define_module ();
  guile_audio_port_define_module ();
  guile_audio_position_define_module ();
  guile_audio_supported_file_define_module ();
  guile_audio_track_define_module ();
  guile_audio_track_processor_define_module ();
  guile_audio_tracklist_define_module ();
  guile_plugins_plugin_define_module ();
  guile_plugins_plugin_manager_define_module ();
  guile_zrythm_define_module ();
}
