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

#ifndef __GUI_WIDGETS_HELP_TOOLBAR_H__
#define __GUI_WIDGETS_HELP_TOOLBAR_H__

#include <gtk/gtk.h>

#define HELP_TOOLBAR_WIDGET_TYPE (help_toolbar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  HelpToolbarWidget,
  help_toolbar_widget,
  Z,
  HELP_TOOLBAR_WIDGET,
  GtkBox)

/**
 * \file
 */

#define MW_HELP_TOOLBAR MW_HEADER_NOTEBOOK->help_toolbar

typedef struct _HelpToolbarWidget
{
  GtkBox      parent_instance;
  GtkButton * about;
  GtkButton * chat;
  GtkButton * manual;
  GtkButton * shortcuts;
  GtkButton * donate_btn;
  GtkButton * report_a_bug_btn;
} HelpToolbarWidget;

#endif
