// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/fader.h"
#include "dsp/parameter.h"
#include "dsp/port.h"
#include "dsp/processor_base.h"
#include "plugins/internal_plugin_base.h"
#include "plugins/plugin.h"
#include "plugins/plugin_group.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/channel_send.h"

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
    port_registry_ = std::make_unique<dsp::PortRegistry> ();
    param_registry_ =
      std::make_unique<dsp::ProcessorParameterRegistry> (*port_registry_);
    plugin_registry_ = std::make_unique<plugins::PluginRegistry> ();

    name_provider_ = [] { return u8"Test Channel"; };
    should_be_muted_cb_ = [] (bool solo_status) { return false; };

    // Set up mock transport
    mock_transport_ = std::make_unique<dsp::graph_test::MockTransport> ();
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
      *plugin_registry_,
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      dsp::PortType::Audio, name_provider_, false, should_be_muted_cb_);
  }

  std::unique_ptr<Channel> createMidiChannel ()
  {
    return std::make_unique<Channel> (
      *plugin_registry_,
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      dsp::PortType::Midi, name_provider_, false, should_be_muted_cb_);
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

    auto ref = plugin_registry_->create_object<plugins::InternalPluginBase> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      [] { return fs::path ("/tmp"); }, nullptr);
    auto * pl = ref.get_object_as<plugins::InternalPluginBase> ();
    pl->set_configuration (config);

    return ref;
  }

  std::unique_ptr<dsp::PortRegistry>               port_registry_;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry_;
  std::unique_ptr<plugins::PluginRegistry>         plugin_registry_;
  Channel::NameProvider                            name_provider_;
  dsp::Fader::ShouldBeMutedCallback                should_be_muted_cb_;
  units::sample_rate_t sample_rate_{ units::sample_rate (48000) };
  nframes_t            max_block_length_{ 1024 };
  std::unique_ptr<dsp::graph_test::MockTransport> mock_transport_;

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

TEST_F (ChannelTest, HardLimitFaderOutput)
{
  // Test channel with hard limiting enabled
  auto hard_limit_channel = std::make_unique<Channel> (
    *plugin_registry_,
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    dsp::PortType::Audio, name_provider_, true, should_be_muted_cb_);

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
    *plugin_registry_,
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    dsp::PortType::Audio, custom_name_provider, false, should_be_muted_cb_);

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
    *plugin_registry_,
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    dsp::PortType::Audio, name_provider_, false, custom_should_be_muted);

  EXPECT_FALSE (callback_called); // Not called during construction
  channel->fader ()->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);
  channel->fader ()->process_block (
    { .g_start_frame_ = 0,
      .g_start_frame_w_offset_ = 0,
      .local_offset_ = 0,
      .nframes_ = max_block_length_ },
    *mock_transport_);
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
    std::get<plugins::InternalPluginBase *> (mock_plugin_ref.get_object ()));

  // Test removing instrument
  midi_channel_->instruments ()->remove_plugin (mock_plugin_ref.id ());
  EXPECT_EQ (midi_channel_->instruments ()->rowCount (), 0);
}
} // namespace zrythm::structure::tracks
