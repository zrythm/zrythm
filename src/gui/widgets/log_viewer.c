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

#include "gui/widgets/log_viewer.h"
#include "project.h"
#include "utils/io.h"
#include "utils/log.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  LogViewerWidget,
  log_viewer_widget,
  GTK_TYPE_WINDOW)

static void
on_destroy (
  GtkWidget * widget,
  LogViewerWidget * self)
{
  LOG->viewer = NULL;
}

/**
 * Creates a log viewer widget.
 */
LogViewerWidget *
log_viewer_widget_new (void)
{
  LogViewerWidget * self =
    g_object_new (
      LOG_VIEWER_WIDGET_TYPE, NULL);

  g_return_val_if_fail (
    GTK_IS_TEXT_BUFFER (LOG->messages_buf), NULL);
  gtk_text_view_set_buffer (
    self->text_view, LOG->messages_buf);

  g_signal_connect (
    G_OBJECT (self), "destroy",
    G_CALLBACK (on_destroy), NULL);

  return self;
}

static void
log_viewer_widget_class_init (
  LogViewerWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "log_viewer.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, LogViewerWidget, x)

  BIND_CHILD (text_view);
  BIND_CHILD (scrolled_win);
}

static void
log_viewer_widget_init (
  LogViewerWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_window_set_title (
    GTK_WINDOW (self), _("Log Viewer"));
  gtk_window_set_icon_name (
    GTK_WINDOW (self), "zrythm");
}

