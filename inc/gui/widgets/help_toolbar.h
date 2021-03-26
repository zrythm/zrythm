/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_HELP_TOOLBAR_H__
#define __GUI_WIDGETS_HELP_TOOLBAR_H__

#include <gtk/gtk.h>

#define HELP_TOOLBAR_WIDGET_TYPE \
  (help_toolbar_widget_get_type ())
G_DECLARE_FINAL_TYPE (HelpToolbarWidget,
                      help_toolbar_widget,
                      Z,
                      HELP_TOOLBAR_WIDGET,
                      GtkToolbar)

/**
 * \file
 */

#define MW_HELP_TOOLBAR \
  MW_HEADER_NOTEBOOK->help_toolbar

typedef struct _HelpToolbarWidget
{
  GtkToolbar         parent_instance;
  GtkToolButton *    chat;
  GtkToolButton *    manual;
  GtkToolButton *    shortcuts;
  GtkToolButton *     donate_btn;
  GtkToolButton *     report_a_bug_btn;
} HelpToolbarWidget;

#endif
