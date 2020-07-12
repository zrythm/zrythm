/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * LV2 state management.
 */

#ifndef __PLUGINS_LV2_LV2_STATE_H__
#define __PLUGINS_LV2_LV2_STATE_H__

#include "zrythm-config.h"

#include <lv2/state/state.h>

/**
 * @addtogroup lv2
 *
 * @{
 */

typedef int (*PresetSink)
  (Lv2Plugin*           plugin,
  const LilvNode* node,
  const LilvNode* title,
  void*           data);

void
lv2_state_apply_state (
  Lv2Plugin* plugin,
  LilvState* state);

/**
 * Saves the plugin state to the filesystem and
 * returns the state.
 */
LilvState *
lv2_state_save_to_file (
  Lv2Plugin *  pl,
  bool         is_backup);

/**
 * Saves the plugin state into a new LilvState that
 * can be applied to any plugin with the same URI
 * (like clones).
 *
 * Must be free'd with lilv_state_free().
 */
LilvState *
lv2_state_save_to_memory (
  Lv2Plugin* plugin);

/**
 * Saves the plugin state to a string after writing
 * the required files.
 */
LilvState *
lv2_state_save_to_string (
  Lv2Plugin *  pl,
  bool         is_backup);

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

/**
 * LV2 State makePath feature for temporary files.
 *
 * Should be passed to LV2_Descriptor::instantiate()
 * and this function must return an absolute path.
 */
char *
lv2_state_make_path_temp (
  LV2_State_Make_Path_Handle handle,
  const char*                path);

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

int
lv2_state_apply_preset (
  Lv2Plugin* plugin,
  const LilvNode* preset);

/**
 * Deletes the current preset.
 */
int
lv2_state_delete_current_preset (
  Lv2Plugin* plugin);

int
lv2_state_load_presets (
  Lv2Plugin* plugin,
  PresetSink sink,
  void* data);

int
lv2_state_unload_presets (
  Lv2Plugin* plugin);

/**
 * @}
 */

#endif
