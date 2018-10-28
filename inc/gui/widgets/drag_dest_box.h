/*
 * gui/widgets/drag_dest_box.h - A dnd destination box used by mixer and tracklist
 *                                widgets
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

#ifndef __GUI_WIDGETS_DRAG_DEST_BOX_H__
#define __GUI_WIDGETS_DRAG_DEST_BOX_H__

#include <gtk/gtk.h>

#define DRAG_DEST_BOX_WIDGET_TYPE                  (drag_dest_box_widget_get_type ())
#define DRAG_DEST_BOX_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DRAG_DEST_BOX_WIDGET_TYPE, DragDestBoxWidget))
#define DRAG_DEST_BOX_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), DRAG_DEST_BOX_WIDGET, DragDestBoxWidgetClass))
#define IS_DRAG_DEST_BOX_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DRAG_DEST_BOX_WIDGET_TYPE))
#define IS_DRAG_DEST_BOX_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), DRAG_DEST_BOX_WIDGET_TYPE))
#define DRAG_DEST_BOX_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), DRAG_DEST_BOX_WIDGET_TYPE, DragDestBoxWidgetClass))

typedef struct Channel Channel;

typedef enum DragDestBoxType
{
  DRAG_DEST_BOX_TYPE_MIXER,
  DRAG_DEST_BOX_TYPE_TRACKLIST
} DragDestBoxType;

typedef struct DragDestBoxWidget
{
  GtkBox                  parent_instance;
  DragDestBoxType         type;
} DragDestBoxWidget;

typedef struct DragDestBoxWidgetClass
{
  GtkBoxClass             parent_class;
} DragDestBoxWidgetClass;

/**
 * Creates a drag destination box widget.
 */
DragDestBoxWidget *
drag_dest_box_widget_new (GtkOrientation  orientation,
                          int             spacing,
                          DragDestBoxType type);

#endif


