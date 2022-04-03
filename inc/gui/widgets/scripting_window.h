/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Scripting window.
 */

#ifndef __GUI_WIDGETS_SCRIPTING_WINDOW_H__
#define __GUI_WIDGETS_SCRIPTING_WINDOW_H__

#include <gtk/gtk.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored \
  "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#pragma GCC diagnostic pop

#define SCRIPTING_WINDOW_WIDGET_TYPE \
  (scripting_window_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ScriptingWindowWidget,
  scripting_window_widget,
  Z,
  SCRIPTING_WINDOW_WIDGET,
  GtkWindow)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * The export dialog.
 */
typedef struct _ScriptingWindowWidget
{
  GtkWindow         parent_instance;
  GtkButton *       execute_btn;
  GtkLabel *        output;
  GtkViewport *     source_viewport;
  GtkSourceView *   editor;
  GtkSourceBuffer * buffer;
} ScriptingWindowWidget;

/**
 * Creates a bounce dialog.
 */
ScriptingWindowWidget *
scripting_window_widget_new (void);

/**
 * @}
 */

#endif
