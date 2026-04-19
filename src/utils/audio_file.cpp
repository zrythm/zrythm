// SPDX-FileCopyrightText: © 2023-2024, 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/audio_file.h"
#include "utils/dsp.h"
#include "utils/exceptions.h"
#include "utils/logger.h"
#include "utils/resampler.h"

#include <fmt/std.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace zrythm::utils::audio
{

struct AudioFile::Impl
{
  std::filesystem::path                    filepath_;
  AudioFileMetadata                        metadata_;
  bool                                     for_writing_ = false;
  std::unique_ptr<juce::AudioFormatReader> reader_;
  std::unique_ptr<juce::AudioFormatWriter> writer_;

  /**
   * Ensures there is an opened file.
   *
   * @throw ZrythmException on error.
   */
  void ensure_file_is_open ();
};

AudioFile::AudioFile () = default;

AudioFile::AudioFile (std::filesystem::path filepath, bool for_writing)
    : impl_ (std::make_unique<Impl> ())
{
  impl_->filepath_ = std::move (filepath);
  impl_->for_writing_ = for_writing;
}

AudioFile::AudioFile (AudioFile &&other) noexcept = default;

AudioFile &
AudioFile::operator= (AudioFile &&other) noexcept = default;

AudioFile::~AudioFile () = default;

void
AudioFile::Impl::ensure_file_is_open ()
{
  if (for_writing_ && writer_ != nullptr)
    return;
  if (!for_writing_ && reader_ != nullptr)
    return;

  juce::AudioFormatManager format_mgr;
  format_mgr.registerBasicFormats ();

  if (for_writing_)
    {
      // TODO
      z_return_if_reached ();
#if 0
      auto format = format_mgr.findFormatForFileExtension (
        utils::Utf8String::from_path(filepath_.extension ()).to_juce_string ());
      if (format == nullptr)
        {
        throw ZrythmException (
          fmt::sprintf ("Failed to find format for file '%s'", filepath_));
      }
      juce::File file (filepath_);
      auto       outputStream = std::make_unique<juce::FileOutputStream> (file);
      if (outputStream->failedToOpen ())
        {
          throw ZrythmException (
            fmt::sprintf ("Failed to open file '%s' for writing", filepath_));
        }

      writer_.reset (
        format->createWriterFor (outputStream.release (), sample_rate_));
      ;
      if (writer_ == nullptr)
        {
          throw ZrythmException (
            fmt::sprintf ("Failed to create reader for file '%s'", filepath_));
        }
#endif
    }
  else
    {
      reader_ =
        std::unique_ptr<juce::AudioFormatReader> (format_mgr.createReaderFor (
          utils::Utf8String::from_path (filepath_).to_juce_file ()));
      if (reader_ == nullptr)
        {
          throw ZrythmException (
            fmt::format ("Failed to create reader for file '{}'", filepath_));
        }
    }
}

AudioFileMetadata
AudioFile::read_metadata ()
{
  if (impl_->metadata_.filled)
    return impl_->metadata_;

  impl_->ensure_file_is_open ();

  /* obtain metadata */
  impl_->metadata_.channels = static_cast<int> (impl_->reader_->numChannels);
  impl_->metadata_.num_frames = impl_->reader_->lengthInSamples;
  impl_->metadata_.samplerate = (int) impl_->reader_->sampleRate;
  impl_->metadata_.bit_depth = static_cast<int> (impl_->reader_->bitsPerSample);
  impl_->metadata_.bit_rate =
    impl_->metadata_.bit_depth * impl_->metadata_.channels
    * impl_->metadata_.samplerate;
  impl_->metadata_.length =
    impl_->metadata_.samplerate > 0
      ? (impl_->metadata_.num_frames * 1000) / impl_->metadata_.samplerate
      : 0;
  if (impl_->reader_->metadataValues.containsKey ("bpm"))
    impl_->metadata_.bpm =
      impl_->reader_->metadataValues.getValue ("bpm", "").getFloatValue ();
  else if (impl_->reader_->metadataValues.containsKey ("beatsperminute"))
    impl_->metadata_.bpm =
      impl_->reader_->metadataValues.getValue ("beatsperminute", "")
        .getFloatValue ();
  else if (impl_->reader_->metadataValues.containsKey ("tempo"))
    impl_->metadata_.bpm =
      impl_->reader_->metadataValues.getValue ("tempo", "").getFloatValue ();

  impl_->metadata_.filled = true;
  return impl_->metadata_;
}

void
AudioFile::read_samples_interleaved (
  bool    in_parts,
  float * samples,
  size_t  start_from,
  size_t  num_frames_to_read)
{
  read_metadata ();

  start_from = in_parts ? start_from : 0;
  num_frames_to_read =
    in_parts
      ? num_frames_to_read
      : static_cast<size_t> (impl_->metadata_.num_frames);

  zrythm::utils::audio::AudioBuffer buffer (
    impl_->metadata_.channels, static_cast<int> (num_frames_to_read));
  bool success = impl_->reader_->read (
    &buffer, 0, static_cast<int> (num_frames_to_read),
    static_cast<int64_t> (start_from), true, true);
  if (!success)
    {
      throw ZrythmException (
        fmt::format (
          "Failed to read frames at {} from file '{}'", start_from,
          impl_->filepath_));
    }

  buffer.interleave_samples ();
  float_ranges::copy (
    samples, buffer.getReadPointer (0),
    num_frames_to_read * static_cast<size_t> (impl_->metadata_.channels));
}

void
AudioFile::read_full (
  zrythm::utils::audio::AudioBuffer &buffer,
  std::optional<size_t>              samplerate)
{
  read_metadata ();

  /* read frames in file's sample rate */
  zrythm::utils::audio::AudioBuffer interleaved_buffer (
    1,
    impl_->metadata_.channels * static_cast<int> (impl_->metadata_.num_frames));

  read_samples_interleaved (
    false, interleaved_buffer.getWritePointer (0), 0,
    static_cast<size_t> (impl_->metadata_.num_frames));
  interleaved_buffer.deinterleave_samples (
    static_cast<size_t> (impl_->metadata_.channels));

  if (samplerate.has_value () && samplerate != impl_->metadata_.samplerate)
    {
      /* resample to project's sample rate */
      Resampler r (
        interleaved_buffer, impl_->metadata_.samplerate,
        static_cast<double> (*samplerate), Resampler::Quality::VeryHigh);
      while (!r.is_done ())
        {
          r.process ();
        }
      buffer = r.get_out_frames ();
    }
  else
    {
      buffer = std::move (interleaved_buffer);
    }
}

} //  namespace zrythm::utils::audio
