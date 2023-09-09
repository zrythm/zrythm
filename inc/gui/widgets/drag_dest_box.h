/*
 * Copyright (C) 2018-2019, 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * @file
 *
 * Drag dest box.
 */

#ifndef __GUI_WIDGETS_DRAG_DEST_BOX_H__
#define __GUI_WIDGETS_DRAG_DEST_BOX_H__

#include <gtk/gtk.h>

typedef struct Channel Channel;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define DRAG_DEST_BOX_WIDGET_TYPE (drag_dest_box_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  DragDestBoxWidget,
  drag_dest_box_widget,
  Z,
  DRAG_DEST_BOX_WIDGET,
  GtkBox)
#define TRACKLIST_DRAG_DEST_BOX MW_TRACKLIST->ddbox
#define MIXER_DRAG_DEST_BOX MW_MIXER->ddbox

typedef enum DragDestBoxType
{
  DRAG_DEST_BOX_TYPE_MIXER,
  DRAG_DEST_BOX_TYPE_TRACKLIST,
  DRAG_DEST_BOX_TYPE_MODULATORS,
} DragDestBoxType;

/**
 * DnD destination box used by mixer and tracklist
 * widgets.
 */
typedef struct _DragDestBoxWidget
{
  GtkBox            parent_instance;
  GtkGestureDrag *  drag;
  GtkGestureClick * click;
  GtkGestureClick * right_click;
  DragDestBoxType   type;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
} DragDestBoxWidget;

/**
 * Creates a drag destination box widget.
 */
DragDestBoxWidget *
drag_dest_box_widget_new (
  GtkOrientation  orientation,
  int             spacing,
  DragDestBoxType type);

/**
 * @}
 */

#endif
