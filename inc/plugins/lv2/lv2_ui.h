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
 * LV2 UI related code.
 */

#ifndef __PLUGINS_LV2_LV2_UI_H__
#define __PLUGINS_LV2_LV2_UI_H__

#include "plugins/lv2/suil.h"

typedef struct Lv2Plugin Lv2Plugin;

/**
 * @addtogroup lv2
 *
 * @{
 */

/**
 * Returns if the UI of the plugin is resizable.
 */
int
lv2_ui_is_resizable (
  Lv2Plugin* plugin);

/**
 * Inits the LV2 plugin UI.
 */
void
lv2_ui_init (
  Lv2Plugin* plugin);

/**
 * Instantiates the plugin UI.
 */
void
lv2_ui_instantiate (
  Lv2Plugin *  plugin,
  const char * native_ui_type,
  void*        parent);

/**
 * Write events from the plugin's UI to the plugin.
 */
void
lv2_ui_write_events_from_ui_to_plugin (
  Lv2Plugin *    plugin,
  uint32_t       port_index,
  uint32_t       buffer_size,
  uint32_t       protocol, ///< format
  const void*    buffer);

/**
 * @}
 */

#endif
