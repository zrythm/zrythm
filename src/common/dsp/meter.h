// SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Meter DSP.
 */

#ifndef __AUDIO_METER_H__
#define __AUDIO_METER_H__

#include "common/dsp/kmeter_dsp.h"
#include "common/dsp/peak_dsp.h"
#include "common/dsp/true_peak_dsp.h"
#include "common/utils/types.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"

class TruePeakDsp;
class KMeterDsp;
class PeakDsp;
class Port;

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

  /** @note True peak is intensive, only use it
   * where needed (mixer). */
  METER_ALGORITHM_TRUE_PEAK,
  METER_ALGORITHM_RMS,
  METER_ALGORITHM_K,
};

/**
 * A Meter used by a single GUI element.
 */
class Meter
{
public:
  Meter (Port &port);

  /**
   * Get the current meter value.
   *
   * This should only be called once in a draw cycle.
   */
  void get_value (AudioValueFormat format, float * val, float * max);

public:
  /** Port associated with this meter. */
  Port * port_;

  /** True peak processor. */
  std::unique_ptr<TruePeakDsp> true_peak_processor_;
  std::unique_ptr<TruePeakDsp> true_peak_max_processor_;

  /** Current true peak. */
  float true_peak_;
  float true_peak_max_;

  /** K RMS processor, if K meter. */
  std::unique_ptr<KMeterDsp> kmeter_processor_;

  std::unique_ptr<PeakDsp> peak_processor_;

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

  gint64 last_midi_trigger_time_;

private:
  std::vector<float> tmp_buf_;
};

#endif
