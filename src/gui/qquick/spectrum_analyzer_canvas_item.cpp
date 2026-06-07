// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cmath>

#include "dsp/audio_port.h"
#include "dsp/port_observation_manager.h"
#include "dsp/port_observation_token.h"
#include "gui/qquick/spectrum_analyzer_canvas_item.h"
#include "gui/qquick/spectrum_analyzer_canvas_renderer.h"
#include "utils/float_ranges.h"
#include "utils/math_utils.h"
#include "utils/qt.h"

#include <juce_dsp/juce_dsp.h>
#include <kiss_fft.h>

namespace
{

class KissFftConfig
{
public:
  explicit KissFftConfig (int fft_size)
  {
    cfg_ = kiss_fft_alloc (fft_size, 0, nullptr, nullptr);
  }
  ~KissFftConfig ()
  {
    if (cfg_ != nullptr)
      kiss_fft_free (cfg_);
  }
  kiss_fft_cfg get_config () const { return cfg_; }

private:
  kiss_fft_cfg cfg_{};
};

}

namespace zrythm::gui::qquick
{

struct SpectrumAnalyzerCanvasItem::Impl
{
  QPointer<dsp::AudioPort>              port_;
  QPointer<dsp::PortObservationManager> observation_manager_;
  std::optional<dsp::ObservationToken>  observation_token_;

  std::size_t fft_size_{ 256 };
  float       sample_rate_ = 44100.0f;
  QColor      spectrum_color_;

  QVector<float>     spectrum_data_;
  std::vector<float> mono_buffer_;
  std::vector<float> new_spectrum_;

  std::unique_ptr<KissFftConfig>                       fft_config_;
  std::vector<kiss_fft_cpx>                            fft_in_;
  std::vector<kiss_fft_cpx>                            fft_out_;
  std::unique_ptr<juce::dsp::WindowingFunction<float>> windowing_func_;

  utils::QObjectUniquePtr<QTimer> timer_;
  uint64_t                        spectrum_generation_ = 0;
};

SpectrumAnalyzerCanvasItem::SpectrumAnalyzerCanvasItem (QQuickItem * parent)
    : QCanvasPainterItem (parent), impl_ (std::make_unique<Impl> ())
{
  init_fft (256);

  impl_->timer_ = utils::make_qobject_unique<QTimer> (this);
  impl_->timer_->setInterval (1000 / 60);
  connect (
    impl_->timer_.get (), &QTimer::timeout, this,
    &SpectrumAnalyzerCanvasItem::process_audio);
  impl_->timer_->start ();
}

SpectrumAnalyzerCanvasItem::~SpectrumAnalyzerCanvasItem () = default;

QCanvasPainterItemRenderer *
SpectrumAnalyzerCanvasItem::createItemRenderer () const
{
  return new SpectrumAnalyzerCanvasRenderer ();
}

zrythm::dsp::AudioPort *
SpectrumAnalyzerCanvasItem::stereoPort () const
{
  return impl_->port_;
}

void
SpectrumAnalyzerCanvasItem::setStereoPort (zrythm::dsp::AudioPort * port)
{
  if (impl_->port_ == port)
    return;

  if (impl_->port_ != nullptr)
    {
      impl_->observation_token_.reset ();
      QObject::disconnect (impl_->port_, &QObject::destroyed, this, nullptr);
      impl_->port_.clear ();
    }

  impl_->port_ = port;
  if (impl_->port_ != nullptr)
    {
      connect (impl_->port_, &QObject::destroyed, this, [this] () {
        impl_->observation_token_.reset ();
        impl_->port_.clear ();
      });
      try_create_token ();
    }

  Q_EMIT stereoPortChanged ();
}

zrythm::dsp::PortObservationManager *
SpectrumAnalyzerCanvasItem::portObservationManager () const
{
  return impl_->observation_manager_;
}

void
SpectrumAnalyzerCanvasItem::setPortObservationManager (
  zrythm::dsp::PortObservationManager * manager)
{
  if (impl_->observation_manager_ == manager)
    return;
  impl_->observation_manager_ = manager;
  try_create_token ();
  Q_EMIT portObservationManagerChanged ();
}

void
SpectrumAnalyzerCanvasItem::try_create_token ()
{
  if (impl_->port_ == nullptr || impl_->observation_manager_ == nullptr)
    return;
  impl_->observation_token_.emplace (
    *impl_->observation_manager_, *impl_->port_);
}

int
SpectrumAnalyzerCanvasItem::fftSize () const
{
  return static_cast<int> (impl_->fft_size_);
}

float
SpectrumAnalyzerCanvasItem::sampleRate () const
{
  return impl_->sample_rate_;
}

QColor
SpectrumAnalyzerCanvasItem::spectrumColor () const
{
  return impl_->spectrum_color_;
}

void
SpectrumAnalyzerCanvasItem::setFftSize (int size)
{
  if (size <= 0)
    return;
  if (static_cast<std::size_t> (size) == impl_->fft_size_)
    return;
  init_fft (size);
  Q_EMIT fftSizeChanged ();
}

void
SpectrumAnalyzerCanvasItem::setSampleRate (float rate)
{
  if (utils::math::floats_equal (impl_->sample_rate_, rate))
    return;
  impl_->sample_rate_ = rate;
  Q_EMIT sampleRateChanged ();
}

void
SpectrumAnalyzerCanvasItem::setSpectrumColor (const QColor &color)
{
  if (impl_->spectrum_color_ == color)
    return;
  impl_->spectrum_color_ = color;
  Q_EMIT spectrumColorChanged ();
}

float
SpectrumAnalyzerCanvasItem::scaled_frequency (
  float sample_rate,
  int   bin,
  int   num_bins,
  float min_frequency,
  float max_frequency)
{
  if (num_bins <= 0 || max_frequency <= min_frequency)
    return 0.0f;

  const float max_freq = sample_rate / 2.0f;
  const float hz_per_bin = max_freq / static_cast<float> (num_bins);
  const float freq = hz_per_bin * static_cast<float> (bin);
  const float b =
    utils::math::fast_log (max_frequency / min_frequency)
    / (max_frequency - min_frequency);
  const float a = max_frequency / std::exp (max_frequency * b);
  const float scaled_freq =
    (utils::math::fast_log ((freq + 1.0f) / a) / b) - 1.0f;
  return std::max (0.0f, std::min (1.0f, scaled_freq / max_frequency));
}

float
SpectrumAnalyzerCanvasItem::getScaledFrequency (
  int   bin,
  int   num_bins,
  float min_frequency,
  float max_frequency) const
{
  return scaled_frequency (
    impl_->sample_rate_, bin, num_bins, min_frequency, max_frequency);
}

float
SpectrumAnalyzerCanvasItem::getFrequencyForBin (int bin, int num_bins) const
{
  if (num_bins <= 0)
    return 0.0f;
  const float max_freq = impl_->sample_rate_ / 2.0f;
  const float hz_per_bin = max_freq / static_cast<float> (num_bins);
  return hz_per_bin * static_cast<float> (bin);
}

const QVector<float> &
SpectrumAnalyzerCanvasItem::spectrumData () const
{
  return impl_->spectrum_data_;
}

uint64_t
SpectrumAnalyzerCanvasItem::spectrumGeneration () const
{
  return impl_->spectrum_generation_;
}

void
SpectrumAnalyzerCanvasItem::process_audio ()
{
  if (!impl_->port_ || !impl_->observation_token_)
    return;

  auto &cache = impl_->observation_token_->cache ();
  if (cache.audio.size () < 2)
    return;

  const auto &ch0 = cache.audio[0];
  const auto &ch1 = cache.audio[1];

  if (ch0.empty () && ch1.empty ())
    return;

  const auto n = impl_->fft_size_;

  // Mix to mono: (ch0 + ch1) / 2
  auto &mono = impl_->mono_buffer_;
  if (!ch0.empty ())
    {
      const auto copy_n = std::min (n, ch0.size ());
      utils::float_ranges::copy (
        { mono.data (), copy_n }, { ch0.data (), copy_n });
      if (copy_n < n)
        utils::float_ranges::fill ({ &mono[copy_n], n - copy_n }, 0.f);
      // Scale ch0 by 0.5
      utils::float_ranges::product (
        { mono.data (), copy_n }, { mono.data (), copy_n }, 0.5f);
    }
  else
    {
      utils::float_ranges::fill ({ mono.data (), n }, 0.f);
    }

  if (!ch1.empty ())
    {
      const auto copy_n = std::min (n, ch1.size ());
      // mono = ch0 * 0.5 + ch1 * 0.5
      utils::float_ranges::mix_product (
        { mono.data (), copy_n }, { ch1.data (), copy_n }, 0.5f);
    }

  cache.clear_audio ();

  // Apply window
  impl_->windowing_func_->multiplyWithWindowingTable (mono.data (), n);

  // FFT
  for (size_t i = 0; i < n; ++i)
    {
      impl_->fft_in_[i].r = mono[i];
      impl_->fft_in_[i].i = 0.0f;
    }
  kiss_fft (
    impl_->fft_config_->get_config (), impl_->fft_in_.data (),
    impl_->fft_out_.data ());

  // Magnitude spectrum
  for (size_t i = 0; i < n / 2; ++i)
    {
      float magnitude = std::sqrt (
        impl_->fft_out_[i].r * impl_->fft_out_[i].r
        + impl_->fft_out_[i].i * impl_->fft_out_[i].i);
      magnitude = 20.0f * utils::math::fast_log10 (magnitude + 1e-10f);
      magnitude = std::max (-60.0f, std::min (0.0f, magnitude));
      impl_->new_spectrum_[i] = (magnitude + 60.0f) / 60.0f;
    }

  // Peak fall smoothing
  for (size_t i = 0; i < impl_->new_spectrum_.size (); ++i)
    {
      if (impl_->new_spectrum_[i] > impl_->spectrum_data_[i])
        impl_->spectrum_data_[i] = impl_->new_spectrum_[i];
      else
        impl_->spectrum_data_[i] *= 0.95f;
    }

  ++impl_->spectrum_generation_;
  update ();
}

void
SpectrumAnalyzerCanvasItem::init_fft (int size)
{
  const auto n = static_cast<std::size_t> (size);
  impl_->fft_size_ = n;
  impl_->spectrum_data_.resize (size / 2);
  impl_->spectrum_data_.fill (0.f);
  impl_->mono_buffer_.resize (n);
  impl_->fft_in_.resize (n);
  impl_->fft_out_.resize (n);
  impl_->windowing_func_ = std::make_unique<juce::dsp::WindowingFunction<float>> (
    n, juce::dsp::WindowingFunction<float>::WindowingMethod::hann, false);
  impl_->fft_config_ = std::make_unique<KissFftConfig> (size);
  impl_->new_spectrum_.resize (n / 2);
}

}
