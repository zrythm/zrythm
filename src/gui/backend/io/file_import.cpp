// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/io/file_descriptor.h"
#include "gui/backend/io/file_import.h"
#include "gui/backend/io/midi_file.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/midi_region.h"
#include "structure/tracks/track.h"
#include "structure/tracks/tracklist.h"

// TODO
#if 0
G_DEFINE_TYPE (FileImport, file_import, G_TYPE_OBJECT);

typedef enum
{
  Z_IO_FILE_IMPORT_ERROR_FAILED,
} ZIOFileImportError;

#  define Z_IO_FILE_IMPORT_ERROR z_io_file_import_error_quark ()
GQuark
z_io_file_import_error_quark (void);
G_DEFINE_QUARK (
  z - io - file - import - error - quark,
  z_io_file_import_error)

/**
 * Returns a new FileImport instance.
 */
FileImport *
file_import_new (const std::string &filepath, const FileImportInfo * import_nfo)
{
  z_return_val_if_fail (!filepath.empty (), nullptr);

  auto * self =
    static_cast<FileImport *> (g_object_new (FILE_IMPORT_TYPE, nullptr));

  self->import_info = std::make_unique<FileImportInfo> (*import_nfo);
  self->filepath = filepath;

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

  Track::Type    track_type;
  FileDescriptor file = FileDescriptor (self->filepath);
  if (file.is_supported () && file.is_audio ())
    {
      track_type = Track::Type::Audio;
    }
  else if (file.is_midi ())
    {
      track_type = Track::Type::Midi;
    }
  else
    {
      auto     descr = FileDescriptor::get_type_description (file.type_);
      GError * err = NULL;
      g_set_error (
        &err, Z_IO_FILE_IMPORT_ERROR, Z_IO_FILE_IMPORT_ERROR_FAILED,
        QObject::tr ("Unsupported file type %s"), descr.c_str ());
      g_task_return_error (task, err);
      return;
    }

  int num_nonempty_midi_tracks = 0;
  if (track_type == Track::Type::Midi)
    {
      num_nonempty_midi_tracks = MidiFile (self->filepath).get_num_tracks (true);
      if (num_nonempty_midi_tracks == 0)
        {
          g_task_return_new_error (
            task, Z_IO_FILE_IMPORT_ERROR, Z_IO_FILE_IMPORT_ERROR_FAILED,
            QObject::tr ("The MIDI file at %s contains no data"),
            self->filepath.c_str ());
          return;
        }
    }

  g_task_return_error_if_cancelled (task);
  if (g_task_had_error (task))
    return;

  Track * track = NULL;
  if (self->import_info->track_name_hash_)
    {
      if (track_type == Track::Type::Midi)
        {
          if (num_nonempty_midi_tracks > 1)
            {
              g_task_return_new_error (
                task, Z_IO_FILE_IMPORT_ERROR, Z_IO_FILE_IMPORT_ERROR_FAILED,
                QObject::tr (
                  "This MIDI file contains %d "
                  "tracks. It cannot be dropped "
                  "into an existing track"),
                num_nonempty_midi_tracks);
              return;
            }
        }

      track = TRACKLIST->find_track_by_name_hash (
        self->import_info->track_name_hash_);
      if (!track)
        {
          g_task_return_new_error (
            task, Z_IO_FILE_IMPORT_ERROR, Z_IO_FILE_IMPORT_ERROR_FAILED,
            QObject::tr ("Failed to get track from hash %u"),
            self->import_info->track_name_hash_);
          return;
        }
    }

  std::visit (
    [&] (auto &&t) {
      using TrackT = base_type<decltype (t)>;
      using TrackLaneT = TrackT::LanedTrackImpl::TrackLaneType;
      int lane_pos =
        self->import_info->lane_ >= 0
          ? self->import_info->lane_
          : (!t || t->lanes_.size () == 1 ? 0 : t->lanes_.size () - 2);
      int idx_in_lane =
        t ? std::get<TrackLaneT *> (t->lanes_.at (lane_pos))->regions_.size ()
          : 0;
      switch (track_type)
        {
        case Track::Type::Audio:
          {
            try
              {
                self->regions.emplace_back (std::make_shared<AudioRegion> (
                  -1, self->filepath, true, nullptr, 0, std::nullopt, 0,
                  ENUM_INT_TO_VALUE (BitDepth, 0), self->import_info->pos_,
                  self->import_info->track_name_hash_, lane_pos, idx_in_lane));
              }
            catch (const ZrythmException &e)
              {
                g_task_return_new_error (
                  task, Z_IO_FILE_IMPORT_ERROR, Z_IO_FILE_IMPORT_ERROR_FAILED,
                  QObject::tr (
                    "Failed to create an audio region for file %s: %s"),
                  self->filepath.c_str (), e.what ());
                return;
              }
          }
          break;
        case Track::Type::Midi:
          for (int i = 0; i < num_nonempty_midi_tracks; i++)
            {
              try
                {
                  self->regions.emplace_back (std::make_shared<MidiRegion> (
                    self->import_info->pos_, self->filepath,
                    self->import_info->track_name_hash_, lane_pos, idx_in_lane,
                    i));
                }
              catch (const ZrythmException &e)
                {
                  g_task_return_new_error (
                    task, Z_IO_FILE_IMPORT_ERROR, Z_IO_FILE_IMPORT_ERROR_FAILED,
                    QObject::tr (
                      "Failed to create a MIDI region for file %s: %s"),
                    self->filepath.c_str (), e.what ());
                  return;
                }
            }
          break;
        default:
          break;
        }
    },
    convert_to_variant<LanedTrackPtrVariant> (track));

  g_task_return_error_if_cancelled (task);
  if (g_task_had_error (task))
    {
      z_debug ("task was cancelled - return early");
      return;
    }

  z_debug (
    "returning {} region arrays from task (FileImport {})...",
    self->regions.size (), fmt::ptr (self));
  g_task_return_pointer (
    task, &self->regions, (GDestroyNotify) g_ptr_array_unref);
}

void
file_import_async (
  FileImport *        self,
  GObject *           owner,
  GCancellable *      cancellable,
  GAsyncReadyCallback callback,
  gpointer            callback_data)
{
  z_debug ("Starting an async file import operation for: {}", self->filepath);
  GTask * task = g_task_new (owner, cancellable, callback, callback_data);

  if (owner)
    {
      self->owner = owner;
    }
  g_task_set_return_on_cancel (task, false);
  g_task_set_check_cancellable (task, true);
  g_task_set_source_tag (task, (gpointer) file_import_async);
  g_task_set_task_data (task, self, nullptr);
  g_task_run_in_thread (task, file_import_thread_func);
  g_object_unref (task);
}

std::vector<std::shared_ptr<Region>>
file_import_sync (FileImport * self, GError ** error)
{
  z_debug ("Starting a sync file import operation for: {}", self->filepath);
  GTask * task = g_task_new (nullptr, nullptr, nullptr, nullptr);

  g_task_set_return_on_cancel (task, false);
  g_task_set_check_cancellable (task, true);
  g_task_set_source_tag (task, (gpointer) file_import_sync);
  g_task_set_task_data (task, self, nullptr);
  g_task_run_in_thread_sync (task, file_import_thread_func);
  z_return_val_if_fail (
    g_task_get_completed (task), std::vector<std::shared_ptr<Region>> ());
  auto regions = file_import_finish (self, G_ASYNC_RESULT (task), error);
  g_object_unref (task);
  return regions;
}

std::vector<std::shared_ptr<Region>>
file_import_finish (FileImport * self, GAsyncResult * result, GError ** error)
{
  z_debug ("file_import_finish (FileImport {})", fmt::ptr (self));
  z_return_val_if_fail (
    g_task_is_valid (result, self->owner),
    std::vector<std::shared_ptr<Region>> ());
  auto * regions_array = static_cast<std::vector<std::shared_ptr<Region>> *> (
    g_task_propagate_pointer ((GTask *) result, error));
  return regions_array ? *regions_array : std::vector<std::shared_ptr<Region>> ();
}

static void
file_import_dispose (GObject * obj)
{
  FileImport * self = Z_FILE_IMPORT (obj);

  z_debug ("disposing file import instance {}...", fmt::ptr (self));

  /*g_clear_object (&self->owner);*/

  G_OBJECT_CLASS (file_import_parent_class)->dispose (obj);
}

static void
file_import_finalize (GObject * obj)
{
  FileImport * self = Z_FILE_IMPORT (obj);

  z_debug ("finalizing file import instance {}...", fmt::ptr (self));

  std::destroy_at (&self->regions);
  std::destroy_at (&self->filepath);
  std::destroy_at (&self->import_info);

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
  std::construct_at (&self->regions);
  std::construct_at (&self->filepath);
  std::construct_at (&self->import_info);
}

#endif
