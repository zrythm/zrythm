/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/arranger_selections.h"
#include "audio/marker.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/dialogs/string_entry_dialog.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  StringEntryDialogWidget,
  string_entry_dialog_widget,
  GTK_TYPE_DIALOG)

/**
 * Closes the dialog, optionally saving.
 */
static void
on_response (
  GtkDialog *               dialog,
  gint                      response,
  StringEntryDialogWidget * self)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      const char * text =
        gtk_editable_get_text (GTK_EDITABLE (self->entry));

      self->setter (self->obj, text);
    }

  gtk_window_destroy (GTK_WINDOW (self));
}

static void
on_entry_activate (
  GtkEntry *                btn,
  StringEntryDialogWidget * self)
{
  gtk_dialog_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);
}

/**
 * Creates the popover.
 */
StringEntryDialogWidget *
string_entry_dialog_widget_new (
  const char *        label,
  void *              obj,
  GenericStringGetter getter,
  GenericStringSetter setter)
{
  StringEntryDialogWidget * self = g_object_new (
    STRING_ENTRY_DIALOG_WIDGET_TYPE, "icon-name", "zrythm",
    "title", _ ("Please enter a value"), NULL);

  self->obj = obj;
  self->getter = getter;
  self->setter = setter;

  gtk_window_set_transient_for (
    GTK_WINDOW (self), GTK_WINDOW (MAIN_WINDOW));

  gtk_label_set_text (self->label, label);

  /* setup text */
  gtk_editable_set_text (
    GTK_EDITABLE (self->entry), getter (obj));

  return self;
}

static const char *
str_get (char ** str)
{
  if (*str)
    {
      return *str;
    }
  else
    {
      return "";
    }
}

static void
str_set (char ** str, const char * in_str)
{
  if (*str)
    {
      g_free_and_null (*str);
    }

  if (strlen (in_str) > 0)
    {
      *str = g_strdup (in_str);
    }
}

/**
 * Runs a new dialog and returns the string, or
 * NULL if cancelled.
 */
char *
string_entry_dialog_widget_new_return_string (
  const char * label)
{
  char *                    str = NULL;
  StringEntryDialogWidget * self =
    string_entry_dialog_widget_new (
      label, &str, (GenericStringGetter) str_get,
      (GenericStringSetter) str_set);
  z_gtk_dialog_run (GTK_DIALOG (self), true);

  return str;
}

static void
string_entry_dialog_widget_class_init (
  StringEntryDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "string_entry_dialog.ui");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child ( \
    klass, StringEntryDialogWidget, child)

  BIND_CHILD (ok);
  BIND_CHILD (cancel);
  BIND_CHILD (entry);
  BIND_CHILD (label);

#undef BIND_CHILD
}

static void
string_entry_dialog_widget_init (
  StringEntryDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect (
    G_OBJECT (self->entry), "activate",
    G_CALLBACK (on_entry_activate), self);
  g_signal_connect (
    G_OBJECT (self), "response", G_CALLBACK (on_response),
    self);
}
