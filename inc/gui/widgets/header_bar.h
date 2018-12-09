/*
 * gui/widgets/header_bar.h - Main window widget
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

#ifndef __GUI_WIDGETS_HEADER_BAR_H__
#define __GUI_WIDGETS_HEADER_BAR_H__

#include <gtk/gtk.h>

#define HEADER_BAR_WIDGET_TYPE                  (header_bar_widget_get_type ())
G_DECLARE_FINAL_TYPE (HeaderBarWidget,
                      header_bar_widget,
                      HEADER_BAR,
                      WIDGET,
                      GtkHeaderBar)
#define MW_HEADER_BAR MAIN_WINDOW->header_bar

typedef struct _HeaderBarWidget
{
  GtkHeaderBar             parent_instance;
  GtkMenuItem *            file;
  GtkImageMenuItem *       file_new;
  GtkImageMenuItem *       file_open;
  GtkImageMenuItem *       file_save;
  GtkImageMenuItem *       file_save_as;
  GtkImageMenuItem *       file_export;
  GtkImageMenuItem *       file_quit;
  GtkMenuItem *            edit;
  GtkMenuItem *            view;
  GtkMenuItem *            help;
  GtkImageMenuItem *       help_about;
  GtkBox *                 window_buttons;
  GtkButton *              minimize;
  GtkButton *              maximize;
  GtkButton *              close;
} HeaderBarWidget;

HeaderBarWidget *
header_bar_widget_new ();

void
header_bar_widget_set_title (HeaderBarWidget * self,
                             const char * title);

#endif
