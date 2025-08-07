// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_port.h"

#include <QObject>
#include <QTimer>
#include <QVector>
#include <QtQmlIntegration>

#include <juce_wrapper.h>
#include <kiss_fft.h>

/**
 * @brief Spectrum analyzer processor for QML.
 *
 * This class processes audio from two ports (left/right) and provides
 * frequency spectrum data for visualization in QML.
 */
class SpectrumAnalyzerProcessor : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::dsp::AudioPort * leftPort READ leftPort WRITE setLeftPort NOTIFY
      leftPortChanged REQUIRED)
  Q_PROPERTY (
    zrythm::dsp::AudioPort * rightPort READ rightPort WRITE setRightPort NOTIFY
      rightPortChanged REQUIRED)
  Q_PROPERTY (
    QVector<float> spectrumData READ spectrumData NOTIFY spectrumDataChanged)
  Q_PROPERTY (int fftSize READ fftSize WRITE setFftSize NOTIFY fftSizeChanged)
  Q_PROPERTY (
    float sampleRate READ sampleRate WRITE setSampleRate NOTIFY sampleRateChanged)
  QML_ELEMENT

public:
  explicit SpectrumAnalyzerProcessor (QObject * parent = nullptr);
  ~SpectrumAnalyzerProcessor () override;

  class KissFftConfig
  {
  public:
    KissFftConfig (int fft_size)
    {
      cfg_ = kiss_fft_alloc (fft_size, 0, nullptr, nullptr);
    }
    ~KissFftConfig ()
    {
      if (cfg_ != nullptr)
        {
          kiss_fft_free (cfg_);
        }
    }
    kiss_fft_cfg get_config () const { return cfg_; }

  private:
    kiss_fft_cfg cfg_{};
  };

  // ================================================================
  // QML Interface
  // ================================================================

  dsp::AudioPort * leftPort () const { return left_port_obj_; }
  void             setLeftPort (dsp::AudioPort * port_var);

  dsp::AudioPort * rightPort () const { return right_port_obj_; }
  void             setRightPort (dsp::AudioPort * port_var);

  QVector<float> spectrumData () const { return spectrum_data_; }
  int            fftSize () const { return static_cast<int> (fft_size_); }
  float          sampleRate () const { return sample_rate_; }

  void setFftSize (int size);
  void setSampleRate (float rate);

  // Frequency scaling methods for QML
  Q_INVOKABLE float getScaledFrequency (
    int   bin,
    int   num_bins,
    float min_frequency,
    float max_frequency) const;
  Q_INVOKABLE float getFrequencyForBin (int bin, int num_bins) const;

Q_SIGNALS:
  void spectrumDataChanged ();
  void fftSizeChanged ();
  void sampleRateChanged ();
  void leftPortChanged ();
  void rightPortChanged ();

  // ================================================================

private:
  void process_audio ();

  // Port handling
  QPointer<dsp::AudioPort> left_port_obj_;
  QPointer<dsp::AudioPort> right_port_obj_;
  std::optional<dsp::RingBufferOwningPortMixin::RingBufferReader>
    left_ring_reader_;
  std::optional<dsp::RingBufferOwningPortMixin::RingBufferReader>
    right_ring_reader_;

  // FFT processing
  QVector<float>     spectrum_data_;
  std::vector<float> left_buffer_;
  std::vector<float> right_buffer_;
  std::vector<float> mono_buffer_;
  std::vector<float> new_spectrum_;

  std::unique_ptr<KissFftConfig> fft_config_;
  std::vector<kiss_fft_cpx>      fft_in_;
  std::vector<kiss_fft_cpx>      fft_out_;

  std::size_t fft_size_{};
  float       sample_rate_ = 44100.0f;
  QTimer *    update_timer_ = nullptr;

  std::unique_ptr<juce::dsp::WindowingFunction<float>> windowing_func_;

  // Temporary buffer for audio processing
  juce::AudioSampleBuffer buffer_;
};
