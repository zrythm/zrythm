// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdexcept>

#include "controllers/recording_materializer.h"
#include "dsp/file_audio_source.h"
#include "dsp/midi_event.h"
#include "structure/arrangement/audio_source_object.h"
#include "structure/arrangement/midi_control_event.h"
#include "utils/audio.h"
#include "utils/logger.h"
#include "utils/midi.h"

namespace zrythm::controllers
{

RecordingMaterializer::RecordingMaterializer (
  RecordingCoordinator  &recording_coordinator,
  undo::UndoStack       &undo_stack,
  ArrangerObjectCreators creators,
  RecordingModeProvider  recording_mode_provider,
  QObject *              parent)
    : QObject (parent), recording_coordinator_ (recording_coordinator),
      undo_stack_ (&undo_stack), creators_ (std::move (creators)),
      recording_mode_provider_ (std::move (recording_mode_provider))
{
  QObject::connect (
    &recording_coordinator_, &RecordingCoordinator::audioDataReady, this,
    &RecordingMaterializer::on_audio_data_ready);
  QObject::connect (
    &recording_coordinator_, &RecordingCoordinator::midiDataReady, this,
    &RecordingMaterializer::on_midi_data_ready);
  QObject::connect (
    &recording_coordinator_, &RecordingCoordinator::recordingSessionEnded, this,
    &RecordingMaterializer::finalize_recording_macro);
}

RecordingMaterializer::~RecordingMaterializer ()
{
  disconnect (&recording_coordinator_, nullptr, this, nullptr);
  if (recording_macro_active_)
    {
      z_warning (
        "RecordingMaterializer destroyed with active recording macro — "
        "finalizing");
      finalize_recording_macro ();
    }
}

void
RecordingMaterializer::on_audio_data_ready (
  structure::tracks::TrackUuid             track_id,
  const std::vector<RecordingAudioPacket> &packets)
{
  for (const auto &packet : packets)
    {
      if (!packet.transport_recording)
        continue;

      auto      &state = track_states_[track_id];
      const auto num_frames =
        static_cast<int> (packet.nframes.in (units::samples));

      const bool discontinuous =
        !state.last_end_position.has_value ()
        || packet.timeline_position != *state.last_end_position;

      if (discontinuous)
        {
          handle_discontinuity (state);
        }

      scratch_buf_.setSize (2, num_frames, false, false, true);
      scratch_buf_.copyFrom (0, 0, packet.l_frames.data (), num_frames);
      scratch_buf_.copyFrom (1, 0, packet.r_frames.data (), num_frames);

      bool newly_created = false;
      if (!state.current_clip.has_value ())
        {
          state.current_clip = get_or_create_clip (
            state, track_id, packet.timeline_position, scratch_buf_);

          if (!state.current_clip.has_value ())
            {
              z_warning (
                "Failed to create clip for track {} at position {} — "
                "dropping {} frames",
                track_id, packet.timeline_position,
                packet.nframes.in (units::samples));
              continue;
            }

          newly_created = true;
        }

      auto * clip =
        state.current_clip->get_object_as<structure::arrangement::AudioClip> ();
      auto * source = get_audio_source_for_clip (*clip);

      if (!newly_created)
        {
          source->expand_with_frames (scratch_buf_);
        }

      // Length is the tick span of the recorded audio at its timeline
      // position
      const auto &tm = clip->get_tempo_map ();
      const auto  clip_start_samples =
        tm.tick_to_samples_rounded (clip->position ()->asTick ());
      const auto clip_end_samples =
        clip_start_samples
        + units::samples (static_cast<int64_t> (source->get_num_frames ()));
      clip->length ()->setTicks (
        (tm.samples_to_tick (clip_end_samples) - clip->position ()->asTick ())
          .asDouble ());

      state.last_end_position =
        packet.timeline_position
        + units::samples (static_cast<int64_t> (num_frames));
    }
}

std::optional<structure::arrangement::ArrangerObjectUuidReference>
RecordingMaterializer::get_or_create_clip (
  TrackRecordingState             &state,
  structure::tracks::TrackUuid     track_id,
  units::sample_t                  start_position,
  const utils::audio::AudioBuffer &initial_frames)
{
  ensure_recording_macro ();

  auto result = creators_.audio_clip (
    track_id, start_position, initial_frames, state.current_lane_index);
  if (!result.has_value ())
    return std::nullopt;
  state.current_lane_index = result->actual_lane_index;
  return result->clip;
}

dsp::FileAudioSource *
RecordingMaterializer::get_audio_source_for_clip (
  structure::arrangement::AudioClip &clip)
{
  auto children = clip.get_children_view ();
  assert (!children.empty ());
  auto * source_obj = children.front ();
  auto   clip_ref = source_obj->audio_source_ref ();
  return clip_ref.get_object_as<dsp::FileAudioSource> ();
}

void
RecordingMaterializer::finalize_recording_macro ()
{
  if (!recording_macro_active_)
    return;

  for (auto &[track_id, state] : track_states_)
    {
      force_complete_pending_notes (state);
    }

  if (!undo_stack_.isNull ())
    {
      undo_stack_->endMacro ();
    }
  recording_macro_active_ = false;
  track_states_.clear ();
}

void
RecordingMaterializer::ensure_recording_macro ()
{
  if (!recording_macro_active_)
    {
      assert (!undo_stack_.isNull ());
      undo_stack_->beginMacro (QObject::tr ("Record"));
      recording_macro_active_ = true;
    }
}

// ============================================================================

void
RecordingMaterializer::handle_discontinuity (TrackRecordingState &state)
{
  force_complete_pending_notes (state);
  state.unended_notes.clear ();

  if (!state.current_clip.has_value ())
    return;

  const auto mode = recording_mode_provider_ ();
  switch (mode)
    {
    case RecordingMode::TakesMuted:
      {
        auto * prev_obj = state.current_clip->get ();
        if (prev_obj != nullptr)
          {
            prev_obj->mute ()->setMuted (true);
          }
      }
      [[fallthrough]];
    case RecordingMode::Takes:
      state.current_lane_index++;
      break;
    case RecordingMode::OverwriteEvents:
    case RecordingMode::MergeEvents:
      z_warning (
        "Recording mode {} is not yet implemented, falling back to Takes behavior",
        mode);
      state.current_lane_index++;
      break;
    }
  state.current_clip.reset ();
}

bool
RecordingMaterializer::ensure_midi_clip (
  TrackRecordingState         &state,
  structure::tracks::TrackUuid track_id,
  units::sample_t              start_position)
{
  if (state.current_clip.has_value ())
    return true;

  ensure_recording_macro ();

  auto result =
    creators_.midi_clip (track_id, start_position, state.current_lane_index);
  if (!result.has_value ())
    return false;
  state.current_lane_index = result->actual_lane_index;
  state.current_clip = std::move (result->clip);
  return true;
}

void
RecordingMaterializer::force_complete_pending_notes (TrackRecordingState &state)
{
  if (
    state.unended_notes.empty () || !state.current_clip.has_value ()
    || !state.last_end_position.has_value ())
    return;

  auto * clip =
    state.current_clip->get_object_as<structure::arrangement::MidiClip> ();
  if (clip == nullptr)
    return;

  const auto clip_start = clip->get_tempo_map ().tick_to_samples_rounded (
    clip->position ()->asTick ());

  for (const auto &[key, pending_deque] : state.unended_notes)
    {
      const auto pitch = static_cast<uint8_t> (key & 0xFF);
      for (const auto &pending : pending_deque)
        {
          creators_.midi_note (
            *clip, pending.start_position - clip_start,
            *state.last_end_position - clip_start, pitch, pending.velocity,
            pending.channel);
        }
    }
}

void
RecordingMaterializer::on_midi_data_ready (
  structure::tracks::TrackUuid            track_id,
  const std::vector<RecordingMidiPacket> &packets)
{
  for (const auto &packet : packets)
    {
      if (!packet.transport_recording)
        continue;

      auto &state = track_states_[track_id];

      if (
        !state.last_end_position.has_value ()
        || packet.timeline_position != *state.last_end_position)
        {
          handle_discontinuity (state);
        }

      ensure_midi_clip (state, track_id, packet.timeline_position);

      const auto packet_end = packet.timeline_position + packet.nframes;

      const auto get_midi_clip_and_offset = [&] (units::sample_t pos)
        -> std::optional<
          std::pair<structure::arrangement::MidiClip *, units::sample_t>> {
        if (!ensure_midi_clip (state, track_id, pos))
          return std::nullopt;
        auto * clip =
          state.current_clip->get_object_as<structure::arrangement::MidiClip> ();
        if (clip == nullptr)
          return std::nullopt;
        const auto clip_start = clip->get_tempo_map ().tick_to_samples_rounded (
          clip->position ()->asTick ());
        return std::pair{ clip, clip_start };
      };

      for (const auto &ev : packet.midi_events)
        {
          const auto d = ev.data ();
          if (d.empty ())
            continue;
          const auto status = d[0];
          const auto type =
            static_cast<uint8_t> (status & utils::midi::MIDI_STATUS_MASK);
          const auto channel =
            static_cast<uint8_t> (status & utils::midi::MIDI_CHANNEL_MASK);
          const auto d1 =
            d.size () > 1
              ? static_cast<uint8_t> (d[1] & utils::midi::MIDI_DATA_MASK)
              : uint8_t{ 0 };
          const auto d2 =
            d.size () > 2
              ? static_cast<uint8_t> (d[2] & utils::midi::MIDI_DATA_MASK)
              : uint8_t{ 0 };

          const auto event_sample_pos =
            packet.timeline_position
            + units::samples (
              static_cast<int64_t> (ev.time ().in (units::samples)));

          if (type == utils::midi::MIDI_CH1_NOTE_ON && d2 > 0)
            {
              if (!ensure_midi_clip (state, track_id, event_sample_pos))
                {
                  z_warning (
                    "Failed to create MIDI clip for note-on (pitch={}, "
                    "ch={}) at position {} — dropping note",
                    d1, channel, event_sample_pos);
                  continue;
                }
              const auto key = static_cast<uint16_t> (
                (static_cast<uint16_t> (channel) << 8) | d1);
              state.unended_notes[key].push_back (
                { event_sample_pos, d2, channel });
            }
          else if (
            type == utils::midi::MIDI_CH1_NOTE_OFF
            || (type == utils::midi::MIDI_CH1_NOTE_ON && d2 == 0))
            {
              const auto key = static_cast<uint16_t> (
                (static_cast<uint16_t> (channel) << 8) | d1);
              auto note_it = state.unended_notes.find (key);
              if (
                note_it != state.unended_notes.end ()
                && !note_it->second.empty ())
                {
                  if (!state.current_clip.has_value ())
                    {
                      z_warning (
                        "Note-off (pitch={}, ch={}) at position {} has no "
                        "active clip — stale note dropped",
                        d1, channel, event_sample_pos);
                      note_it->second.pop_front ();
                      if (note_it->second.empty ())
                        state.unended_notes.erase (note_it);
                      continue;
                    }

                  auto * clip = state.current_clip->get_object_as<
                    structure::arrangement::MidiClip> ();
                  if (clip != nullptr)
                    {
                      const auto &pending = note_it->second.front ();
                      const auto  clip_start =
                        clip->get_tempo_map ().tick_to_samples_rounded (
                          clip->position ()->asTick ());
                      creators_.midi_note (
                        *clip, pending.start_position - clip_start,
                        event_sample_pos - clip_start, d1, pending.velocity,
                        pending.channel);
                    }

                  note_it->second.pop_front ();
                  if (note_it->second.empty ())
                    state.unended_notes.erase (note_it);
                }
            }
          else if (type == utils::midi::MIDI_CH1_CTRL_CHANGE)
            {
              auto r = get_midi_clip_and_offset (event_sample_pos);
              if (r.has_value ())
                {
                  auto [clip, clip_start] = *r;
                  creators_.midi_control_event (
                    *clip, event_sample_pos - clip_start,
                    structure::arrangement::MidiControlEvent::EventType::
                      ControlChange,
                    channel, d1, d2);
                }
            }
          else if (type == utils::midi::MIDI_CH1_PITCH_WHEEL_RANGE)
            {
              auto r = get_midi_clip_and_offset (event_sample_pos);
              if (r.has_value ())
                {
                  auto [clip, clip_start] = *r;
                  const auto pb_value = (d2 << 7) | d1;
                  creators_.midi_control_event (
                    *clip, event_sample_pos - clip_start,
                    structure::arrangement::MidiControlEvent::EventType::PitchBend,
                    channel, 0, pb_value);
                }
            }
          else if (type == utils::midi::MIDI_CH1_CHAN_AFTERTOUCH)
            {
              auto r = get_midi_clip_and_offset (event_sample_pos);
              if (r.has_value ())
                {
                  auto [clip, clip_start] = *r;
                  creators_.midi_control_event (
                    *clip, event_sample_pos - clip_start,
                    structure::arrangement::MidiControlEvent::EventType::
                      ChannelPressure,
                    channel, 0, d1);
                }
            }
          else if (type == utils::midi::MIDI_CH1_POLY_AFTERTOUCH)
            {
              auto r = get_midi_clip_and_offset (event_sample_pos);
              if (r.has_value ())
                {
                  auto [clip, clip_start] = *r;
                  creators_.midi_control_event (
                    *clip, event_sample_pos - clip_start,
                    structure::arrangement::MidiControlEvent::EventType::
                      PolyKeyPressure,
                    channel, d1, d2);
                }
            }
          else if (type == utils::midi::MIDI_CH1_PROG_CHANGE)
            {
              auto r = get_midi_clip_and_offset (event_sample_pos);
              if (r.has_value ())
                {
                  auto [clip, clip_start] = *r;
                  creators_.midi_control_event (
                    *clip, event_sample_pos - clip_start,
                    structure::arrangement::MidiControlEvent::EventType::
                      ProgramChange,
                    channel, 0, d1);
                }
            }
        }

      if (state.current_clip.has_value ())
        {
          auto * clip = state.current_clip->get_object_as<
            structure::arrangement::MidiClip> ();
          if (clip != nullptr)
            {
              const auto clip_start =
                clip->get_tempo_map ().tick_to_samples_rounded (
                  clip->position ()->asTick ());
              if (packet_end < clip_start)
                {
                  throw std::runtime_error (
                    fmt::format (
                      "Computed negative MIDI clip length: {}",
                      packet_end - clip_start));
                }
              // Length is the tick span of [clip_start, packet_end] at the
              // recording tempo
              const auto length_tick =
                clip->get_tempo_map ().samples_to_tick (packet_end)
                - clip->position ()->asTick ();
              clip->length ()->setTicks (length_tick.asDouble ());
            }
        }

      state.last_end_position = packet.timeline_position + packet.nframes;
    }
}
}
