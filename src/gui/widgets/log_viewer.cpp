/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "gui/widgets/log_viewer.h"
#include "project.h"
#include "utils/io.h"
#include "utils/log.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (LogViewerWidget, log_viewer_widget, GTK_TYPE_WINDOW)

static void
on_destroy (GtkWidget * widget, LogViewerWidget * self)
{
  LOG->viewer = NULL;
}

static void
result_func (
  GtkSourceFileLoader * loader,
  GAsyncResult *        res,
  LogViewerWidget *     self)
{
  gboolean success = FALSE;

  success = gtk_source_file_loader_load_finish (loader, res, NULL);
  g_return_if_fail (success);
}

/**
 * Creates a log viewer widget.
 */
LogViewerWidget *
log_viewer_widget_new (void)
{
  LogViewerWidget * self = static_cast<LogViewerWidget *> (
    g_object_new (LOG_VIEWER_WIDGET_TYPE, NULL));

  g_return_val_if_fail (LOG->log_filepath, self);

  GtkSourceBuffer * buf = gtk_source_buffer_new (NULL);
  self->src_view = GTK_SOURCE_VIEW (gtk_source_view_new_with_buffer (buf));
  GtkSourceFile * src_file = gtk_source_file_new ();
  GFile *         file = g_file_new_for_path (LOG->log_filepath);
  gtk_source_file_set_location (src_file, file);
  g_object_unref (file);
  GtkSourceFileLoader * file_loader = gtk_source_file_loader_new (buf, src_file);
  gtk_source_file_loader_load_async (
    file_loader, G_PRIORITY_DEFAULT, NULL, NULL, NULL, NULL,
    (GAsyncReadyCallback) result_func, self);
  gtk_widget_set_visible (GTK_WIDGET (self->src_view), true);

  gtk_box_append (GTK_BOX (self->source_view_box), GTK_WIDGET (self->src_view));

  g_signal_connect (G_OBJECT (self), "destroy", G_CALLBACK (on_destroy), NULL);

  return self;
}

static void
log_viewer_widget_class_init (LogViewerWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "log_viewer.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, LogViewerWidget, x)

  BIND_CHILD (source_view_box);
  BIND_CHILD (scrolled_win);
}

static void
log_viewer_widget_init (LogViewerWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_window_set_title (GTK_WINDOW (self), _ ("Log Viewer"));
  gtk_window_set_icon_name (GTK_WINDOW (self), "zrythm");
}
