/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * LV2 state related code.
 */

#ifndef __PLUGINS_LV2_LV2_STATE_H__
#define __PLUGINS_LV2_LV2_STATE_H__

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

/**
 * Deletes the current preset.
 */
int
lv2_state_delete_current_preset (
  Lv2Plugin* plugin);

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
lv2_state_load_presets (
  Lv2Plugin* plugin,
  PresetSink sink,
  void* data);

int
lv2_state_unload_presets (
  Lv2Plugin* plugin);

void
lv2_state_apply_state (
  Lv2Plugin* plugin,
  LilvState* state);

int
lv2_state_apply_preset (
  Lv2Plugin* plugin,
  const LilvNode* preset);

void
lv2_state_save (
  Lv2Plugin* plugin,
  const char* dir);

char *
lv2_state_make_path (
  LV2_State_Make_Path_Handle handle,
  const char*                path);

/**
 * @}
 */

#endif

