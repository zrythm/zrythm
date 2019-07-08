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
  GtkDrawingArea         parent_instance;
  Channel                * channel; ///< the channel this belongs to
  int                    slot_index; ///< the channel slot index
  GtkGestureMultiPress *   multipress;
  GtkGestureDrag *         drag;

  /** For multipress. */
  int                      n_press;

  GtkGestureMultiPress *   right_mouse_mp;
} ChannelSlotWidget;

/**
 * Creates a new ChannelSlot widget and binds it to the given value.
 */
ChannelSlotWidget *
channel_slot_widget_new (int slot_index,
                         ChannelWidget * cw);

#endif
