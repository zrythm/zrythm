// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/audio_region.h"
#include "common/dsp/audio_track.h"
#include "common/dsp/clip.h"
#include "common/dsp/engine.h"
#include "common/dsp/tempo_track.h"
#include "common/dsp/tracklist.h"
#include "common/io/audio_file.h"
#include "common/utils/audio.h"
#include "common/utils/debug.h"
#include "common/utils/dsp.h"
#include "common/utils/exceptions.h"
#include "common/utils/flags.h"
#include "common/utils/gtest_wrapper.h"
#include "common/utils/hash.h"
#include "common/utils/io.h"
#include "common/utils/logger.h"
#include "common/utils/objects.h"
#include "gui/backend/backend/project.h"

#include <fmt/printf.h>

using namespace zrythm;

void
AudioClip::update_channel_caches (size_t start_from)
{
  auto num_channels = get_num_channels ();
  z_return_if_fail_cmp (num_channels, >, 0);
  z_return_if_fail_cmp (num_frames_, >, 0);

  /* copy the frames to the channel caches */
  ch_frames_.setSize (num_channels, num_frames_, true, false, false);
  const auto frames_read_ptr =
    frames_.getReadPointer (0, start_from * num_channels);
  auto frames_to_write = num_frames_ - start_from;
  for (decltype (num_channels) i = 0; i < num_channels; ++i)
    {
      auto ch_frames_write_ptr = ch_frames_.getWritePointer (i, start_from);
      for (decltype (frames_to_write) j = 0; j < frames_to_write; ++j)
        {
          ch_frames_write_ptr[j] = frames_read_ptr[j * num_channels + i];
        }
    }
}

void
AudioClip::init_from_file (const std::string &full_path, bool set_bpm)
{
  samplerate_ = (int) AUDIO_ENGINE->sample_rate_;
  z_return_if_fail (samplerate_ > 0);

  /* read metadata */
  AudioFile file (full_path);
  try
    {
      file.read_metadata ();
    }
  catch (ZrythmException &e)
    {
      throw ZrythmException (
        fmt::format ("Failed to read metadata from file '{}'", full_path));
    }
  num_frames_ = file.metadata_.num_frames;
  channels_ = file.metadata_.channels;
  bit_depth_ = audio_bit_depth_int_to_enum (file.metadata_.bit_depth);
  bpm_ = file.metadata_.bpm;

  try
    {
      /* read frames into project's samplerate */
      file.read_full (ch_frames_, samplerate_);
    }
  catch (ZrythmException &e)
    {
      throw ZrythmException (
        fmt::format ("Failed to read frames from file '{}'", full_path));
    }
  num_frames_ = ch_frames_.getNumSamples ();
  channels_ = ch_frames_.getNumChannels ();

  name_ = juce::File (full_path).getFileNameWithoutExtension ().toStdString ();
  if (set_bpm)
    {
      z_return_if_fail (PROJECT && P_TEMPO_TRACK);
      bpm_ = P_TEMPO_TRACK->get_current_bpm ();
    }
  use_flac_ = use_flac (bit_depth_);

  /* interleave into frames_ */
  frames_ = ch_frames_;
  AudioFile::interleave_buffer (frames_);
}

void
AudioClip::init_loaded ()
{
  std::string filepath =
    get_path_in_pool_from_name (name_, use_flac_, F_NOT_BACKUP);

  bpm_t bpm = bpm_;
  try
    {
      init_from_file (filepath, false);
    }
  catch (ZrythmException &e)
    {
      throw ZrythmException (
        fmt::format ("Failed to initialize audio file: {}", e.what ()));
    }
  bpm_ = bpm;
}

AudioClip::AudioClip (const std::string &full_path)
{
  init_from_file (full_path, false);

  pool_id_ = -1;
  bpm_ = P_TEMPO_TRACK->get_current_bpm ();
}

AudioClip::AudioClip (
  const float *          arr,
  const unsigned_frame_t nframes,
  const channels_t       channels,
  BitDepth               bit_depth,
  const std::string     &name)
{
  z_return_if_fail (channels > 0);
  frames_.setSize (1, nframes * channels, true, false, false);
  num_frames_ = nframes;
  channels_ = channels;
  samplerate_ = (int) AUDIO_ENGINE->sample_rate_;
  z_return_if_fail (samplerate_ > 0);
  name_ = name;
  bit_depth_ = bit_depth;
  use_flac_ = use_flac (bit_depth);
  pool_id_ = -1;
  dsp_copy (
    frames_.getWritePointer (0), arr, (size_t) nframes * (size_t) channels);
  bpm_ = P_TEMPO_TRACK->get_current_bpm ();
  update_channel_caches (0);
}

AudioClip::AudioClip (
  const channels_t       channels,
  const unsigned_frame_t nframes,
  const std::string     &name)
{
  channels_ = channels;
  frames_.setSize (nframes * channels, 1, true, false, false);
  num_frames_ = nframes;
  name_ = name;
  pool_id_ = -1;
  bpm_ = P_TEMPO_TRACK->get_current_bpm ();
  samplerate_ = (int) AUDIO_ENGINE->sample_rate_;
  bit_depth_ = BitDepth::BIT_DEPTH_32;
  use_flac_ = false;
  z_return_if_fail (samplerate_ > 0);
  dsp_fill (
    frames_.getWritePointer (0), DENORMAL_PREVENTION_VAL (AUDIO_ENGINE),
    (size_t) nframes * (size_t) channels_);
  update_channel_caches (0);
}

void
AudioClip::init_after_cloning (const AudioClip &other)
{
  name_ = other.name_;
  frames_ = other.frames_;
  ch_frames_ = other.ch_frames_;
  bpm_ = other.bpm_;
  samplerate_ = other.samplerate_;
  bit_depth_ = other.bit_depth_;
  use_flac_ = other.use_flac_;
  pool_id_ = other.pool_id_;
  file_hash_ = other.file_hash_;
  num_frames_ = other.num_frames_;
  channels_ = other.channels_;
}

std::string
AudioClip::get_path_in_pool_from_name (
  const std::string &name,
  bool               use_flac,
  bool               is_backup)
{
  auto prj_pool_dir = PROJECT->get_path (ProjectPath::POOL, is_backup);
  if (!utils::io::path_exists (prj_pool_dir))
    {
      z_error ("{} does not exist", prj_pool_dir);
      return "";
    }
  std::string basename =
    utils::io::file_strip_ext (name) + (use_flac ? ".FLAC" : ".wav");
  return prj_pool_dir / basename;
}

std::string
AudioClip::get_path_in_pool (bool is_backup) const
{
  return get_path_in_pool_from_name (name_, use_flac_, is_backup);
}

void
AudioClip::write_to_pool (bool parts, bool is_backup)
{
  AudioClip * pool_clip = AUDIO_POOL->get_clip (pool_id_);
  z_return_if_fail (pool_clip == this);

  AUDIO_POOL->print ();
  z_debug ("attempting to write clip {} ({}) to pool...", name_, pool_id_);

  /* generate a copy of the given filename in the project dir */
  std::string path_in_main_project = get_path_in_pool (F_NOT_BACKUP);
  std::string new_path = get_path_in_pool (is_backup);
  z_return_if_fail (!path_in_main_project.empty ());
  z_return_if_fail (!new_path.empty ());

  /* whether a new write is needed */
  bool need_new_write = true;

  /* skip if file with same hash already exists */
  if (utils::io::path_exists (new_path) && !parts)
    {
      bool same_hash =
        !file_hash_.empty ()
        && file_hash_ == hash_get_from_file (new_path, HASH_ALGORITHM_XXH3_64);

      if (same_hash)
        {
          z_debug ("skipping writing to existing clip {} in pool", new_path);
          need_new_write = false;
        }
    }

  /* if writing to backup and same file exists in main project dir, copy (first
   * try reflink) */
  if (need_new_write && !file_hash_.empty () && is_backup)
    {
      bool exists_in_main_project = false;
      if (utils::io::path_exists (path_in_main_project))
        {
          exists_in_main_project =
            file_hash_
            == hash_get_from_file (path_in_main_project, HASH_ALGORITHM_XXH3_64);
        }

      if (exists_in_main_project)
        {
          /* try reflink and fall back to normal copying */
          z_debug (
            "reflinking clip from main project ('{}' to '{}')",
            path_in_main_project, new_path);

          if (!utils::io::reflink_file (path_in_main_project, new_path))
            {
              z_debug ("failed to reflink, copying instead");
              z_debug (
                "copying clip from main project ('{}' to '{}')",
                path_in_main_project, new_path);
              utils::io::copy_file (new_path, path_in_main_project);
            }
        }
    }

  if (need_new_write)
    {
      z_debug (
        "writing clip {} to pool (parts {}, is backup  {}): '{}'", name_, parts,
        is_backup, new_path);
      write_to_file (new_path, parts);
      if (!parts)
        {
          /* store file hash */
          file_hash_ = hash_get_from_file (new_path, HASH_ALGORITHM_XXH3_64);
        }
    }

  AUDIO_POOL->print ();
}

void
AudioClip::finalize_buffered_write ()
{
  if (frames_written_ == 0)
    {
      z_debug ("nothing to write, skipping");
      return;
    }

  z_return_if_fail (writer_);
  z_return_if_fail (writer_path_.has_value ());
  writer_->flush ();
  writer_.reset ();
  if (ZRYTHM_TESTING)
    {
      verify_recorded_file (writer_path_.value ());
    }
  writer_path_.reset ();
}

void
AudioClip::write_to_file (const std::string &filepath, bool parts)
{
  z_return_if_fail (samplerate_ > 0);
  z_return_if_fail (frames_written_ < SIZE_MAX);
  const auto before_frames = (size_t) frames_written_;

  auto create_writer_for_filepath = [&] () {
    juce::File file (filepath);
    auto       out_stream = std::make_unique<juce::FileOutputStream> (file);

    if (!out_stream || !out_stream->openedOk ())
      {
        throw ZrythmException (
          fmt::format ("Failed to open file '{}' for writing", filepath));
      }

    auto format = std::unique_ptr<juce::AudioFormat> (
      use_flac_
        ? static_cast<juce::AudioFormat *> (new juce::FlacAudioFormat ())
        : static_cast<juce::AudioFormat *> (new juce::WavAudioFormat ()));

    auto writer = std::unique_ptr<
      juce::AudioFormatWriter> (format->createWriterFor (
      out_stream.release (), samplerate_, channels_,
      audio_bit_depth_enum_to_int (bit_depth_), {}, 0));

    if (writer == nullptr)
      {
        throw ZrythmException (
          "Failed to create audio writer for file: " + filepath);
      }
    return writer;
  };

  if (parts)
    {
      z_return_if_fail_cmp (num_frames_, >=, frames_written_);
      unsigned_frame_t nframes_to_write_this_time =
        num_frames_ - frames_written_;
      z_return_if_fail_cmp (nframes_to_write_this_time, <, SIZE_MAX);

      // create writer if first time
      if (frames_written_ == 0)
        {
          writer_ = create_writer_for_filepath ();
          writer_path_.emplace (filepath);
        }
      else
        {
          z_return_if_fail (writer_ != nullptr);
        }
      writer_->writeFromAudioSampleBuffer (
        ch_frames_, frames_written_, nframes_to_write_this_time);

      frames_written_ = num_frames_;
      last_write_ = Zrythm::getInstance ()->get_monotonic_time_usecs ();
    }
  else
    {
      z_return_if_fail_cmp (num_frames_, <, SIZE_MAX);
      size_t nframes = num_frames_;

      auto writer = create_writer_for_filepath ();
      writer->writeFromAudioSampleBuffer (ch_frames_, 0, nframes);
      writer->flush ();
      if (ZRYTHM_TESTING)
        {
          verify_recorded_file (filepath);
        }
    }

  update_channel_caches (before_frames);
}

bool
AudioClip::verify_recorded_file (const fs::path &filepath) const
{
  AudioClip new_clip (filepath.string ());
  if (num_frames_ != new_clip.num_frames_)
    {
      z_error ("{} != {}", num_frames_, new_clip.num_frames_);
      return false;
    }

  constexpr float epsilon = 0.0001f;
  z_return_val_if_fail (
    audio_frames_equal (
      ch_frames_.getReadPointer (0), new_clip.ch_frames_.getReadPointer (0),
      (size_t) new_clip.num_frames_, epsilon),
    false);
  z_return_val_if_fail (
    audio_frames_equal (
      frames_.getReadPointer (0), new_clip.frames_.getReadPointer (0),
      (size_t) new_clip.num_frames_ * new_clip.channels_, epsilon),
    false);
  return true;
}

bool
AudioClip::is_in_use (bool check_undo_stack) const
{
  for (auto track : TRACKLIST->tracks_ | type_is<AudioTrack>{})
    {
      for (auto &lane_var : track->lanes_)
        {
          auto * lane = std::get<AudioLane *> (lane_var);
          for (auto region_var : lane->region_list_->regions_)
            {
              auto * region = std::get<AudioRegion *> (region_var);
              if (region->pool_id_ == pool_id_)
                return true;
            }
        }
    }

  if (check_undo_stack)
    {
      if (UNDO_MANAGER->contains_clip (*this))
        {
          return true;
        }
    }

  return false;
}

#if 0
typedef struct AppLaunchData
{
  GFile *         file;
  GtkAppChooser * app_chooser;
} AppLaunchData;

static void
app_launch_data_free (AppLaunchData * data, GClosure * closure)
{
  free (data);
}

static void
on_launch_clicked (GtkButton * btn, AppLaunchData * data)
{
  GError * err = NULL;
  GList *  file_list = NULL;
  file_list = g_list_append (file_list, data->file);
  GAppInfo * app_nfo = gtk_app_chooser_get_app_info (data->app_chooser);
  bool       success = g_app_info_launch (app_nfo, file_list, nullptr, &err);
  g_list_free (file_list);
  if (!success)
    {
      z_info ("app launch unsuccessful");
    }
}
#endif

std::unique_ptr<AudioClip>
AudioClip::edit_in_ext_program ()
{
#if 0
  GError * err = NULL;
  char *   tmp_dir = g_dir_make_tmp ("zrythm-audio-clip-tmp-XXXXXX", &err);
  if (!tmp_dir)
    {
      throw ZrythmException (
        fmt::format ("Failed to create tmp dir: {}", err->message));
    }
  std::string abs_path = Glib::build_filename (tmp_dir, "tmp.wav");
  try
    {
      write_to_file (abs_path, false);
    }
  catch (ZrythmException &e)
    {
      throw ZrythmException (
        fmt::format ("Failed to write audio file '{}'", abs_path));
    }

  GFile *     file = g_file_new_for_path (abs_path.c_str ());
  GFileInfo * file_info = g_file_query_info (
    file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_QUERY_INFO_NONE,
    nullptr, &err);
  if (!file_info)
    {
      throw ZrythmException (
        fmt::format ("Failed to query file info for {}", abs_path));
    }
  const char * content_type = g_file_info_get_content_type (file_info);

  GtkWidget * dialog = gtk_dialog_new_with_buttons (
    QObject::tr ("Edit in external app"), GTK_WINDOW (MAIN_WINDOW),
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, QObject::tr ("_OK"),
    GTK_RESPONSE_ACCEPT, QObject::tr ("_Cancel"), GTK_RESPONSE_REJECT, nullptr);

  /* populate content area */
  GtkWidget * content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  GtkWidget * main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start (main_box, 4);
  gtk_widget_set_margin_end (main_box, 4);
  gtk_widget_set_margin_top (main_box, 4);
  gtk_widget_set_margin_bottom (main_box, 4);
  gtk_box_append (GTK_BOX (content_area), main_box);

  GtkWidget * lbl = gtk_label_new ("");
  gtk_label_set_selectable (GTK_LABEL (lbl), true);
  char * markup = g_markup_printf_escaped (
    QObject::tr ("Edit the file at <u>%s</u>, then press OK"), abs_path.c_str ());
  gtk_label_set_markup (GTK_LABEL (lbl), markup);
  gtk_box_append (GTK_BOX (main_box), lbl);

  GtkWidget * launch_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_halign (launch_box, GTK_ALIGN_CENTER);
  GtkWidget * app_chooser_button = gtk_app_chooser_button_new (content_type);
  gtk_box_append (GTK_BOX (launch_box), app_chooser_button);
  GtkWidget *     btn = gtk_button_new_with_label (QObject::tr ("Launch"));
  AppLaunchData * data = object_new (AppLaunchData);
  data->file = file;
  data->app_chooser = GTK_APP_CHOOSER (app_chooser_button);
  g_signal_connect_data (
    G_OBJECT (btn), "clicked", G_CALLBACK (on_launch_clicked), data,
    (GClosureNotify) app_launch_data_free, (GConnectFlags) 0);
  gtk_box_append (GTK_BOX (launch_box), btn);
  gtk_box_append (GTK_BOX (main_box), launch_box);

  int ret = z_gtk_dialog_run (GTK_DIALOG (dialog), true);
  if (ret != GTK_RESPONSE_ACCEPT)
    {
      z_debug ("operation cancelled");
      return nullptr;
    }

  /* ok - reload from file */
  return std::make_unique<AudioClip> (abs_path);
#endif
  return nullptr;
}

void
AudioClip::remove (bool backup)
{
  std::string path = get_path_in_pool (backup);
  z_debug ("removing clip at {}", path);
  z_return_if_fail (path.length () > 0);
  utils::io::remove (path);
}