// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/clip.h"
#include "utils/audio.h"
#include "utils/audio_file.h"
#include "utils/debug.h"
#include "utils/exceptions.h"
#include "utils/logger.h"

#include <fmt/printf.h>

using namespace zrythm;

AudioClip::AudioClip (
  const std::string &full_path,
  sample_rate_t      project_sample_rate,
  bpm_t              current_bpm)
{
  init_from_file (full_path, project_sample_rate, std::nullopt);

  bpm_ = current_bpm;
}

AudioClip::AudioClip (
  const utils::audio::AudioBuffer &buf,
  BitDepth                         bit_depth,
  sample_rate_t                    project_sample_rate,
  bpm_t                            current_bpm,
  const std::string               &name)
{
  samplerate_ = project_sample_rate;
  z_return_if_fail (samplerate_ > 0);
  name_ = name;
  bit_depth_ = bit_depth;
  use_flac_ = should_use_flac (bit_depth);

  ch_frames_ = buf;

  bpm_ = current_bpm;
}

AudioClip::AudioClip (
  const channels_t       channels,
  const unsigned_frame_t nframes,
  sample_rate_t          project_sample_rate,
  bpm_t                  current_bpm,
  const std::string     &name)
{
  ch_frames_.setSize (channels, nframes, false, true, false);
  name_ = name;
  bpm_ = current_bpm;
  samplerate_ = project_sample_rate;
  bit_depth_ = BitDepth::BIT_DEPTH_32;
  use_flac_ = false;
  z_return_if_fail (samplerate_ > 0);
}

void
AudioClip::init_from_file (
  const std::string   &full_path,
  sample_rate_t        project_sample_rate,
  std::optional<bpm_t> bpm_to_set)
{
  samplerate_ = project_sample_rate;
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
  bit_depth_ = utils::audio::bit_depth_int_to_enum (file.metadata_.bit_depth);
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

  name_ = juce::File (full_path).getFileNameWithoutExtension ().toStdString ();
  if (bpm_to_set.has_value ())
    {
      bpm_ = bpm_to_set.value ();
    }
  use_flac_ = should_use_flac (bit_depth_);
}

void
AudioClip::init_loaded (const fs::path &full_path)
{
  bpm_t bpm = bpm_;
  try
    {
      init_from_file (full_path, samplerate_, std::nullopt);
    }
  catch (ZrythmException &e)
    {
      throw ZrythmException (
        fmt::format ("Failed to initialize audio file: {}", e.what ()));
    }
  bpm_ = bpm;
}

void
AudioClip::init_after_cloning (const AudioClip &other, ObjectCloneType clone_type)
{
  UuidIdentifiableObject::copy_members_from (other);
  name_ = other.name_;
  ch_frames_ = other.ch_frames_;
  bpm_ = other.bpm_;
  samplerate_ = other.samplerate_;
  bit_depth_ = other.bit_depth_;
  use_flac_ = other.use_flac_;
  file_hash_ = other.file_hash_;
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
  writer_path_.reset ();
}

void
AudioClip::replace_frames_from_interleaved (
  const float *    frames,
  unsigned_frame_t start_frame,
  unsigned_frame_t num_frames_per_channel,
  channels_t       channels)
{
  utils::audio::AudioBuffer buf (1, num_frames_per_channel * channels);
  buf.deinterleave_samples (channels);
  replace_frames (buf, start_frame);
}

void
AudioClip::replace_frames (
  const utils::audio::AudioBuffer &src_frames,
  unsigned_frame_t                 start_frame)
{
  z_return_if_fail_cmp (
    src_frames.getNumChannels (), ==, ch_frames_.getNumChannels ());

  /* this is needed because if the file hash doesn't change
   * the actual file write is skipped to save time */
  file_hash_ = 0;

  for (int i = 0; i < src_frames.getNumChannels (); ++i)
    {
      ch_frames_.copyFrom (
        i, start_frame, src_frames.getReadPointer (i, 0),
        src_frames.getNumSamples ());
    }
}

void
AudioClip::expand_with_frames (const utils::audio::AudioBuffer &frames)
{
  z_return_if_fail (frames.getNumChannels () == ch_frames_.getNumChannels ());
  z_return_if_fail (frames.getNumSamples () > 0);

  unsigned_frame_t prev_end = ch_frames_.getNumSamples ();
  ch_frames_.setSize (
    ch_frames_.getNumChannels (),
    ch_frames_.getNumSamples () + frames.getNumSamples (), true, false);
  replace_frames (frames, prev_end);
}

bool
AudioClip::enough_time_elapsed_since_last_write () const
{
  /* write to pool if 2 seconds passed since last write */
  auto                           cur_time = get_monotonic_time_usecs ();
  constexpr utils::MonotonicTime usec_to_wait = 2 * 1000 * 1000;
  return cur_time - last_write_ > usec_to_wait;
}

void
AudioClip::write_to_file (const std::string &filepath, bool parts)
{
  z_return_if_fail (samplerate_ > 0);
  z_return_if_fail (frames_written_ < SIZE_MAX);

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
      out_stream.release (), samplerate_, get_num_channels (),
      utils::audio::bit_depth_enum_to_int (bit_depth_), {}, 0));

    if (writer == nullptr)
      {
        throw ZrythmException (
          "Failed to create audio writer for file: " + filepath);
      }
    return writer;
  };

  const auto num_frames = get_num_frames ();
  if (parts)
    {
      z_return_if_fail_cmp (num_frames, >=, (int) frames_written_);
      unsigned_frame_t nframes_to_write_this_time = num_frames - frames_written_;
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

      frames_written_ = num_frames;
      last_write_ = get_monotonic_time_usecs ();
    }
  else
    {
      z_return_if_fail_cmp (num_frames, <, (int) INT_MAX);
      size_t nframes = num_frames;

      auto writer = create_writer_for_filepath ();
      writer->writeFromAudioSampleBuffer (ch_frames_, 0, nframes);
      writer->flush ();
    }
}

bool
AudioClip::verify_recorded_file (
  const fs::path &filepath,
  sample_rate_t   project_sample_rate,
  bpm_t           current_bpm) const
{
  AudioClip new_clip (filepath.string (), project_sample_rate, current_bpm);
  if (get_num_frames () != new_clip.get_num_frames ())
    {
      z_error ("{} != {}", get_num_frames (), new_clip.get_num_frames ());
      return false;
    }

  constexpr float epsilon = 0.0001f;
  z_return_val_if_fail (
    utils::audio::frames_equal (
      ch_frames_.getReadPointer (0), new_clip.ch_frames_.getReadPointer (0),
      (size_t) new_clip.get_num_frames (), epsilon),
    false);

  return true;
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
AudioClip::define_fields (const Context &ctx)
{
  using T = utils::serialization::ISerializable<AudioClip>;
  T::call_all_base_define_fields<UuidIdentifiableObject> (ctx);
  T::serialize_fields (
    ctx, T::make_field ("name", name_), T::make_field ("fileHash", file_hash_),
    T::make_field ("bpm", bpm_), T::make_field ("bitDepth", bit_depth_),
    T::make_field ("useFlac", use_flac_),
    T::make_field ("samplerate", samplerate_));
}
