// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/audio_file.h"
#include "utils/dsp.h"
#include "utils/exceptions.h"
#include "utils/logger.h"
#include "utils/resampler.h"

namespace zrythm::utils::audio
{

void
AudioFile::ensure_file_is_open ()
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
  if (metadata_.filled)
    return metadata_;

  ensure_file_is_open ();

  /* obtain metadata */
  metadata_.channels = static_cast<int> (reader_->numChannels);
  metadata_.num_frames = reader_->lengthInSamples;
  metadata_.samplerate = (int) reader_->sampleRate;
  metadata_.bit_depth = static_cast<int> (reader_->bitsPerSample);
  metadata_.bit_rate =
    metadata_.bit_depth * metadata_.channels * metadata_.samplerate;
  metadata_.length =
    metadata_.samplerate > 0
      ? (metadata_.num_frames * 1000) / metadata_.samplerate
      : 0;
  if (reader_->metadataValues.containsKey ("bpm"))
    metadata_.bpm =
      reader_->metadataValues.getValue ("bpm", "").getFloatValue ();
  else if (reader_->metadataValues.containsKey ("beatsperminute"))
    metadata_.bpm =
      reader_->metadataValues.getValue ("beatsperminute", "").getFloatValue ();
  else if (reader_->metadataValues.containsKey ("tempo"))
    metadata_.bpm =
      reader_->metadataValues.getValue ("tempo", "").getFloatValue ();

  metadata_.filled = true;
  return metadata_;
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
    in_parts ? num_frames_to_read : static_cast<size_t> (metadata_.num_frames);

  zrythm::utils::audio::AudioBuffer buffer (
    metadata_.channels, static_cast<int> (num_frames_to_read));
  bool success = reader_->read (
    &buffer, 0, static_cast<int> (num_frames_to_read),
    static_cast<int64_t> (start_from), true, true);
  if (!success)
    {
      throw ZrythmException (
        fmt::format (
          "Failed to read frames at {} from file '{}'", start_from, filepath_));
    }

  buffer.interleave_samples ();
  float_ranges::copy (
    samples, buffer.getReadPointer (0),
    num_frames_to_read * static_cast<size_t> (metadata_.channels));
}

void
AudioFile::read_full (
  zrythm::utils::audio::AudioBuffer &buffer,
  std::optional<size_t>              samplerate)
{
  read_metadata ();

  /* read frames in file's sample rate */
  zrythm::utils::audio::AudioBuffer interleaved_buffer (
    1, metadata_.channels * static_cast<int> (metadata_.num_frames));

  read_samples_interleaved (
    false, interleaved_buffer.getWritePointer (0), 0,
    static_cast<size_t> (metadata_.num_frames));
  interleaved_buffer.deinterleave_samples (
    static_cast<size_t> (metadata_.channels));

  if (samplerate.has_value () && samplerate != metadata_.samplerate)
    {
      /* resample to project's sample rate */
      Resampler r (
        interleaved_buffer, metadata_.samplerate,
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

} //  namespace zrythm::utils::io
