// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdexcept>

#include "utils/audio_file_writer.h"
#include "utils/logger.h"

#include <QtConcurrentRun>

namespace zrythm::utils
{

void
AudioFileWriter::write (
  QPromise<void>           &promise,
  WriteOptions              options,
  const fs::path           &file_path,
  juce::AudioSampleBuffer &&buffer)
{
  try
    {
      z_debug ("Writing audio to {}...", file_path);

      // Setup audio format manager
      juce::AudioFormatManager format_manager;
      format_manager.registerBasicFormats ();

      // Determine format from file extension
      const auto file_juce =
        utils::Utf8String::from_path (file_path).to_juce_file ();
      std::unique_ptr<juce::AudioFormat> format;
      const auto file_extension = file_juce.getFileExtension ().toLowerCase ();

      if (file_extension == ".wav")
        {
          format = std::make_unique<juce::WavAudioFormat> ();
        }
      else if (file_extension == ".aiff" || file_extension == ".aif")
        {
          format = std::make_unique<juce::AiffAudioFormat> ();
        }
      else if (file_extension == ".flac")
        {
          format = std::make_unique<juce::FlacAudioFormat> ();
        }
      else if (file_extension == ".ogg" || file_extension == ".oga")
        {
          format = std::make_unique<juce::OggVorbisAudioFormat> ();
        }
      else
        {
          throw std::runtime_error (
            QObject::tr ("Unsupported audio format: %1")
              .arg (file_extension.toStdString ())
              .toStdString ());
        }

      // Create parent directories if needed
      if (!file_juce.getParentDirectory ().createDirectory ())
        {
          throw std::runtime_error (
            QObject::tr ("Failed to create parent directories for %1")
              .arg (file_path.string ())
              .toStdString ());
        }

      // Create output stream
      std::unique_ptr<juce::OutputStream> file_output_stream =
        std::make_unique<juce::FileOutputStream> (file_juce);
      if (
        !dynamic_cast<juce::FileOutputStream &> (*file_output_stream).openedOk ())
        {
          throw std::runtime_error (
            QObject::tr ("Failed to open output file: %1")
              .arg (file_path.string ())
              .toStdString ());
        }

      // Create writer
      auto writer =
        format->createWriterFor (file_output_stream, options.writer_options_);
      if (writer == nullptr)
        {
          throw std::runtime_error (
            QObject::tr ("Failed to create audio writer for %1")
              .arg (file_path.string ())
              .toStdString ());
        }

      const auto total_samples = buffer.getNumSamples ();
      const auto block_size = options.block_length_.in<int> (units::samples);

      // Setup progress reporting
      promise.setProgressRange (0, total_samples);

      // Write in blocks
      int samples_written = 0;
      while (samples_written < total_samples)
        {
          promise.suspendIfRequested ();
          if (promise.isCanceled ())
            {
              z_debug ("Audio file writing cancelled");
              return;
            }

          // Calculate block size for this iteration
          const auto samples_remaining = total_samples - samples_written;
          const auto samples_to_write = std::min (block_size, samples_remaining);

          // Write block
          if (!writer->writeFromAudioSampleBuffer (
                buffer, samples_written, samples_to_write))
            {
              throw std::runtime_error (
                QObject::tr ("Failed to write audio data to %1")
                  .arg (file_path.string ())
                  .toStdString ());
            }

          samples_written += samples_to_write;

          // Update progress
          promise.setProgressValueAndText (
            samples_written, QObject::tr ("Writing audio file..."));
        }

      z_debug ("Successfully wrote {} samples to {}", total_samples, file_path);
    }
  catch (const std::exception &e)
    {
      z_warning ("Audio file writing failed: {}", e.what ());
      promise.setException (std::current_exception ());
    }
}

QFuture<void>
AudioFileWriter::write_async (
  WriteOptions              options,
  const fs::path           &file_path,
  juce::AudioSampleBuffer &&buffer)
{
  // This is a hack to work around the fact that QtConcurrent::run only supports
  // copyable arguments, and AudioSampleBuffer is not copyable
  return QtConcurrent::run (
    [inner_buffer = std::move (buffer)] (
      QPromise<void> &promise, WriteOptions inner_options,
      const fs::path &inner_file_path) {
      AudioFileWriter::write (
        promise, inner_options, inner_file_path,
        std::move (const_cast<juce::AudioSampleBuffer &> (inner_buffer)));
    },
    options, file_path);
}

} // namespace zrythm::utils
