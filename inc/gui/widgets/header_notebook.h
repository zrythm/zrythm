/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_HEADER_NOTEBOOK_H__
#define __GUI_WIDGETS_HEADER_NOTEBOOK_H__

#include <gtk/gtk.h>

#define HEADER_NOTEBOOK_WIDGET_TYPE \
  (header_notebook_widget_get_type ())
G_DECLARE_FINAL_TYPE (HeaderNotebookWidget,
                      header_notebook_widget,
                      Z,
                      HEADER_NOTEBOOK_WIDGET,
                      GtkNotebook)

/**
 * \file
 *
 * Header notebook.
 */

#define MW_HEADER_NOTEBOOK MAIN_WINDOW->header_notebook

typedef struct _HomeToolbarWidget HomeToolbarWidget;
typedef struct _ProjectToolbarWidget
  ProjectToolbarWidget;
typedef struct _ViewToolbarWidget ViewToolbarWidget;
typedef struct _HelpToolbarWidget HelpToolbarWidget;

/**
 * Header notebook to be used at the very top of the
 * main window.
 */
typedef struct _HeaderNotebookWidget
{
  GtkNotebook         parent_instance;

  /** Notebook toolbars. */
  HomeToolbarWidget * home_toolbar;
  ProjectToolbarWidget * project_toolbar;
  ViewToolbarWidget * view_toolbar;
  HelpToolbarWidget * help_toolbar;
  GtkToolButton * left_panel;
  GtkToolButton * bot_panel;
  GtkToolButton * top_panel;
  GtkToolButton * right_panel;

  GtkToolButton *     preferences;
  GtkToolButton *     z_icon;
  GtkLabel *          prj_name_label;
} HeaderNotebookWidget;

HeaderNotebookWidget *
header_notebook_widget_new ();

void
header_notebook_widget_setup (
  HeaderNotebookWidget * self,
  const char * title);

void
header_notebook_widget_set_subtitle (
  HeaderNotebookWidget * self,
  const char * subtitle);

#endif
