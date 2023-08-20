// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_region.h"
#include "dsp/midi_region.h"
#include "dsp/supported_file.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "io/file_import.h"
#include "io/midi_file.h"
#include "project.h"
#include "utils/error.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (FileImport, file_import, G_TYPE_OBJECT);

typedef enum
{
  Z_IO_FILE_IMPORT_ERROR_FAILED,
} ZIOFileImportError;

#define Z_IO_FILE_IMPORT_ERROR z_io_file_import_error_quark ()
GQuark
z_io_file_import_error_quark (void);
G_DEFINE_QUARK (
  z - io - file - import - error - quark,
  z_io_file_import_error)

/**
 * Returns a new FileImport instance.
 */
FileImport *
file_import_new (
  const char *           filepath,
  const FileImportInfo * import_nfo)
{
  g_return_val_if_fail (filepath, NULL);

  FileImport * self = g_object_new (FILE_IMPORT_TYPE, NULL);

  self->import_info = file_import_info_clone (import_nfo);
  self->filepath = g_strdup (filepath);

  return self;
}

static void
file_import_thread_func (
  GTask *        task,
  gpointer       source_object,
  gpointer       task_data,
  GCancellable * cancellable)
{
  FileImport * self = Z_FILE_IMPORT (task_data);

  TrackType       track_type = 0;
  SupportedFile * file =
    supported_file_new_from_path (self->filepath);
  if (
    supported_file_type_is_supported (file->type)
    && supported_file_type_is_audio (file->type))
    {
      track_type = TRACK_TYPE_AUDIO;
    }
  else if (supported_file_type_is_midi (file->type))
    {
      track_type = TRACK_TYPE_MIDI;
    }
  else
    {
      char * descr =
        supported_file_type_get_description (file->type);
      GError * err = NULL;
      g_set_error (
        &err, Z_IO_FILE_IMPORT_ERROR,
        Z_IO_FILE_IMPORT_ERROR_FAILED,
        _ ("Unsupported file type %s"), descr);
      g_free (descr);
      supported_file_free (file);
      g_task_return_error (task, err);
      return;
    }
  object_free_w_func_and_null (supported_file_free, file);

  int num_nonempty_midi_tracks = 0;
  if (track_type == TRACK_TYPE_MIDI)
    {
      num_nonempty_midi_tracks =
        midi_file_get_num_tracks (self->filepath, true);
      if (num_nonempty_midi_tracks == 0)
        {
          g_task_return_new_error (
            task, Z_IO_FILE_IMPORT_ERROR,
            Z_IO_FILE_IMPORT_ERROR_FAILED,
            _ ("The MIDI file at %s contains no data"),
            self->filepath);
          return;
        }
    }

  g_task_return_error_if_cancelled (task);
  if (g_task_had_error (task))
    return;

  Track * track = NULL;
  if (self->import_info->track_name_hash)
    {
      if (track_type == TRACK_TYPE_MIDI)
        {
          if (num_nonempty_midi_tracks > 1)
            {
              g_task_return_new_error (
                task, Z_IO_FILE_IMPORT_ERROR,
                Z_IO_FILE_IMPORT_ERROR_FAILED,
                _ ("This MIDI file contains %d "
                   "tracks. It cannot be dropped "
                   "into an existing track"),
                num_nonempty_midi_tracks);
              return;
            }
        }

      track = tracklist_find_track_by_name_hash (
        TRACKLIST, self->import_info->track_name_hash);
      if (!track)
        {
          g_task_return_new_error (
            task, Z_IO_FILE_IMPORT_ERROR,
            Z_IO_FILE_IMPORT_ERROR_FAILED,
            _ ("Failed to get track from hash %u"),
            self->import_info->track_name_hash);
          return;
        }
    }

  int lane_pos =
    self->import_info->lane >= 0
      ? self->import_info->lane
      : (
        !track || track->num_lanes == 1
          ? 0
          : track->num_lanes - 2);
  int idx_in_lane =
    track ? track->lanes[lane_pos]->num_regions : 0;
  switch (track_type)
    {
    case TRACK_TYPE_AUDIO:
      {
        GError *  err = NULL;
        ZRegion * region = audio_region_new (
          -1, self->filepath, true, NULL, 0, NULL, 0, 0,
          &self->import_info->pos,
          self->import_info->track_name_hash, lane_pos,
          idx_in_lane, &err);
        if (!region)
          {
            /*g_ptr_array_unref (regions);*/
            g_task_return_error (task, err);
            return;
          }
        g_ptr_array_add (self->regions, region);
      }
      break;
    case TRACK_TYPE_MIDI:
      for (int i = 0; i < num_nonempty_midi_tracks; i++)
        {
          ZRegion * region = midi_region_new_from_midi_file (
            &self->import_info->pos, self->filepath,
            self->import_info->track_name_hash, lane_pos,
            idx_in_lane, i);
          if (!region)
            {
              /*g_ptr_array_unref (regions);*/
              g_task_return_new_error (
                task, Z_IO_FILE_IMPORT_ERROR,
                Z_IO_FILE_IMPORT_ERROR_FAILED,
                _ ("Failed to create a MIDI region for file %s"),
                self->filepath);
              return;
            }
          g_ptr_array_add (self->regions, region);
        }
      break;
    default:
      break;
    }

  g_task_return_error_if_cancelled (task);
  if (g_task_had_error (task))
    {
      g_debug ("task was cancelled - return early");
      return;
    }

  g_debug (
    "returning %u region arrays from task (FileImport %p)...",
    self->regions->len, self);
  g_task_return_pointer (
    task, self->regions, (GDestroyNotify) g_ptr_array_unref);
}

/**
 * Begins file import for a single file.
 *
 * @param owner Passed to the task as the owner object. This is
 *   to avoid the task callback being called after the owner
 *   object is deleted.
 * @param callback_data User data to pass to the callback.
 */
void
file_import_async (
  FileImport *        self,
  GObject *           owner,
  GCancellable *      cancellable,
  GAsyncReadyCallback callback,
  gpointer            callback_data)
{
  g_message (
    "Starting an async file import operation for: %s",
    self->filepath);
  GTask * task =
    g_task_new (owner, cancellable, callback, callback_data);

  if (owner)
    {
      self->owner = owner;
    }
  g_task_set_return_on_cancel (task, false);
  g_task_set_check_cancellable (task, true);
  g_task_set_source_tag (task, file_import_async);
  g_task_set_task_data (task, self, NULL);
  g_task_run_in_thread (task, file_import_thread_func);
  g_object_unref (task);
}

GPtrArray *
file_import_sync (FileImport * self, GError ** error)
{
  g_message (
    "Starting a sync file import operation for: %s",
    self->filepath);
  GTask * task = g_task_new (NULL, NULL, NULL, NULL);

  g_task_set_return_on_cancel (task, false);
  g_task_set_check_cancellable (task, true);
  g_task_set_source_tag (task, file_import_sync);
  g_task_set_task_data (task, self, NULL);
  g_task_run_in_thread_sync (task, file_import_thread_func);
  g_return_val_if_fail (g_task_get_completed (task), NULL);
  GPtrArray * regions =
    file_import_finish (self, G_ASYNC_RESULT (task), error);
  g_object_unref (task);
  return regions;
}

/**
 * To be called by the provided GAsyncReadyCallback to retrieve
 * retun values and error details, passing the GAsyncResult
 * which was passed to the callback.
 *
 * @return A pointer array of regions. The caller is
 *   responsible for freeing the pointer array and the regions.
 */
GPtrArray *
file_import_finish (
  FileImport *   self,
  GAsyncResult * result,
  GError **      error)
{
  g_debug ("file_import_finish (FileImport %p)", self);
  g_return_val_if_fail (
    g_task_is_valid (result, self->owner), NULL);
  GPtrArray * regions_array =
    g_task_propagate_pointer ((GTask *) result, error);
  if (regions_array)
    {
      /* ownership is transferred to caller */
      self->regions = NULL;
    }
  return regions_array;
}

static void
file_import_dispose (GObject * obj)
{
  FileImport * self = Z_FILE_IMPORT (obj);

  g_debug ("disposing file import instance %p...", self);

  /*g_clear_object (&self->owner);*/

  G_OBJECT_CLASS (file_import_parent_class)->dispose (obj);
}

static void
file_import_finalize (GObject * obj)
{
  FileImport * self = Z_FILE_IMPORT (obj);

  g_debug ("finalizing file import instance %p...", self);

  g_free_and_null (self->filepath);
  object_free_w_func_and_null (
    file_import_info_free, self->import_info);
  object_free_w_func_and_null (
    g_ptr_array_unref, self->regions);

  G_OBJECT_CLASS (file_import_parent_class)->finalize (obj);
}

static void
file_import_class_init (FileImportClass * klass)
{
  GObjectClass * object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = file_import_dispose;
  object_class->finalize = file_import_finalize;
}

static void
file_import_init (FileImport * self)
{
  self->regions = g_ptr_array_new_with_free_func (
    (GDestroyNotify) arranger_object_free);
}
