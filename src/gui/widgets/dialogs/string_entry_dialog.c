// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_selections.h"
#include "dsp/marker.h"
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

G_DEFINE_TYPE (
  StringEntryDialogWidget,
  string_entry_dialog_widget,
  ADW_TYPE_MESSAGE_DIALOG)

/**
 * Closes the dialog, optionally saving.
 */
static void
on_response (
  AdwMessageDialog *        dialog,
  char *                    response,
  StringEntryDialogWidget * self)
{
  if (string_is_equal (response, "ok"))
    {
      const char * text =
        gtk_editable_get_text (GTK_EDITABLE (self->entry));

      self->setter (self->obj, text);
    }
}

static void
on_entry_activate (
  GtkEntry *                btn,
  StringEntryDialogWidget * self)
{
  adw_message_dialog_response (
    ADW_MESSAGE_DIALOG (self), "ok");
  gtk_window_close (GTK_WINDOW (self));
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
    "heading", label, NULL);

  self->obj = obj;
  self->getter = getter;
  self->setter = setter;

  gtk_window_set_transient_for (
    GTK_WINDOW (self), GTK_WINDOW (MAIN_WINDOW));

  /*gtk_label_set_text (self->label, label);*/

  /* setup text */
  gtk_editable_set_text (
    GTK_EDITABLE (self->entry), getter (obj));

  return self;
}

#if 0
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
#endif

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

  BIND_CHILD (entry);
  /*BIND_CHILD (label);*/

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
