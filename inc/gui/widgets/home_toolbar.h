/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
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
  HomeToolbarWidget, home_toolbar_widget,
  Z, HOME_TOOLBAR_WIDGET, GtkToolbar)

typedef struct _ToolboxWidget ToolboxWidget;
typedef struct _ButtonWithMenuWidget
  ButtonWithMenuWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_HOME_TOOLBAR \
  MW_HEADER->home_toolbar

/**
 * The Home toolbar in the top.
 */
typedef struct _HomeToolbarWidget
{
  GtkToolbar         parent_instance;
  GtkButton *        undo_btn;
  ButtonWithMenuWidget * undo;
  GtkButton *        redo_btn;
  ButtonWithMenuWidget * redo;
  GtkToolButton *    cut;
  GtkToolButton *    copy;
  GtkToolButton *    paste;
  GtkToolButton *    duplicate;
  GtkToolButton *    delete;
  GtkToolButton *    clear_selection;
  GtkToolButton *    select_all;
  GtkToolButton *    loop_selection;
  ToolboxWidget *    toolbox;
} HomeToolbarWidget;

HomeToolbarWidget *
home_toolbar_widget_new ();

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
