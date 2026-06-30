// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>

#include "dsp/midi_event.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/tracks/track_processor.h"
#include "utils/midi.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "../../dsp/graph_helpers.h"
#include <boost/container/static_vector.hpp>
#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{
static constexpr auto AUDIO_BUFFER_DIFF_TOLERANCE = 1e-5f;

class TrackProcessorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);
    transport_ = std::make_unique<dsp::graph_test::MockTransport> ();
    registry_ = std::make_unique<utils::ObjectRegistry> ();
  }

  template <typename ClipT>
  auto create_region (int64_t timeline_pos_samples, int64_t length)
  {
    auto clip_ref = utils::create_object<ClipT> (
      *registry_, *tempo_map_wrapper_, *registry_, nullptr);
    auto * clip = clip_ref.template get_object_as<ClipT> ();
    clip->position ()->setTicks (
      tempo_map_->samples_to_tick (units::samples (timeline_pos_samples))
        .asDouble ());
    clip->length ()->setTicks (
      tempo_map_->samples_to_tick (units::samples (length)).asDouble ());

    if constexpr (std::is_same_v<ClipT, arrangement::AudioClip>)
      {
        // Create a mock audio source
        auto sample_buffer =
          std::make_unique<utils::audio::AudioBuffer> (2, length);

        // Fill with test data: left channel = 0.5, right channel = -0.5
        for (int i = 0; i < sample_buffer->getNumSamples (); i++)
          {
            sample_buffer->setSample (0, i, 0.5f);
            sample_buffer->setSample (1, i, -0.5f);
          }

        auto source_ref = utils::create_object<dsp::FileAudioSource> (
          *registry_, *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32,
          units::sample_rate (44100), units::bpm (120.0), u8"DummySource");
        auto audio_source_object_ref =
          utils::create_object<arrangement::AudioSourceObject> (
            *registry_, *tempo_map_wrapper_, *registry_, source_ref);
        clip->set_source (audio_source_object_ref);

        clip->prepare_to_play (block_length_, sample_rate_);
      }
    return clip_ref;
  }

  void add_midi_note (
    arrangement::MidiClip * clip,
    int                     note,
    int                     velocity,
    int64_t                 start_frames,
    int64_t                 end_frames)
  {
    auto note_ref = utils::create_object<arrangement::MidiNote> (
      *registry_, *tempo_map_wrapper_);
    note_ref.get_object_as<arrangement::MidiNote> ()->setPitch (note);
    note_ref.get_object_as<arrangement::MidiNote> ()->setVelocity (velocity);
    note_ref.get_object_as<arrangement::MidiNote> ()->position ()->setTicks (
      tempo_map_->samples_to_tick (units::samples (start_frames)).asDouble ());
    note_ref.get_object_as<arrangement::MidiNote> ()->length ()->setTicks (
      tempo_map_->samples_to_tick (units::samples (end_frames - start_frames))
        .asDouble ());
    clip->ArrangerObjectOwner<structure::arrangement::MidiNote>::add_object (
      note_ref);
  }

  // Helper function to create a MIDI event provider from a MIDI sequence
  TrackProcessor::MidiEventProviderProcessFunc
  create_midi_event_provider_from_sequence (const juce::MidiMessageSequence &seq)
  {
    return
      [seq] (
        const dsp::graph::ProcessBlockInfo &time_nfo,
        dsp::MidiEventBuffer               &output_buffer) {
        // Convert juce MIDI sequence to output buffer
        for (int i = 0; i < seq.getNumEvents (); ++i)
          {
            const auto * event = seq.getEventPointer (i);
            if (event == nullptr)
              continue;

            // Convert juce MIDI message to our format
            const auto &msg = event->message;
            if (
              static_cast<size_t> (msg.getRawDataSize ())
              <= dsp::RealtimeMidiEvent::inline_capacity)
              {
                auto new_event = dsp::midi_event::make_raw_rt (
                  std::span<const midi_byte_t>{
                    reinterpret_cast<const midi_byte_t *> (msg.getRawData ()),
                    static_cast<size_t> (msg.getRawDataSize ()) },
                  units::samples (static_cast<uint32_t> (msg.getTimeStamp ())));

                // Only add events that are within the time range
                if (new_event.time_ < time_nfo.nframes_)
                  {
                    output_buffer.push_back (new_event.time_, new_event.data ());
                  }
              }
          }
      };
  }

  std::unique_ptr<dsp::TempoMap>                  tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>           tempo_map_wrapper_;
  std::unique_ptr<dsp::graph_test::MockTransport> transport_;
  std::unique_ptr<utils::ObjectRegistry>          registry_;
  units::sample_rate_t sample_rate_{ units::sample_rate (44100) };
  units::sample_u32_t  block_length_{ units::samples (256u) };
};

TEST_F (TrackProcessorTest, AudioTrackInitialState)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Test Audio Track"; },
    [] { return true; }, TrackProcessor::Capabilities::AudioTrack, *registry_);

  EXPECT_TRUE (processor.is_audio ());
  EXPECT_FALSE (processor.is_midi ());
  EXPECT_EQ (processor.get_node_name (), u8"Test Audio Track Processor");

  // Check audio ports
  EXPECT_EQ (processor.get_input_ports ().size (), 1);
  EXPECT_EQ (processor.get_output_ports ().size (), 1);

  // Check audio-specific parameters
  EXPECT_EQ (processor.get_parameters ().size (), 4);
}

TEST_F (TrackProcessorTest, MidiTrackInitialState)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"Test MIDI Track"; },
    [] { return true; }, TrackProcessor::Capabilities::PianoRoll, *registry_);

  EXPECT_FALSE (processor.is_audio ());
  EXPECT_TRUE (processor.is_midi ());
  EXPECT_EQ (processor.get_node_name (), u8"Test MIDI Track Processor");

  // Check MIDI ports
  EXPECT_EQ (processor.get_input_ports ().size (), 3); // MIDI in + Piano roll +
                                                       // HW MIDI in
  EXPECT_EQ (processor.get_output_ports ().size (), 1); // MIDI out

  // Check no audio-specific parameters
  EXPECT_EQ (processor.get_parameters ().size (), 0);
}

TEST_F (TrackProcessorTest, DisabledTrackProcessing)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Disabled Track"; },
    [] { return false; }, TrackProcessor::Capabilities::AudioTrack, *registry_);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  // Fill inputs with test data
  auto &stereo_in = processor.get_stereo_in_port ();
  for (int i = 0; i < stereo_in.buffers ()->getNumSamples (); ++i)
    {
      stereo_in.buffers ()->setSample (0, i, 0.5f);
      stereo_in.buffers ()->setSample (1, i, 0.5f);
    }

  // Should not process when disabled
  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Verify output buffers are cleared (no processing occurred)
  auto &stereo_out = processor.get_stereo_out_port ();
  for (
    const auto i : std::views::iota (0u, time_nfo.nframes_.in (units::samples)))
    {
      EXPECT_EQ (stereo_out.buffers ()->getSample (0, i), 0.0f);
      EXPECT_EQ (stereo_out.buffers ()->getSample (1, i), 0.0f);
    }
}

TEST_F (TrackProcessorTest, AudioTrackProcessingWithMonitoringOff)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Audio Track"; },
    [] { return true; }, TrackProcessor::Capabilities::AudioTrack, *registry_);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  // Disable monitoring so that input ports are ignored
  processor.get_monitor_audio_param ().setBaseValue (
    processor.get_monitor_audio_param ().range ().normalized_from_enum (
      TrackProcessor::MonitorMode::Off));

  // Fill input buffers with test data
  auto &stereo_in = processor.get_stereo_in_port ();
  for (
    const auto i : std::views::iota (0u, time_nfo.nframes_.in (units::samples)))
    {
      stereo_in.buffers ()->setSample (0, i, 0.8f);
      stereo_in.buffers ()->setSample (1, i, 0.5f);
    }

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Verify output is empty
  auto &stereo_out = processor.get_stereo_out_port ();
  for (
    const auto i : std::views::iota (0u, time_nfo.nframes_.in (units::samples)))
    {
      EXPECT_NEAR (
        stereo_out.buffers ()->getSample (0, i), 0.f,
        AUDIO_BUFFER_DIFF_TOLERANCE);
      EXPECT_NEAR (
        stereo_out.buffers ()->getSample (1, i), 0.f,
        AUDIO_BUFFER_DIFF_TOLERANCE);
    }
}

TEST_F (TrackProcessorTest, AudioTrackProcessingWithInputGain)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Audio Track"; },
    [] { return true; }, TrackProcessor::Capabilities::AudioTrack, *registry_);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  // Set input gain to 2.0
  auto &input_gain_param = processor.get_input_gain_param ();
  input_gain_param.setBaseValue (input_gain_param.range ().convertTo0To1 (2.0f));

  // Enable monitoring so that input ports are used on this audio track
  processor.get_monitor_audio_param ().setBaseValue (
    processor.get_monitor_audio_param ().range ().normalized_from_enum (
      TrackProcessor::MonitorMode::On));

  // Enable stereo mode
  processor.get_mono_param ().setBaseValue (0.f);

  // Fill input buffers with test data
  auto &stereo_in = processor.get_stereo_in_port ();
  for (
    const auto i : std::views::iota (0u, time_nfo.nframes_.in (units::samples)))
    {
      stereo_in.buffers ()->setSample (0, i, 1.0f);
      stereo_in.buffers ()->setSample (1, i, 0.5f);
    }

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Verify output is scaled by input gain
  auto &stereo_out = processor.get_stereo_out_port ();
  for (
    const auto i : std::views::iota (0u, time_nfo.nframes_.in (units::samples)))
    {
      EXPECT_NEAR (
        stereo_out.buffers ()->getSample (0, i), 2.0f,
        AUDIO_BUFFER_DIFF_TOLERANCE);
      EXPECT_NEAR (
        stereo_out.buffers ()->getSample (1, i), 1.0f,
        AUDIO_BUFFER_DIFF_TOLERANCE);
    }
}

TEST_F (TrackProcessorTest, AudioTrackProcessingWithMono)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Audio Track Mono"; },
    [] { return true; }, TrackProcessor::Capabilities::AudioTrack, *registry_);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  // Enable monitoring so that input ports are used on this audio track
  processor.get_monitor_audio_param ().setBaseValue (
    processor.get_monitor_audio_param ().range ().normalized_from_enum (
      TrackProcessor::MonitorMode::On));

  // Enable mono mode
  auto &mono_param = processor.get_mono_param ();
  mono_param.setBaseValue (1.f);

  // Fill input buffers with different data
  auto &stereo_in = processor.get_stereo_in_port ();
  for (
    const auto i : std::views::iota (0u, time_nfo.nframes_.in (units::samples)))
    {
      stereo_in.buffers ()->setSample (0, i, 0.8f);
      stereo_in.buffers ()->setSample (1, i, 0.5f);
    }

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Verify both outputs use left channel data
  auto &stereo_out = processor.get_stereo_out_port ();
  for (
    const auto i : std::views::iota (0u, time_nfo.nframes_.in (units::samples)))
    {
      EXPECT_NEAR (
        stereo_out.buffers ()->getSample (0, i), 0.8f,
        AUDIO_BUFFER_DIFF_TOLERANCE);
      EXPECT_NEAR (
        stereo_out.buffers ()->getSample (1, i), 0.8f,
        AUDIO_BUFFER_DIFF_TOLERANCE);
    }
}

TEST_F (TrackProcessorTest, AudioTrackProcessingWithOutputGain)
{
  TrackProcessor::FillEventsCallback fill_events_cb =
    [&] (
      const dsp::ITransport                        &itransport,
      const dsp::graph::ProcessBlockInfo           &time_nfo,
      dsp::MidiEventBuffer *                        midi_events,
      std::optional<TrackProcessor::StereoPortPair> stereo_ports) {
      for (size_t i = 0; i < 256; ++i)
        {
          stereo_ports->first[i] = 0.8f;
          stereo_ports->second[i] = 0.8f;
        }
    };
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Audio Track Output"; },
    [] { return true; }, TrackProcessor::Capabilities::AudioTrack, *registry_,
    fill_events_cb);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  // Set output gain to 0.5
  auto &output_gain_param = processor.get_output_gain_param ();
  output_gain_param.setBaseValue (
    output_gain_param.range ().convertTo0To1 (0.5f));

  // Enable rolling so that events are taken into account
  ON_CALL (*transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Rolling));

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Verify output is scaled by output gain
  auto &stereo_out = processor.get_stereo_out_port ();
  for (
    const auto i : std::views::iota (0u, time_nfo.nframes_.in (units::samples)))
    {
      EXPECT_NEAR (
        stereo_out.buffers ()->getSample (0, i), 0.4f,
        AUDIO_BUFFER_DIFF_TOLERANCE);
      EXPECT_NEAR (
        stereo_out.buffers ()->getSample (1, i), 0.4f,
        AUDIO_BUFFER_DIFF_TOLERANCE);
    }
}

class MonitorModeAutoTest
    : public TrackProcessorTest,
      public ::testing::WithParamInterface<bool>
{
};

TEST_P (MonitorModeAutoTest, DependsOnRecordingArmed)
{
  const bool recording_armed = GetParam ();

  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Auto Monitor Test"; },
    [] { return true; },
    TrackProcessor::Capabilities::AudioTrack
      | TrackProcessor::Capabilities::Recording,
    *registry_);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  processor.set_recording_armed (recording_armed);

  processor.get_monitor_audio_param ().setBaseValue (
    processor.get_monitor_audio_param ().range ().normalized_from_enum (
      TrackProcessor::MonitorMode::Auto));

  auto &stereo_in = processor.get_stereo_in_port ();
  for (
    const auto i : std::views::iota (0u, time_nfo.nframes_.in (units::samples)))
    {
      stereo_in.buffers ()->setSample (0, i, 0.9f);
      stereo_in.buffers ()->setSample (1, i, 0.6f);
    }

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  const float expected_left = recording_armed ? 0.9f : 0.f;
  const float expected_right = recording_armed ? 0.6f : 0.f;
  auto       &stereo_out = processor.get_stereo_out_port ();
  EXPECT_NEAR (
    stereo_out.buffers ()->getSample (0, 0), expected_left,
    AUDIO_BUFFER_DIFF_TOLERANCE);
  EXPECT_NEAR (
    stereo_out.buffers ()->getSample (1, 0), expected_right,
    AUDIO_BUFFER_DIFF_TOLERANCE);
}

INSTANTIATE_TEST_SUITE_P (
  RecordingArmed,
  MonitorModeAutoTest,
  ::testing::Values (true, false));

TEST_F (TrackProcessorTest, MidiTrackProcessingWithTransform)
{
  auto transform_temp = dsp::MidiEventBuffer::make_reserved ();
  TrackProcessor::TransformMidiInputsFunc transform_func =
    [&transform_temp] (dsp::MidiEventBuffer &events) {
      // Transform all note events to channel 2 and transpose up by 2 semitones
      transform_temp.clear ();
      for (const auto &event : events)
        {
          auto d = event.data ();
          if (
            utils::midi::midi_is_note_on (d)
            || utils::midi::midi_is_note_off (d))
            {
              // Change channel to 2 (0x91 for note on, 0x81 for note off)
              // Transpose note up by 2 semitones
              std::array<midi_byte_t, 3> arr{};
              std::ranges::copy (d, arr.begin ());
              arr[0] = (arr[0] & 0xF0) | 0x01;
              arr[1] = std::min (127, arr[1] + 2);
              transform_temp.push_back (
                event.time (),
                std::span<const midi_byte_t>{ arr.data (), d.size () });
            }
          else
            {
              transform_temp.push_back (event.time (), d);
            }
        }
      events.swap (transform_temp);
    };

  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"MIDI Transform"; },
    [] { return true; }, TrackProcessor::Capabilities::PianoRoll, *registry_,
    std::nullopt, transform_func);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  // Add some MIDI events to input
  auto      &midi_in = processor.get_midi_in_port ();
  const auto _e1 =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
  midi_in.buffer_.push_back (_e1.time_, _e1.data ());
  const auto _e2 =
    dsp::midi_event::make_note_on (0, 64, 90, units::samples (10u));
  midi_in.buffer_.push_back (_e2.time_, _e2.data ());
  const auto _e3 = dsp::midi_event::make_note_off (0, 60, units::samples (20u));
  midi_in.buffer_.push_back (_e3.time_, _e3.data ());

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Verify events were transformed
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_EQ (midi_out.buffer_.size (), 3);

  // Check that events were properly transformed
  const auto &events = midi_out.buffer_;
  bool        found_transformed_note_on_1 = false;
  bool        found_transformed_note_on_2 = false;
  bool        found_transformed_note_off = false;

  for (const auto &event : events)
    {
      auto ed = event.data ();
      if (
        utils::midi::midi_is_note_on (ed)
        && utils::midi::midi_get_channel_0_to_15 (ed) == 0x01)
        {
          // Note on, channel 2
          if (
            utils::midi::midi_get_note_number (ed) == 62
            && utils::midi::midi_get_velocity (ed) == 100)
            {
              found_transformed_note_on_1 = true;
            }
          else if (
            utils::midi::midi_get_note_number (ed) == 66
            && utils::midi::midi_get_velocity (ed) == 90)
            {
              found_transformed_note_on_2 = true;
            }
        }
      else if (
        utils::midi::midi_is_note_off (ed)
        && utils::midi::midi_get_channel_0_to_15 (ed) == 0x01)
        {
          // Note off, channel 2
          if (utils::midi::midi_get_note_number (ed) == 62)
            {
              found_transformed_note_off = true;
            }
        }
    }

  EXPECT_TRUE (found_transformed_note_on_1);
  EXPECT_TRUE (found_transformed_note_on_2);
  EXPECT_TRUE (found_transformed_note_off);
}

TEST_F (TrackProcessorTest, JsonSerialization)
{
  // Create an audio track processor
  TrackProcessor audio_processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Test Audio Track"; },
    [] { return true; }, TrackProcessor::Capabilities::AudioTrack, *registry_);

  // Set some parameter values
  audio_processor.get_input_gain_param ().setBaseValue (
    audio_processor.get_input_gain_param ().range ().convertTo0To1 (1.5f));
  audio_processor.get_output_gain_param ().setBaseValue (
    audio_processor.get_output_gain_param ().range ().convertTo0To1 (0.8f));
  audio_processor.get_mono_param ().setBaseValue (1.0f); // Enable mono
  audio_processor.get_monitor_audio_param ().setBaseValue (
    audio_processor.get_monitor_audio_param ().range ().normalized_from_enum (
      TrackProcessor::MonitorMode::Off));

  // Serialize to JSON
  nlohmann::json j;
  to_json (j, audio_processor);

  // Deserialize back to a new processor
  TrackProcessor deserialized_processor (
    *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Deserialized Audio Track"; }, [] { return true; },
    TrackProcessor::Capabilities::AudioTrack, *registry_);
  from_json (j, deserialized_processor);

  // Verify deserialized processor has same state
  EXPECT_EQ (deserialized_processor.is_audio (), audio_processor.is_audio ());
  EXPECT_EQ (deserialized_processor.is_midi (), audio_processor.is_midi ());

  // Verify parameter values are preserved
  EXPECT_NEAR (
    deserialized_processor.get_input_gain_param ().range ().convertFrom0To1 (
      deserialized_processor.get_input_gain_param ().baseValue ()),
    1.5f, AUDIO_BUFFER_DIFF_TOLERANCE);
  EXPECT_NEAR (
    deserialized_processor.get_output_gain_param ().range ().convertFrom0To1 (
      deserialized_processor.get_output_gain_param ().baseValue ()),
    0.8f, AUDIO_BUFFER_DIFF_TOLERANCE);
  EXPECT_TRUE (deserialized_processor.get_mono_param ().range ().isToggled (
    deserialized_processor.get_mono_param ().baseValue ()));
  EXPECT_EQ (
    deserialized_processor.get_monitor_audio_param ()
      .range ()
      .enum_value<TrackProcessor::MonitorMode> (
        deserialized_processor.get_monitor_audio_param ().baseValue ()),
    TrackProcessor::MonitorMode::Off);
}

TEST_F (TrackProcessorTest, JsonSerializationMidiTrack)
{
  TrackProcessor midi_processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"Test MIDI Track"; },
    [] { return true; }, TrackProcessor::Capabilities::PianoRoll, *registry_);

  nlohmann::json j;
  to_json (j, midi_processor);

  TrackProcessor deserialized_processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"Deserialized MIDI Track"; },
    [] { return true; }, TrackProcessor::Capabilities::PianoRoll, *registry_);
  from_json (j, deserialized_processor);

  EXPECT_EQ (deserialized_processor.is_audio (), midi_processor.is_audio ());
  EXPECT_EQ (deserialized_processor.is_midi (), midi_processor.is_midi ());
}

TEST_F (TrackProcessorTest, MidiTrackProcessingWithAppendFunc)
{
  TrackProcessor::AppendMidiInputsToOutputsFunc append_func =
    [] (
      dsp::MidiEventBuffer &out_events, const dsp::MidiEventBuffer &in_events,
      const dsp::graph::ProcessBlockInfo &time_nfo) {
      // Append all input events to output, but transpose them up by 5 semitones
      for (const auto &event : in_events)
        {
          auto d = event.data ();
          if (
            utils::midi::midi_is_note_on (d)
            || utils::midi::midi_is_note_off (d))
            {
              // Transpose note up by 5 semitones
              std::array<midi_byte_t, 3> arr{};
              std::ranges::copy (d, arr.begin ());
              arr[1] = std::min (127, arr[1] + 5);
              out_events.push_back (
                event.time (),
                std::span<const midi_byte_t>{ arr.data (), d.size () });
            }
          else
            {
              out_events.push_back (event.time (), d);
            }
        }
    };

  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"MIDI Append"; },
    [] { return true; }, TrackProcessor::Capabilities::PianoRoll, *registry_,
    std::nullopt, std::nullopt, append_func);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  // Add some MIDI events to input
  auto      &midi_in = processor.get_midi_in_port ();
  const auto _e1 =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
  midi_in.buffer_.push_back (_e1.time_, _e1.data ());
  const auto _e2 =
    dsp::midi_event::make_note_on (0, 64, 90, units::samples (10u));
  midi_in.buffer_.push_back (_e2.time_, _e2.data ());
  const auto _e3 = dsp::midi_event::make_note_off (0, 60, units::samples (20u));
  midi_in.buffer_.push_back (_e3.time_, _e3.data ());

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Verify events were appended and transformed
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_EQ (midi_out.buffer_.size (), 3);

  // Check that events were properly appended and transposed
  const auto &events = midi_out.buffer_;
  bool        found_transposed_note_on_1 = false;
  bool        found_transposed_note_on_2 = false;
  bool        found_transposed_note_off = false;

  for (const auto &event : events)
    {
      auto ed = event.data ();
      if (
        utils::midi::midi_is_note_on (ed)
        && utils::midi::midi_get_channel_0_to_15 (ed) == 0x00)
        {
          // Note on, channel 1
          if (
            utils::midi::midi_get_note_number (ed) == 65
            && utils::midi::midi_get_velocity (ed) == 100)
            {
              found_transposed_note_on_1 = true;
            }
          else if (
            utils::midi::midi_get_note_number (ed) == 69
            && utils::midi::midi_get_velocity (ed) == 90)
            {
              found_transposed_note_on_2 = true;
            }
        }
      else if (
        utils::midi::midi_is_note_off (ed)
        && utils::midi::midi_get_channel_0_to_15 (ed) == 0x00)
        {
          // Note off, channel 1
          if (utils::midi::midi_get_note_number (ed) == 65)
            {
              found_transposed_note_off = true;
            }
        }
    }

  EXPECT_TRUE (found_transposed_note_on_1);
  EXPECT_TRUE (found_transposed_note_on_2);
  EXPECT_TRUE (found_transposed_note_off);
}

TEST_F (TrackProcessorTest, RecordingCallback)
{
  bool                                               recording_called = false;
  units::sample_t                                    recorded_timeline_position;
  int64_t                                            recorded_nframes = 0;
  const dsp::MidiEventBuffer *                       recorded_midi_events;
  std::optional<TrackProcessor::ConstStereoPortPair> recorded_stereo_ports;

  TrackProcessor::RecordingCallbackRT recording_cb =
    [&recording_called, &recorded_timeline_position, &recorded_nframes,
     &recorded_midi_events, &recorded_stereo_ports] (
      units::sample_t              timeline_position, const dsp::ITransport &,
      const dsp::MidiEventBuffer * midi_events,
      std::optional<TrackProcessor::ConstStereoPortPair> stereo_ports,
      units::sample_u32_t                                nframes) {
      recording_called = true;
      recorded_timeline_position = timeline_position;
      if (stereo_ports.has_value ())
        recorded_nframes = stereo_ports->first.size ();
      recorded_midi_events = midi_events;
      recorded_stereo_ports = stereo_ports;
    };

  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Recording Track"; },
    [] { return true; },
    TrackProcessor::Capabilities::AudioTrack
      | TrackProcessor::Capabilities::Recording,
    *registry_, std::nullopt, std::nullopt, std::nullopt, recording_cb);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (44100), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);
  processor.set_recording_armed (true);

  ON_CALL (*transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Rolling));

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  EXPECT_TRUE (recording_called);
  EXPECT_EQ (recorded_timeline_position, units::samples (44100));
  EXPECT_EQ (recorded_nframes, 256);
}

TEST_F (TrackProcessorTest, GetFullDesignationForPort)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Test Track"; },
    [] { return true; }, TrackProcessor::Capabilities::AudioTrack, *registry_);

  const auto &ports = processor.get_input_ports ();
  ASSERT_FALSE (ports.empty ());
  auto designation = processor.get_full_designation_for_port (
    *ports[0].get_object_as<dsp::AudioPort> ());
  EXPECT_FALSE (designation.empty ());
}

TEST_F (TrackProcessorTest, MidiTrackProcessingWithPianoRoll)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"MIDI Piano Roll"; },
    [] { return true; }, TrackProcessor::Capabilities::PianoRoll, *registry_);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  // Add events to piano roll port
  auto      &piano_roll = processor.get_piano_roll_port ();
  const auto _ev =
    dsp::midi_event::make_note_on (0, 72, 100, units::samples (0u));
  piano_roll.buffer_.push_back (_ev.time_, _ev.data ());

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Verify events were processed
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_GT (midi_out.buffer_.size (), 0);
}

TEST_F (TrackProcessorTest, AudioTrackProcessingWithRecording)
{
  bool                                               recording_called = false;
  units::sample_t                                    recorded_timeline_position;
  uint64_t                                           recorded_nframes = 0;
  const dsp::MidiEventBuffer *                       recorded_midi_events;
  std::optional<TrackProcessor::ConstStereoPortPair> recorded_stereo_ports;

  TrackProcessor::RecordingCallbackRT recording_cb =
    [&recording_called, &recorded_timeline_position, &recorded_nframes,
     &recorded_midi_events, &recorded_stereo_ports] (
      units::sample_t              timeline_position, const dsp::ITransport &,
      const dsp::MidiEventBuffer * midi_events,
      std::optional<TrackProcessor::ConstStereoPortPair> stereo_ports,
      units::sample_u32_t                                nframes) {
      recording_called = true;
      recorded_timeline_position = timeline_position;
      if (stereo_ports.has_value ())
        recorded_nframes = stereo_ports->first.size ();
      recorded_midi_events = midi_events;
      recorded_stereo_ports = stereo_ports;
    };

  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Audio Recording"; },
    [] { return true; },
    TrackProcessor::Capabilities::AudioTrack
      | TrackProcessor::Capabilities::Recording,
    *registry_, std::nullopt, std::nullopt, std::nullopt, recording_cb);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (88200), units::samples (512));

  processor.prepare_for_processing (nullptr, sample_rate_, units::samples (512));
  processor.set_recording_armed (true);

  // Fill input buffers with test pattern
  auto &stereo_in = processor.get_stereo_in_port ();
  for (
    const auto i : std::views::iota (0u, time_nfo.nframes_.in (units::samples)))
    {
      stereo_in.buffers ()->setSample (
        0, i, 0.8f + 0.2f * std::sin (static_cast<float> (i) * 0.1f));
      stereo_in.buffers ()->setSample (
        1, i, 0.6f + 0.3f * std::cos (static_cast<float> (i) * 0.1f));
    }

  ON_CALL (*transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Rolling));

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  EXPECT_TRUE (recording_called);
  EXPECT_EQ (recorded_timeline_position, units::samples (88200));
  EXPECT_EQ (recorded_nframes, 512);
  EXPECT_EQ (recorded_midi_events, nullptr); // No MIDI events for audio track
  EXPECT_TRUE (recorded_stereo_ports.has_value ());
  EXPECT_EQ (recorded_stereo_ports->first.size (), 512);
  EXPECT_EQ (recorded_stereo_ports->second.size (), 512);
}

TEST_F (TrackProcessorTest, AudioTrackProcessingHasZeroLatency)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Zero Latency Track"; },
    [] { return true; }, TrackProcessor::Capabilities{}, *registry_);

  EXPECT_EQ (processor.get_single_playback_latency (), units::samples (0));
}

TEST_F (TrackProcessorTest, MidiTrackProcessingWithEmptyInput)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"Empty MIDI Track"; },
    [] { return true; }, TrackProcessor::Capabilities::PianoRoll, *registry_);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  // Process with no input events
  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Should complete without errors
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_EQ (midi_out.buffer_.size (), 0);
}

TEST_F (TrackProcessorTest, AudioBusTrackProcessingWithLargeBuffer)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Large Buffer Track"; },
    [] { return true; }, TrackProcessor::Capabilities{}, *registry_);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (1024));

  processor.prepare_for_processing (
    nullptr, sample_rate_, units::samples (1024));

  auto &stereo_in = processor.get_stereo_in_port ();
  for (
    const auto i : std::views::iota (0u, time_nfo.nframes_.in (units::samples)))
    {
      stereo_in.buffers ()->setSample (0, i, 0.5f);
      stereo_in.buffers ()->setSample (1, i, 0.3f);
    }

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Verify processing completed
  auto &stereo_out = processor.get_stereo_out_port ();
  EXPECT_NEAR (
    stereo_out.buffers ()->getSample (0, 0), 0.5f, AUDIO_BUFFER_DIFF_TOLERANCE);
  EXPECT_NEAR (
    stereo_out.buffers ()->getSample (1, 0), 0.3f, AUDIO_BUFFER_DIFF_TOLERANCE);
}

TEST_F (TrackProcessorTest, MidiTrackProcessingWithMultipleEvents)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"Multi MIDI Track"; },
    [] { return true; }, TrackProcessor::Capabilities::PianoRoll, *registry_);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  // Add multiple MIDI events
  auto      &midi_in = processor.get_midi_in_port ();
  const auto _e1 =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
  midi_in.buffer_.push_back (_e1.time_, _e1.data ());
  const auto _e2 =
    dsp::midi_event::make_note_on (0, 64, 90, units::samples (10u));
  midi_in.buffer_.push_back (_e2.time_, _e2.data ());
  const auto _e3 =
    dsp::midi_event::make_note_on (0, 67, 80, units::samples (20u));
  midi_in.buffer_.push_back (_e3.time_, _e3.data ());

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Verify events were processed
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_GE (midi_out.buffer_.size (), 3);
}

// ============================================================================
// Hardware MIDI input channel filtering
// ============================================================================

class TrackProcessorHwMidiChannelFilterTest
    : public TrackProcessorTest,
      public ::testing::WithParamInterface<int>
{
};

TEST_P (TrackProcessorHwMidiChannelFilterTest, FiltersHwMidiInputOnly)
{
  const int selected_channel = GetParam ();

  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"HW MIDI Filter"; },
    [] { return true; }, TrackProcessor::Capabilities::PianoRoll, *registry_,
    std::nullopt, std::nullopt, std::nullopt,
    [] (
      units::sample_t, const dsp::ITransport &, const dsp::MidiEventBuffer *,
      std::optional<TrackProcessor::ConstStereoPortPair>, units::sample_u32_t) {
    });

  processor.set_hw_midi_channel (selected_channel);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  auto      &midi_in = processor.get_midi_in_port ();
  const auto _e1 =
    dsp::midi_event::make_note_on (0, 40, 50, units::samples (0u));
  midi_in.buffer_.push_back (_e1.time_, _e1.data ());
  const auto _e2 =
    dsp::midi_event::make_note_on (4, 41, 50, units::samples (10u));
  midi_in.buffer_.push_back (_e2.time_, _e2.data ());

  auto      &hw_in = processor.get_hw_midi_in_port ();
  const auto _e3 =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
  hw_in.buffer_.push_back (_e3.time_, _e3.data ());
  const auto _e4 =
    dsp::midi_event::make_note_on (2, 64, 90, units::samples (10u));
  hw_in.buffer_.push_back (_e4.time_, _e4.data ());
  const auto _e5 =
    dsp::midi_event::make_note_on (4, 67, 80, units::samples (20u));
  hw_in.buffer_.push_back (_e5.time_, _e5.data ());

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  auto       &midi_out = processor.get_midi_out_port ();
  const auto &events = midi_out.buffer_;

  if (selected_channel == 0)
    {
      EXPECT_EQ (events.size (), 5)
        << "Omni: all events from both ports should pass";
    }
  else if (
    selected_channel == 1 || selected_channel == 3 || selected_channel == 5)
    {
      EXPECT_EQ (events.size (), 3)
        << "Channel " << selected_channel
        << ": 2 midi_in (unfiltered) + 1 hw_midi_in (channel "
        << selected_channel << " only)";
      const midi_byte_t expected_pitch =
        selected_channel == 1 ? 60 : (selected_channel == 3 ? 64 : 67);
      bool found_hw_filtered = false;
      for (const auto &ev : events)
        {
          if (utils::midi::midi_get_note_number (ev.data ()) == expected_pitch)
            found_hw_filtered = true;
        }
      EXPECT_TRUE (found_hw_filtered)
        << "Channel " << selected_channel
        << " event from hw_midi_in should be present";
    }
  else
    {
      EXPECT_EQ (events.size (), 2)
        << "Channel " << selected_channel
        << ": no hw_midi_in events on this channel, only 2 midi_in";
    }
}

INSTANTIATE_TEST_SUITE_P (
  ChannelFilter,
  TrackProcessorHwMidiChannelFilterTest,
  ::testing::Values (0, 1, 3, 16));

TEST_F (TrackProcessorTest, HwMidiChannelFilterPassesSystemMessages)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"SysMsg Filter"; },
    [] { return true; }, TrackProcessor::Capabilities::PianoRoll, *registry_,
    std::nullopt, std::nullopt, std::nullopt,
    [] (
      units::sample_t, const dsp::ITransport &, const dsp::MidiEventBuffer *,
      std::optional<TrackProcessor::ConstStereoPortPair>, units::sample_u32_t) {
    });

  processor.set_hw_midi_channel (2);
  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  auto      &hw_in = processor.get_hw_midi_in_port ();
  const auto _e1 =
    dsp::midi_event::make_note_on (1, 60, 100, units::samples (0u));
  hw_in.buffer_.push_back (_e1.time_, _e1.data ());
  {
    const std::array<midi_byte_t, 1> clock_bytes = {
      utils::midi::MIDI_CLOCK_BEAT
    };
    const auto _ev =
      dsp::midi_event::make_raw_rt (clock_bytes, units::samples (10u));
    hw_in.buffer_.push_back (_ev.time_, _ev.data ());
  }
  {
    const std::array<midi_byte_t, 1> start_bytes = {
      utils::midi::MIDI_CLOCK_START
    };
    const auto _ev =
      dsp::midi_event::make_raw_rt (start_bytes, units::samples (20u));
    hw_in.buffer_.push_back (_ev.time_, _ev.data ());
  }
  const auto _e2 =
    dsp::midi_event::make_note_on (4, 64, 90, units::samples (30u));
  hw_in.buffer_.push_back (_e2.time_, _e2.data ());

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));
  processor.process_block (time_nfo, *transport_, *tempo_map_);

  auto       &midi_out = processor.get_midi_out_port ();
  const auto &events = midi_out.buffer_;

  EXPECT_EQ (events.size (), 3)
    << "Channel 2 note-on + 2 system messages should pass, channel 5 filtered";
  int sys_msg_count = 0;
  for (const auto &ev : events)
    {
      if (ev.data ()[0] >= utils::midi::MIDI_SYSTEM_MESSAGE)
        ++sys_msg_count;
    }
  EXPECT_EQ (sys_msg_count, 2)
    << "MIDI clock and start should pass through channel filter";
}

TEST_F (TrackProcessorTest, HwMidiInputFilteredEventsReachRecordingCallback)
{
  int  channel = 2;
  auto recorded_events = dsp::MidiEventBuffer::make_reserved ();

  TrackProcessor::RecordingCallbackRT recording_cb =
    [&recorded_events] (
      units::sample_t, const dsp::ITransport &,
      const dsp::MidiEventBuffer * midi_events,
      std::optional<TrackProcessor::ConstStereoPortPair>, units::sample_u32_t) {
      if (midi_events != nullptr)
        {
          for (const auto &ev : *midi_events)
            recorded_events.push_back (ev.time (), ev.data ());
        }
    };

  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"HW MIDI Rec"; },
    [] { return true; },
    TrackProcessor::Capabilities::PianoRoll
      | TrackProcessor::Capabilities::Recording,
    *registry_, std::nullopt, std::nullopt, std::nullopt, recording_cb);

  processor.set_hw_midi_channel (channel);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);
  processor.set_recording_armed (true);

  auto      &hw_in = processor.get_hw_midi_in_port ();
  const auto _e1 =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
  hw_in.buffer_.push_back (_e1.time_, _e1.data ());
  const auto _e2 =
    dsp::midi_event::make_note_on (1, 64, 90, units::samples (10u));
  hw_in.buffer_.push_back (_e2.time_, _e2.data ());
  const auto _e3 =
    dsp::midi_event::make_note_on (4, 67, 80, units::samples (20u));
  hw_in.buffer_.push_back (_e3.time_, _e3.data ());

  ON_CALL (*transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Rolling));
  ON_CALL (*transport_, recording_preroll_frames_remaining ())
    .WillByDefault (::testing::Return (units::samples (0)));
  ON_CALL (*transport_, punch_enabled ())
    .WillByDefault (::testing::Return (false));
  ON_CALL (*transport_, loop_enabled ())
    .WillByDefault (::testing::Return (false));

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  ASSERT_EQ (recorded_events.size (), 1u)
    << "Only channel 2 event should reach recording callback";
  {
    auto it = recorded_events.begin ();
    EXPECT_EQ (utils::midi::midi_get_note_number ((*it).data ()), 64);
  }
}

TEST_F (TrackProcessorTest, AudioTrackParameterRangeValidation)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Param Range Track"; },
    [] { return true; }, TrackProcessor::Capabilities::AudioTrack, *registry_);

  // Test input gain parameter range
  auto &input_gain_param = processor.get_input_gain_param ();
  EXPECT_GE (input_gain_param.range ().minf_, 0.0f);
  EXPECT_LE (input_gain_param.range ().maxf_, 4.0f);

  // Test output gain parameter range
  auto &output_gain_param = processor.get_output_gain_param ();
  EXPECT_GE (output_gain_param.range ().minf_, 0.0f);
  EXPECT_LE (output_gain_param.range ().maxf_, 4.0f);

  // Test mono parameter (boolean)
  auto &mono_param = processor.get_mono_param ();
  EXPECT_EQ (mono_param.range ().minf_, 0.0f);
  EXPECT_EQ (mono_param.range ().maxf_, 1.0f);
}

TEST_F (TrackProcessorTest, MidiTrackProcessingWithTransformAndAppend)
{
  auto transform_temp = dsp::MidiEventBuffer::make_reserved ();
  TrackProcessor::TransformMidiInputsFunc transform_func =
    [&transform_temp] (dsp::MidiEventBuffer &events) {
      // Transform: change all notes to channel 3 and reduce velocity by 20
      transform_temp.clear ();
      for (const auto &event : events)
        {
          auto d = event.data ();
          if (utils::midi::midi_is_note_on (d))
            {
              // Note on, change channel to 3 and reduce velocity
              std::array<midi_byte_t, 3> arr{};
              std::ranges::copy (d, arr.begin ());
              arr[0] = (arr[0] & 0xF0) | 0x02;
              arr[2] = std::max (0, arr[2] - 20);
              transform_temp.push_back (
                event.time (),
                std::span<const midi_byte_t>{ arr.data (), d.size () });
            }
          else if (utils::midi::midi_is_note_off (d))
            {
              // Note off, change channel to 3
              std::array<midi_byte_t, 3> arr{};
              std::ranges::copy (d, arr.begin ());
              arr[0] = (arr[0] & 0xF0) | 0x02;
              transform_temp.push_back (
                event.time (),
                std::span<const midi_byte_t>{ arr.data (), d.size () });
            }
          else
            {
              transform_temp.push_back (event.time (), d);
            }
        }
      events.swap (transform_temp);
    };

  TrackProcessor::AppendMidiInputsToOutputsFunc append_func =
    [] (
      dsp::MidiEventBuffer &out_events, const dsp::MidiEventBuffer &in_events,
      const dsp::graph::ProcessBlockInfo &time_nfo) {
      // Append all transformed events to output
      for (const auto &event : in_events)
        {
          out_events.push_back (event.time (), event.data ());
        }
    };

  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"MIDI Transform Append"; },
    [] { return true; }, TrackProcessor::Capabilities::PianoRoll, *registry_,
    std::nullopt, transform_func, append_func);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  auto &midi_in = processor.get_midi_in_port ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    midi_in.buffer_.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 64, 80, units::samples (10u));
    midi_in.buffer_.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (20u));
    midi_in.buffer_.push_back (_ev.time_, _ev.data ());
  }

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Verify events were both transformed and appended
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_EQ (midi_out.buffer_.size (), 3);

  // Check that events were properly transformed and appended
  const auto &events = midi_out.buffer_;
  bool        found_transformed_note_on_1 = false;
  bool        found_transformed_note_on_2 = false;
  bool        found_transformed_note_off = false;

  for (const auto &event : events)
    {
      auto ed = event.data ();
      if (
        utils::midi::midi_is_note_on (ed)
        && utils::midi::midi_get_channel_0_to_15 (ed) == 0x02)
        {
          // Note on, channel 3
          if (
            utils::midi::midi_get_note_number (ed) == 60
            && utils::midi::midi_get_velocity (ed) == 80)
            {
              found_transformed_note_on_1 = true;
            }
          else if (
            utils::midi::midi_get_note_number (ed) == 64
            && utils::midi::midi_get_velocity (ed) == 60)
            {
              found_transformed_note_on_2 = true;
            }
        }
      else if (
        utils::midi::midi_is_note_off (ed)
        && utils::midi::midi_get_channel_0_to_15 (ed) == 0x02)
        {
          // Note off, channel 3
          if (utils::midi::midi_get_note_number (ed) == 60)
            {
              found_transformed_note_off = true;
            }
        }
    }

  EXPECT_TRUE (found_transformed_note_on_1);
  EXPECT_TRUE (found_transformed_note_on_2);
  EXPECT_TRUE (found_transformed_note_off);
}

TEST_F (TrackProcessorTest, MidiTrackProcessingWithCustomMidiEventProvider)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"MIDI Custom Provider"; },
    [] { return true; }, TrackProcessor::Capabilities::PianoRoll, *registry_);

  // Create a MIDI sequence with note events
  juce::MidiMessageSequence midi_sequence;
  midi_sequence.addEvent (
    juce::MidiMessage::noteOn (1, 60, static_cast<uint8_t> (100)), 0.0);
  midi_sequence.addEvent (juce::MidiMessage::noteOff (1, 60), 50.0);
  midi_sequence.addEvent (
    juce::MidiMessage::noteOn (1, 64, static_cast<uint8_t> (80)), 25.0);
  midi_sequence.addEvent (juce::MidiMessage::noteOff (1, 64), 75.0);
  midi_sequence.updateMatchedPairs ();

  // Set the custom MIDI event provider
  processor.set_custom_midi_event_provider (
    create_midi_event_provider_from_sequence (midi_sequence));

  // Activate the custom event provider
  processor.set_midi_providers_active (
    TrackProcessor::ActiveMidiEventProviders::Custom, true);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  // Enable rolling so that events are taken into account
  ON_CALL (*transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Rolling));

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Verify events were processed and output
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_GT (midi_out.buffer_.size (), 0);

  // Check that the expected events were produced
  const auto &events = midi_out.buffer_;
  bool        found_note_60_on = false;
  bool        found_note_60_off = false;
  bool        found_note_64_on = false;
  bool        found_note_64_off = false;

  for (const auto &event : events)
    {
      auto ed = event.data ();
      if (utils::midi::midi_is_note_on (ed)) // Note on
        {
          if (
            utils::midi::midi_get_note_number (ed) == 60
            && utils::midi::midi_get_velocity (ed) == 100)
            {
              found_note_60_on = true;
            }
          else if (
            utils::midi::midi_get_note_number (ed) == 64
            && utils::midi::midi_get_velocity (ed) == 80)
            {
              found_note_64_on = true;
            }
        }
      else if (utils::midi::midi_is_note_off (ed)) // Note off
        {
          if (utils::midi::midi_get_note_number (ed) == 60)
            {
              found_note_60_off = true;
            }
          else if (utils::midi::midi_get_note_number (ed) == 64)
            {
              found_note_64_off = true;
            }
        }
    }

  EXPECT_TRUE (found_note_60_on);
  EXPECT_TRUE (found_note_60_off);
  EXPECT_TRUE (found_note_64_on);
  EXPECT_TRUE (found_note_64_off);
}

TEST_F (TrackProcessorTest, MidiTrackProcessingWithEmptyCustomMidiEventProvider)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"MIDI Empty Provider"; },
    [] { return true; }, TrackProcessor::Capabilities::PianoRoll, *registry_);

  // Set empty MIDI sequence
  juce::MidiMessageSequence empty_sequence;
  processor.set_custom_midi_event_provider (
    create_midi_event_provider_from_sequence (empty_sequence));

  // Activate the custom event provider
  processor.set_midi_providers_active (
    TrackProcessor::ActiveMidiEventProviders::Custom, true);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  // Enable rolling so that events would be processed if they existed
  ON_CALL (*transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Rolling));

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Verify no events were produced from empty sequence
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_EQ (midi_out.buffer_.size (), 0);
}

TEST_F (
  TrackProcessorTest,
  MidiTrackProcessingWithMultipleCustomMidiEventProviders)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"MIDI Multiple Provider"; },
    [] { return true; }, TrackProcessor::Capabilities::PianoRoll, *registry_);

  // Create a MIDI sequence with multiple events
  juce::MidiMessageSequence midi_sequence;
  midi_sequence.addEvent (
    juce::MidiMessage::noteOn (1, 60, static_cast<uint8_t> (100)), 0.0);
  midi_sequence.addEvent (
    juce::MidiMessage::noteOn (1, 64, static_cast<uint8_t> (90)), 10.0);
  midi_sequence.addEvent (
    juce::MidiMessage::noteOn (1, 67, static_cast<uint8_t> (80)), 20.0);
  midi_sequence.addEvent (juce::MidiMessage::noteOff (1, 60), 50.0);
  midi_sequence.addEvent (juce::MidiMessage::noteOff (1, 64), 60.0);
  midi_sequence.addEvent (juce::MidiMessage::noteOff (1, 67), 70.0);
  midi_sequence.updateMatchedPairs ();

  processor.set_custom_midi_event_provider (
    create_midi_event_provider_from_sequence (midi_sequence));

  // Activate the custom event provider
  processor.set_midi_providers_active (
    TrackProcessor::ActiveMidiEventProviders::Custom, true);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (512));

  processor.prepare_for_processing (nullptr, sample_rate_, units::samples (512));

  // Enable rolling
  ON_CALL (*transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Rolling));

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Verify multiple events were processed
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_GE (midi_out.buffer_.size (), 6);
}

TEST_F (
  TrackProcessorTest,
  MidiTrackProcessingWithOverlappingCustomMidiEventProviders)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi,
    [] { return u8"MIDI Overlapping Provider"; }, [] { return true; },
    TrackProcessor::Capabilities::PianoRoll, *registry_);

  // Create MIDI sequence with overlapping events - note on before previous note
  // off
  juce::MidiMessageSequence midi_sequence;
  midi_sequence.addEvent (
    juce::MidiMessage::noteOn (1, 60, static_cast<uint8_t> (100)), 0.0);
  midi_sequence.addEvent (
    juce::MidiMessage::noteOn (1, 60, static_cast<uint8_t> (80)),
    20.0); // Note on before previous note off (overlap)
  midi_sequence.addEvent (
    juce::MidiMessage::noteOff (1, 60), 25.0); // First note off
  midi_sequence.addEvent (
    juce::MidiMessage::noteOff (1, 60), 75.0); // Second note off
  midi_sequence.updateMatchedPairs ();

  processor.set_custom_midi_event_provider (
    create_midi_event_provider_from_sequence (midi_sequence));

  // Activate the custom event provider
  processor.set_midi_providers_active (
    TrackProcessor::ActiveMidiEventProviders::Custom, true);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  // Enable rolling
  ON_CALL (*transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Rolling));

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // Verify overlapping events were processed correctly
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_GT (midi_out.buffer_.size (), 0);

  // Count note on/off events for note 60
  int note_60_on_count = 0;
  int note_60_off_count = 0;

  for (const auto &event : midi_out.buffer_)
    {
      auto ed = event.data ();
      if (
        utils::midi::midi_is_note_on (ed)
        && utils::midi::midi_get_note_number (ed) == 60)
        {
          note_60_on_count++;
        }
      else if (
        utils::midi::midi_is_note_off (ed)
        && utils::midi::midi_get_note_number (ed) == 60)
        {
          note_60_off_count++;
        }
    }

  // Should have multiple note on/off events for the same note
  EXPECT_GE (note_60_on_count, 2);
  EXPECT_GE (note_60_off_count, 2);
}

TEST_F (TrackProcessorTest, RecordingCapability)
{
  TrackProcessor no_rec (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"No Rec"; },
    [] { return true; }, TrackProcessor::Capabilities::AudioTrack, *registry_);
  EXPECT_FALSE (no_rec.is_recording_armed ());

  TrackProcessor with_rec (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"With Rec"; },
    [] { return true; },
    TrackProcessor::Capabilities::AudioTrack
      | TrackProcessor::Capabilities::Recording,
    *registry_);
  with_rec.prepare_for_processing (nullptr, sample_rate_, block_length_);

  EXPECT_FALSE (with_rec.is_recording_armed ());
  with_rec.set_recording_armed (true);
  EXPECT_TRUE (with_rec.is_recording_armed ());
  with_rec.set_recording_armed (false);
  EXPECT_FALSE (with_rec.is_recording_armed ());
}

TEST_F (TrackProcessorTest, RecordingCallbackSimpleRange)
{
  struct RecordingCall
  {
    units::sample_t timeline_position;
    size_t          nframes;
  };
  std::vector<RecordingCall> recording_calls;

  TrackProcessor::RecordingCallbackRT recording_cb =
    [&recording_calls] (
      units::sample_t timeline_position, const dsp::ITransport &,
      const dsp::MidiEventBuffer *,
      std::optional<TrackProcessor::ConstStereoPortPair> stereo_ports,
      units::sample_u32_t                                nframes) {
      recording_calls.push_back (
        {
          timeline_position,
          stereo_ports.has_value ()
            ? stereo_ports->first.size ()
            : static_cast<size_t> (0),
        });
    };
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Recording Simple"; },
    [] { return true; },
    TrackProcessor::Capabilities::AudioTrack
      | TrackProcessor::Capabilities::Recording,
    *registry_, std::nullopt, std::nullopt, std::nullopt, recording_cb);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (44100), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);
  processor.set_recording_armed (true);

  ON_CALL (*transport_, recording_preroll_frames_remaining ())
    .WillByDefault (::testing::Return (units::samples (0)));
  ON_CALL (*transport_, loop_enabled ())
    .WillByDefault (::testing::Return (false));
  ON_CALL (*transport_, punch_enabled ())
    .WillByDefault (::testing::Return (false));
  ON_CALL (*transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Rolling));

  recording_calls.reserve (4);
  processor.process_block (time_nfo, *transport_, *tempo_map_);

  ASSERT_EQ (recording_calls.size (), 1u);
  EXPECT_EQ (recording_calls[0].timeline_position, units::samples (44100));
  EXPECT_EQ (recording_calls[0].nframes, 256u);
}

TEST_F (TrackProcessorTest, RecordingCallbackSkippedDuringPreroll)
{
  bool recording_called = false;

  TrackProcessor::RecordingCallbackRT recording_cb =
    [&recording_called] (
      units::sample_t, const dsp::ITransport &, const dsp::MidiEventBuffer *,
      std::optional<TrackProcessor::ConstStereoPortPair>,
      units::sample_u32_t nframes) { recording_called = true; };

  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Recording Preroll"; },
    [] { return true; },
    TrackProcessor::Capabilities::AudioTrack
      | TrackProcessor::Capabilities::Recording,
    *registry_, std::nullopt, std::nullopt, std::nullopt, recording_cb);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);
  processor.set_recording_armed (true);

  ON_CALL (*transport_, recording_preroll_frames_remaining ())
    .WillByDefault (::testing::Return (units::samples (1024)));
  EXPECT_CALL (*transport_, recording_preroll_frames_remaining ())
    .Times (::testing::AtLeast (1));

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  EXPECT_FALSE (recording_called);
}

TEST_F (TrackProcessorTest, PunchRecordingPassesCorrectAudioSpanToCallback)
{
  constexpr auto max_block_length = units::samples (512u);

  // Punch range [128, 384) within [0, 512) block.
  // handle_recording will split into sub-ranges.  Only the punch range
  // sub-arc has recording_enabled == true in the transport, so the
  // callback should only fire for that sub-range with nframes == 256.
  const auto punch_start = units::samples (128);
  const auto punch_end = units::samples (384);
  const auto punch_nframes = units::samples (256u);

  struct CapturedCall
  {
    units::sample_t                             timeline_position;
    boost::container::static_vector<float, 512> l_samples;
    boost::container::static_vector<float, 512> r_samples;
  };
  std::vector<CapturedCall> calls;

  TrackProcessor::RecordingCallbackRT capture_cb =
    [&] (
      units::sample_t timeline_position, const dsp::ITransport &transport,
      const dsp::MidiEventBuffer *,
      std::optional<TrackProcessor::ConstStereoPortPair> stereo_ports,
      units::sample_u32_t                                nframes) {
      if (stereo_ports.has_value ())
        {
          auto &sp = *stereo_ports;
          calls.push_back (
            {
              timeline_position,
              { sp.first.begin (),  sp.first.end ()  },
              { sp.second.begin (), sp.second.end () },
          });
        }
    };

  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Test Audio Track"; },
    [] { return true; },
    TrackProcessor::Capabilities::AudioTrack
      | TrackProcessor::Capabilities::Recording,
    *registry_, std::nullopt, std::nullopt, std::nullopt, capture_cb);

  processor.prepare_for_processing (nullptr, sample_rate_, max_block_length);
  processor.set_recording_armed (true);

  // Fill the stereo input buffer with known per-sample data so we can
  // verify the callback receives the right slice.
  auto &stereo_in = processor.get_stereo_in_port ();
  auto &in_buf_unique = stereo_in.buffers ();
  ASSERT_NE (in_buf_unique, nullptr);
  auto * in_buf = in_buf_unique.get ();
  ASSERT_EQ (in_buf->getNumChannels (), 2);
  ASSERT_GE (
    in_buf->getNumSamples (), max_block_length.in<int> (units::samples));

  for (int ch = 0; ch < 2; ++ch)
    {
      for (int i = 0; i < in_buf->getNumSamples (); ++i)
        {
          // Left: 0.0, 1.0, 2.0, ...  Right: 0.0, -1.0, -2.0, ...
          in_buf->setSample (
            ch, i, ch == 0 ? static_cast<float> (i) : -static_cast<float> (i));
        }
    }

  ON_CALL (*transport_, punch_enabled ())
    .WillByDefault (::testing::Return (true));
  ON_CALL (*transport_, loop_enabled ())
    .WillByDefault (::testing::Return (false));
  ON_CALL (*transport_, get_punch_range_positions ())
    .WillByDefault (::testing::Return (std::make_pair (punch_start, punch_end)));
  ON_CALL (*transport_, recording_enabled ())
    .WillByDefault (::testing::Return (true));
  ON_CALL (*transport_, recording_preroll_frames_remaining ())
    .WillByDefault (::testing::Return (units::samples (0)));
  ON_CALL (*transport_, metronome_countin_frames_remaining ())
    .WillByDefault (::testing::Return (units::samples (0)));
  ON_CALL (*transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Rolling));

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), max_block_length);

  calls.reserve (4);
  processor.process_block (time_nfo, *transport_, *tempo_map_);

  // The callback should only fire for the punch range [128, 384).
  // Pre-punch [0, 128) and post-punch [384, 512) should NOT trigger
  // recording.
  ASSERT_EQ (calls.size (), 1u)
    << "Expected exactly 1 recording callback (for the punch range), got "
    << calls.size () << "\nCalls received:\n"
    <<
    [&calls] () {
      std::string s;
      for (size_t i = 0; i < calls.size (); ++i)
        {
          s += fmt::format (
            "  #{}: start={}, span_size={}\n", i,
            calls[i].timeline_position.in (units::samples),
            calls[i].l_samples.size ());
        }
      return s;
    }();

  const auto &call = calls[0];

  // --- Verify the callback was for the punch range ---
  EXPECT_EQ (call.timeline_position, punch_start);

  // --- Verify sizes ---
  const auto expected_nframes = punch_nframes.in<size_t> (units::samples);
  EXPECT_EQ (call.l_samples.size (), expected_nframes);
  EXPECT_EQ (call.r_samples.size (), expected_nframes);

  // --- Verify audio contents match the buffer slice [128, 384) ---
  for (size_t i = 0; i < expected_nframes; ++i)
    {
      const auto buf_idx = static_cast<int64_t> (i) + 128;
      EXPECT_FLOAT_EQ (call.l_samples[i], static_cast<float> (buf_idx))
        << "L sample mismatch at span index " << i << " (buffer index "
        << buf_idx << ")";
      EXPECT_FLOAT_EQ (call.r_samples[i], -static_cast<float> (buf_idx))
        << "R sample mismatch at span index " << i << " (buffer index "
        << buf_idx << ")";
    }

  // --- Sanity: last sample should NOT be the buffer boundary ---
  // If the bug were present (span = 512 instead of 256), we'd see
  // sample index 383+1 = 384 worth of data, ending at value 511.
  ASSERT_FALSE (call.l_samples.empty ());
  EXPECT_FLOAT_EQ (call.l_samples.back (), 383.0f)
    << "Last sample should be at punch_end - 1";
  EXPECT_FLOAT_EQ (call.r_samples.back (), -383.0f);
}

TEST_F (TrackProcessorTest, RecordingCallbackNotFiredWhenTransportPaused)
{
  constexpr auto max_block_length = units::samples (256u);

  int                                 callback_count = 0;
  TrackProcessor::RecordingCallbackRT capture_cb =
    [&] (
      units::sample_t, const dsp::ITransport &, const dsp::MidiEventBuffer *,
      std::optional<TrackProcessor::ConstStereoPortPair>,
      units::sample_u32_t nframes) { ++callback_count; };

  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Test Audio Track"; },
    [] { return true; },
    TrackProcessor::Capabilities::AudioTrack
      | TrackProcessor::Capabilities::Recording,
    *registry_, std::nullopt, std::nullopt, std::nullopt, capture_cb);

  processor.prepare_for_processing (nullptr, sample_rate_, max_block_length);
  processor.set_recording_armed (true);

  ON_CALL (*transport_, recording_enabled ())
    .WillByDefault (::testing::Return (true));
  ON_CALL (*transport_, recording_preroll_frames_remaining ())
    .WillByDefault (::testing::Return (units::samples (0)));
  ON_CALL (*transport_, metronome_countin_frames_remaining ())
    .WillByDefault (::testing::Return (units::samples (0)));
  ON_CALL (*transport_, punch_enabled ())
    .WillByDefault (::testing::Return (false));
  ON_CALL (*transport_, loop_enabled ())
    .WillByDefault (::testing::Return (false));

  EXPECT_CALL (*transport_, get_play_state ()).Times (::testing::AtLeast (1));

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), max_block_length);

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  EXPECT_EQ (callback_count, 0)
    << "Recording callback should not fire when transport is not rolling";
}

TEST_F (TrackProcessorTest, RecordingAcrossLoopBoundaryViaGraphNode)
{
  constexpr auto max_block_length = units::samples (512u);

  struct CapturedCall
  {
    units::sample_t                             timeline_position;
    boost::container::static_vector<float, 512> l_samples;
    boost::container::static_vector<float, 512> r_samples;
  };
  std::vector<CapturedCall> calls;

  TrackProcessor::RecordingCallbackRT recording_cb =
    [&calls] (
      units::sample_t timeline_position, const dsp::ITransport &,
      const dsp::MidiEventBuffer *,
      std::optional<TrackProcessor::ConstStereoPortPair> stereo_ports,
      units::sample_u32_t                                nframes) {
      if (stereo_ports.has_value ())
        {
          auto &sp = *stereo_ports;
          calls.push_back (
            {
              .timeline_position = timeline_position,
              .l_samples = { sp.first.begin (),  sp.first.end ()  },
              .r_samples = { sp.second.begin (), sp.second.end () }
          });
        }
    };

  auto processor = std::make_unique<TrackProcessor> (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Recording Loop"; },
    [] { return true; },
    TrackProcessor::Capabilities::AudioTrack
      | TrackProcessor::Capabilities::Recording,
    *registry_, std::nullopt, std::nullopt, std::nullopt, recording_cb);

  processor->prepare_for_processing (nullptr, sample_rate_, max_block_length);
  processor->set_recording_armed (true);

  // Fill input buffer with known per-sample data
  auto &stereo_in = processor->get_stereo_in_port ();
  auto &in_buf_unique = stereo_in.buffers ();
  ASSERT_NE (in_buf_unique, nullptr);
  auto * in_buf = in_buf_unique.get ();
  ASSERT_EQ (in_buf->getNumChannels (), 2);
  ASSERT_GE (
    in_buf->getNumSamples (), max_block_length.in<int> (units::samples));
  for (int ch = 0; ch < 2; ++ch)
    {
      for (int i = 0; i < in_buf->getNumSamples (); ++i)
        {
          in_buf->setSample (
            ch, i, ch == 0 ? static_cast<float> (i) : -static_cast<float> (i));
        }
    }

  ON_CALL (*transport_, recording_preroll_frames_remaining ())
    .WillByDefault (::testing::Return (units::samples (0)));
  ON_CALL (*transport_, loop_enabled ()).WillByDefault (::testing::Return (true));
  ON_CALL (*transport_, get_loop_range_positions ())
    .WillByDefault (
      ::testing::Return (
        std::make_pair (units::samples (0), units::samples (384))));
  ON_CALL (*transport_, punch_enabled ())
    .WillByDefault (::testing::Return (false));
  ON_CALL (*transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Rolling));
  ON_CALL (*transport_, get_playhead_position_in_audio_thread ())
    .WillByDefault (::testing::Return (units::samples (256)));

  // Wrap in GraphNode so process_chunks_after_splitting_at_loop_points
  // handles the loop split.
  dsp::graph::GraphNode node (1, *processor);

  // 512-frame cycle starting at position 256.
  // Loop end is at 384, so the graph splits into:
  //   Chunk 1: [256, 384) = 128 frames (pre-loop)
  //   Chunk 2: [0, 128)   = 128 frames (post-loop, wraps to loop start)
  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (256), max_block_length);
  calls.reserve (4);
  node.process (time_nfo, units::samples (0), *transport_, *tempo_map_);

  ASSERT_EQ (calls.size (), 2u);

  // Chunk 1: [256, 384) — buffer offset 0, 128 frames
  EXPECT_EQ (calls[0].timeline_position, units::samples (256));
  ASSERT_EQ (calls[0].l_samples.size (), 128u);
  for (size_t i = 0; i < 128u; ++i)
    {
      EXPECT_FLOAT_EQ (calls[0].l_samples[i], static_cast<float> (i))
        << "L sample mismatch at chunk 1 index " << i;
      EXPECT_FLOAT_EQ (calls[0].r_samples[i], -static_cast<float> (i))
        << "R sample mismatch at chunk 1 index " << i;
    }

  // Chunk 2: [0, 384) — buffer offset 128, 384 frames
  EXPECT_EQ (calls[1].timeline_position, units::samples (0));
  ASSERT_EQ (calls[1].l_samples.size (), 384u);
  for (size_t i = 0; i < 384u; ++i)
    {
      const auto buf_idx = static_cast<int64_t> (i) + 128;
      EXPECT_FLOAT_EQ (calls[1].l_samples[i], static_cast<float> (buf_idx))
        << "L sample mismatch at chunk 2 index " << i;
      EXPECT_FLOAT_EQ (calls[1].r_samples[i], -static_cast<float> (buf_idx))
        << "R sample mismatch at chunk 2 index " << i;
    }
}

TEST_F (TrackProcessorTest, MonitoringPreservesExistingClipMaterialDuringPlayback)
{
  constexpr float clip_value = 0.5f;
  constexpr float input_value = 0.3f;
  constexpr auto  num_frames = 256u;

  TrackProcessor::FillEventsCallback fill_clip =
    [&] (
      const dsp::ITransport &, const dsp::graph::ProcessBlockInfo &time_nfo,
      dsp::MidiEventBuffer *,
      std::optional<TrackProcessor::StereoPortPair> stereo_ports) {
      if (!stereo_ports.has_value ())
        return;
      for (
        const auto i :
        std::views::iota (0u, time_nfo.nframes_.in (units::samples)))
        {
          stereo_ports
            ->first[time_nfo.buffer_offset_.in<size_t> (units::samples) + i] +=
            clip_value;
          stereo_ports
            ->second[time_nfo.buffer_offset_.in<size_t> (units::samples) + i] +=
            clip_value;
        }
    };

  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Monitor+Clip Track"; },
    [] { return true; }, TrackProcessor::Capabilities::AudioTrack, *registry_,
    fill_clip);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (num_frames));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  processor.get_monitor_audio_param ().setBaseValue (
    processor.get_monitor_audio_param ().range ().normalized_from_enum (
      TrackProcessor::MonitorMode::On));

  processor.get_mono_param ().setBaseValue (0.f);

  auto &stereo_in = processor.get_stereo_in_port ();
  for (const auto i : std::views::iota (0u, num_frames))
    {
      stereo_in.buffers ()->setSample (0, i, input_value);
      stereo_in.buffers ()->setSample (1, i, input_value);
    }

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  auto &stereo_out = processor.get_stereo_out_port ();
  for (const auto i : std::views::iota (0u, num_frames))
    {
      EXPECT_NEAR (
        stereo_out.buffers ()->getSample (0, i), clip_value + input_value,
        AUDIO_BUFFER_DIFF_TOLERANCE)
        << "L channel: expected clip (" << clip_value << ") + input ("
        << input_value << ") = " << (clip_value + input_value) << " but got "
        << stereo_out.buffers ()->getSample (0, i) << " at sample " << i;
      EXPECT_NEAR (
        stereo_out.buffers ()->getSample (1, i), clip_value + input_value,
        AUDIO_BUFFER_DIFF_TOLERANCE)
        << "R channel: expected clip (" << clip_value << ") + input ("
        << input_value << ") = " << (clip_value + input_value) << " but got "
        << stereo_out.buffers ()->getSample (1, i) << " at sample " << i;
    }
}

TEST_F (TrackProcessorTest, DisarmedTrackSkipsRecording)
{
  bool recording_called = false;

  TrackProcessor::RecordingCallbackRT recording_cb =
    [&recording_called] (
      units::sample_t, const dsp::ITransport &, const dsp::MidiEventBuffer *,
      std::optional<TrackProcessor::ConstStereoPortPair>,
      units::sample_u32_t nframes) { recording_called = true; };

  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Audio, [] { return u8"Disarmed Track"; },
    [] { return true; },
    TrackProcessor::Capabilities::AudioTrack
      | TrackProcessor::Capabilities::Recording,
    *registry_, std::nullopt, std::nullopt, std::nullopt, recording_cb);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (44100), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  ON_CALL (*transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Rolling));
  EXPECT_CALL (*transport_, get_play_state ()).Times (::testing::AtLeast (1));

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  EXPECT_FALSE (recording_called)
    << "Recording callback should not fire for disarmed track";
}

TEST_F (TrackProcessorTest, PunchRecordingPassesCorrectMidiEventsToCallback)
{
  constexpr auto max_block_length = units::samples (512u);
  const auto     punch_start = units::samples (128);
  const auto     punch_end = units::samples (384);

  struct CapturedMidi
  {
    units::sample_t                        timeline_position;
    std::array<dsp::RealtimeMidiEvent, 16> events;
    size_t                                 event_count = 0;
    units::sample_u32_t                    nframes;
  };
  std::vector<CapturedMidi> calls;
  calls.reserve (4);

  TrackProcessor::RecordingCallbackRT capture_cb =
    [&] (
      units::sample_t              timeline_position, const dsp::ITransport &,
      const dsp::MidiEventBuffer * midi_events,
      std::optional<TrackProcessor::ConstStereoPortPair>,
      units::sample_u32_t nframes) {
      if (midi_events != nullptr)
        {
          CapturedMidi cm{ timeline_position, {}, 0, nframes };
          for (const auto &ev : *midi_events)
            {
              cm.events[cm.event_count++] =
                dsp::midi_event::make_raw_rt (ev.data (), ev.time ());
            }
          calls.push_back (std::move (cm));
        }
    };

  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"MIDI Punch Rec"; },
    [] { return true; },
    TrackProcessor::Capabilities::PianoRoll
      | TrackProcessor::Capabilities::Recording,
    *registry_, std::nullopt, std::nullopt, std::nullopt, capture_cb);

  processor.prepare_for_processing (nullptr, sample_rate_, max_block_length);
  processor.set_recording_armed (true);

  auto      &hw_in = processor.get_hw_midi_in_port ();
  const auto _e1 =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (50u));
  hw_in.buffer_.push_back (_e1.time_, _e1.data ());
  const auto _e2 =
    dsp::midi_event::make_note_on (0, 62, 90, units::samples (200u));
  hw_in.buffer_.push_back (_e2.time_, _e2.data ());
  const auto _e3 =
    dsp::midi_event::make_note_on (0, 64, 80, units::samples (350u));
  hw_in.buffer_.push_back (_e3.time_, _e3.data ());
  const auto _e4 =
    dsp::midi_event::make_note_on (0, 65, 70, units::samples (400u));
  hw_in.buffer_.push_back (_e4.time_, _e4.data ());

  ON_CALL (*transport_, punch_enabled ())
    .WillByDefault (::testing::Return (true));
  ON_CALL (*transport_, loop_enabled ())
    .WillByDefault (::testing::Return (false));
  ON_CALL (*transport_, get_punch_range_positions ())
    .WillByDefault (::testing::Return (std::make_pair (punch_start, punch_end)));
  ON_CALL (*transport_, recording_enabled ())
    .WillByDefault (::testing::Return (true));
  ON_CALL (*transport_, recording_preroll_frames_remaining ())
    .WillByDefault (::testing::Return (units::samples (0)));
  ON_CALL (*transport_, metronome_countin_frames_remaining ())
    .WillByDefault (::testing::Return (units::samples (0)));
  ON_CALL (*transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Rolling));

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), max_block_length);

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  ASSERT_EQ (calls.size (), 1u)
    << "Expected exactly 1 recording callback (for the punch range)";

  const auto &call = calls[0];
  EXPECT_EQ (call.timeline_position, punch_start);
  EXPECT_EQ (call.nframes, units::samples (256u));

  ASSERT_EQ (call.event_count, 2u)
    << "Only events within punch range [128,384) should be recorded";

  EXPECT_EQ (call.events[0].time_, units::samples (72u))
    << "Event at block offset 200 should be shifted to 200 - 128 = 72";
  EXPECT_EQ (utils::midi::midi_get_note_number (call.events[0].data ()), 62);

  EXPECT_EQ (call.events[1].time_, units::samples (222u))
    << "Event at block offset 350 should be shifted to 350 - 128 = 222";
  EXPECT_EQ (utils::midi::midi_get_note_number (call.events[1].data ()), 64);
}

TEST_F (TrackProcessorTest, MidiOutputSortedNoteOffBeforeNoteOnAtSameTimestamp)
{
  TrackProcessor processor (
    *tempo_map_, dsp::PortType::Midi, [] { return u8"Sort Test MIDI Track"; },
    [] { return true; }, TrackProcessor::Capabilities::PianoRoll, *registry_,
    std::nullopt, std::nullopt,
    [] (
      dsp::MidiEventBuffer &out_events, const dsp::MidiEventBuffer &in_events,
      const dsp::graph::ProcessBlockInfo &time_nfo) {
      for (const auto &ev : in_events)
        {
          if (
            ev.time () >= time_nfo.buffer_offset_
            && ev.time () < time_nfo.buffer_offset_ + time_nfo.nframes_)
            {
              out_events.push_back (ev.time (), ev.data ());
            }
        }
    });

  const auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  processor.prepare_for_processing (nullptr, sample_rate_, block_length_);

  // Inject events into midi_in in WRONG order: note-on, then note-off, then
  // another note-on, then all-notes-off — all at timestamp 10.
  auto &midi_in = processor.get_midi_in_port ();
  midi_in.buffer_.clear ();
  const auto _e1 =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (10));
  midi_in.buffer_.push_back (_e1.time_, _e1.data ());
  const auto _e2 = dsp::midi_event::make_note_off (0, 60, units::samples (10));
  midi_in.buffer_.push_back (_e2.time_, _e2.data ());
  const auto _e3 =
    dsp::midi_event::make_note_on (0, 64, 80, units::samples (10));
  midi_in.buffer_.push_back (_e3.time_, _e3.data ());
  const auto _e4 = dsp::midi_event::make_all_notes_off (0, units::samples (10));
  midi_in.buffer_.push_back (_e4.time_, _e4.data ());

  processor.process_block (time_nfo, *transport_, *tempo_map_);

  const auto &midi_out = processor.get_midi_out_port ();
  ASSERT_EQ (midi_out.buffer_.size (), 4u)
    << "Expected all 4 MIDI events in output";

  // At timestamp 10, note-off and all-notes-off must precede note-on events.
  const auto is_note_on = [] (const dsp::MidiEventView &ev) {
    return utils::midi::midi_is_note_on (ev.data ());
  };
  const auto is_note_off_or_all_off = [] (const dsp::MidiEventView &ev) {
    const auto d = ev.data ();
    return utils::midi::midi_is_note_off (d)
           || utils::midi::midi_is_all_notes_off (d);
  };

  // Events 0,1 should be note-off/all-notes-off; events 2,3 should be note-on
  auto it = midi_out.buffer_.begin ();
  EXPECT_TRUE (is_note_off_or_all_off (*it))
    << "First event at t=10 should be note-off or all-notes-off";
  ++it;
  EXPECT_TRUE (is_note_off_or_all_off (*it))
    << "Second event at t=10 should be note-off or all-notes-off";
  ++it;
  EXPECT_TRUE (is_note_on (*it)) << "Third event at t=10 should be note-on";
  ++it;
  EXPECT_TRUE (is_note_on (*it)) << "Fourth event at t=10 should be note-on";
}

} // namespace zrythm::structure::tracks
