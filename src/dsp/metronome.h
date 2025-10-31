// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_sample_processor.h"
#include "dsp/tempo_map.h"
#include "utils/types.h"

#include <QtQmlIntegration>

#include <juce_wrapper.h>

namespace zrythm::dsp
{
/**
 * Metronome processor.
 */
class Metronome : public QObject, public AudioSampleProcessor
{
  Q_OBJECT
  Q_PROPERTY (float volume READ volume WRITE setVolume NOTIFY volumeChanged)
  Q_PROPERTY (bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  Metronome (
    ProcessorBaseDependencies dependencies,
    const dsp::ITransport    &transport,
    const dsp::TempoMap      &tempo_map,
    juce::AudioSampleBuffer   emphasis_sample,
    juce::AudioSampleBuffer   normal_sample,
    bool                      initially_enabled = true,
    float                     initial_volume = 1.f,
    QObject *                 parent = nullptr);

  // ============================================================================
  // QML Interface
  // ============================================================================

  float volume () const { return volume_.load (); }
  void  setVolume (float volume)
  {
    if (utils::values_equal_for_qproperty_type (volume_.load (), volume))
      return;

    volume_.store (volume);
    Q_EMIT volumeChanged (volume);
  }
  Q_SIGNAL void volumeChanged (float volume);

  bool enabled () const { return enabled_.load (); }
  void setEnabled (bool enabled)
  {
    if (utils::values_equal_for_qproperty_type (enabled_.load (), enabled))
      return;

    enabled_.store (enabled);
    Q_EMIT enabledChanged (enabled);
  }
  Q_SIGNAL void enabledChanged (bool enabled);

  // ============================================================================

  void custom_process_block (EngineProcessTimeInfo time_nfo) noexcept override;

  void custom_prepare_for_processing (
    sample_rate_t sample_rate,
    nframes_t     max_block_length) override;

private:
  /**
   * Finds all metronome events (beat and bar changes)
   * within the given range and adds them to the
   * queue.
   *
   * @param end_pos End position, exclusive.
   * @param loffset Local offset (this is where @p start_pos starts at).
   */
  void find_and_queue_metronome_samples (
    units::sample_t start_pos,
    units::sample_t end_pos,
    units::sample_t loffset);

  /**
   * Queues metronome events for preroll count-in.
   */
  void queue_metronome_countin (const EngineProcessTimeInfo &time_nfo);

  /**
   * Queues a metronomem tick at the given local offset.
   *
   * Realtime function.
   */
  void queue_metronome (
    bool                 emphasis,
    units::sample_t      offset,
    std::source_location loc = std::source_location::current ());

private:
  const dsp::ITransport &transport_;
  const dsp::TempoMap   &tempo_map_;

  /** The emphasis sample. */
  juce::AudioSampleBuffer emphasis_sample_buffer_;

  /** The normal sample. */
  juce::AudioSampleBuffer normal_sample_buffer_;

  /**
   * @brief Volume in amplitude (0.0 - 2.0)
   */
  std::atomic<float> volume_;

  std::atomic_bool enabled_;
};

} // namespace zrythm::dsp
