// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "io/audio_file.h"
#include "utils/dsp.h"
#include "utils/exceptions.h"
#include "utils/logger.h"
#include "utils/objects.h"
#include "utils/resampler.h"

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
      reader_ = std::unique_ptr<juce::AudioFormatReader>(format_mgr.createReaderFor(juce::File(filepath_)));
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
AudioFile::read_samples (
  bool    in_parts,
  float * samples,
  size_t  start_from,
  size_t  num_frames_to_read)
{
  read_metadata ();

  start_from = in_parts ? start_from : 0;
  num_frames_to_read = in_parts ? num_frames_to_read : metadata_.num_frames;

  juce::AudioBuffer<float> buffer (metadata_.channels, num_frames_to_read);
  bool                     success =
    reader_->read (&buffer, 0, num_frames_to_read, start_from, true, true);
  if (!success)
    {
      throw ZrythmException (fmt::sprintf (
        "Failed to read frames at %zu from file '%s'", start_from, filepath_));
    }

  interleave_buffer (buffer);
  dsp_copy (samples, buffer.getReadPointer (0), num_frames_to_read);
}

void
AudioFile::read_full (
  juce::AudioBuffer<float> &buffer,
  std::optional<size_t>     samplerate)
{
  read_metadata ();

  /* read frames in file's sample rate */
  buffer.setSize (metadata_.channels, (size_t) metadata_.num_frames);
  for (typeof (metadata_.channels) i = 0; i < metadata_.channels; ++i)
    {
      read_samples (false, buffer.getWritePointer (i), 0, metadata_.num_frames);

      if (samplerate.has_value () && samplerate != metadata_.samplerate)
        {
          /* resample to project's sample rate */
          GError *    err = NULL;
          Resampler * r = resampler_new (
            buffer.getReadPointer (i), metadata_.num_frames,
            metadata_.samplerate, *samplerate, metadata_.channels,
            RESAMPLER_QUALITY_VERY_HIGH, &err);
          if (!r)
            {
              ZrythmException ex (
                fmt::sprintf ("Failed to create resampler", err->message));
              g_error_free (err);
              throw ex;
            }
          while (!resampler_is_done (r))
            {
              bool success = resampler_process (r, &err);
              if (!success)
                {
                  ZrythmException ex (
                    fmt::sprintf ("Resampler failed to process", err->message));
                  g_error_free (err);
                  throw ex;
                }
            }
          buffer.setSize (metadata_.channels, r->num_out_frames);
          dsp_copy (
            buffer.getWritePointer (i), r->out_frames, r->num_out_frames);

          /* resampler is not RAII yet */
          object_free_w_func_and_null (resampler_free, r);
        }
    }
}