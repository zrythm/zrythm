/*
 * Copyright (C) 2019, 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_VIEW_TOOLBAR_H__
#define __GUI_WIDGETS_VIEW_TOOLBAR_H__

#include <gtk/gtk.h>

#define VIEW_TOOLBAR_WIDGET_TYPE \
  (view_toolbar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ViewToolbarWidget,
  view_toolbar_widget,
  Z,
  VIEW_TOOLBAR_WIDGET,
  GtkBox)

#define MW_VIEW_TOOLBAR \
  MW_HEADER_NOTEBOOK->view_toolbar

typedef struct _ViewToolbarWidget
{
  GtkBox      parent_instance;
  GtkButton * status_bar;
  GtkButton * fullscreen;
  GtkButton * left_panel;
  GtkButton * bot_panel;
  GtkButton * top_panel;
  GtkButton * right_panel;
} ViewToolbarWidget;

#endif
