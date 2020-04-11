/*
 * Copyright (C) 2019 Alexandros Theodotou
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

/** \file
 */

#ifndef __GUI_WIDGETS_CHANNEL_SLOT_H__
#define __GUI_WIDGETS_CHANNEL_SLOT_H__

#include <gtk/gtk.h>

#define CHANNEL_SLOT_WIDGET_TYPE \
  (channel_slot_widget_get_type ())
G_DECLARE_FINAL_TYPE (ChannelSlotWidget,
                      channel_slot_widget,
                      Z,
                      CHANNEL_SLOT_WIDGET,
                      GtkDrawingArea)

typedef struct Plugin Plugin;
typedef struct Channel Channel;

typedef struct _ChannelSlotWidget
{
  GtkDrawingArea       parent_instance;
  /** The Channel this belongs to. */
  Channel *            channel;
  /** The Channel slot index. */
  int                  slot_index;
  GtkGestureMultiPress * multipress;
  GtkGestureDrag *     drag;

  /**
   * Previous plugin name at
   * this slot in the last draw callback, or NULL.
   *
   * If this changes, the tooltip is changed.
   */
  char *               pl_name;

  /** For multipress. */
  int                  n_press;

  GtkGestureMultiPress * right_mouse_mp;

  /** Layout cache for empty slot. */
  PangoLayout *        empty_slot_layout;
  /** Layout cache for plugin name. */
  PangoLayout *        pl_name_layout;

  /** Whether to open the plugin inspector on click
   * or not. */
  bool                 open_plugin_inspector_on_click;
} ChannelSlotWidget;

/**
 * Creates a new ChannelSlot widget and binds it to
 * the given value.
 */
ChannelSlotWidget *
channel_slot_widget_new (
  int       slot_index,
  Channel * ch,
  bool      open_plugin_inspector_on_click);

#endif
