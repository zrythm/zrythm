// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph_node.h"
#include "dsp/itransport.h"
#include "dsp/midi_event_buffer.h"
#include "dsp/timeline_data_cache.h"
#include "structure/arrangement/clip_renderer.h"
#include "utils/expandable_tick_range.h"
#include "utils/qt.h"

#include <farbot/RealtimeObject.hpp>

namespace zrythm::structure::arrangement
{

/**
 * @brief Base class for timeline data providers.
 *
 * Provides common functionality for all timeline data provider types.
 * This is an abstract base class that defines the interface that
 * all derived provider classes must implement.
 */
class TimelineDataProvider : public QObject
{
  Q_OBJECT

public:
  using IntervalType = std::pair<units::sample_t, units::sample_t>;

  explicit TimelineDataProvider (QObject * parent = nullptr) : QObject (parent)
  {
  }

  ~TimelineDataProvider () override;

  /**
   * @brief Generate the event sequence to be used during realtime processing.
   *
   * To be called as needed from the UI thread when a new cache is requested.
   *
   * @tparam ClipType Either MidiClip, AudioClip, ChordClip, or
   * AutomationClip
   * @param self The derived class instance (explicit this parameter)
   * @param tempo_map The tempo map for timing conversion.
   * @param clips The clips to process.
   * @param affected_range The range of ticks to process.
   */
  template <ClipObject ClipType>
  void generate_events (
    this auto                            &self,
    const dsp::TempoMap                  &tempo_map,
    utils::RangeOf<const ClipType *> auto clips,
    utils::ExpandableTickRange            affected_range)
  {
    // Convert tick range to sample range
    const auto sample_interval = [&affected_range, &tempo_map] ()
      -> std::pair<units::sample_t, units::sample_t> {
      if (!affected_range.is_full_content ())
        {
          const auto tick_range = affected_range.range ().value ();
          return std::make_pair (
            tempo_map.tick_to_samples_rounded (
              dsp::TimelineTick{ units::ticks (tick_range.first) }),
            tempo_map.tick_to_samples_rounded (
              dsp::TimelineTick{ units::ticks (tick_range.second) }));
        }
      return std::make_pair (
        units::samples (static_cast<int64_t> (0)),
        units::samples (std::numeric_limits<int64_t>::max ()));
    }();

    // Remove existing caches at given interval (or all caches if no interval
    // given)

    if (affected_range.is_full_content ())
      {
        self.get_base_cache ()->clear ();
      }
    else
      {
        self.get_base_cache ()->remove_sequences_matching_interval (
          sample_interval);
      }

    const auto clips_inside_interval_filter_func =
      [&affected_range, sample_interval] (const auto &clip) {
        if (affected_range.is_full_content ())
          return true;

        return clip->is_hit_by_range (sample_interval);
      };

    const auto cache_clip = [&] (const auto * r) {
      // Skip muted clips
      if (r->mute ()->muted ())
        return;

      if constexpr (
        std::is_same_v<ClipType, arrangement::MidiClip>
        || std::is_same_v<ClipType, arrangement::ChordClip>)
        {
          self.cache_midi_clip (*r, tempo_map);
        }
      else if constexpr (std::is_same_v<ClipType, arrangement::AudioClip>)
        {
          self.cache_audio_clip (*r);
        }
      else if constexpr (std::is_same_v<ClipType, arrangement::AutomationClip>)
        {
          self.cache_automation_clip (*r, tempo_map);
        }
    };

    // Go through each clip and add a cache
    std::ranges::for_each (
      std::views::filter (clips, clips_inside_interval_filter_func), cache_clip);

    // Finalize
    self.get_base_cache ()->finalize_changes ();
  }

  virtual void clear_all_caches () = 0;
  virtual void
  remove_sequences_matching_interval_from_all_caches (IntervalType interval) = 0;

  virtual const dsp::TimelineDataCache * get_base_cache () const = 0;

protected:
  virtual dsp::TimelineDataCache * get_base_cache () = 0;
  /** Last transport state we've seen */
  dsp::ITransport::PlayState last_seen_transport_state_{
    dsp::ITransport::PlayState::Paused
  };

  /** Next expected transport position (for detecting jumps) */
  units::sample_t next_expected_transport_position_;
};

/**
 * @brief MIDI-specific timeline data provider.
 *
 * Handles caching of MIDI sequences with thread-safe access and
 * range-based updates.
 */
class MidiTimelineDataProvider : public TimelineDataProvider
{
  friend class TimelineDataProvider;

public:
  explicit MidiTimelineDataProvider (QObject * parent = nullptr);

  const dsp::TimelineDataCache * get_base_cache () const override
  {
    return midi_cache_.get ();
  }

  /**
   * Process MIDI events for the given time range.
   */
  void process_midi_events (
    const dsp::graph::ProcessBlockInfo &time_nfo,
    dsp::ITransport::PlayState          transport_state,
    dsp::MidiEventBuffer &output_buffer) noexcept [[clang::nonblocking]];

  void clear_all_caches () override;
  void remove_sequences_matching_interval_from_all_caches (
    IntervalType interval) override;

  std::span<const dsp::SampleBasedMidiEvent> midi_events () const;

  /**
   * @brief Generate the MIDI event sequence to be used during realtime
   * processing.
   *
   * To be called as needed from the UI thread when a new cache is requested.
   *
   * @param tempo_map The tempo map for timing conversion.
   * @param midi_clips The MIDI clips to process.
   * @param affected_range The range of ticks to process.
   */
  void generate_midi_events (
    const dsp::TempoMap                               &tempo_map,
    utils::RangeOf<const arrangement::MidiClip *> auto midi_clips,
    utils::ExpandableTickRange                         affected_range)
  {
    generate_events<arrangement::MidiClip> (
      tempo_map, midi_clips, affected_range);
    set_midi_events (midi_cache_->midi_events ());
  }

  /**
   * @brief Generate the MIDI event sequence to be used during realtime
   * processing.
   *
   * To be called as needed from the UI thread when a new cache is requested.
   *
   * @param tempo_map The tempo map for timing conversion.
   * @param chord_clips The Chord clips to process.
   * @param affected_range The range of ticks to process.
   */
  void generate_midi_events (
    const dsp::TempoMap                                &tempo_map,
    utils::RangeOf<const arrangement::ChordClip *> auto chord_clips,
    utils::ExpandableTickRange                          affected_range)
  {
    generate_events<arrangement::ChordClip> (
      tempo_map, chord_clips, affected_range);
    set_midi_events (midi_cache_->midi_events ());
  }

protected:
  dsp::TimelineDataCache * get_base_cache () override
  {
    return midi_cache_.get ();
  }

private:
  /**
   * Caches a MIDI-like clip (MidiClip or ChordClip) to the MIDI cache.
   */
  template <typename MidiClipType>
  void
  cache_midi_clip (const MidiClipType &clip, const dsp::TempoMap &tempo_map)
  {
    // MIDI clip processing
    juce::MidiMessageSequence clip_seq;

    // Serialize clip (timings in timeline ticks)
    arrangement::ClipRenderer::serialize_to_sequence (clip, clip_seq);
    clip_seq.addTimeToMessages (clip.position ()->ticks ());

    // Convert JUCE sequence to native events with sample timestamps
    std::vector<dsp::SampleBasedMidiEvent> native_events;
    native_events.reserve (clip_seq.getNumEvents ());
    for (const auto &event : clip_seq)
      {
        const auto sample_time = tempo_map.tick_to_samples_rounded (
          dsp::TimelineTick{ units::ticks (event->message.getTimeStamp ()) });
        const auto * raw = event->message.getRawData ();
        const auto   raw_size =
          static_cast<size_t> (event->message.getRawDataSize ());
        native_events.push_back (
          dsp::midi_event::make_raw (
            std::span<const midi_byte_t>{ raw, raw_size }, sample_time));
      }

    // Add to cache
    midi_cache_->add_midi_sequence (
      std::make_pair (
        tempo_map.tick_to_samples_rounded (clip.position ()->asTick ()),
        clip.get_end_position_samples (true)),
      native_events);
  }

  /**
   * @brief Set the MIDI events for realtime access.
   *
   * @param events The MIDI events sequence.
   */
  void set_midi_events (std::span<const dsp::SampleBasedMidiEvent> events);

  utils::QObjectUniquePtr<dsp::MidiTimelineDataCache> midi_cache_;

  farbot::RealtimeObject<
    std::vector<dsp::SampleBasedMidiEvent>,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    active_midi_playback_sequence_;
};

/**
 * @brief Audio-specific timeline data provider.
 *
 * Handles caching of audio clips with thread-safe access and
 * range-based updates.
 */
class AudioTimelineDataProvider : public TimelineDataProvider
{
  friend class TimelineDataProvider;

public:
  explicit AudioTimelineDataProvider (QObject * parent = nullptr);

  const dsp::TimelineDataCache * get_base_cache () const override
  {
    return audio_cache_.get ();
  }

  /**
   * Process audio events for the given time range.
   */
  void process_audio_events (
    const dsp::graph::ProcessBlockInfo &time_nfo,
    dsp::ITransport::PlayState          transport_state,
    std::span<float>                    output_left,
    std::span<float> output_right) noexcept [[clang::nonblocking]];

  void clear_all_caches () override;
  void remove_sequences_matching_interval_from_all_caches (
    IntervalType interval) override;

  std::span<const dsp::AudioTimelineDataCache::AudioClipEntry>
  audio_clips () const;

  /**
   * @brief Generate the audio event sequence to be used during realtime
   * processing.
   *
   * To be called as needed from the UI thread when a new cache is requested.
   *
   * @param tempo_map The tempo map for timing conversion.
   * @param audio_clips The audio clips to process.
   * @param affected_range The range of ticks to process.
   */
  void generate_audio_events (
    const dsp::TempoMap                                &tempo_map,
    utils::RangeOf<const arrangement::AudioClip *> auto audio_clips,
    utils::ExpandableTickRange                          affected_range)
  {
    generate_events<arrangement::AudioClip> (
      tempo_map, audio_clips, affected_range);
    set_audio_clips (audio_cache_->audio_clips ());
  }

protected:
  dsp::TimelineDataCache * get_base_cache () override
  {
    return audio_cache_.get ();
  }

private:
  /**
   * Caches an AudioClip to the audio cache.
   */
  void cache_audio_clip (const arrangement::AudioClip &clip);

  /**
   * @brief Set the audio clips for realtime access.
   *
   * @param clips The audio clip entries.
   */
  void set_audio_clips (
    std::span<const dsp::AudioTimelineDataCache::AudioClipEntry> clips);

  utils::QObjectUniquePtr<dsp::AudioTimelineDataCache> audio_cache_;

  farbot::RealtimeObject<
    std::vector<dsp::AudioTimelineDataCache::AudioClipEntry>,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    active_audio_clips_;
};

/**
 * @brief Automation-specific timeline data provider.
 *
 * Handles caching of automation sequences with thread-safe access and
 * range-based updates.
 */
class AutomationTimelineDataProvider : public TimelineDataProvider
{
  friend class TimelineDataProvider;

public:
  explicit AutomationTimelineDataProvider (QObject * parent = nullptr);

  const dsp::TimelineDataCache * get_base_cache () const override
  {
    return automation_cache_.get ();
  }

  /**
   * Process automation events for the given time range.
   */
  void process_automation_events (
    const dsp::graph::ProcessBlockInfo &time_nfo,
    dsp::ITransport::PlayState          transport_state,
    std::span<float> output_values) noexcept [[clang::nonblocking]];

  /**
   * Get the automation value for a specific sample position.
   *
   * @param sample_position The sample position to get the value for
   * @return The automation value at the given position, if any
   */
  std::optional<float>
  get_automation_value_rt (units::sample_t sample_position) noexcept
    [[clang::nonblocking]];

  void clear_all_caches () override;
  void remove_sequences_matching_interval_from_all_caches (
    IntervalType interval) override;

  std::span<const dsp::AutomationTimelineDataCache::AutomationCacheEntry>
  automation_sequences () const;

  /**
   * @brief Generate the automation event sequence to be used during realtime
   * processing.
   *
   * To be called as needed from the UI thread when a new cache is requested.
   *
   * @param tempo_map The tempo map for timing conversion.
   * @param automation_clips The automation clips to process.
   * @param affected_range The range of ticks to process.
   */
  void generate_automation_events (
    const dsp::TempoMap                                     &tempo_map,
    utils::RangeOf<const arrangement::AutomationClip *> auto automation_clips,
    utils::ExpandableTickRange                               affected_range)
  {
    generate_events<arrangement::AutomationClip> (
      tempo_map, automation_clips, affected_range);
    set_automation_sequences (automation_cache_->automation_sequences ());
  }

protected:
  dsp::TimelineDataCache * get_base_cache () override
  {
    return automation_cache_.get ();
  }

private:
  /**
   * Caches an AutomationClip to the automation cache.
   */
  void cache_automation_clip (
    const arrangement::AutomationClip &clip,
    const dsp::TempoMap               &tempo_map);

  /**
   * @brief Set the automation sequences for realtime access.
   *
   * @param sequences The automation cache entries.
   */
  void set_automation_sequences (
    std::span<const dsp::AutomationTimelineDataCache::AutomationCacheEntry>
      sequences);

  utils::QObjectUniquePtr<dsp::AutomationTimelineDataCache> automation_cache_;

  farbot::RealtimeObject<
    std::vector<dsp::AutomationTimelineDataCache::AutomationCacheEntry>,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    active_automation_sequences_;
};

} // namespace zrythm::structure::arrangement
