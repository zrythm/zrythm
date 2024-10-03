// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_region.h"
#include "dsp/engine.h"
#include "dsp/midi_region.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/dialogs/file_import_progress_dialog.h"
#include "io/file_import.h"
#include "project.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  FileImportProgressDialog,
  file_import_progress_dialog,
  ADW_TYPE_MESSAGE_DIALOG);

static void
update_content (FileImportProgressDialog * self)
{
  adw_message_dialog_format_body (
    ADW_MESSAGE_DIALOG (self), _ ("Imported %d of %d files..."),
    self->num_files_total - self->num_files_remaining, self->num_files_total);
  if (self->num_files_remaining == 0)
    {
      adw_message_dialog_format_body (
        ADW_MESSAGE_DIALOG (self),
        _ ("Adding %d files to project... This may take a few seconds - please wait."),
        self->num_files_total);
      adw_message_dialog_set_response_enabled (
        ADW_MESSAGE_DIALOG (self), "cancel", false);
    }
}

static void
response_cb (
  AdwMessageDialog *         dialog,
  char *                     response,
  FileImportProgressDialog * self)
{
  g_debug ("response: %s", response);

  if (string_is_equal (response, "cancel"))
    {
      g_cancellable_cancel (self->cancellable);
      gtk_window_close (GTK_WINDOW (self));
    }
  else if (string_is_equal (response, "ok"))
    {
      gtk_window_close (GTK_WINDOW (self));
      return;
    }
  else if (string_is_equal (response, "close"))
    {
      return;
    }
  else
    {
      g_critical ("unknown response %s", response);
    }
}

static void
handle_regions (FileImportProgressDialog * self)
{
  GError * err = NULL;
  bool     success = tracklist_import_regions (
    self->region_arrays, self->import_info, self->tracks_ready_cb, &err);
  if (success)
    {
      ui_show_notification_idle_printf (
        _ ("Imported %d files"), self->num_files_total);
    }
  else
    {
      HANDLE_ERROR_LITERAL (err, _ ("Failed to import regions"));
    }
  adw_message_dialog_response (ADW_MESSAGE_DIALOG (self), "ok");
}

/**
 * Callback to provide to file_import_async().
 *
 * Called on the main thread when a file import finishes (either
 * done or cancelled).
 */
static void
import_async_ready_cb (GObject * source_object, GAsyncResult * res, gpointer data)
{
  FileImportProgressDialog * self =
    Z_FILE_IMPORT_PROGRESS_DIALOG (source_object);
  GError *     err = NULL;
  FileImport * fi = Z_FILE_IMPORT (data);
  GPtrArray *  regions = file_import_finish (fi, res, &err);
  self->num_files_remaining--;
  g_debug ("async ready: files remaining %d", self->num_files_remaining);
  if (regions)
    {
      g_message ("Imported regions for %s", fi->filepath);
      g_ptr_array_add (self->region_arrays, regions);
    }
  else
    {
      if (g_cancellable_is_cancelled (self->cancellable))
        {
          ui_show_notification_idle (err->message);
          g_error_free (err);
        }
      else
        {
          HANDLE_ERROR_LITERAL (err, _ ("Failed to import files"));
          g_cancellable_cancel (self->cancellable);
        }
    }
  update_content (self);

  if (
    self->num_files_remaining == 0
    && !g_cancellable_is_cancelled (self->cancellable))
    {
      g_idle_add_once ((GSourceOnceFunc) handle_regions, self);
    }
}

/**
 * Runs the dialog and imports each file asynchronously while
 * presenting progress info.
 */
void
file_import_progress_dialog_run (FileImportProgressDialog * self)
{
  int          i = 0;
  const char * filepath;
  while ((filepath = self->filepaths[i++]) != NULL)
    {
      FileImport * fi = file_import_new (filepath, self->import_info);
      g_ptr_array_add (self->file_imports, fi);
      file_import_async (
        fi, G_OBJECT (self), self->cancellable, import_async_ready_cb, fi);
    }

  gtk_window_present (GTK_WINDOW (self));
}

/**
 * Creates an instance of FileImportProgressDialog for the
 * given array of filepaths (NULL-delimited).
 */
FileImportProgressDialog *
file_import_progress_dialog_new (
  const char **       filepaths,
  FileImportInfo *    import_info,
  TracksReadyCallback tracks_ready_cb,
  GtkWidget *         parent)
{
  FileImportProgressDialog * self =
    g_object_new (FILE_IMPORT_PROGRESS_PROGRESS_DIALOG_TYPE, NULL);

  self->import_info = file_import_info_clone (import_info);
  self->filepaths = string_array_clone (filepaths);
  while (filepaths[self->num_files_total] != NULL)
    {
      self->num_files_total++;
      self->num_files_remaining++;
    }

  self->tracks_ready_cb = tracks_ready_cb;

  update_content (self);

  gtk_window_set_transient_for (
    GTK_WINDOW (self), parent ? GTK_WINDOW(parent) : GTK_WINDOW (UI_ACTIVE_WINDOW_OR_NULL));

  return self;
}

static void
dispose (GObject * obj)
{
  FileImportProgressDialog * self = Z_FILE_IMPORT_PROGRESS_DIALOG (obj);

  g_debug ("disposing import dialog...");

  object_free_w_func_and_null (g_ptr_array_unref, self->file_imports);
  g_clear_object (&self->cancellable);

  G_OBJECT_CLASS (file_import_progress_dialog_parent_class)->dispose (obj);
}

static void
finalize (GObject * obj)
{
  FileImportProgressDialog * self = Z_FILE_IMPORT_PROGRESS_DIALOG (obj);

  g_debug ("finalizing import dialog...");

  object_free_w_func_and_null (g_strfreev, self->filepaths);
  object_free_w_func_and_null (file_import_info_free, self->import_info);
  object_free_w_func_and_null (g_ptr_array_unref, self->region_arrays);

  G_OBJECT_CLASS (file_import_progress_dialog_parent_class)->finalize (obj);
}

static void
file_import_progress_dialog_class_init (FileImportProgressDialogClass * klass)
{
  GObjectClass * object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = dispose;
  object_class->finalize = finalize;
}

static void
file_import_progress_dialog_init (FileImportProgressDialog * self)
{
  self->cancellable = g_cancellable_new ();
  self->file_imports = g_ptr_array_new_with_free_func (g_object_unref);
  self->region_arrays =
    g_ptr_array_new_with_free_func ((GDestroyNotify) g_ptr_array_unref);

  adw_message_dialog_set_heading (ADW_MESSAGE_DIALOG (self), _ ("File Import"));
  adw_message_dialog_add_responses (
    ADW_MESSAGE_DIALOG (self), "cancel", _ ("_Cancel"), NULL);

  gtk_window_set_modal (GTK_WINDOW (self), true);

  g_signal_connect (self, "response", G_CALLBACK (response_cb), self);
}
