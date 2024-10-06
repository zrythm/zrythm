// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/objects.h"
#include "common/utils/resources.h"
#include "common/utils/string.h"
#include "gui/backend/gtk_widgets/dialogs/string_entry_dialog.h"
#include "gui/backend/gtk_widgets/main_window.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

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
      const char * text = gtk_editable_get_text (GTK_EDITABLE (self->entry));

      self->setter (text);
    }
}

static void
on_entry_activate (GtkEntry * btn, StringEntryDialogWidget * self)
{
  adw_message_dialog_response (ADW_MESSAGE_DIALOG (self), "ok");
  gtk_window_close (GTK_WINDOW (self));
}

/**
 * Creates the popover.
 */
StringEntryDialogWidget *
string_entry_dialog_widget_new (
  const std::string  &label,
  GenericStringGetter getter,
  GenericStringSetter setter)
{
  StringEntryDialogWidget * self = static_cast<
    StringEntryDialogWidget *> (g_object_new (
    STRING_ENTRY_DIALOG_WIDGET_TYPE, "icon-name", "zrythm", "heading",
    label.c_str (), nullptr));

  self->getter = getter;
  self->setter = setter;

  gtk_window_set_transient_for (GTK_WINDOW (self), GTK_WINDOW (MAIN_WINDOW));

  /*gtk_label_set_text (self->label, label);*/

  /* setup text */
  gtk_editable_set_text (GTK_EDITABLE (self->entry), getter ().c_str ());

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
finalize (StringEntryDialogWidget * self)
{
  std::destroy_at (&self->getter);
  std::destroy_at (&self->setter);
}

static void
string_entry_dialog_widget_class_init (StringEntryDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "string_entry_dialog.ui");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child (klass, StringEntryDialogWidget, child)

  BIND_CHILD (entry);
  /*BIND_CHILD (label);*/

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;

#undef BIND_CHILD
}

static void
string_entry_dialog_widget_init (StringEntryDialogWidget * self)
{
  std::construct_at (&self->getter);
  std::construct_at (&self->setter);

  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect (
    G_OBJECT (self->entry), "activate", G_CALLBACK (on_entry_activate), self);
  g_signal_connect (G_OBJECT (self), "response", G_CALLBACK (on_response), self);
}
