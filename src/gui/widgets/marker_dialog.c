/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "actions/edit_marker_action.h"
#include "audio/marker.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker_dialog.h"
#include "gui/widgets/marker.h"
#include "project.h"
#include "utils/resources.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  MarkerDialogWidget,
  marker_dialog_widget,
  GTK_TYPE_DIALOG)

/**
 * Closes the dialog, optionally saving.
 */
static void
on_response (
  GtkDialog *          dialog,
  gint                 response,
  MarkerDialogWidget * self)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      const char * text =
        gtk_entry_get_text (self->entry);

      /* TODO validate, if false return */

      UndoableAction * ua =
        edit_marker_action_new (
          self->marker->marker, text);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }

  gtk_widget_destroy (GTK_WIDGET (self));
}

static void
on_entry_activate (
  GtkEntry * btn,
  MarkerDialogWidget * self)
{
  gtk_dialog_response (
    GTK_DIALOG (self),
    GTK_RESPONSE_ACCEPT);
}

/**
 * Creates the popover.
 */
MarkerDialogWidget *
marker_dialog_widget_new (
  MarkerWidget * owner)
{
  MarkerDialogWidget * self =
    g_object_new (
      MARKER_DIALOG_WIDGET_TYPE,
      NULL);

  self->marker =
    marker_get_main_marker (owner->marker)->widget;

  gtk_window_set_transient_for (
    GTK_WINDOW (self),
    GTK_WINDOW (MAIN_WINDOW));

  /* setup text */
  gtk_entry_set_text (
    self->entry,
    self->marker->marker->name);

  return self;
}

static void
marker_dialog_widget_class_init (
  MarkerDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "marker_dialog.ui");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    MarkerDialogWidget, \
    child)

  BIND_CHILD (ok);
  BIND_CHILD (cancel);
  BIND_CHILD (entry);

#undef BIND_CHILD
}

static void
marker_dialog_widget_init (
  MarkerDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect (
    G_OBJECT (self->entry), "activate",
    G_CALLBACK (on_entry_activate), self);
   g_signal_connect (
    G_OBJECT (self), "response",
    G_CALLBACK (on_response), self);
}
