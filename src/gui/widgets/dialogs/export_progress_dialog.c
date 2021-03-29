/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Export dialog.
 */

#include <stdio.h>

#include "audio/engine.h"
#include "audio/exporter.h"
#include "gui/widgets/dialogs/export_progress_dialog.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ExportProgressDialogWidget,
  export_progress_dialog_widget,
  GTK_TYPE_DIALOG)

static void
on_closed (
  GtkDialog *                  dialog,
  ExportProgressDialogWidget * self)
{
  self->info->cancelled = true;
}

static void
on_open_directory_clicked (
  GtkButton * btn,
  ExportProgressDialogWidget * self)
{
  char * dir = io_get_dir (self->info->file_uri);
  io_open_directory (dir);
  g_free (dir);
}

static void
on_ok_clicked (
  GtkButton * btn,
  ExportProgressDialogWidget * self)
{
  gtk_dialog_response (
    GTK_DIALOG (self), 0);
}

static void
on_cancel_clicked (
  GtkButton * btn,
  ExportProgressDialogWidget * self)
{
  self->info->cancelled = true;
}

static gboolean
tick_cb (
  GtkWidget *                  widget,
  GdkFrameClock *              frame_clock,
  ExportProgressDialogWidget * self)
{
  ExportSettings * info = self->info;

  gtk_progress_bar_set_fraction (
    GTK_PROGRESS_BAR (widget),
    info->progress);

  if (math_doubles_equal (info->progress, 1.0) ||
      info->has_error || info->cancelled)
    {
      if (self->autoclose || info->cancelled)
        {
          gtk_dialog_response (
            GTK_DIALOG (self), 0);
        }
      else
        {
          gtk_widget_set_visible (
            GTK_WIDGET (self->ok), true);
          gtk_widget_set_visible (
            GTK_WIDGET (self->cancel), false);
          gtk_widget_set_visible (
            GTK_WIDGET (self->open_directory),
            self->show_open_dir_btn);
          gtk_label_set_text (
            self->label, _("Exported"));
        }

      if (self->info->has_error)
        {
          ui_show_error_message (
            MAIN_WINDOW, self->info->error_str);
        }
      return G_SOURCE_REMOVE;
    }
  else
    {
      return G_SOURCE_CONTINUE;
    }
}

/**
 * Creates an export dialog widget and displays it.
 */
ExportProgressDialogWidget *
export_progress_dialog_widget_new (
  ExportSettings * info,
  bool             autoclose,
  bool             show_open_dir_btn,
  bool             cancelable)
{
  ExportProgressDialogWidget * self =
    g_object_new (
      EXPORT_PROGRESS_DIALOG_WIDGET_TYPE, NULL);

  self->info = info;
  self->autoclose = autoclose;
  self->show_open_dir_btn = show_open_dir_btn;

  if (cancelable)
    {
      gtk_widget_set_visible (
        GTK_WIDGET (self->cancel), true);
    }

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self->progress_bar),
    (GtkTickCallback) tick_cb, self, NULL);

  g_signal_connect (
    G_OBJECT (self), "close",
    G_CALLBACK (on_closed), self);

  return self;
}

static void
export_progress_dialog_widget_class_init (
  ExportProgressDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "export_progress_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ExportProgressDialogWidget, x)

  BIND_CHILD (label);
  BIND_CHILD (progress_bar);
  BIND_CHILD (open_directory);
  BIND_CHILD (ok);
  BIND_CHILD (cancel);
  gtk_widget_class_bind_template_callback (
    klass, on_open_directory_clicked);
  gtk_widget_class_bind_template_callback (
    klass, on_ok_clicked);
  gtk_widget_class_bind_template_callback (
    klass, on_cancel_clicked);
}

static void
export_progress_dialog_widget_init (
  ExportProgressDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_window_set_title (
    GTK_WINDOW (self),
    _("Export Progress"));
}
