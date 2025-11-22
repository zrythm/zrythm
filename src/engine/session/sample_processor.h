// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/fader.h"
#include "dsp/graph.h"
#include "dsp/midi_event.h"
#include "gui/dsp/sample_playback.h"
#include "structure/tracks/tracklist.h"
#include "utils/types.h"

#include <moodycamel/lightweightsemaphore.h>

class FileDescriptor;
class ChordPreset;

namespace zrythm::engine::device_io
{
class AudioEngine;
};

#define SAMPLE_PROCESSOR (AUDIO_ENGINE->sample_processor_)

namespace zrythm::engine::session
{
/**
 * A processor to be used in the routing graph for playing samples independent
 * of the timeline.
 *
 * Also used for auditioning files.
 */
class SampleProcessor final : public QObject, public dsp::graph::IProcessable
{
  Q_OBJECT
public:
  using PluginConfiguration = zrythm::plugins::PluginConfiguration;

public:
  SampleProcessor () = default;
  SampleProcessor (engine::device_io::AudioEngine * engine);
  Z_DISABLE_COPY_MOVE (SampleProcessor)
  ~SampleProcessor () override;

  friend void init_from (
    SampleProcessor       &obj,
    const SampleProcessor &other,
    utils::ObjectCloneType clone_type);

  void init_loaded (engine::device_io::AudioEngine * engine);

  /**
   * @brief Loads the instrument from the settings.
   *
   * To be called when the engine is activated, once.
   */
  void load_instrument_if_empty ();

  /**
   * Process the samples for the given number of frames.
   */
  void process_block (
    EngineProcessTimeInfo  time_nfo,
    const dsp::ITransport &transport) noexcept override;

  utils::Utf8String get_node_name () const override
  {
    return u8"Sample Processor";
  }

  nframes_t get_single_playback_latency () const override { return 0; }

  /**
   * Removes a SamplePlayback from the array.
   */
  void remove_sample_playback (SamplePlayback &sp);

  /**
   * Adds a sample to play to the queue from a file path.
   */
  void queue_sample_from_file (const char * path);

  /**
   * Adds a file (audio or MIDI) to the queue.
   */
  void queue_file (const FileDescriptor &file);

  /**
   * Adds a chord preset to the queue.
   */
  void queue_chord_preset (const ChordPreset &chord_pset);

  /**
   * Stops playback of files (auditioning).
   */
  void stop_file_playback ();

private:
  static constexpr auto kFaderKey = "fader"sv;
  friend void           to_json (nlohmann::json &j, const SampleProcessor &sp)
  {
    j[kFaderKey] = sp.fader_;
  }
  friend void from_json (const nlohmann::json &j, SampleProcessor &sp)
  {
    // TODO
    // j.at (kFaderKey).get_to (sp.fader_);
  }

  void init_common ();

  void queue_file_or_chord_preset (
    const FileDescriptor * file,
    const ChordPreset *    chord_pset);

public:
  /** An array of samples currently being played. */
  std::vector<SamplePlayback> current_samples_;

  /** Tracklist for file auditioning. */
  std::unique_ptr<structure::tracks::Tracklist> tracklist_;

  /** Instrument for MIDI auditioning. */
  std::unique_ptr<zrythm::plugins::PluginConfiguration> instrument_setting_;

  std::unique_ptr<dsp::MidiEvents> midi_events_;

  /** Fader connected to the main output. */
  std::unique_ptr<dsp::Fader> fader_;

  /** Playhead for the tracklist (used when auditioning files). */
  // Position playhead_;

  /**
   * Position the file ends at.
   *
   * Once this position is reached,
   * SampleProcessor.roll will be set to false.
   */
  // Position file_end_pos_;

  /** Whether to roll or not. */
  bool roll_ = false;

  /** Pointer to owner audio engine, if any. */
  engine::device_io::AudioEngine * audio_engine_ = nullptr;

  /** Semaphore to be locked while rebuilding the sample processor tracklist and
   * graph. */
  moodycamel::LightweightSemaphore rebuilding_sem_{ 1 };
};

}
