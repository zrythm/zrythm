// SPDX-FileCopyrightText: © 2019, 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
file_browser_window_widget_new (void)
{
  FileBrowserWindowWidget * self = g_object_new (
    FILE_BROWSER_WINDOW_WIDGET_TYPE, "title", _ ("File Browser"), NULL);

  self->file_browser = file_browser_widget_new ();
  GtkBox * box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_box_append (box, GTK_WIDGET (self->file_browser));
  gtk_window_set_child (GTK_WINDOW (self), GTK_WIDGET (box));

  return self;
}

static void
file_browser_window_widget_class_init (FileBrowserWindowWidgetClass * _klass)
{
}

static void
file_browser_window_widget_init (FileBrowserWindowWidget * self)
{
}
