// SPDX-FileCopyrightText: © 2022-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2016-2022 Christian Hergert <chergert@redhat.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ---
 */

#include "zrythm-config.h"

#include "gui/widgets/file_chooser_entry.h"

#include <glib/gi18n.h>

struct _IdeFileChooserEntry
{
  GtkWidget parent_class;

  GtkWidget * hbox;

  GtkEntry *  entry;
  GtkButton * button;

  GtkFileChooserDialog * dialog;
  GtkFileFilter *        filter;
  GFile *                file;
  gchar *                title;

  GtkFileChooserAction action;

  guint local_only : 1;
  guint create_folders : 1;
  guint do_overwrite_confirmation : 1;
  guint select_multiple : 1;
  guint show_hidden : 1;
};

enum
{
  PROP_0,
  PROP_ACTION,
  PROP_CREATE_FOLDERS,
  PROP_DO_OVERWRITE_CONFIRMATION,
  PROP_FILE,
  PROP_FILTER,
  PROP_LOCAL_ONLY,
  PROP_SHOW_HIDDEN,
  PROP_MAX_WIDTH_CHARS,
  PROP_TITLE,
  N_PROPS
};

G_DEFINE_TYPE (IdeFileChooserEntry, ide_file_chooser_entry, GTK_TYPE_WIDGET)

static GParamSpec * properties[N_PROPS];

static void
ide_file_chooser_entry_sync_to_dialog (IdeFileChooserEntry * self)
{
  GtkWidget * toplevel;
  GtkWidget * default_widget;

  g_assert (IDE_IS_FILE_CHOOSER_ENTRY (self));

  if (self->dialog == NULL)
    return;

  g_object_set (
    self->dialog, "action", self->action, "create-folders", self->create_folders,
    "do-overwrite-confirmation", self->do_overwrite_confirmation, "local-only",
    self->local_only, "show-hidden", self->show_hidden, "filter", self->filter,
    "title", self->title, NULL);

  if (self->file != NULL)
    gtk_file_chooser_set_file (
      GTK_FILE_CHOOSER (self->dialog), self->file, NULL);

  toplevel = GTK_WIDGET (gtk_widget_get_native (GTK_WIDGET (self)));

  if (GTK_IS_WINDOW (toplevel))
    gtk_window_set_transient_for (
      GTK_WINDOW (self->dialog), GTK_WINDOW (toplevel));

  default_widget = gtk_dialog_get_widget_for_response (
    GTK_DIALOG (self->dialog), GTK_RESPONSE_OK);

  switch (self->action)
    {
    case GTK_FILE_CHOOSER_ACTION_OPEN:
      gtk_button_set_label (GTK_BUTTON (default_widget), _ ("Open"));
      break;

    case GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
      gtk_button_set_label (GTK_BUTTON (default_widget), _ ("Select"));
      break;

    case GTK_FILE_CHOOSER_ACTION_SAVE:
      gtk_button_set_label (GTK_BUTTON (default_widget), _ ("Save"));
      break;

    default:
      break;
    }
}

static void
ide_file_chooser_entry_dialog_response (
  IdeFileChooserEntry *  self,
  gint                   response_id,
  GtkFileChooserDialog * dialog)
{
  g_assert (IDE_IS_FILE_CHOOSER_ENTRY (self));
  g_assert (GTK_IS_FILE_CHOOSER_DIALOG (dialog));

  if (response_id == GTK_RESPONSE_OK)
    {
      g_autoptr (GFile) file = NULL;

      file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      if (file != NULL)
        ide_file_chooser_entry_set_file (self, file);
    }

  self->dialog = NULL;

  gtk_window_destroy (GTK_WINDOW (dialog));
}

static void
ide_file_chooser_entry_ensure_dialog (IdeFileChooserEntry * self)
{
  g_assert (IDE_IS_FILE_CHOOSER_ENTRY (self));

  if (self->dialog == NULL)
    {
      self->dialog = g_object_new (
        GTK_TYPE_FILE_CHOOSER_DIALOG, "local-only", TRUE, "modal", TRUE, NULL);
      g_signal_connect_object (
        self->dialog, "response",
        G_CALLBACK (ide_file_chooser_entry_dialog_response), self,
        G_CONNECT_SWAPPED);
      gtk_dialog_add_buttons (
        GTK_DIALOG (self->dialog), _ ("Cancel"), GTK_RESPONSE_CANCEL,
        _ ("Open"), GTK_RESPONSE_OK, NULL);
      gtk_dialog_set_default_response (
        GTK_DIALOG (self->dialog), GTK_RESPONSE_OK);
    }

  ide_file_chooser_entry_sync_to_dialog (self);
}

static void
ide_file_chooser_entry_button_clicked (
  IdeFileChooserEntry * self,
  GtkButton *           button)
{
  g_assert (IDE_IS_FILE_CHOOSER_ENTRY (self));
  g_assert (GTK_IS_BUTTON (button));

  ide_file_chooser_entry_ensure_dialog (self);
  gtk_window_present (GTK_WINDOW (self->dialog));
}

static GFile *
file_expand (const gchar * path)
{
  g_autofree gchar * relative = NULL;
  g_autofree gchar * scheme = NULL;

  if (path == NULL)
    return g_file_new_for_path (g_get_home_dir ());

  scheme = g_uri_parse_scheme (path);
  if (scheme != NULL)
    return g_file_new_for_uri (path);

  if (g_path_is_absolute (path))
    return g_file_new_for_path (path);

  relative = g_build_filename (
    g_get_home_dir (), path[0] == '~' ? &path[1] : path, NULL);

  return g_file_new_for_path (relative);
}

static void
ide_file_chooser_entry_changed (IdeFileChooserEntry * self, GtkEntry * entry)
{
  g_autoptr (GFile) file = NULL;

  g_assert (IDE_IS_FILE_CHOOSER_ENTRY (self));
  g_assert (GTK_IS_ENTRY (entry));

  file = file_expand (gtk_editable_get_text (GTK_EDITABLE (entry)));

  if (g_set_object (&self->file, file))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FILE]);
}

static void
ide_file_chooser_entry_dispose (GObject * object)
{
  IdeFileChooserEntry * self = (IdeFileChooserEntry *) object;

  gtk_widget_unparent (GTK_WIDGET (self->hbox));

  if (self->dialog)
    {
      gtk_window_destroy (GTK_WINDOW (self->dialog));
      self->dialog = NULL;
    }

  G_OBJECT_CLASS (ide_file_chooser_entry_parent_class)->dispose (object);
}

static void
ide_file_chooser_entry_finalize (GObject * object)
{
  IdeFileChooserEntry * self = (IdeFileChooserEntry *) object;

  g_clear_object (&self->file);
  g_clear_object (&self->filter);
  g_clear_pointer (&self->title, g_free);

  G_OBJECT_CLASS (ide_file_chooser_entry_parent_class)->finalize (object);
}

static void
ide_file_chooser_entry_get_property (
  GObject *    object,
  guint        prop_id,
  GValue *     value,
  GParamSpec * pspec)
{
  IdeFileChooserEntry * self = IDE_FILE_CHOOSER_ENTRY (object);

  switch (prop_id)
    {
    case PROP_ACTION:
      g_value_set_enum (value, self->action);
      break;

    case PROP_LOCAL_ONLY:
      g_value_set_boolean (value, self->local_only);
      break;

    case PROP_CREATE_FOLDERS:
      g_value_set_boolean (value, self->create_folders);
      break;

    case PROP_DO_OVERWRITE_CONFIRMATION:
      g_value_set_boolean (value, self->do_overwrite_confirmation);
      break;

    case PROP_SHOW_HIDDEN:
      g_value_set_boolean (value, self->show_hidden);
      break;

    case PROP_FILTER:
      g_value_set_object (value, self->filter);
      break;

    case PROP_FILE:
      g_value_take_object (value, ide_file_chooser_entry_get_file (self));
      break;

    case PROP_MAX_WIDTH_CHARS:
      g_value_set_int (
        value, gtk_editable_get_max_width_chars (GTK_EDITABLE (self->entry)));
      break;

    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_file_chooser_entry_set_property (
  GObject *      object,
  guint          prop_id,
  const GValue * value,
  GParamSpec *   pspec)
{
  IdeFileChooserEntry * self = IDE_FILE_CHOOSER_ENTRY (object);

  switch (prop_id)
    {
    case PROP_ACTION:
      self->action = g_value_get_enum (value);
      break;

    case PROP_LOCAL_ONLY:
      self->local_only = g_value_get_boolean (value);
      break;

    case PROP_CREATE_FOLDERS:
      self->create_folders = g_value_get_boolean (value);
      break;

    case PROP_DO_OVERWRITE_CONFIRMATION:
      self->do_overwrite_confirmation = g_value_get_boolean (value);
      break;

    case PROP_SHOW_HIDDEN:
      self->show_hidden = g_value_get_boolean (value);
      break;

    case PROP_FILTER:
      g_clear_object (&self->filter);
      self->filter = g_value_dup_object (value);
      break;

    case PROP_FILE:
      ide_file_chooser_entry_set_file (self, g_value_get_object (value));
      break;

    case PROP_MAX_WIDTH_CHARS:
      gtk_editable_set_max_width_chars (
        GTK_EDITABLE (self->entry), g_value_get_int (value));
      break;

    case PROP_TITLE:
      g_free (self->title);
      self->title = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }

  ide_file_chooser_entry_sync_to_dialog (self);
}

static void
ide_file_chooser_entry_class_init (IdeFileChooserEntryClass * klass)
{
  GObjectClass *   object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass * widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = ide_file_chooser_entry_dispose;
  object_class->finalize = ide_file_chooser_entry_finalize;
  object_class->get_property = ide_file_chooser_entry_get_property;
  object_class->set_property = ide_file_chooser_entry_set_property;

  properties[PROP_ACTION] = g_param_spec_enum (
    "action", NULL, NULL, GTK_TYPE_FILE_CHOOSER_ACTION,
    GTK_FILE_CHOOSER_ACTION_OPEN, (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties[PROP_CREATE_FOLDERS] = g_param_spec_boolean (
    "create-folders", NULL, NULL, FALSE,
    (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties[PROP_DO_OVERWRITE_CONFIRMATION] = g_param_spec_boolean (
    "do-overwrite-confirmation", NULL, NULL, FALSE,
    (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties[PROP_LOCAL_ONLY] = g_param_spec_boolean (
    "local-only", NULL, NULL, FALSE,
    (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties[PROP_SHOW_HIDDEN] = g_param_spec_boolean (
    "show-hidden", NULL, NULL, FALSE,
    (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties[PROP_FILTER] = g_param_spec_object (
    "filter", NULL, NULL, GTK_TYPE_FILE_FILTER,
    (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties[PROP_FILE] = g_param_spec_object (
    "file", NULL, NULL, G_TYPE_FILE,
    (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties[PROP_MAX_WIDTH_CHARS] = g_param_spec_int (
    "max-width-chars", NULL, NULL, -1, G_MAXINT, -1,
    (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties[PROP_TITLE] = g_param_spec_string (
    "title", NULL, NULL, NULL, (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
ide_file_chooser_entry_init (IdeFileChooserEntry * self)
{
  self->hbox = g_object_new (
    GTK_TYPE_BOX, "orientation", GTK_ORIENTATION_HORIZONTAL, "visible", TRUE,
    NULL);
  gtk_widget_add_css_class (self->hbox, "linked");
  gtk_widget_set_parent (self->hbox, GTK_WIDGET (self));

  self->entry =
    g_object_new (GTK_TYPE_ENTRY, "hexpand", TRUE, "visible", TRUE, NULL);
  g_signal_connect_object (
    self->entry, "changed", G_CALLBACK (ide_file_chooser_entry_changed), self,
    G_CONNECT_SWAPPED);
  gtk_box_append (GTK_BOX (self->hbox), GTK_WIDGET (self->entry));

  self->button = g_object_new (
    GTK_TYPE_BUTTON, "label", _ ("Browse…"), "visible", TRUE, NULL);
  g_signal_connect_object (
    self->button, "clicked", G_CALLBACK (ide_file_chooser_entry_button_clicked),
    self, G_CONNECT_SWAPPED);
  gtk_box_append (GTK_BOX (self->hbox), GTK_WIDGET (self->button));
}

static gchar *
file_collapse (GFile * file)
{
  gchar * path = NULL;

  g_assert (!file || G_IS_FILE (file));

  if (file == NULL)
    return g_strdup ("");

  if (!g_file_is_native (file))
    return g_file_get_uri (file);

  path = g_file_get_path (file);

  if (path == NULL)
    return g_strdup ("");

  if (!g_path_is_absolute (path))
    {
      g_autofree gchar * freeme = path;

      path = g_build_filename (g_get_home_dir (), freeme, NULL);
    }

  if (g_str_has_prefix (path, g_get_home_dir ()))
    {
      g_autofree gchar * freeme = path;

      path = g_build_filename ("~", freeme + strlen (g_get_home_dir ()), NULL);
    }

  return path;
}

/**
 * ide_file_chooser_entry_get_file:
 *
 * Returns the currently selected file or %NULL if there is no selection.
 *
 * Returns: (nullable) (transfer full): A #GFile or %NULL.
 */
GFile *
ide_file_chooser_entry_get_file (IdeFileChooserEntry * self)
{
  g_return_val_if_fail (IDE_IS_FILE_CHOOSER_ENTRY (self), NULL);

  return self->file ? g_object_ref (self->file) : NULL;
}

void
ide_file_chooser_entry_set_file (IdeFileChooserEntry * self, GFile * file)
{
  g_autofree gchar * collapsed = NULL;

  g_return_if_fail (IDE_IS_FILE_CHOOSER_ENTRY (self));

  if (
    self->file == file
    || (self->file && file && g_file_equal (self->file, file)))
    return;

  if (file != NULL)
    g_object_ref (file);

  g_clear_object (&self->file);
  self->file = file;

  collapsed = file_collapse (file);
  gtk_editable_set_text (GTK_EDITABLE (self->entry), collapsed);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FILE]);
}

GtkWidget *
ide_file_chooser_entry_new (const gchar * title, GtkFileChooserAction action)
{
  return g_object_new (
    IDE_TYPE_FILE_CHOOSER_ENTRY, "title", title, "action", action, NULL);
}

/**
 * ide_file_chooser_entry_get_entry:
 * @self: a #IdeFileChooserEntry
 *
 * Gets the entry used by the #GtkEntry.
 *
 * Returns: (transfer none): a #GtkEntry
 */
GtkEntry *
ide_file_chooser_entry_get_entry (IdeFileChooserEntry * self)
{
  g_return_val_if_fail (IDE_IS_FILE_CHOOSER_ENTRY (self), NULL);

  return self->entry;
}
