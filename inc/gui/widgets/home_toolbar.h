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
 * Home toolbar.
 */

#ifndef __GUI_WIDGETS_HOME_TOOLBAR_H__
#define __GUI_WIDGETS_HOME_TOOLBAR_H__

#include <gtk/gtk.h>

#define HOME_TOOLBAR_WIDGET_TYPE \
  (home_toolbar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  HomeToolbarWidget,
  home_toolbar_widget,
  Z,
  HOME_TOOLBAR_WIDGET,
  GtkBox)

typedef struct _ToolboxWidget ToolboxWidget;
typedef struct _ButtonWithMenuWidget
  ButtonWithMenuWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_HOME_TOOLBAR MW_HEADER->home_toolbar

/**
 * The Home toolbar in the top.
 */
typedef struct _HomeToolbarWidget
{
  GtkBox                 parent_instance;
  GtkButton *            undo_btn;
  ButtonWithMenuWidget * undo;
  GtkButton *            redo_btn;
  ButtonWithMenuWidget * redo;
  GtkButton *            cut;
  GtkButton *            copy;
  GtkButton *            paste;
  GtkButton *            duplicate;
  GtkButton * delete;
  GtkButton *     nudge_left;
  GtkButton *     nudge_right;
  GtkButton *     clear_selection;
  GtkButton *     select_all;
  GtkButton *     loop_selection;
  ToolboxWidget * toolbox;
} HomeToolbarWidget;

void
home_toolbar_widget_refresh_undo_redo_buttons (
  HomeToolbarWidget * self);

void
home_toolbar_widget_setup (
  HomeToolbarWidget * self);

/**
 * @}
 */

#endif
