/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Channel slot.
 */

#ifndef __GUI_WIDGETS_CHANNEL_SLOT_H__
#define __GUI_WIDGETS_CHANNEL_SLOT_H__

#include <gtk/gtk.h>

#define CHANNEL_SLOT_WIDGET_TYPE \
  (channel_slot_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChannelSlotWidget,
  channel_slot_widget,
  Z,
  CHANNEL_SLOT_WIDGET,
  GtkWidget)

typedef struct Plugin  Plugin;
typedef struct Channel Channel;

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _ChannelSlotWidget
{
  GtkWidget parent_instance;

  PluginSlotType type;

  /** The Track this belongs to. */
  Track * track;

  /** The Channel slot index. */
  int               slot_index;
  GtkGestureClick * click;
  GtkGestureDrag *  drag;

  /**
   * Previous plugin name at
   * this slot in the last draw callback, or NULL.
   *
   * If this changes, the tooltip is changed.
   */
  char * pl_name;

  /** For multipress. */
  int n_press;

  GtkGestureClick * right_mouse_mp;

  /** Layout cache for empty slot. */
  PangoLayout * empty_slot_layout;
  /** Layout cache for plugin name. */
  PangoLayout * pl_name_layout;

  /** Cache to check if the selection state was
   * changed. */
  bool was_selected;

  /** Whether to open the plugin inspector on click
   * or not. */
  bool open_plugin_inspector_on_click;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
} ChannelSlotWidget;

/**
 * Creates a new ChannelSlot widget whose track
 * and plugin can change.
 */
ChannelSlotWidget *
channel_slot_widget_new_instrument (void);

/**
 * Creates a new ChannelSlot widget and binds it to
 * the given value.
 */
ChannelSlotWidget *
channel_slot_widget_new (
  int            slot_index,
  Track *        track,
  PluginSlotType type,
  bool           open_plugin_inspector_on_click);

void
channel_slot_widget_set_instrument (
  ChannelSlotWidget * self,
  Track *             track);

/**
 * Sets or unsets state flags and redraws the
 * widget.
 *
 * @param set True to set, false to unset.
 */
void
channel_slot_widget_set_state_flags (
  ChannelSlotWidget * self,
  GtkStateFlags       flags,
  bool                set);

/**
 * @}
 */

#endif
