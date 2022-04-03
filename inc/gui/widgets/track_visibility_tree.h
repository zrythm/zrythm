/*
 * Copyright (C) 2019, 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Track visibility list store.
 */

#ifndef __GUI_WIDGETS_TRACK_VISIBILITY_TREE_H__
#define __GUI_WIDGETS_TRACK_VISIBILITY_TREE_H__

#include <gtk/gtk.h>

#define TRACK_VISIBILITY_TREE_WIDGET_TYPE \
  (track_visibility_tree_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TrackVisibilityTreeWidget,
  track_visibility_tree_widget,
  Z,
  TRACK_VISIBILITY_TREE_WIDGET,
  GtkBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TRACK_VISIBILITY_TREE \
  MW_VISIBILITY->track_visibility

typedef struct _TrackVisibilityTreeWidget
{
  GtkBox parent_instance;

  GtkScrolledWindow * scroll;

  /* The tree views */
  GtkTreeView *  tree;
  GtkTreeModel * tree_model;
} TrackVisibilityTreeWidget;

/**
 * Refreshes the tree model.
 */
void
track_visibility_tree_widget_refresh (
  TrackVisibilityTreeWidget * self);

/**
 * Instantiates a new TrackVisibilityTreeWidget.
 */
TrackVisibilityTreeWidget *
track_visibility_tree_widget_new (void);

/**
 * @}
 */

#endif
