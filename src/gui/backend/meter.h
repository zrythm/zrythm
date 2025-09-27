// SPDX-FileCopyrightText: © 2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/kmeter_dsp.h"
#include "dsp/peak_dsp.h"
#include "dsp/port.h"
#include "dsp/true_peak_dsp.h"
#include "utils/types.h"
#include "utils/variant_helpers.h"

#include <QtQmlIntegration>

#include <boost/container/static_vector.hpp>

/**
 * @addtogroup dsp
 *
 * @{
 */

enum class MeterAlgorithm
{
  /** Use default algorithm for the port. */
  METER_ALGORITHM_AUTO,

  METER_ALGORITHM_DIGITAL_PEAK,

  /** @note True peak is intensive, only use it where needed (mixer). */
  METER_ALGORITHM_TRUE_PEAK,
  METER_ALGORITHM_RMS,
  METER_ALGORITHM_K,
};

/**
 * @brief A meter processor for a single GUI element.
 *
 * This class is responsible for processing the meter values for a single GUI
 * element, such as a volume meter. It supports various meter algorithms,
 * including digital peak, true peak, RMS, and K-meter.
 *
 * The meter processor is associated with a port, which can be either an
 * AudioPort or a MidiPort. The meter values are updated based on the data
 * from the associated port.
 *
 * The meter processor emits the `valuesChanged` signal whenever the meter
 * values are updated, allowing the GUI to update the display accordingly.
 */
class MeterProcessor : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (QVariant port READ port WRITE setPort REQUIRED)
  Q_PROPERTY (int channel READ channel WRITE setChannel)
  Q_PROPERTY (
    float currentAmplitude READ currentAmplitude NOTIFY currentAmplitudeChanged)
  Q_PROPERTY (float peakAmplitude READ peakAmplitude NOTIFY peakAmplitudeChanged)

public:
  using MeterPortVariant = std::variant<dsp::MidiPort, dsp::AudioPort>;
  using MeterPortPtrVariant = to_pointer_variant<MeterPortVariant>;

  MeterProcessor (QObject * parent = nullptr);

  // ================================================================
  // QML Interface
  // ================================================================

  QVariant port () const { return QVariant::fromValue (port_obj_); }
  void     setPort (QVariant port_var);

  int  channel () const { return channel_; }
  void setChannel (int channel);

  float currentAmplitude () const
  {
    return current_amp_.load (std::memory_order_relaxed);
  }
  Q_SIGNAL void currentAmplitudeChanged (float value);

  float peakAmplitude () const
  {
    return peak_amp_.load (std::memory_order_relaxed);
  }
  Q_SIGNAL void peakAmplitudeChanged (float value);

  Q_INVOKABLE float toDBFS (float amp) const;
  Q_INVOKABLE float toFader (float amp) const;

  // ================================================================

private:
  /**
   * Get the current meter value.
   *
   * This should only be called once in a draw cycle.
   */
  void get_value (AudioValueFormat format, float * val, float * max);

public:
  /** Port associated with this meter. */
  QPointer<QObject> port_obj_;

  // RAII request for port ring buffers to be filled
  std::optional<dsp::RingBufferOwningPortMixin::RingBufferReader>
    ring_buffer_reader_;

  /** True peak processor. */
  std::unique_ptr<zrythm::dsp::TruePeakDsp> true_peak_processor_;
  std::unique_ptr<zrythm::dsp::TruePeakDsp> true_peak_max_processor_;

  /** Current true peak. */
  float true_peak_ = 0.f;
  float true_peak_max_ = 0.f;

  /** K RMS processor, if K meter. */
  std::unique_ptr<zrythm::dsp::KMeterDsp> kmeter_processor_;

  std::unique_ptr<zrythm::dsp::PeakDsp> peak_processor_;

  /**
   * Algorithm to use.
   *
   * Auto by default.
   */
  MeterAlgorithm algorithm_ = MeterAlgorithm::METER_ALGORITHM_AUTO;

  /** Previous max, used when holding the max value. */
  float prev_max_ = 0.f;

  /** Last meter value (in amplitude), used to show a falloff and avoid sudden
   * dips. */
  float last_amp_ = 0.f;

  /** Time the last val was taken at (last draw time). */
  SteadyTimePoint last_draw_time_;

  qint64 last_midi_trigger_time_ = 0;

private:
  std::vector<float> tmp_buf_;

  std::atomic<float> current_amp_ = 0.f;
  std::atomic<float> peak_amp_ = 0.f;

  // Audio port channel, if audio meter.
  int channel_{};

  boost::container::static_vector<dsp::MidiEvent, 256> tmp_events_;
};
