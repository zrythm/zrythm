/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_VIEW_TOOLBAR_H__
#define __GUI_WIDGETS_VIEW_TOOLBAR_H__

#include <gtk/gtk.h>

#define VIEW_TOOLBAR_WIDGET_TYPE \
  (view_toolbar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ViewToolbarWidget,
  view_toolbar_widget,
  Z, VIEW_TOOLBAR_WIDGET,
  GtkToolbar)

/**
 * \file
 */

#define MW_VIEW_TOOLBAR \
  MW_HEADER_NOTEBOOK->view_toolbar

typedef struct _ViewToolbarWidget
{
  GtkToolbar         parent_instance;
  GtkToolButton * status_bar;
  GtkToolButton *    zoom_in;
  GtkToolButton *    zoom_out;
  GtkToolButton *    original_size;
  GtkToolButton *    best_fit;
  GtkToolButton *    fullscreen;
  GtkToolButton * left_panel;
  GtkToolButton * bot_panel;
  GtkToolButton * top_panel;
  GtkToolButton * right_panel;
} ViewToolbarWidget;

#endif
