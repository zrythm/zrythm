// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/fader.h"
#include "dsp/parameter.h"
#include "dsp/port.h"
#include "dsp/processor_base.h"
#include "plugins/faust/faust_plugin.h"
#include "plugins/plugin.h"
#include "plugins/plugin_group.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/channel_send.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <QObject>

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{

class ChannelTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();

    name_provider_ = [] { return u8"Test Channel"; };
    should_be_muted_cb_ = [] (bool solo_status) { return false; };

    // Set up mock transport
    mock_transport_ = std::make_unique<dsp::graph_test::MockTransport> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (sample_rate_);
  }

  void TearDown () override
  {
    if (audio_channel_)
      {
        audio_channel_->get_fader ().release_resources ();
      }
    if (midi_channel_)
      {
        midi_channel_->get_fader ().release_resources ();
      }
  }

  std::unique_ptr<Channel> createAudioChannel ()
  {
    return std::make_unique<Channel> (
      *registry_, dsp::PortType::Audio, dsp::PortType::Audio, name_provider_,
      false, should_be_muted_cb_);
  }

  std::unique_ptr<Channel> createMidiChannel ()
  {
    return std::make_unique<Channel> (
      *registry_, dsp::PortType::Midi, dsp::PortType::Midi, name_provider_,
      false, should_be_muted_cb_);
  }

  std::unique_ptr<Channel> createInstrumentChannel ()
  {
    return std::make_unique<Channel> (
      *registry_, dsp::PortType::Midi, dsp::PortType::Audio, name_provider_,
      false, should_be_muted_cb_);
  }

  plugins::PluginUuidReference createMockPlugin ()
  {
    // Create a plugin descriptor
    auto descriptor = std::make_unique<plugins::PluginDescriptor> ();
    descriptor->name_ = u8"Test Plugin";
    descriptor->protocol_ = plugins::Protocol::ProtocolType::Internal;

    // Create configuration
    plugins::PluginConfiguration config;
    config.descr_ = std::move (descriptor);

    auto ref = utils::create_object<plugins::FaustPlugin> (
      *registry_, *registry_, nullptr);
    auto * pl = ref.get_object_as<plugins::FaustPlugin> ();
    pl->set_configuration (config);

    return ref;
  }

  std::unique_ptr<utils::ObjectRegistry> registry_;
  Channel::NameProvider                  name_provider_;
  dsp::Fader::ShouldBeMutedCallback      should_be_muted_cb_;
  units::sample_rate_t sample_rate_{ units::sample_rate (48000) };
  units::sample_u32_t  max_block_length_{ units::samples (1024) };
  std::unique_ptr<dsp::graph_test::MockTransport> mock_transport_;
  std::unique_ptr<dsp::TempoMap>                  tempo_map_;

  std::unique_ptr<Channel> audio_channel_;
  std::unique_ptr<Channel> midi_channel_;
};

TEST_F (ChannelTest, ConstructionAndBasicProperties)
{
  // Test audio channel construction
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);
  EXPECT_EQ (audio_channel_->is_audio (), true);
  EXPECT_EQ (audio_channel_->is_midi (), false);
  EXPECT_NE (audio_channel_->fader (), nullptr);

  // Test MIDI channel construction
  midi_channel_ = createMidiChannel ();
  ASSERT_NE (midi_channel_, nullptr);
  EXPECT_EQ (midi_channel_->is_audio (), false);
  EXPECT_EQ (midi_channel_->is_midi (), true);
  EXPECT_NE (midi_channel_->fader (), nullptr);
}

TEST_F (ChannelTest, QmlInterfaceProperties)
{
  audio_channel_ = createAudioChannel ();
  midi_channel_ = createMidiChannel ();

  // Test audio channel QML properties
  EXPECT_NE (audio_channel_->fader (), nullptr);
  EXPECT_NE (audio_channel_->audioOutPort (), nullptr);
  EXPECT_EQ (audio_channel_->getMidiOut (), nullptr);
  EXPECT_NE (audio_channel_->inserts (), nullptr);
  EXPECT_NE (audio_channel_->midiFx (), nullptr);

  // Test MIDI channel QML properties
  EXPECT_NE (midi_channel_->fader (), nullptr);
  EXPECT_EQ (midi_channel_->audioOutPort (), nullptr);
  EXPECT_NE (midi_channel_->getMidiOut (), nullptr);
  EXPECT_NE (midi_channel_->inserts (), nullptr);
  EXPECT_NE (midi_channel_->midiFx (), nullptr);
}

TEST_F (ChannelTest, PluginListsInitialization)
{
  audio_channel_ = createAudioChannel ();
  midi_channel_ = createMidiChannel ();

  // Test that plugin lists are properly initialized
  EXPECT_NE (audio_channel_->inserts (), nullptr);
  EXPECT_NE (audio_channel_->midiFx (), nullptr);
  EXPECT_EQ (audio_channel_->inserts ()->rowCount (), 0);
  EXPECT_EQ (audio_channel_->midiFx ()->rowCount (), 0);

  EXPECT_NE (midi_channel_->inserts (), nullptr);
  EXPECT_NE (midi_channel_->midiFx (), nullptr);
  EXPECT_EQ (midi_channel_->inserts ()->rowCount (), 0);
  EXPECT_EQ (midi_channel_->midiFx ()->rowCount (), 0);
}

TEST_F (ChannelTest, ChannelSendsInitialization)
{
  audio_channel_ = createAudioChannel ();
  midi_channel_ = createMidiChannel ();

  // Test that sends are properly initialized
  EXPECT_EQ (audio_channel_->pre_fader_sends ().size (), 1);
  EXPECT_EQ (audio_channel_->post_fader_sends ().size (), 1);
  EXPECT_EQ (midi_channel_->pre_fader_sends ().size (), 1);
  EXPECT_EQ (midi_channel_->post_fader_sends ().size (), 1);

  // Test send properties
  auto &audio_pre_send = audio_channel_->pre_fader_sends ().front ();
  auto &audio_post_send = audio_channel_->post_fader_sends ().front ();
  EXPECT_EQ (audio_pre_send->is_prefader (), true);
  EXPECT_EQ (audio_post_send->is_prefader (), false);
}

TEST_F (ChannelTest, FaderIntegration)
{
  audio_channel_ = createAudioChannel ();
  midi_channel_ = createMidiChannel ();

  // Test fader integration
  EXPECT_NE (audio_channel_->fader (), nullptr);
  EXPECT_NE (midi_channel_->fader (), nullptr);

  // Test that faders have correct signal types
  EXPECT_EQ (audio_channel_->fader ()->is_audio (), true);
  EXPECT_EQ (audio_channel_->fader ()->is_midi (), false);
  EXPECT_EQ (midi_channel_->fader ()->is_audio (), false);
  EXPECT_EQ (midi_channel_->fader ()->is_midi (), true);
}

TEST_F (ChannelTest, PassthroughProcessors)
{
  audio_channel_ = createAudioChannel ();
  midi_channel_ = createMidiChannel ();

  // Test audio channel passthrough processors
  EXPECT_NO_THROW (audio_channel_->get_audio_pre_fader ());
  EXPECT_NO_THROW (audio_channel_->get_audio_post_fader ());

  // Test MIDI channel passthrough processors
  EXPECT_NO_THROW (midi_channel_->get_midi_pre_fader ());
  EXPECT_NO_THROW (midi_channel_->get_midi_post_fader ());
}

TEST_F (ChannelTest, SplitSignalTypeCapabilities)
{
  audio_channel_ = createAudioChannel ();
  midi_channel_ = createMidiChannel ();
  auto instrument_channel = createInstrumentChannel ();

  // Audio channels carry no MIDI processing stages.
  EXPECT_FALSE (audio_channel_->has_midi_input ());
  EXPECT_FALSE (audio_channel_->has_midi_pre_fader ());
  EXPECT_EQ (audio_channel_->postMidiFxPort (), nullptr);

  // MIDI channels keep their full MIDI chain.
  EXPECT_TRUE (midi_channel_->has_midi_input ());
  EXPECT_TRUE (midi_channel_->has_midi_pre_fader ());
  EXPECT_NE (midi_channel_->postMidiFxPort (), nullptr);

  // Instrument channels receive MIDI and output audio: their MIDI chain ends
  // at the event tap, and their fader stages process audio.
  EXPECT_TRUE (instrument_channel->has_midi_input ());
  EXPECT_FALSE (instrument_channel->has_midi_pre_fader ());
  EXPECT_TRUE (instrument_channel->has_midi_event_tap ());
  EXPECT_TRUE (instrument_channel->is_audio ());
  EXPECT_EQ (instrument_channel->getMidiOut (), nullptr);
  EXPECT_NE (instrument_channel->audioOutPort (), nullptr);
  EXPECT_NE (instrument_channel->postMidiFxPort (), nullptr);
}

// Each channel kind reports exactly its fixed processing stages: audio
// channels carry the audio pre/post-faders, MIDI channels the MIDI
// pre/post-faders, instrument channels the MIDI event tap plus the audio
// pre/post-faders; every kind carries the fader.
TEST_F (ChannelTest, BuiltinProcessorsMatchChannelSignalTypes)
{
  audio_channel_ = createAudioChannel ();
  midi_channel_ = createMidiChannel ();
  auto instrument_channel = createInstrumentChannel ();

  const auto contains = [] (const auto &processors, const auto * processor) {
    return std::ranges::find (processors, processor) != processors.end ();
  };

  const auto audio_processors = audio_channel_->get_builtin_processors ();
  EXPECT_EQ (audio_processors.size (), 3u);
  EXPECT_TRUE (contains (audio_processors, audio_channel_->fader ()));
  EXPECT_TRUE (
    contains (audio_processors, &audio_channel_->get_audio_pre_fader ()));
  EXPECT_TRUE (
    contains (audio_processors, &audio_channel_->get_audio_post_fader ()));

  const auto midi_processors = midi_channel_->get_builtin_processors ();
  EXPECT_EQ (midi_processors.size (), 3u);
  EXPECT_TRUE (contains (midi_processors, midi_channel_->fader ()));
  EXPECT_TRUE (
    contains (midi_processors, &midi_channel_->get_midi_pre_fader ()));
  EXPECT_TRUE (
    contains (midi_processors, &midi_channel_->get_midi_post_fader ()));

  const auto instrument_processors =
    instrument_channel->get_builtin_processors ();
  EXPECT_EQ (instrument_processors.size (), 4u);
  EXPECT_TRUE (contains (instrument_processors, instrument_channel->fader ()));
  EXPECT_TRUE (contains (
    instrument_processors, &instrument_channel->get_midi_event_tap ()));
  EXPECT_TRUE (contains (
    instrument_processors, &instrument_channel->get_audio_pre_fader ()));
  EXPECT_TRUE (contains (
    instrument_processors, &instrument_channel->get_audio_post_fader ()));
}

TEST_F (ChannelTest, PostMidiFxPortIsStablePostMidiFxOutput)
{
  midi_channel_ = createMidiChannel ();
  EXPECT_EQ (
    midi_channel_->postMidiFxPort (),
    &midi_channel_->get_midi_pre_fader ().get_midi_out_port (0));

  auto instrument_channel = createInstrumentChannel ();
  EXPECT_EQ (
    instrument_channel->postMidiFxPort (),
    &instrument_channel->get_midi_event_tap ().get_midi_out_port (0));

  // The port pointer stays identical for the channel's lifetime.
  const auto * first = instrument_channel->postMidiFxPort ();
  EXPECT_EQ (instrument_channel->postMidiFxPort (), first);
}

TEST_F (ChannelTest, HardLimitFaderOutput)
{
  // Test channel with hard limiting enabled
  auto hard_limit_channel = std::make_unique<Channel> (
    *registry_, dsp::PortType::Audio, dsp::PortType::Audio, name_provider_,
    true, should_be_muted_cb_);

  EXPECT_NE (hard_limit_channel->fader (), nullptr);
  EXPECT_TRUE (hard_limit_channel->fader ()->hard_limiting_enabled ());
}

TEST_F (ChannelTest, NameProviderFunctionality)
{
  bool name_provider_called = false;
  auto custom_name_provider = [&name_provider_called] {
    name_provider_called = true;
    return u8"Custom Channel Name";
  };

  auto channel = std::make_unique<Channel> (
    *registry_, dsp::PortType::Audio, dsp::PortType::Audio,
    custom_name_provider, false, should_be_muted_cb_);

  EXPECT_TRUE (name_provider_called);
  EXPECT_EQ (channel->fader ()->get_node_name (), u8"Custom Channel Name Fader");
}

TEST_F (ChannelTest, ShouldBeMutedCallback)
{
  bool callback_called = false;
  auto custom_should_be_muted = [&callback_called] (bool solo_status) {
    callback_called = true;
    return solo_status;
  };

  auto channel = std::make_unique<Channel> (
    *registry_, dsp::PortType::Audio, dsp::PortType::Audio, name_provider_,
    false, custom_should_be_muted);

  EXPECT_FALSE (callback_called); // Not called during construction
  channel->fader ()->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);
  channel->fader ()->process_block (
    { .transport_position_ = units::samples (0),
      .buffer_offset_ = units::samples (0),
      .nframes_ = max_block_length_ },
    *mock_transport_, *tempo_map_);
  EXPECT_TRUE (callback_called);
}

TEST_F (ChannelTest, InstrumentManagement)
{
  midi_channel_ = createMidiChannel ();

  // Test initial instrument state
  EXPECT_EQ (midi_channel_->instruments ()->rowCount (), 0);

  // Test setting instrument with mock plugin
  auto mock_plugin_ref = createMockPlugin ();
  midi_channel_->instruments ()->append_plugin (mock_plugin_ref);

  // Test that instrument is properly set
  EXPECT_EQ (midi_channel_->instruments ()->rowCount (), 1);

  // Test instrument Q_PROPERTY
  EXPECT_EQ (
    midi_channel_->instruments ()->element_at_idx (0).value<plugins::Plugin *> (),
    qobject_cast<plugins::FaustPlugin *> (mock_plugin_ref.get ()));

  // Test removing instrument
  midi_channel_->instruments ()->remove_plugin (mock_plugin_ref.id ());
  EXPECT_EQ (midi_channel_->instruments ()->rowCount (), 0);
}
} // namespace zrythm::structure::tracks
