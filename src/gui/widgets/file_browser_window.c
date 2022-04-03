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

#include "gui/widgets/file_browser.h"
#include "gui/widgets/file_browser_window.h"
#include "settings/settings.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  FileBrowserWindowWidget,
  file_browser_window_widget,
  GTK_TYPE_WINDOW)

FileBrowserWindowWidget *
file_browser_window_widget_new ()
{
  FileBrowserWindowWidget * self = g_object_new (
    FILE_BROWSER_WINDOW_WIDGET_TYPE, "title",
    _ ("File Browser"), NULL);

  self->file_browser = file_browser_widget_new ();
  GtkBox * box = GTK_BOX (
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_box_append (
    box, GTK_WIDGET (self->file_browser));
  gtk_window_set_child (
    GTK_WINDOW (self), GTK_WIDGET (box));

  return self;
}

static void
file_browser_window_widget_class_init (
  FileBrowserWindowWidgetClass * _klass)
{
}

static void
file_browser_window_widget_init (
  FileBrowserWindowWidget * self)
{
}
