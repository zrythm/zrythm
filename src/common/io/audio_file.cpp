// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/io/audio_file.h"
#include "common/utils/dsp.h"
#include "common/utils/exceptions.h"
#include "common/utils/logger.h"
#include "common/utils/objects.h"
#include "common/utils/resampler.h"

#include <glib/gi18n.h>

#include <fmt/printf.h>

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
        fs::path (filepath_).extension ().string ());
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
      reader_ = std::unique_ptr<juce::AudioFormatReader> (
        format_mgr.createReaderFor (juce::File (filepath_)));
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
  metadata_.channels = reader_->numChannels;
  metadata_.num_frames = reader_->lengthInSamples;
  metadata_.samplerate = (int) reader_->sampleRate;
  metadata_.bit_depth = reader_->bitsPerSample;
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
AudioFile::interleave_buffer (juce::AudioBuffer<float> &buffer)
{
  const int numChannels = buffer.getNumChannels ();
  const int numSamples = buffer.getNumSamples ();

  // Create a temporary buffer to hold the interleaved data
  juce::AudioBuffer<float> tempBuffer (1, numChannels * numSamples);

  // Interleave the channels
  int writeIndex = 0;
  for (int sample = 0; sample < numSamples; ++sample)
    {
      for (int channel = 0; channel < numChannels; ++channel)
        {
          tempBuffer.setSample (
            0, writeIndex++, buffer.getSample (channel, sample));
        }
    }

  // Copy the interleaved data back to the original buffer
  buffer = std::move (tempBuffer);
}

void
AudioFile::deinterleave_buffer (
  juce::AudioBuffer<float> &buffer,
  size_t                    num_channels)
{
  const size_t total_samples = buffer.getNumSamples () / num_channels;

  // Create a temporary buffer to hold the deinterleaved data
  juce::AudioSampleBuffer tempBuffer (num_channels, total_samples);

  // Deinterleave the channels
  size_t read_index = 0;
  for (size_t sample = 0; sample < total_samples; ++sample)
    {
      for (size_t channel = 0; channel < num_channels; ++channel)
        {
          tempBuffer.setSample (
            channel, sample, buffer.getSample (0, read_index++));
        }
    }

  // Copy the deinterleaved data back to the original buffer
  buffer = std::move (tempBuffer);
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
  num_frames_to_read = in_parts ? num_frames_to_read : metadata_.num_frames;

  juce::AudioSampleBuffer buffer (metadata_.channels, num_frames_to_read);
  bool                    success =
    reader_->read (&buffer, 0, num_frames_to_read, start_from, true, true);
  if (!success)
    {
      throw ZrythmException (fmt::sprintf (
        "Failed to read frames at %zu from file '%s'", start_from, filepath_));
    }

  interleave_buffer (buffer);
  dsp_copy (
    samples, buffer.getReadPointer (0), num_frames_to_read * metadata_.channels);
}

void
AudioFile::read_full (
  juce::AudioSampleBuffer &buffer,
  std::optional<size_t>    samplerate)
{
  read_metadata ();

  /* read frames in file's sample rate */
  juce::AudioSampleBuffer interleaved_buffer (
    1, metadata_.channels * (size_t) metadata_.num_frames);

  read_samples_interleaved (
    false, interleaved_buffer.getWritePointer (0), 0, metadata_.num_frames);
  deinterleave_buffer (interleaved_buffer, metadata_.channels);

  if (samplerate.has_value () && samplerate != metadata_.samplerate)
    {
      /* resample to project's sample rate */
      Resampler r (
        interleaved_buffer, metadata_.samplerate, *samplerate,
        Resampler::Quality::VeryHigh);
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