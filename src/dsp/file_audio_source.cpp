// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <mutex>
#include <utility>

#include "dsp/file_audio_source.h"
#include "utils/audio.h"
#include "utils/audio_file.h"
#include "utils/debug.h"
#include "utils/logger.h"

#include <fmt/printf.h>

namespace zrythm::dsp
{

FileAudioSource::FileAudioSource (
  const fs::path &full_path,
  sample_rate_t   project_sample_rate,
  bpm_t           current_bpm,
  QObject *       parent)
    : QObject (parent)
{
  init_from_file (full_path, project_sample_rate, current_bpm);
}

FileAudioSource::FileAudioSource (
  const utils::audio::AudioBuffer &buf,
  BitDepth                         bit_depth,
  sample_rate_t                    project_sample_rate,
  bpm_t                            current_bpm,
  const utils::Utf8String         &name,
  QObject *                        parent)
    : QObject (parent)
{
  if (buf.getNumSamples () == 0)
    {
      throw std::runtime_error ("Buffer is empty");
    }

  samplerate_ = project_sample_rate;
  z_return_if_fail (samplerate_ > 0);
  name_ = name;
  bit_depth_ = bit_depth;
  ch_frames_ = buf;
  bpm_ = current_bpm;
}

FileAudioSource::FileAudioSource (
  const channels_t         channels,
  const unsigned_frame_t   nframes,
  sample_rate_t            project_sample_rate,
  bpm_t                    current_bpm,
  const utils::Utf8String &name,
  QObject *                parent)
    : QObject (parent)
{
  ch_frames_.setSize (channels, static_cast<int> (nframes), false, true, false);
  name_ = name;
  bpm_ = current_bpm;
  samplerate_ = project_sample_rate;
  bit_depth_ = BitDepth::BIT_DEPTH_32;
  z_return_if_fail (samplerate_ > 0);
}

void
FileAudioSource::init_from_file (
  const fs::path      &full_path,
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

  name_ = utils::Utf8String::from_path (
    utils::io::path_get_basename_without_ext (full_path));
  if (bpm_to_set.has_value ())
    {
      bpm_ = bpm_to_set.value ();
    }
}

void
init_from (
  FileAudioSource       &obj,
  const FileAudioSource &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<FileAudioSource::UuidIdentifiableObject &> (obj),
    static_cast<const FileAudioSource::UuidIdentifiableObject &> (other),
    clone_type);
  obj.name_ = other.name_;
  obj.ch_frames_ = other.ch_frames_;
  obj.bpm_ = other.bpm_;
  obj.samplerate_ = other.samplerate_;
  obj.bit_depth_ = other.bit_depth_;
}

void
FileAudioSource::replace_frames_from_interleaved (
  const float *    frames,
  unsigned_frame_t start_frame,
  unsigned_frame_t num_frames_per_channel,
  channels_t       channels)
{
  utils::audio::AudioBuffer buf (
    1, static_cast<int> (num_frames_per_channel) * static_cast<int> (channels));
  buf.deinterleave_samples (channels);
  replace_frames (buf, start_frame);
}

void
FileAudioSource::replace_frames (
  const utils::audio::AudioBuffer &src_frames,
  unsigned_frame_t                 start_frame)
{
  z_return_if_fail_cmp (
    src_frames.getNumChannels (), ==, ch_frames_.getNumChannels ());

  for (int i = 0; i < src_frames.getNumChannels (); ++i)
    {
      ch_frames_.copyFrom (
        i, static_cast<int> (start_frame), src_frames.getReadPointer (i, 0),
        src_frames.getNumSamples ());
    }

  Q_EMIT samplesChanged ();
}

void
FileAudioSource::expand_with_frames (const utils::audio::AudioBuffer &frames)
{
  z_return_if_fail (frames.getNumChannels () == ch_frames_.getNumChannels ());
  z_return_if_fail (frames.getNumSamples () > 0);

  unsigned_frame_t prev_end = ch_frames_.getNumSamples ();
  ch_frames_.setSize (
    ch_frames_.getNumChannels (),
    ch_frames_.getNumSamples () + frames.getNumSamples (), true, false);
  replace_frames (frames, prev_end);
}

// ========================================================================

FileAudioSourceWriter::FileAudioSourceWriter (
  const FileAudioSource &source,
  fs::path               path,
  bool                   parts)
    : parts_ (parts), source_ (source), writer_path_ (std::move (path))
{
}

bool
FileAudioSourceWriter::enough_time_elapsed_since_last_write () const
{
  assert (parts_);

  /* write to pool if 2 seconds passed since last write */
  auto                           cur_time = get_monotonic_time_usecs ();
  constexpr utils::MonotonicTime usec_to_wait = 2 * 1000 * 1000;
  return cur_time - last_write_ > usec_to_wait;
}

void
FileAudioSourceWriter::finalize_buffered_write ()
{
  assert (parts_);

  if (frames_written_ == 0)
    {
      z_debug ("nothing to write, skipping");
      return;
    }

  z_return_if_fail (writer_);
  writer_->flush ();
  writer_.reset ();
}

void
FileAudioSourceWriter::write_to_file ()
{
  assert (source_.get_samplerate () > 0);
  assert (frames_written_ < std::numeric_limits<size_t>::max ());

  // Use a static mutex to protect JUCE object creation to avoid data races
  // in LeakedObjectDetector counters during multi-threaded execution
  static std::mutex juce_creation_mutex;

  auto create_writer_for_filepath = [&] () {
    std::lock_guard lock (juce_creation_mutex);

    auto file = utils::Utf8String::from_path (writer_path_).to_juce_file ();
    std::unique_ptr<juce::OutputStream> out_stream =
      std::make_unique<juce::FileOutputStream> (file);

    if (
      !out_stream
      || !dynamic_cast<juce::FileOutputStream &> (*out_stream).openedOk ())
      {
        throw ZrythmException (
          fmt::format ("Failed to open file '{}' for writing", writer_path_));
      }

    auto format = std::make_unique<juce::WavAudioFormat> ();

    juce::AudioFormatWriterOptions options;
    options =
      options.withSampleRate (source_.get_samplerate ())
        .withNumChannels (source_.get_num_channels ())
        .withBitsPerSample (
          utils::audio::bit_depth_enum_to_int (source_.get_bit_depth ()))
        .withQualityOptionIndex (0);

    auto writer = format->createWriterFor (out_stream, options);

    if (writer == nullptr)
      {
        throw ZrythmException (
          fmt::format (
            "Failed to create audio writer for file: {}", writer_path_));
      }
    return writer;
  };

  const auto num_frames = source_.get_num_frames ();
  if (parts_)
    {
      z_return_if_fail_cmp (num_frames, >=, (int) frames_written_);
      unsigned_frame_t nframes_to_write_this_time = num_frames - frames_written_;
      z_return_if_fail_cmp (nframes_to_write_this_time, <, SIZE_MAX);

      // create writer if first time
      if (frames_written_ == 0)
        {
          writer_ = create_writer_for_filepath ();
        }
      else
        {
          z_return_if_fail (writer_ != nullptr);
        }
      writer_->writeFromAudioSampleBuffer (
        source_.get_samples (), static_cast<int> (frames_written_),
        static_cast<int> (nframes_to_write_this_time));

      frames_written_ = num_frames;
      last_write_ = get_monotonic_time_usecs ();
    }
  else
    {
      z_return_if_fail_cmp (num_frames, <, (int) INT_MAX);
      size_t nframes = num_frames;

      auto writer = create_writer_for_filepath ();
      writer->writeFromAudioSampleBuffer (
        source_.get_samples (), 0, static_cast<int> (nframes));
      writer->flush ();
    }
}

} // namespace zrythm::dsp
