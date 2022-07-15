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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Collection of guile module declarations.
 */

#ifndef __GUILE_MODULES_H__
#define __GUILE_MODULES_H__

#include <libguile.h>

/**
 * @addtogroup guile
 *
 * @{
 */

/** Guile function prefix. */
//#define FUNC_NAME s_

/** Macro. */
#if defined(SCM_MAGIC_SNARF_DOCS) \
  || defined(SCM_MAGIC_SNARFER)
#  define SNARF_MODE 1
#endif

extern SCM position_type;
extern SCM track_type;
extern SCM tracklist_type;

void
guile_actions_channel_send_action_define_module (void);
void
guile_actions_tracklist_selections_action_define_module (void);
void
guile_actions_port_connection_action_define_module (void);
void
guile_actions_undo_manager_define_module (void);
void
guile_audio_channel_define_module (void);
void
guile_audio_midi_note_define_module (void);
void
guile_audio_midi_region_define_module (void);
void
guile_audio_port_define_module (void);
void
guile_audio_position_define_module (void);
void
guile_audio_supported_file_define_module (void);
void
guile_audio_track_define_module (void);
void
guile_audio_track_processor_define_module (void);
void
guile_audio_tracklist_define_module (void);
void
guile_plugins_plugin_define_module (void);
void
guile_plugins_plugin_manager_define_module (void);
void
guile_project_define_module (void);
void
guile_zrythm_define_module (void);

/**
 * @}
 */

#endif
