/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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
/*
  Copyright 2007-2016 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
 * \file
 *
 * LV2 plugin window.
 */

#ifndef __PLUGINS_LV2_GTK_H__
#define __PLUGINS_LV2_GTK_H__

typedef struct Lv2Plugin  Lv2Plugin;
typedef struct Lv2Control Lv2Control;
typedef struct PluginGtkPresetMenu
                    PluginGtkPresetMenu;
typedef struct Port Port;

#include <stdint.h>

#include <gtk/gtk.h>

#include <lilv/lilv.h>

/**
 * @addtogroup plugins
 *
 * @{
 */

/**
 * Called when there is a UI port event from the
 * plugin.
 */
void
lv2_gtk_ui_port_event (
  Lv2Plugin *  plugin,
  uint32_t     port_index,
  uint32_t     buffer_size,
  uint32_t     protocol,
  const void * buffer);

/**
 * Called by generic UI callbacks when e.g. a slider
 * changes value.
 */
void
lv2_gtk_set_float_control (
  Lv2Plugin * lv2_plugin,
  Port *      port,
  float       value);

/**
 * Opens the LV2 plugin's UI (either wrapped with
 * suil or external).
 *
 * Use plugin_gtk_*() for generic UIs.
 */
int
lv2_gtk_open_ui (Lv2Plugin * plugin);

#if 0
PluginGtkPresetMenu*
lv2_gtk_get_bank_menu (
  Lv2Plugin* plugin,
  PluginGtkPresetMenu* menu,
  const LilvNode* bank);

void
lv2_gtk_on_save_activate (
  Lv2Plugin * plugin);

int
lv2_gtk_add_preset_to_menu (
  Lv2Plugin*      plugin,
  const LilvNode* node,
  const LilvNode* title,
  void*           data);
#endif

/**
 * Called by plugin_gtk_on_save_preset_activate()
 * on accept.
 */
void
lv2_gtk_on_save_preset_activate (
  GtkWidget *  widget,
  Lv2Plugin *  plugin,
  const char * path,
  const char * uri,
  bool         add_prefix);

GtkWidget *
lv2_gtk_build_control_widget (
  Lv2Plugin * plugin,
  GtkWindow * window);

void
lv2_gtk_on_delete_preset_activate (
  GtkWidget * widget,
  Lv2Plugin * plugin);

/**
 * @}
 */

#endif
