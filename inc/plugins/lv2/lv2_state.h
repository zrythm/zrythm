/*
 * SPDX-FileCopyrightText: © 2018-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * LV2 state management.
 */

#ifndef __PLUGINS_LV2_LV2_STATE_H__
#define __PLUGINS_LV2_LV2_STATE_H__

#include "zrythm-config.h"

#include <glib.h>

#include <lilv/lilv.h>
#include <lv2/state/state.h>

typedef struct Lv2Plugin Lv2Plugin;

/**
 * @addtogroup lv2
 *
 * @{
 */

typedef int (*PresetSink) (
  Lv2Plugin *      plugin,
  const LilvNode * node,
  const LilvNode * title,
  void *           data);

/* handled by lilv */
#if 0
/**
 * LV2 state mapPath feature - abstract path
 * callback.
 */
char *
lv2_state_get_abstract_path (
  LV2_State_Map_Path_Handle handle,
  const char *              absolute_path);

/**
 * LV2 state mapPath feature - absolute path
 * callback.
 */
char *
lv2_state_get_absolute_path (
  LV2_State_Map_Path_Handle handle,
  const char *              abstract_path);

/**
 * LV2 State freePath implementation.
 */
void
lv2_state_free_path (
  LV2_State_Free_Path_Handle handle,
  char *                     path);

/**
 * LV2 State makePath feature for save only.
 *
 * Should be passed to LV2_State_Interface::save().
 * and this function must return an absolute path.
 */
char *
lv2_state_make_path_save (
  LV2_State_Make_Path_Handle handle,
  const char*                path);
#endif

NONNULL void
lv2_state_apply_state (Lv2Plugin * plugin, LilvState * state);

/**
 * Saves the plugin state to the filesystem and
 * returns the state.
 */
WARN_UNUSED_RESULT NONNULL LilvState *
lv2_state_save_to_file (Lv2Plugin * pl, bool is_backup);

/**
 * Saves the plugin state into a new LilvState that
 * can be applied to any plugin with the same URI
 * (like clones).
 *
 * Must be free'd with lilv_state_free().
 */
LilvState *
lv2_state_save_to_memory (Lv2Plugin * plugin);

/**
 * Saves the plugin state to a string after writing
 * the required files.
 */
LilvState *
lv2_state_save_to_string (Lv2Plugin * pl, bool is_backup);

/**
 * LV2 State makePath feature for temporary files.
 *
 * Should be passed to LV2_Descriptor::instantiate()
 * and this function must return an absolute path.
 */
char *
lv2_state_make_path_temp (LV2_State_Make_Path_Handle handle, const char * path);

/**
 * Saves the preset.
 */
int
lv2_state_save_preset (
  Lv2Plugin *  plugin,
  const char * dir,
  const char * uri,
  const char * label,
  const char * filename);

/**
 * Applies the given preset, or the preset in the
 * path if @ref preset is NULL.
 *
 * @return Whether the preset was applied.
 */
bool
lv2_state_apply_preset (
  Lv2Plugin *      plugin,
  const LilvNode * preset,
  const char *     path,
  GError **        error);

/**
 * Deletes the current preset.
 */
int
lv2_state_delete_current_preset (Lv2Plugin * plugin);

int
lv2_state_load_presets (Lv2Plugin * plugin, PresetSink sink, void * data);

int
lv2_state_unload_presets (Lv2Plugin * plugin);

/**
 * @}
 */

#endif
