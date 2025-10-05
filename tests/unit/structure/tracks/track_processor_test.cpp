// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/tracks/track_processor.h"

#include "../../dsp/graph_helpers.h"
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
    transport_ = std::make_unique<dsp::graph_test::MockTransport> ();
  }

  template <typename RegionT>
  auto
  create_region (signed_frame_t timeline_pos_samples, signed_frame_t length)
  {
    auto region_ref = obj_registry_.create_object<RegionT> (
      *tempo_map_, obj_registry_, file_audio_source_registry_, nullptr);
    auto * region = region_ref.template get_object_as<RegionT> ();
    region->position ()->setSamples (static_cast<double> (timeline_pos_samples));
    region->bounds ()->length ()->setSamples (static_cast<double> (length));

    if constexpr (std::is_same_v<RegionT, arrangement::AudioRegion>)
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

        auto source_ref = file_audio_source_registry_.create_object<
          dsp::FileAudioSource> (
          *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32, 44100, 120.f,
          u8"DummySource");
        auto audio_source_object_ref =
          obj_registry_.create_object<arrangement::AudioSourceObject> (
            *tempo_map_, file_audio_source_registry_, source_ref);
        region->set_source (audio_source_object_ref);

        region->prepare_to_play (block_length_, sample_rate_);
      }
    return region_ref;
  }

  void add_midi_note (
    arrangement::MidiRegion * region,
    int                       note,
    int                       velocity,
    signed_frame_t            start_frames,
    signed_frame_t            end_frames)
  {
    auto note_ref =
      obj_registry_.create_object<arrangement::MidiNote> (*tempo_map_);
    note_ref.get_object_as<arrangement::MidiNote> ()->setPitch (note);
    note_ref.get_object_as<arrangement::MidiNote> ()->setVelocity (velocity);
    note_ref.get_object_as<arrangement::MidiNote> ()->position ()->setSamples (
      static_cast<double> (start_frames));
    note_ref.get_object_as<arrangement::MidiNote> ()
      ->bounds ()
      ->length ()
      ->setSamples (static_cast<double> (end_frames - start_frames));
    region->add_object (note_ref);
  }

  // Helper function to create a MIDI event provider from a MIDI sequence
  TrackProcessor::MidiEventProviderProcessFunc
  create_midi_event_provider_from_sequence (const juce::MidiMessageSequence &seq)
  {
    return
      [seq] (
        const EngineProcessTimeInfo &time_nfo,
        dsp::MidiEventVector        &output_buffer) {
        // Convert juce MIDI sequence to output buffer
        for (int i = 0; i < seq.getNumEvents (); ++i)
          {
            const auto * event = seq.getEventPointer (i);
            if (event == nullptr)
              continue;

            // Convert juce MIDI message to our format
            const auto &msg = event->message;
            if (msg.getRawDataSize () <= 3)
              {
                dsp::MidiEvent new_event;
                new_event.time_ =
                  static_cast<unsigned_frame_t> (msg.getTimeStamp ());
                std::copy_n (
                  msg.getRawData (), msg.getRawDataSize (),
                  new_event.raw_buffer_.begin ());

                // Only add events that are within the time range
                if (new_event.time_ < time_nfo.nframes_)
                  {
                    output_buffer.push_back (new_event);
                  }
              }
          }
      };
  }

  std::unique_ptr<dsp::TempoMap>                  tempo_map_;
  std::unique_ptr<dsp::graph_test::MockTransport> transport_;
  dsp::PortRegistry                               port_registry_;
  dsp::ProcessorParameterRegistry           param_registry_{ port_registry_ };
  arrangement::ArrangerObjectRegistry       obj_registry_;
  dsp::FileAudioSourceRegistry              file_audio_source_registry_;
  TrackProcessor::ProcessorBaseDependencies dependencies_{
    .port_registry_ = port_registry_,
    .param_registry_ = param_registry_
  };
  sample_rate_t sample_rate_{ 44100 };
  nframes_t     block_length_{ 256 };
};

TEST_F (TrackProcessorTest, AudioTrackInitialState)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Test Audio Track"; }, [] { return true; }, false, false,
    true, dependencies_);

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
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"Test MIDI Track"; }, [] { return true; }, true, false, false,
    dependencies_);

  EXPECT_FALSE (processor.is_audio ());
  EXPECT_TRUE (processor.is_midi ());
  EXPECT_EQ (processor.get_node_name (), u8"Test MIDI Track Processor");

  // Check MIDI ports
  EXPECT_EQ (processor.get_input_ports ().size (), 2);  // MIDI in + Piano roll
  EXPECT_EQ (processor.get_output_ports ().size (), 1); // MIDI out

  // Check no audio-specific parameters
  EXPECT_EQ (processor.get_parameters ().size (), 0);
}

TEST_F (TrackProcessorTest, MidiTrackWithCCInitialState)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"Test MIDI Track with CC"; }, [] { return true; }, true, true,
    false, dependencies_);

  EXPECT_FALSE (processor.is_audio ());
  EXPECT_TRUE (processor.is_midi ());

  // Check MIDI ports
  EXPECT_EQ (processor.get_input_ports ().size (), 2);  // MIDI in + Piano roll
  EXPECT_EQ (processor.get_output_ports ().size (), 1); // MIDI out

  // Check MIDI CC parameters (16 channels * 128 CCs + 16 pitch bend + 16 poly
  // pressure + 16 channel pressure)
  EXPECT_EQ (processor.get_parameters ().size (), (16 * 128) + 16 + 16 + 16);
}

TEST_F (TrackProcessorTest, DisabledTrackProcessing)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Disabled Track"; }, [] { return false; }, false, false, true,
    dependencies_);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Fill inputs with test data
  auto &stereo_in = processor.get_stereo_in_port ();
  for (int i = 0; i < stereo_in.buffers ()->getNumSamples (); ++i)
    {
      stereo_in.buffers ()->setSample (0, i, 0.5f);
      stereo_in.buffers ()->setSample (1, i, 0.5f);
    }

  // Should not process when disabled
  processor.process_block (time_nfo);

  // Verify output buffers are cleared (no processing occurred)
  auto &stereo_out = processor.get_stereo_out_port ();
  for (size_t i = 0; i < time_nfo.nframes_; ++i)
    {
      EXPECT_EQ (stereo_out.buffers ()->getSample (0, i), 0.0f);
      EXPECT_EQ (stereo_out.buffers ()->getSample (1, i), 0.0f);
    }
}

TEST_F (TrackProcessorTest, AudioTrackProcessingWithMonitoringOff)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Audio Track"; }, [] { return true; }, false, false, true,
    dependencies_);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Disable monitoring so that input ports are ignored
  processor.get_monitor_audio_param ().setBaseValue (0.f);

  // Fill input buffers with test data
  auto &stereo_in = processor.get_stereo_in_port ();
  for (size_t i = 0; i < time_nfo.nframes_; ++i)
    {
      stereo_in.buffers ()->setSample (0, i, 0.8f);
      stereo_in.buffers ()->setSample (1, i, 0.5f);
    }

  processor.process_block (time_nfo);

  // Verify output is empty
  auto &stereo_out = processor.get_stereo_out_port ();
  for (size_t i = 0; i < time_nfo.nframes_; ++i)
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
    *transport_, *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Audio Track"; }, [] { return true; }, false, false, true,
    dependencies_);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Set input gain to 2.0
  auto &input_gain_param = processor.get_input_gain_param ();
  input_gain_param.setBaseValue (input_gain_param.range ().convertTo0To1 (2.0f));

  // Enable monitoring so that input ports are used on this audio track
  processor.get_monitor_audio_param ().setBaseValue (1.f);

  // Enable stereo mode
  processor.get_mono_param ().setBaseValue (0.f);

  // Fill input buffers with test data
  auto &stereo_in = processor.get_stereo_in_port ();
  for (size_t i = 0; i < time_nfo.nframes_; ++i)
    {
      stereo_in.buffers ()->setSample (0, i, 1.0f);
      stereo_in.buffers ()->setSample (1, i, 0.5f);
    }

  processor.process_block (time_nfo);

  // Verify output is scaled by input gain
  auto &stereo_out = processor.get_stereo_out_port ();
  for (size_t i = 0; i < time_nfo.nframes_; ++i)
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
    *transport_, *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Audio Track Mono"; }, [] { return true; }, false, false,
    true, dependencies_);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Enable monitoring so that input ports are used on this audio track
  processor.get_monitor_audio_param ().setBaseValue (1.f);

  // Enable mono mode
  auto &mono_param = processor.get_mono_param ();
  mono_param.setBaseValue (1.f);

  // Fill input buffers with different data
  auto &stereo_in = processor.get_stereo_in_port ();
  for (size_t i = 0; i < time_nfo.nframes_; ++i)
    {
      stereo_in.buffers ()->setSample (0, i, 0.8f);
      stereo_in.buffers ()->setSample (1, i, 0.5f);
    }

  processor.process_block (time_nfo);

  // Verify both outputs use left channel data
  auto &stereo_out = processor.get_stereo_out_port ();
  for (size_t i = 0; i < time_nfo.nframes_; ++i)
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
      const dsp::ITransport &itransport, const EngineProcessTimeInfo &time_nfo,
      dsp::MidiEventVector *                        midi_events,
      std::optional<TrackProcessor::StereoPortPair> stereo_ports) {
      for (size_t i = 0; i < 256; ++i)
        {
          stereo_ports->first[i] = 0.8f;
          stereo_ports->second[i] = 0.8f;
        }
    };
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Audio Track Output"; }, [] { return true; }, false, false,
    true, dependencies_, fill_events_cb);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Set output gain to 0.5
  auto &output_gain_param = processor.get_output_gain_param ();
  output_gain_param.setBaseValue (
    output_gain_param.range ().convertTo0To1 (0.5f));

  // Enable rolling so that events are taken into account
  EXPECT_CALL (*transport_, get_play_state ())
    .WillOnce (::testing::Return (dsp::ITransport::PlayState::Rolling));

  processor.process_block (time_nfo);

  // Verify output is scaled by output gain
  auto &stereo_out = processor.get_stereo_out_port ();
  for (size_t i = 0; i < time_nfo.nframes_; ++i)
    {
      EXPECT_NEAR (
        stereo_out.buffers ()->getSample (0, i), 0.4f,
        AUDIO_BUFFER_DIFF_TOLERANCE);
      EXPECT_NEAR (
        stereo_out.buffers ()->getSample (1, i), 0.4f,
        AUDIO_BUFFER_DIFF_TOLERANCE);
    }
}

TEST_F (TrackProcessorTest, MidiTrackProcessingWithTransform)
{
  TrackProcessor::TransformMidiInputsFunc transform_func =
    [] (dsp::MidiEventVector &events) {
      // Transform all note events to channel 2 and transpose up by 2 semitones
      for (auto &event : events)
        {
          if (
            (event.raw_buffer_[0] & 0xF0) == 0x90
            || (event.raw_buffer_[0] & 0xF0) == 0x80)
            {
              // Change channel to 2 (0x91 for note on, 0x81 for note off)
              event.raw_buffer_[0] = (event.raw_buffer_[0] & 0xF0) | 0x01;
              // Transpose note up by 2 semitones
              event.raw_buffer_[1] = std::min (127, event.raw_buffer_[1] + 2);
            }
        }
    };

  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"MIDI Transform"; }, [] { return true; }, true, false, false,
    dependencies_, std::nullopt, transform_func);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Add some MIDI events to input
  auto &midi_in = processor.get_midi_in_port ();
  midi_in.midi_events_.active_events_.add_note_on (1, 60, 100, 0);
  midi_in.midi_events_.active_events_.add_note_on (1, 64, 90, 10);
  midi_in.midi_events_.active_events_.add_note_off (1, 60, 20);

  processor.process_block (time_nfo);

  // Verify events were transformed
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_EQ (midi_out.midi_events_.queued_events_.size (), 3);

  // Check that events were properly transformed
  const auto &events = midi_out.midi_events_.queued_events_;
  bool        found_transformed_note_on_1 = false;
  bool        found_transformed_note_on_2 = false;
  bool        found_transformed_note_off = false;

  for (const auto &event : events)
    {
      if (
        (event.raw_buffer_[0] & 0xF0) == 0x90
        && (event.raw_buffer_[0] & 0x0F) == 0x01)
        {
          // Note on, channel 2
          if (event.raw_buffer_[1] == 62 && event.raw_buffer_[2] == 100)
            {
              found_transformed_note_on_1 = true;
            }
          else if (event.raw_buffer_[1] == 66 && event.raw_buffer_[2] == 90)
            {
              found_transformed_note_on_2 = true;
            }
        }
      else if (
        (event.raw_buffer_[0] & 0xF0) == 0x80
        && (event.raw_buffer_[0] & 0x0F) == 0x01)
        {
          // Note off, channel 2
          if (event.raw_buffer_[1] == 62)
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
    *transport_, *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Test Audio Track"; }, [] { return true; }, false, false,
    true, dependencies_);

  // Set some parameter values
  audio_processor.get_input_gain_param ().setBaseValue (
    audio_processor.get_input_gain_param ().range ().convertTo0To1 (1.5f));
  audio_processor.get_output_gain_param ().setBaseValue (
    audio_processor.get_output_gain_param ().range ().convertTo0To1 (0.8f));
  audio_processor.get_mono_param ().setBaseValue (1.0f); // Enable mono
  audio_processor.get_monitor_audio_param ().setBaseValue (
    0.0f); // Disable monitoring

  // Serialize to JSON
  nlohmann::json j;
  to_json (j, audio_processor);

  // Deserialize back to a new processor
  TrackProcessor deserialized_processor (
    *transport_, *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Deserialized Audio Track"; }, [] { return true; }, false,
    false, true, dependencies_);
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
  EXPECT_TRUE (deserialized_processor.get_mono_param ().range ().is_toggled (
    deserialized_processor.get_mono_param ().baseValue ()));
  EXPECT_FALSE (
    deserialized_processor.get_monitor_audio_param ().range ().is_toggled (
      deserialized_processor.get_monitor_audio_param ().baseValue ()));
}

TEST_F (TrackProcessorTest, JsonSerializationMidiTrack)
{
  // Create a MIDI track processor with CC support
  TrackProcessor midi_processor (
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"Test MIDI Track"; }, [] { return true; }, true, true, false,
    dependencies_);

  // Set some MIDI CC parameter values
  midi_processor.get_midi_cc_param (0, 1).setBaseValue (
    0.75f); // CC 1 on channel 1
  midi_processor.get_midi_cc_param (1, 64).setBaseValue (
    0.5f); // CC 64 (sustain) on channel 2
  midi_processor.get_midi_cc_param (15, 7).setBaseValue (
    0.9f); // CC 7 (volume) on channel 16

  // Serialize to JSON
  nlohmann::json j;
  to_json (j, midi_processor);

  // Deserialize back to a new processor
  TrackProcessor deserialized_processor (
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"Deserialized MIDI Track"; }, [] { return true; }, true, true,
    false, dependencies_);
  from_json (j, deserialized_processor);

  // Verify deserialized processor has same state
  EXPECT_EQ (deserialized_processor.is_audio (), midi_processor.is_audio ());
  EXPECT_EQ (deserialized_processor.is_midi (), midi_processor.is_midi ());

  // Verify MIDI CC parameter values are preserved
  EXPECT_NEAR (
    deserialized_processor.get_midi_cc_param (0, 1).baseValue (), 0.75f,
    AUDIO_BUFFER_DIFF_TOLERANCE);
  EXPECT_NEAR (
    deserialized_processor.get_midi_cc_param (1, 64).baseValue (), 0.5f,
    AUDIO_BUFFER_DIFF_TOLERANCE);
  EXPECT_NEAR (
    deserialized_processor.get_midi_cc_param (15, 7).baseValue (), 0.9f,
    AUDIO_BUFFER_DIFF_TOLERANCE);
}

TEST_F (TrackProcessorTest, MidiTrackProcessingWithAppendFunc)
{
  TrackProcessor::AppendMidiInputsToOutputsFunc append_func =
    [] (
      dsp::MidiEventVector &out_events, const dsp::MidiEventVector &in_events,
      const EngineProcessTimeInfo &time_nfo) {
      // Append all input events to output, but transpose them up by 5 semitones
      for (const auto &event : in_events)
        {
          dsp::MidiEvent new_event = event;
          if (
            (new_event.raw_buffer_[0] & 0xF0) == 0x90
            || (new_event.raw_buffer_[0] & 0xF0) == 0x80)
            {
              // Transpose note up by 5 semitones
              new_event.raw_buffer_[1] =
                std::min (127, new_event.raw_buffer_[1] + 5);
            }
          out_events.push_back (new_event);
        }
    };

  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"MIDI Append"; }, [] { return true; }, true, false, false,
    dependencies_, std::nullopt, std::nullopt, append_func);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Add some MIDI events to input
  auto &midi_in = processor.get_midi_in_port ();
  midi_in.midi_events_.active_events_.add_note_on (1, 60, 100, 0);
  midi_in.midi_events_.active_events_.add_note_on (1, 64, 90, 10);
  midi_in.midi_events_.active_events_.add_note_off (1, 60, 20);

  processor.process_block (time_nfo);

  // Verify events were appended and transformed
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_EQ (midi_out.midi_events_.queued_events_.size (), 3);

  // Check that events were properly appended and transposed
  const auto &events = midi_out.midi_events_.queued_events_;
  bool        found_transposed_note_on_1 = false;
  bool        found_transposed_note_on_2 = false;
  bool        found_transposed_note_off = false;

  for (const auto &event : events)
    {
      if (
        (event.raw_buffer_[0] & 0xF0) == 0x90
        && (event.raw_buffer_[0] & 0x0F) == 0x00)
        {
          // Note on, channel 1
          if (event.raw_buffer_[1] == 65 && event.raw_buffer_[2] == 100)
            {
              found_transposed_note_on_1 = true;
            }
          else if (event.raw_buffer_[1] == 69 && event.raw_buffer_[2] == 90)
            {
              found_transposed_note_on_2 = true;
            }
        }
      else if (
        (event.raw_buffer_[0] & 0xF0) == 0x80
        && (event.raw_buffer_[0] & 0x0F) == 0x00)
        {
          // Note off, channel 1
          if (event.raw_buffer_[1] == 65)
            {
              found_transposed_note_off = true;
            }
        }
    }

  EXPECT_TRUE (found_transposed_note_on_1);
  EXPECT_TRUE (found_transposed_note_on_2);
  EXPECT_TRUE (found_transposed_note_off);
}

TEST_F (TrackProcessorTest, FillEventsFromAudioRegion)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Audio Fill"; }, [] { return true; }, false, false, true,
    dependencies_);

  // Create audio region
  auto   region = create_region<arrangement::AudioRegion> (100, 200);
  auto * region_ptr = region.get_object_as<arrangement::AudioRegion> ();

  dsp::MidiEventVector midi_events;
  using PortBuffer = std::array<float, 256>;
  std::pair<PortBuffer, PortBuffer>             stereo_port_bufs;
  std::optional<TrackProcessor::StereoPortPair> stereo_ports = std::make_pair (
    std::span<float> (stereo_port_bufs.first),
    std::span<float> (stereo_port_bufs.second));

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 100,
    .g_start_frame_w_offset_ = 100,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Test fill_events_from_region_rt
  TrackProcessor::fill_events_from_region_rt (
    *transport_, time_nfo, &midi_events, stereo_ports, *region_ptr);

  // Verify actual data was filled
  EXPECT_EQ (midi_events.size (), 0); // Audio regions don't produce MIDI events

  // Verify audio data was filled with the test pattern (0.5 left, -0.5 right)
  // (skip builtin fades)
  for (
    size_t i = arrangement::AudioRegion::BUILTIN_FADE_FRAMES;
    i < 200 - arrangement::AudioRegion::BUILTIN_FADE_FRAMES; ++i)
    {
      EXPECT_NEAR (stereo_port_bufs.first[i], 0.5f, AUDIO_BUFFER_DIFF_TOLERANCE);
      EXPECT_NEAR (
        stereo_port_bufs.second[i], -0.5f, AUDIO_BUFFER_DIFF_TOLERANCE);
    }
}

TEST_F (TrackProcessorTest, FillEventsFromMidiRegion)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Midi, [] { return u8"MIDI Fill"; },
    [] { return true; }, true, false, false, dependencies_);

  // Create MIDI region with notes
  auto   region = create_region<arrangement::MidiRegion> (100, 200);
  auto * region_ptr = region.get_object_as<arrangement::MidiRegion> ();
  add_midi_note (region_ptr, 60, 80, 0, 100);
  add_midi_note (region_ptr, 64, 80, 50, 150);

  dsp::MidiEventVector                          midi_events;
  std::optional<TrackProcessor::StereoPortPair> stereo_ports = std::nullopt;

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 100,
    .g_start_frame_w_offset_ = 100,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Test fill_events_from_region_rt
  TrackProcessor::fill_events_from_region_rt (
    *transport_, time_nfo, &midi_events, stereo_ports, *region_ptr);

  // Should have events since region is active
  EXPECT_EQ (midi_events.size (), 4);

  // Verify actual MIDI data was produced
  bool found_note_60 = false;
  bool found_note_64 = false;

  for (const auto &event : midi_events)
    {
      if ((event.raw_buffer_[0] & 0xF0) == 0x90) // Note on
        {
          if (event.raw_buffer_[1] == 60 && event.raw_buffer_[2] == 80)
            {
              found_note_60 = true;
            }
          else if (event.raw_buffer_[1] == 64 && event.raw_buffer_[2] == 80)
            {
              found_note_64 = true;
            }
        }
    }

  EXPECT_TRUE (found_note_60) << "Expected note 60 with velocity 80 not found";
  EXPECT_TRUE (found_note_64) << "Expected note 64 with velocity 80 not found";

  // Verify no audio data was produced
  EXPECT_FALSE (stereo_ports.has_value ());
}

TEST_F (TrackProcessorTest, FillEventsFromMutedRegion)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Midi, [] { return u8"MIDI Muted"; },
    [] { return true; }, true, false, false, dependencies_);

  // Create muted MIDI region
  auto   region = create_region<arrangement::MidiRegion> (100, 200);
  auto * region_ptr = region.get_object_as<arrangement::MidiRegion> ();
  region_ptr->mute ()->setMuted (true);
  add_midi_note (region_ptr, 60, 80, 0, 100);

  dsp::MidiEventVector                          midi_events;
  std::optional<TrackProcessor::StereoPortPair> stereo_ports = std::nullopt;

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 100,
    .g_start_frame_w_offset_ = 100,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Test fill_events_from_region_rt with muted region
  TrackProcessor::fill_events_from_region_rt (
    *transport_, time_nfo, &midi_events, stereo_ports, *region_ptr);

  // Should have no events since region is muted
  EXPECT_EQ (midi_events.size (), 0);
}

TEST_F (TrackProcessorTest, FillEventsFromRegionOutsideRange)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"MIDI Outside Range"; }, [] { return true; }, true, false,
    false, dependencies_);

  // Create MIDI region outside processing range
  auto   region = create_region<arrangement::MidiRegion> (500, 200);
  auto * region_ptr = region.get_object_as<arrangement::MidiRegion> ();
  add_midi_note (region_ptr, 60, 80, 0, 100);

  dsp::MidiEventVector                          midi_events;
  std::optional<TrackProcessor::StereoPortPair> stereo_ports = std::nullopt;

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 100,
    .g_start_frame_w_offset_ = 100,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Test fill_events_from_region_rt with region outside range
  TrackProcessor::fill_events_from_region_rt (
    *transport_, time_nfo, &midi_events, stereo_ports, *region_ptr);

  // Should have no events since region is outside processing range
  EXPECT_EQ (midi_events.size (), 0);
}

TEST_F (TrackProcessorTest, FillEventsFromChordRegion)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Midi, [] { return u8"Chord Fill"; },
    [] { return true; }, true, false, false, dependencies_);

  // Create chord region
  auto   region = create_region<arrangement::ChordRegion> (100, 200);
  auto * region_ptr = region.get_object_as<arrangement::ChordRegion> ();

  dsp::MidiEventVector                          midi_events;
  std::optional<TrackProcessor::StereoPortPair> stereo_ports = std::nullopt;

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 100,
    .g_start_frame_w_offset_ = 100,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Test fill_events_from_region_rt
  TrackProcessor::fill_events_from_region_rt (
    *transport_, time_nfo, &midi_events, stereo_ports, *region_ptr);

  // Assert only all notes off event(s) since region end is inside the block
  EXPECT_GE (midi_events.size (), 0);
  EXPECT_TRUE (std::ranges::all_of (midi_events, [] (const auto &ev) {
    return utils::midi::midi_is_all_notes_off (ev.raw_buffer_);
  }));

  // Verify no audio data was produced
  EXPECT_FALSE (stereo_ports.has_value ());

  // For chord regions, events are provided by the implementation so the above
  // tests are enough.
}

TEST_F (TrackProcessorTest, RecordingCallback)
{
  bool                         recording_called = false;
  unsigned_frame_t             recorded_start_frame = 0;
  signed_frame_t               recorded_nframes = 0;
  const dsp::MidiEventVector * recorded_midi_events = nullptr;
  std::optional<TrackProcessor::ConstStereoPortPair> recorded_stereo_ports;

  TrackProcessor::HandleRecordingCallback recording_cb =
    [&recording_called, &recorded_start_frame, &recorded_nframes,
     &recorded_midi_events, &recorded_stereo_ports] (
      const EngineProcessTimeInfo                       &time_nfo,
      const dsp::MidiEventVector *                       midi_events,
      std::optional<TrackProcessor::ConstStereoPortPair> stereo_ports) {
      recording_called = true;
      recorded_start_frame = time_nfo.g_start_frame_;
      recorded_nframes = time_nfo.nframes_;
      recorded_midi_events = midi_events;
      recorded_stereo_ports = stereo_ports;
    };

  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Recording Track"; }, [] { return true; }, false, false, true,
    dependencies_, std::nullopt, std::nullopt, std::nullopt);
  processor.set_handle_recording_callback (recording_cb);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 44100,
    .g_start_frame_w_offset_ = 44100,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  processor.process_block (time_nfo);

  EXPECT_TRUE (recording_called);
  EXPECT_EQ (recorded_start_frame, 44100);
  EXPECT_EQ (recorded_nframes, 256);
}

TEST_F (TrackProcessorTest, GetFullDesignationForPort)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Test Track"; }, [] { return true; }, false, false, true,
    dependencies_);

  const auto &ports = processor.get_input_ports ();
  ASSERT_FALSE (ports.empty ());
  auto designation = processor.get_full_designation_for_port (
    *ports[0].get_object_as<dsp::AudioPort> ());
  EXPECT_FALSE (designation.empty ());
}

TEST_F (TrackProcessorTest, MidiTrackProcessingWithPianoRoll)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"MIDI Piano Roll"; }, [] { return true; }, true, false, false,
    dependencies_);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Add events to piano roll port
  auto &piano_roll = processor.get_piano_roll_port ();
  piano_roll.midi_events_.active_events_.add_note_on (1, 72, 100, 0);

  processor.process_block (time_nfo);

  // Verify events were processed
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_GT (midi_out.midi_events_.queued_events_.size (), 0);
}

TEST_F (TrackProcessorTest, AudioTrackProcessingWithRecording)
{
  bool                         recording_called = false;
  unsigned_frame_t             recorded_start_frame = 0;
  unsigned_frame_t             recorded_nframes = 0;
  const dsp::MidiEventVector * recorded_midi_events = nullptr;
  std::optional<TrackProcessor::ConstStereoPortPair> recorded_stereo_ports;

  TrackProcessor::HandleRecordingCallback recording_cb =
    [&recording_called, &recorded_start_frame, &recorded_nframes,
     &recorded_midi_events, &recorded_stereo_ports] (
      const EngineProcessTimeInfo                       &time_nfo,
      const dsp::MidiEventVector *                       midi_events,
      std::optional<TrackProcessor::ConstStereoPortPair> stereo_ports) {
      recording_called = true;
      recorded_start_frame = time_nfo.g_start_frame_;
      recorded_nframes = time_nfo.nframes_;
      recorded_midi_events = midi_events;
      recorded_stereo_ports = stereo_ports;
    };

  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Audio Recording"; }, [] { return true; }, false, false, true,
    dependencies_, std::nullopt, std::nullopt, std::nullopt);
  processor.set_handle_recording_callback (recording_cb);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 88200,
    .g_start_frame_w_offset_ = 88200,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  processor.prepare_for_processing (sample_rate_, 512);

  // Fill input buffers with test pattern
  auto &stereo_in = processor.get_stereo_in_port ();
  for (size_t i = 0; i < time_nfo.nframes_; ++i)
    {
      stereo_in.buffers ()->setSample (
        0, i, 0.8f + 0.2f * std::sin (static_cast<float> (i) * 0.1f));
      stereo_in.buffers ()->setSample (
        1, i, 0.6f + 0.3f * std::cos (static_cast<float> (i) * 0.1f));
    }

  processor.process_block (time_nfo);

  EXPECT_TRUE (recording_called);
  EXPECT_EQ (recorded_start_frame, 88200);
  EXPECT_EQ (recorded_nframes, 512);
  EXPECT_EQ (recorded_midi_events, nullptr); // No MIDI events for audio track
  EXPECT_TRUE (recorded_stereo_ports.has_value ());
  EXPECT_EQ (recorded_stereo_ports->first.size (), 512);
  EXPECT_EQ (recorded_stereo_ports->second.size (), 512);
}

TEST_F (TrackProcessorTest, MidiTrackProcessingWithCCParameters)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"MIDI CC Track"; }, [] { return true; }, true, true, false,
    dependencies_);

  // Test CC parameter access
  auto &cc_param =
    processor.get_midi_cc_param (0, 64); // Channel 1, CC 64 (sustain)

  // Set CC value
  cc_param.setBaseValue (cc_param.range ().convertTo0To1 (127.0f));
  EXPECT_FLOAT_EQ (
    cc_param.range ().convertFrom0To1 (cc_param.baseValue ()), 127.0f);

  // TODO: test actual processing
}

TEST_F (TrackProcessorTest, AudioTrackProcessingHasZeroLatency)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Zero Latency Track"; }, [] { return true; }, false, false,
    false, dependencies_);

  EXPECT_EQ (processor.get_single_playback_latency (), 0);
}

TEST_F (TrackProcessorTest, MidiTrackProcessingWithEmptyInput)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"Empty MIDI Track"; }, [] { return true; }, true, false,
    false, dependencies_);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Process with no input events
  processor.process_block (time_nfo);

  // Should complete without errors
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_EQ (midi_out.midi_events_.queued_events_.size (), 0);
}

TEST_F (TrackProcessorTest, AudioBusTrackProcessingWithLargeBuffer)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Large Buffer Track"; }, [] { return true; }, false, false,
    false, dependencies_);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 1024
  };

  processor.prepare_for_processing (sample_rate_, 1024);

  auto &stereo_in = processor.get_stereo_in_port ();
  for (size_t i = 0; i < time_nfo.nframes_; ++i)
    {
      stereo_in.buffers ()->setSample (0, i, 0.5f);
      stereo_in.buffers ()->setSample (1, i, 0.3f);
    }

  processor.process_block (time_nfo);

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
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"Multi MIDI Track"; }, [] { return true; }, true, false,
    false, dependencies_);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Add multiple MIDI events
  auto &midi_in = processor.get_midi_in_port ();
  midi_in.midi_events_.active_events_.add_note_on (1, 60, 100, 0);
  midi_in.midi_events_.active_events_.add_note_on (1, 64, 90, 10);
  midi_in.midi_events_.active_events_.add_note_on (1, 67, 80, 20);

  processor.process_block (time_nfo);

  // Verify events were processed
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_GE (midi_out.midi_events_.queued_events_.size (), 3);
}

TEST_F (TrackProcessorTest, AudioTrackParameterRangeValidation)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Audio,
    [] { return u8"Param Range Track"; }, [] { return true; }, false, false,
    true, dependencies_);

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
  TrackProcessor::TransformMidiInputsFunc transform_func =
    [] (dsp::MidiEventVector &events) {
      // Transform: change all notes to channel 3 and reduce velocity by 20
      for (auto &event : events)
        {
          if ((event.raw_buffer_[0] & 0xF0) == 0x90)
            {
              // Note on, change channel to 3 and reduce velocity
              event.raw_buffer_[0] = (event.raw_buffer_[0] & 0xF0) | 0x02;
              event.raw_buffer_[2] = std::max (0, event.raw_buffer_[2] - 20);
            }
          else if ((event.raw_buffer_[0] & 0xF0) == 0x80)
            {
              // Note off, change channel to 3
              event.raw_buffer_[0] = (event.raw_buffer_[0] & 0xF0) | 0x02;
            }
        }
    };

  TrackProcessor::AppendMidiInputsToOutputsFunc append_func =
    [] (
      dsp::MidiEventVector &out_events, const dsp::MidiEventVector &in_events,
      const EngineProcessTimeInfo &time_nfo) {
      // Append all transformed events to output
      for (const auto &event : in_events)
        {
          out_events.push_back (event);
        }
    };

  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"MIDI Transform Append"; }, [] { return true; }, true, false,
    false, dependencies_, std::nullopt, transform_func, append_func);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  auto &midi_in = processor.get_midi_in_port ();
  midi_in.midi_events_.active_events_.add_note_on (1, 60, 100, 0);
  midi_in.midi_events_.active_events_.add_note_on (1, 64, 80, 10);
  midi_in.midi_events_.active_events_.add_note_off (1, 60, 20);

  processor.process_block (time_nfo);

  // Verify events were both transformed and appended
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_EQ (midi_out.midi_events_.queued_events_.size (), 3);

  // Check that events were properly transformed and appended
  const auto &events = midi_out.midi_events_.queued_events_;
  bool        found_transformed_note_on_1 = false;
  bool        found_transformed_note_on_2 = false;
  bool        found_transformed_note_off = false;

  for (const auto &event : events)
    {
      if (
        (event.raw_buffer_[0] & 0xF0) == 0x90
        && (event.raw_buffer_[0] & 0x0F) == 0x02)
        {
          // Note on, channel 3
          if (event.raw_buffer_[1] == 60 && event.raw_buffer_[2] == 80)
            {
              found_transformed_note_on_1 = true;
            }
          else if (event.raw_buffer_[1] == 64 && event.raw_buffer_[2] == 60)
            {
              found_transformed_note_on_2 = true;
            }
        }
      else if (
        (event.raw_buffer_[0] & 0xF0) == 0x80
        && (event.raw_buffer_[0] & 0x0F) == 0x02)
        {
          // Note off, channel 3
          if (event.raw_buffer_[1] == 60)
            {
              found_transformed_note_off = true;
            }
        }
    }

  EXPECT_TRUE (found_transformed_note_on_1);
  EXPECT_TRUE (found_transformed_note_on_2);
  EXPECT_TRUE (found_transformed_note_off);
}

TEST_F (TrackProcessorTest, FillEventsFromRegionWithOffset)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"Offset Region"; }, [] { return true; }, true, false, false,
    dependencies_);

  // Create MIDI region with offset
  auto   region = create_region<arrangement::MidiRegion> (200, 100);
  auto * region_ptr = region.get_object_as<arrangement::MidiRegion> ();
  add_midi_note (region_ptr, 60, 80, 0, 50);

  dsp::MidiEventVector                          midi_events;
  std::optional<TrackProcessor::StereoPortPair> stereo_ports = std::nullopt;

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 150,
    .g_start_frame_w_offset_ = 150,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  TrackProcessor::fill_events_from_region_rt (
    *transport_, time_nfo, &midi_events, stereo_ports, *region_ptr);

  // Should have events since region overlaps processing range
  EXPECT_GT (midi_events.size (), 0);
}

TEST_F (TrackProcessorTest, MidiTrackProcessingWithCustomMidiEventProvider)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"MIDI Custom Provider"; }, [] { return true; }, true, false,
    false, dependencies_);

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

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Enable rolling so that events are taken into account
  EXPECT_CALL (*transport_, get_play_state ())
    .WillOnce (::testing::Return (dsp::ITransport::PlayState::Rolling));

  processor.process_block (time_nfo);

  // Verify events were processed and output
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_GT (midi_out.midi_events_.queued_events_.size (), 0);

  // Check that the expected events were produced
  const auto &events = midi_out.midi_events_.queued_events_;
  bool        found_note_60_on = false;
  bool        found_note_60_off = false;
  bool        found_note_64_on = false;
  bool        found_note_64_off = false;

  for (const auto &event : events)
    {
      if ((event.raw_buffer_[0] & 0xF0) == 0x90) // Note on
        {
          if (event.raw_buffer_[1] == 60 && event.raw_buffer_[2] == 100)
            {
              found_note_60_on = true;
            }
          else if (event.raw_buffer_[1] == 64 && event.raw_buffer_[2] == 80)
            {
              found_note_64_on = true;
            }
        }
      else if ((event.raw_buffer_[0] & 0xF0) == 0x80) // Note off
        {
          if (event.raw_buffer_[1] == 60)
            {
              found_note_60_off = true;
            }
          else if (event.raw_buffer_[1] == 64)
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
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"MIDI Empty Provider"; }, [] { return true; }, true, false,
    false, dependencies_);

  // Set empty MIDI sequence
  juce::MidiMessageSequence empty_sequence;
  processor.set_custom_midi_event_provider (
    create_midi_event_provider_from_sequence (empty_sequence));

  // Activate the custom event provider
  processor.set_midi_providers_active (
    TrackProcessor::ActiveMidiEventProviders::Custom, true);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Enable rolling so that events would be processed if they existed
  EXPECT_CALL (*transport_, get_play_state ())
    .WillOnce (::testing::Return (dsp::ITransport::PlayState::Rolling));

  processor.process_block (time_nfo);

  // Verify no events were produced from empty sequence
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_EQ (midi_out.midi_events_.queued_events_.size (), 0);
}

TEST_F (
  TrackProcessorTest,
  MidiTrackProcessingWithMultipleCustomMidiEventProviders)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"MIDI Multiple Provider"; }, [] { return true; }, true, false,
    false, dependencies_);

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

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  processor.prepare_for_processing (sample_rate_, 512);

  // Enable rolling
  EXPECT_CALL (*transport_, get_play_state ())
    .WillOnce (::testing::Return (dsp::ITransport::PlayState::Rolling));

  processor.process_block (time_nfo);

  // Verify multiple events were processed
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_GE (midi_out.midi_events_.queued_events_.size (), 6);
}

TEST_F (
  TrackProcessorTest,
  MidiTrackProcessingWithOverlappingCustomMidiEventProviders)
{
  TrackProcessor processor (
    *transport_, *tempo_map_, dsp::PortType::Midi,
    [] { return u8"MIDI Overlapping Provider"; }, [] { return true; }, true,
    false, false, dependencies_);

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

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  processor.prepare_for_processing (sample_rate_, block_length_);

  // Enable rolling
  EXPECT_CALL (*transport_, get_play_state ())
    .WillOnce (::testing::Return (dsp::ITransport::PlayState::Rolling));

  processor.process_block (time_nfo);

  // Verify overlapping events were processed correctly
  auto &midi_out = processor.get_midi_out_port ();
  EXPECT_GT (midi_out.midi_events_.queued_events_.size (), 0);

  // Count note on/off events for note 60
  int note_60_on_count = 0;
  int note_60_off_count = 0;

  for (const auto &event : midi_out.midi_events_.queued_events_)
    {
      if ((event.raw_buffer_[0] & 0xF0) == 0x90 && event.raw_buffer_[1] == 60)
        {
          note_60_on_count++;
        }
      else if (
        (event.raw_buffer_[0] & 0xF0) == 0x80 && event.raw_buffer_[1] == 60)
        {
          note_60_off_count++;
        }
    }

  // Should have multiple note on/off events for the same note
  EXPECT_GE (note_60_on_count, 2);
  EXPECT_GE (note_60_off_count, 2);
}

} // namespace zrythm::structure::tracks
