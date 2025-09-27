// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>

#include "gui/backend/spectrum_analyzer.h"

SpectrumAnalyzerProcessor::SpectrumAnalyzerProcessor (QObject * parent)
    : QObject (parent)
{
  setFftSize (256);

  update_timer_ = new QTimer (this);
  update_timer_->setInterval (1000 / 60); // 60 FPS
  connect (
    update_timer_, &QTimer::timeout, this,
    &SpectrumAnalyzerProcessor::process_audio);
  update_timer_->start ();
}

SpectrumAnalyzerProcessor::~SpectrumAnalyzerProcessor ()
{
  if (update_timer_ != nullptr)
    {
      update_timer_->stop ();
    }
}

void
SpectrumAnalyzerProcessor::process_audio ()
{
  if (!port_obj_)
    {
      return;
    }

  // Get ring buffer readers
  auto &ring_reader = ring_reader_;

  if (!ring_reader)
    {
      return;
    }

  // Read audio data
  size_t frames_read = 0;

  auto * left_buffer = buffer_.getWritePointer (0);
  auto * right_buffer = buffer_.getWritePointer (1);

  frames_read = port_obj_->audio_ring_buffers ().at (0).peek_multiple (
    left_buffer, fft_size_);
  if (frames_read < fft_size_)
    {
      // Not enough data, pad with zeros
      utils::float_ranges::fill (
        &left_buffer[frames_read], 0.f, fft_size_ - frames_read);
    }

  frames_read = port_obj_->audio_ring_buffers ().at (1).peek_multiple (
    right_buffer, fft_size_);
  if (frames_read < fft_size_)
    {
      utils::float_ranges::fill (
        &right_buffer[frames_read], 0.f, fft_size_ - frames_read);
    }

  // Combine channels (mono mix)
  // This essentially does: mono = (left + right) / 2
  utils::float_ranges::product (
    mono_buffer_.data (), left_buffer, 0.5f, fft_size_);
  utils::float_ranges::mix_product (
    mono_buffer_.data (), right_buffer, 0.5f, fft_size_);

  // Apply Hanning window
  windowing_func_->multiplyWithWindowingTable (mono_buffer_.data (), fft_size_);

  // Perform FFT

  // Convert to complex
  for (size_t i = 0; i < fft_size_; ++i)
    {
      fft_in_[i].r = mono_buffer_[i];
      fft_in_[i].i = 0.0f;
    }

  kiss_fft (fft_config_->get_config (), fft_in_.data (), fft_out_.data ());

  // Calculate magnitude spectrum
  for (size_t i = 0; i < fft_size_ / 2; ++i)
    {
      float magnitude = std::sqrt (
        (fft_out_[i].r * fft_out_[i].r) + (fft_out_[i].i * fft_out_[i].i));
      // Normalize and apply logarithmic scaling
      magnitude = 20.0f * utils::math::fast_log10 (magnitude + 1e-10f);
      // Clamp to reasonable range
      magnitude = std::max (-60.0f, std::min (0.0f, magnitude));
      // Normalize to 0-1 range
      new_spectrum_[i] = (magnitude + 60.0f) / 60.0f;
    }

  // Apply peak fall smoothing
  for (size_t i = 0; i < new_spectrum_.size (); ++i)
    {
      if (new_spectrum_[i] > spectrum_data_[i])
        {
          spectrum_data_[i] = new_spectrum_[i];
        }
      else
        {
          spectrum_data_[i] *= 0.95f; // Fall factor
        }
    }

  Q_EMIT spectrumDataChanged ();
}

void
SpectrumAnalyzerProcessor::setStereoPort (dsp::AudioPort * port_var)
{
  if (port_obj_ == port_var)
    return;

  port_obj_ = port_var;
  ring_reader_.emplace (*port_obj_);

  QObject::connect (port_obj_, &QObject::destroyed, this, [this] () {
    ring_reader_.reset ();
    port_obj_.clear ();
  });

  Q_EMIT stereoPortChanged ();
}

void
SpectrumAnalyzerProcessor::setFftSize (int size)
{
  const auto size_unsigned = static_cast<size_t> (size);
  if (fft_size_ != size_unsigned)
    {
      fft_size_ = size_unsigned;
      spectrum_data_.resize (size / 2);
      utils::float_ranges::fill (
        spectrum_data_.data (), 0.f, spectrum_data_.size ());
      buffer_.setSize (2, size);
      mono_buffer_.resize (fft_size_);
      fft_in_.resize (fft_size_);
      fft_out_.resize (fft_size_);
      windowing_func_ = std::make_unique<juce::dsp::WindowingFunction<float>> (
        fft_size_, juce::dsp::WindowingFunction<float>::WindowingMethod::hann,
        false);
      fft_config_ = std::make_unique<KissFftConfig> (size);
      new_spectrum_.resize (fft_size_ / 2);
      Q_EMIT fftSizeChanged ();
    }
}

void
SpectrumAnalyzerProcessor::setSampleRate (float rate)
{
  if (sample_rate_ != rate)
    {
      sample_rate_ = rate;
      Q_EMIT sampleRateChanged ();
    }
}

float
SpectrumAnalyzerProcessor::getFrequencyForBin (int bin, int num_bins) const
{
  const float max_freq = sample_rate_ / 2.0f;
  const float hz_per_bin = max_freq / static_cast<float> (num_bins);
  return hz_per_bin * static_cast<float> (bin);
}

float
SpectrumAnalyzerProcessor::getScaledFrequency (
  int   bin,
  int   num_bins,
  float min_frequency,
  float max_frequency) const
{
  const float freq = getFrequencyForBin (bin, num_bins);

  // Logarithmic scaling
  const float b =
    utils::math::fast_log (max_frequency / min_frequency)
    / (max_frequency - min_frequency);
  const float a = max_frequency / std::exp (max_frequency * b);
  const float scaled_freq =
    (utils::math::fast_log ((freq + 1.0f) / a) / b) - 1.0f;

  // Normalize to 0-1 range
  return std::max (0.0f, std::min (1.0f, scaled_freq / max_frequency));
}
