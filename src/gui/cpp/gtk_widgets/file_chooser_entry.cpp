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

#include "gui/cpp/gtk_widgets/file_chooser_entry.h"

#include <glib/gi18n.h>

#include "common/utils/logger.h"

struct _IdeFileChooserEntry
{
  GtkWidget parent_class;

  GtkWidget * hbox;

  GtkEntry *  entry;
  GtkButton * button;

  GtkFileDialog * dialog;
  GtkFileFilter * filter;
  GFile *         file;
  gchar *         title;

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
  g_assert (IDE_IS_FILE_CHOOSER_ENTRY (self));

  if (self->dialog == nullptr)
    return;

#if 0
  GtkWidget * default_widget;
  g_object_set (
    self->dialog, "action", self->action, "create-folders", self->create_folders,
    "do-overwrite-confirmation", self->do_overwrite_confirmation, "local-only",
    self->local_only, "show-hidden", self->show_hidden, "filter", self->filter,
    "title", self->title, nullptr);
#endif

  if (self->file != nullptr)
    {
      if (self->action == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
        {
          gtk_file_dialog_set_initial_folder (
            GTK_FILE_DIALOG (self->dialog), self->file);
        }
      else
        {
          gtk_file_dialog_set_initial_file (
            GTK_FILE_DIALOG (self->dialog), self->file);
        }
    }
}

static void
ide_file_chooser_entry_dialog_async_cb (
  GtkFileDialog *       dialog,
  GAsyncResult *        res,
  IdeFileChooserEntry * self)
{
  g_assert (IDE_IS_FILE_CHOOSER_ENTRY (self));
  g_assert (GTK_IS_FILE_DIALOG (dialog));

  GFile * file = NULL;
  switch (self->action)
    {
    case GTK_FILE_CHOOSER_ACTION_OPEN:
      file = gtk_file_dialog_open_finish (dialog, res, nullptr);
      break;
    case GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
      file = gtk_file_dialog_select_folder_finish (dialog, res, nullptr);
      break;
    case GTK_FILE_CHOOSER_ACTION_SAVE:
      file = gtk_file_dialog_save_finish (dialog, res, nullptr);
      break;
    default:
      break;
    }

  if (file)
    {
      ide_file_chooser_entry_set_file (self, file);
    }
}

static void
ide_file_chooser_entry_ensure_dialog (IdeFileChooserEntry * self)
{
  g_assert (IDE_IS_FILE_CHOOSER_ENTRY (self));

  if (self->dialog == nullptr)
    {
      self->dialog = GTK_FILE_DIALOG (
        g_object_new (GTK_TYPE_FILE_DIALOG, "modal", TRUE, nullptr));
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

  GtkWidget * toplevel = GTK_WIDGET (gtk_widget_get_native (GTK_WIDGET (self)));
  GtkWindow * window = NULL;
  if (GTK_IS_WINDOW (toplevel))
    window = GTK_WINDOW (toplevel);
  switch (self->action)
    {
    case GTK_FILE_CHOOSER_ACTION_OPEN:
      gtk_file_dialog_open (
        self->dialog, window, nullptr,
        (GAsyncReadyCallback) ide_file_chooser_entry_dialog_async_cb, self);
      break;
    case GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
      gtk_file_dialog_select_folder (
        self->dialog, window, nullptr,
        (GAsyncReadyCallback) ide_file_chooser_entry_dialog_async_cb, self);
      break;
    case GTK_FILE_CHOOSER_ACTION_SAVE:
      gtk_file_dialog_save (
        self->dialog, window, nullptr,
        (GAsyncReadyCallback) ide_file_chooser_entry_dialog_async_cb, self);
      break;
    default:
      break;
    }
}

static GFile *
file_expand (const gchar * path)
{
  std::string relative;
  std::string scheme;

  if (path == nullptr)
    return g_file_new_for_path (g_get_home_dir ());

  scheme = Glib::uri_parse_scheme (path);
  if (!scheme.empty ())
    return g_file_new_for_uri (path);

  if (g_path_is_absolute (path))
    return g_file_new_for_path (path);

  relative =
    Glib::build_filename (g_get_home_dir (), path[0] == '~' ? &path[1] : path);

  return g_file_new_for_path (relative.c_str ());
}

static void
ide_file_chooser_entry_changed (IdeFileChooserEntry * self, GtkEntry * entry)
{
  GFile * file = NULL;

  g_assert (IDE_IS_FILE_CHOOSER_ENTRY (self));
  g_assert (GTK_IS_ENTRY (entry));

  file = file_expand (gtk_editable_get_text (GTK_EDITABLE (entry)));

  if (g_set_object (&self->file, file))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FILE]);

  if (file)
    {
      g_object_unref (file);
    }
}

static void
ide_file_chooser_entry_dispose (GObject * object)
{
  IdeFileChooserEntry * self = (IdeFileChooserEntry *) object;

  gtk_widget_unparent (GTK_WIDGET (self->hbox));

  if (self->dialog)
    {
      g_object_unref (self->dialog);
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
      self->action =
        static_cast<GtkFileChooserAction> (g_value_get_enum (value));
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
      self->filter = GTK_FILE_FILTER (g_value_dup_object (value));
      break;

    case PROP_FILE:
      ide_file_chooser_entry_set_file (
        self, G_FILE (g_value_get_object (value)));
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
    "action", nullptr, nullptr, GTK_TYPE_FILE_CHOOSER_ACTION,
    GTK_FILE_CHOOSER_ACTION_OPEN,
    (GParamFlags) ((int) G_PARAM_READWRITE | (int) G_PARAM_STATIC_STRINGS));

  properties[PROP_CREATE_FOLDERS] = g_param_spec_boolean (
    "create-folders", nullptr, nullptr, FALSE,
    (GParamFlags) ((int) G_PARAM_READWRITE | (int) G_PARAM_STATIC_STRINGS));

  properties[PROP_DO_OVERWRITE_CONFIRMATION] = g_param_spec_boolean (
    "do-overwrite-confirmation", nullptr, nullptr, FALSE,
    (GParamFlags) ((int) G_PARAM_READWRITE | (int) G_PARAM_STATIC_STRINGS));

  properties[PROP_LOCAL_ONLY] = g_param_spec_boolean (
    "local-only", nullptr, nullptr, FALSE,
    (GParamFlags) ((int) G_PARAM_READWRITE | (int) G_PARAM_STATIC_STRINGS));

  properties[PROP_SHOW_HIDDEN] = g_param_spec_boolean (
    "show-hidden", nullptr, nullptr, FALSE,
    (GParamFlags) ((int) G_PARAM_READWRITE | (int) G_PARAM_STATIC_STRINGS));

  properties[PROP_FILTER] = g_param_spec_object (
    "filter", nullptr, nullptr, GTK_TYPE_FILE_FILTER,
    (GParamFlags) ((int) G_PARAM_READWRITE | (int) G_PARAM_STATIC_STRINGS));

  properties[PROP_FILE] = g_param_spec_object (
    "file", nullptr, nullptr, G_TYPE_FILE,
    (GParamFlags) ((int) G_PARAM_READWRITE | (int) G_PARAM_STATIC_STRINGS));

  properties[PROP_MAX_WIDTH_CHARS] = g_param_spec_int (
    "max-width-chars", nullptr, nullptr, -1, G_MAXINT, -1,
    (GParamFlags) ((int) G_PARAM_READWRITE | (int) G_PARAM_STATIC_STRINGS));

  properties[PROP_TITLE] = g_param_spec_string (
    "title", nullptr, nullptr, nullptr,
    (GParamFlags) ((int) G_PARAM_READWRITE | (int) G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
ide_file_chooser_entry_init (IdeFileChooserEntry * self)
{
  self->hbox = GTK_WIDGET (g_object_new (
    GTK_TYPE_BOX, "orientation", GTK_ORIENTATION_HORIZONTAL, "visible", TRUE,
    nullptr));
  gtk_widget_add_css_class (self->hbox, "linked");
  gtk_widget_set_parent (self->hbox, GTK_WIDGET (self));

  self->entry = GTK_ENTRY (
    g_object_new (GTK_TYPE_ENTRY, "hexpand", TRUE, "visible", TRUE, nullptr));
  g_signal_connect_object (
    self->entry, "changed", G_CALLBACK (ide_file_chooser_entry_changed), self,
    G_CONNECT_SWAPPED);
  gtk_box_append (GTK_BOX (self->hbox), GTK_WIDGET (self->entry));

  self->button = GTK_BUTTON (g_object_new (
    GTK_TYPE_BUTTON, "label", _ ("Browse…"), "visible", TRUE, nullptr));
  g_signal_connect_object (
    self->button, "clicked", G_CALLBACK (ide_file_chooser_entry_button_clicked),
    self, G_CONNECT_SWAPPED);
  gtk_box_append (GTK_BOX (self->hbox), GTK_WIDGET (self->button));
}

static std::string
file_collapse (GFile * file)
{
  std::string path;

  g_assert (!file || G_IS_FILE (file));

  if (file == nullptr)
    return "";

  if (!g_file_is_native (file))
    return g_file_get_uri (file);

  char * path_gstr = g_file_get_path (file);
  path = path_gstr;
  g_free (path_gstr);

  if (path.empty ())
    return "";

  if (!Glib::path_is_absolute (path))
    {
      auto freeme = path;

      path = Glib::build_filename (g_get_home_dir (), freeme);
    }

  if (g_str_has_prefix (path.c_str (), g_get_home_dir ()))
    {
      auto freeme = path;

      path = Glib::build_filename (
        "~", freeme.c_str () + strlen (g_get_home_dir ()));
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
  z_return_val_if_fail (IDE_IS_FILE_CHOOSER_ENTRY (self), nullptr);

  return self->file ? g_object_ref (self->file) : NULL;
}

void
ide_file_chooser_entry_set_file (IdeFileChooserEntry * self, GFile * file)
{
  std::string collapsed;

  z_return_if_fail (IDE_IS_FILE_CHOOSER_ENTRY (self));

  if (
    self->file == file
    || (self->file && file && g_file_equal (self->file, file)))
    return;

  if (file != nullptr)
    g_object_ref (file);

  g_clear_object (&self->file);
  self->file = file;

  collapsed = file_collapse (file);
  gtk_editable_set_text (GTK_EDITABLE (self->entry), collapsed.c_str ());

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FILE]);
}

GtkWidget *
ide_file_chooser_entry_new (const gchar * title, GtkFileChooserAction action)
{
  return static_cast<GtkWidget *> (g_object_new (
    IDE_TYPE_FILE_CHOOSER_ENTRY, "title", title, "action", action, nullptr));
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
  z_return_val_if_fail (IDE_IS_FILE_CHOOSER_ENTRY (self), nullptr);

  return self->entry;
}
