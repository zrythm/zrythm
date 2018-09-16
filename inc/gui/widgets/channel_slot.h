/*
 * gui/widgets/channel_slot.h - Channel slot
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/** \file
 */

#ifndef __GUI_WIDGETS_CHANNEL_SLOT_H__
#define __GUI_WIDGETS_CHANNEL_SLOT_H__

#include <gtk/gtk.h>

#define CHANNEL_SLOT_WIDGET_TYPE                  (channel_slot_widget_get_type ())
#define CHANNEL_SLOT_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHANNEL_SLOT_WIDGET_TYPE, ChannelSlotWidget))
#define CHANNEL_SLOT_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), CHANNEL_SLOT_WIDGET, ChannelSlotWidgetClass))
#define IS_CHANNEL_SLOT_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHANNEL_SLOT_WIDGET_TYPE))
#define IS_CHANNEL_SLOT_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), CHANNEL_SLOT_WIDGET_TYPE))
#define CHANNEL_SLOT_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), CHANNEL_SLOT_WIDGET_TYPE, ChannelSlotWidgetClass))

typedef struct Plugin Plugin;
typedef struct Channel Channel;

typedef struct ChannelSlotWidget
{
  GtkDrawingArea         parent_instance;
  Channel                * channel; ///< the channel this belongs to
  int                    slot_index; ///< the channel slot index
} ChannelSlotWidget;

typedef struct ChannelSlotWidgetClass
{
  GtkDrawingAreaClass    parent_class;
} ChannelSlotWidgetClass;

/**
 * Creates a new ChannelSlot widget and binds it to the given value.
 */
ChannelSlotWidget *
channel_slot_widget_new (int slot_index, Channel * channel);

#endif
