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

/**
 * \file
 *
 * Export dialog.
 */

#include "audio/engine.h"
#include "audio/exporter.h"
#include "gui/widgets/export_progress_dialog.h"
#include "project.h"
#include "utils/io.h"
#include "utils/resources.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ExportProgressDialogWidget,
  export_progress_dialog_widget,
  GTK_TYPE_DIALOG)

static void
on_open_directory_clicked (
  GtkButton * btn,
  ExportProgressDialogWidget * self)
{
  /* TODO */
}

static void
on_ok_clicked (
  GtkButton * btn,
  ExportProgressDialogWidget * self)
{
  gtk_dialog_response (
    GTK_DIALOG (self), 0);
}

static gboolean
tick_cb (
  GtkWidget *                  widget,
  GdkFrameClock *              frame_clock,
  ExportProgressDialogWidget * self)
{
  gtk_progress_bar_set_fraction (
    GTK_PROGRESS_BAR (widget),
    self->info->progress);

  if (self->info->progress == 1.0)
    {
      gtk_widget_set_visible (
        GTK_WIDGET (self->ok), 1);
      gtk_widget_set_visible (
        GTK_WIDGET (self->open_directory), 1);
      gtk_label_set_text (
        self->label, _("Exported"));
      return G_SOURCE_REMOVE;
    }
  else
    {
      return G_SOURCE_CONTINUE;
    }
}

/**
 * Creates a new export dialog.
 */
ExportProgressDialogWidget *
export_progress_dialog_widget_new (
  ExportSettings * info)
{
  ExportProgressDialogWidget * self =
    g_object_new (
      EXPORT_PROGRESS_DIALOG_WIDGET_TYPE, NULL);

  self->info = info;

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self->progress_bar),
    (GtkTickCallback) tick_cb,
    self,
    NULL);

  return self;
}

static void
export_progress_dialog_widget_class_init (
  ExportProgressDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "export_progress_dialog.ui");

  gtk_widget_class_bind_template_child (
    klass,
    ExportProgressDialogWidget,
    label);
  gtk_widget_class_bind_template_child (
    klass,
    ExportProgressDialogWidget,
    progress_bar);
  gtk_widget_class_bind_template_child (
    klass,
    ExportProgressDialogWidget,
    open_directory);
  gtk_widget_class_bind_template_child (
    klass,
    ExportProgressDialogWidget,
    ok);
  gtk_widget_class_bind_template_callback (
    klass,
    on_open_directory_clicked);
  gtk_widget_class_bind_template_callback (
    klass,
    on_ok_clicked);
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

