/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
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
 */

#include "audio/engine.h"
#include "audio/engine_alsa.h"
#include "audio/engine_jack.h"
#include "audio/engine_pa.h"
#include "gui/widgets/dialogs/export_midi_file_dialog.h"
#include "gui/widgets/active_hardware_mb.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/localization.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (ExportMidiFileDialogWidget,
               export_midi_file_dialog_widget,
               GTK_TYPE_FILE_CHOOSER_DIALOG)

static void
on_export_clicked (
  GtkButton * widget,
  ExportMidiFileDialogWidget * self)
{
  gtk_dialog_response (
    GTK_DIALOG (self),
    GTK_RESPONSE_ACCEPT);
}

static void
on_cancel_clicked (
  GtkButton * widget,
  ExportMidiFileDialogWidget * self)
{
  gtk_dialog_response (
    GTK_DIALOG (self),
    GTK_RESPONSE_NONE);
}

/**
 * Creates a ExportMidiFileDialog.
 */
ExportMidiFileDialogWidget *
export_midi_file_dialog_widget_new_for_region (
  GtkWindow * parent,
  ZRegion *    region)
{
  ExportMidiFileDialogWidget * self =
    g_object_new (
      EXPORT_MIDI_FILE_DIALOG_WIDGET_TYPE,
      NULL);

  self->region = region;
  char * descr =
    g_strdup_printf (
      _("Exporting MIDI region \"%s\""),
      region->name);
  gtk_label_set_text (
    self->description,
    descr);
  g_free (descr);

  char * tmp =
    g_strdup_printf (
      "%s.mid", region->name);
  char * file =
    string_convert_to_filename (tmp);
  g_free (tmp);
  gtk_file_chooser_set_current_name (
    GTK_FILE_CHOOSER (self), file);
  g_free (file);

  gtk_window_set_transient_for (
    GTK_WINDOW (self),
    parent);

  return self;
}

static void
export_midi_file_dialog_widget_class_init (
  ExportMidiFileDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "export_midi_file_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    ExportMidiFileDialogWidget, \
    x)
#define BIND_CALLBACK(x) \
  gtk_widget_class_bind_template_callback ( \
    klass, \
    x)

  BIND_CHILD (description);

  BIND_CALLBACK (on_export_clicked);
  BIND_CALLBACK (on_cancel_clicked);

#undef BIND_CHILD
}

static void
export_midi_file_dialog_widget_init (
  ExportMidiFileDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

