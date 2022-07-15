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
 */

#ifndef __GUI_WIDGETS_SNAP_BOX_H__
#define __GUI_WIDGETS_SNAP_BOX_H__

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define SNAP_BOX_WIDGET_TYPE (snap_box_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  SnapBoxWidget,
  snap_box_widget,
  Z,
  SNAP_BOX_WIDGET,
  GtkBox)

#define MW_SNAP_BOX MW_HOME_TOOLBAR->snap_box

typedef struct _SnapGridWidget SnapGridWidget;
typedef struct SnapGrid        SnapGrid;

typedef struct _SnapBoxWidget
{
  GtkBox            parent_instance;
  GtkToggleButton * snap_to_grid;
  GtkToggleButton * snap_to_grid_keep_offset;
  GtkToggleButton * snap_to_events;
  SnapGridWidget *  snap_grid;
} SnapBoxWidget;

void
snap_box_widget_refresh (SnapBoxWidget * self);

void
snap_box_widget_setup (SnapBoxWidget * self, SnapGrid * sg);

/**
 * @}
 */

#endif
