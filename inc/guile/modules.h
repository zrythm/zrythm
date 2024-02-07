/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
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
// #define FUNC_NAME s_

/** Macro. */
#if defined(SCM_MAGIC_SNARF_DOCS) || defined(SCM_MAGIC_SNARFER)
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
