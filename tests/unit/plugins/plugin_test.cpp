// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin.h"

#include <QSignalSpy>
#include <QTest>

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::plugins
{

class TestPlugin : public Plugin
{
public:
  TestPlugin (
    ProcessorBaseDependencies        dependencies,
    StateDirectoryParentPathProvider state_path_provider,
    QObject *                        parent = nullptr)
      : Plugin (dependencies, std::move (state_path_provider), parent)
  {
    auto bypass_ref = generate_default_bypass_param ();
    add_parameter (bypass_ref);
    bypass_id_ = bypass_ref.id ();
    auto gain_ref = generate_default_gain_param ();
    add_parameter (gain_ref);
    gain_id_ = gain_ref.id ();
  }

  void set_instantiation_failed (bool failed)
  {
    instantiation_failed_ = failed;
  }

  void
  trigger_instantiation_finished (bool successful, const QString &error = {})
  {
    Q_EMIT instantiationFinished (successful, error);
  }

  void save_state (std::optional<fs::path> abs_state_dir) override
  {
    save_state_called_ = true;
    last_save_state_dir_ = abs_state_dir;
  }

  void load_state (std::optional<fs::path> abs_state_dir) override
  {
    load_state_called_ = true;
    last_load_state_dir_ = abs_state_dir;
  }

  void prepare_for_processing_impl (
    units::sample_rate_t sample_rate,
    nframes_t            max_block_length) override
  {
    prepare_called_ = true;
    last_sample_rate_ = sample_rate;
    last_max_block_length_ = max_block_length;
  }

  void process_impl (EngineProcessTimeInfo time_info) noexcept override
  {
    process_called_ = true;
    last_time_info_ = time_info;
  }

  bool                    save_state_called_ = false;
  bool                    load_state_called_ = false;
  bool                    prepare_called_ = false;
  bool                    process_called_ = false;
  std::optional<fs::path> last_save_state_dir_;
  std::optional<fs::path> last_load_state_dir_;
  units::sample_rate_t    last_sample_rate_;
  nframes_t               last_max_block_length_ = 0;
  EngineProcessTimeInfo   last_time_info_{
      .g_start_frame_ = 0,
      .g_start_frame_w_offset_ = 0,
      .local_offset_ = 0,
      .nframes_ = 0
  };
};

class PluginTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    port_registry_ = std::make_unique<dsp::PortRegistry> ();
    param_registry_ =
      std::make_unique<dsp::ProcessorParameterRegistry> (*port_registry_);

    plugin_ = std::make_unique<TestPlugin> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      [] () { return fs::path{ "/tmp/test_state" }; });

    // Set up mock transport
    mock_transport_ = std::make_unique<dsp::graph_test::MockTransport> ();
  }

  void TearDown () override
  {
    if (plugin_)
      {
        plugin_->release_resources ();
      }
  }

  std::unique_ptr<dsp::PortRegistry>               port_registry_;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry_;
  units::sample_rate_t        sample_rate_{ units::sample_rate (48000) };
  nframes_t                   max_block_length_{ 1024 };
  std::unique_ptr<TestPlugin> plugin_;
  std::unique_ptr<dsp::graph_test::MockTransport> mock_transport_;
};

TEST_F (PluginTest, ConstructionAndBasicProperties)
{
  EXPECT_EQ (plugin_->get_node_name (), u8"Plugin");
  EXPECT_EQ (plugin_->get_input_ports ().size (), 0);
  EXPECT_EQ (plugin_->get_output_ports ().size (), 0);
  EXPECT_EQ (plugin_->get_parameters ().size (), 2);
  EXPECT_FALSE (plugin_->uiVisible ());
  EXPECT_EQ (plugin_->programIndex (), -1);
}

TEST_F (PluginTest, SetConfiguration)
{
  // Create a plugin descriptor
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Test Plugin";
  descriptor->protocol_ = Protocol::ProtocolType::Internal;

  // Create configuration
  PluginConfiguration config;
  config.descr_ = std::move (descriptor);

  // Set configuration
  QSignalSpy spy (plugin_.get (), &Plugin::configurationChanged);
  plugin_->set_configuration (config);

  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (plugin_->get_name (), u8"Test Plugin");
  EXPECT_EQ (plugin_->get_protocol (), Protocol::ProtocolType::Internal);
}

TEST_F (PluginTest, ProgramIndex)
{
  QSignalSpy spy (plugin_.get (), &Plugin::programIndexChanged);

  // Test setting program index
  plugin_->setProgramIndex (5);
  EXPECT_EQ (plugin_->programIndex (), 5);
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (spy.takeFirst ().at (0).toInt (), 5);

  // Test setting same index (should not emit signal)
  plugin_->setProgramIndex (5);
  EXPECT_EQ (plugin_->programIndex (), 5);
  EXPECT_EQ (spy.count (), 0);

  // Test setting negative index (should reset)
  plugin_->setProgramIndex (-1);
  EXPECT_EQ (plugin_->programIndex (), -1);
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (spy.takeFirst ().at (0).toInt (), -1);
}

TEST_F (PluginTest, UiVisible)
{
  QSignalSpy spy (plugin_.get (), &Plugin::uiVisibleChanged);

  // Test setting UI visible
  plugin_->setUiVisible (true);
  EXPECT_TRUE (plugin_->uiVisible ());
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (spy.takeFirst ().at (0).toBool (), true);

  // Test setting same value (should not emit signal)
  plugin_->setUiVisible (true);
  EXPECT_TRUE (plugin_->uiVisible ());
  EXPECT_EQ (spy.count (), 0);

  // Test setting UI hidden
  plugin_->setUiVisible (false);
  EXPECT_FALSE (plugin_->uiVisible ());
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (spy.takeFirst ().at (0).toBool (), false);
}

TEST_F (PluginTest, PrepareAndReleaseResources)
{
  // Set configuration first
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Test Plugin";
  PluginConfiguration config;
  config.descr_ = std::move (descriptor);
  plugin_->set_configuration (config);

  // Prepare for processing
  plugin_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  EXPECT_TRUE (plugin_->prepare_called_);
  EXPECT_EQ (plugin_->last_sample_rate_, sample_rate_);
  EXPECT_EQ (plugin_->last_max_block_length_, max_block_length_);

  // Release resources
  plugin_->release_resources ();
}

TEST_F (PluginTest, ProcessingWhenEnabled)
{
  // Set configuration
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Test Plugin";
  PluginConfiguration config;
  config.descr_ = std::move (descriptor);
  plugin_->set_configuration (config);

  // Prepare for processing
  plugin_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  // Create bypass parameter
  auto * bypass = plugin_->bypassParameter ();
  bypass->setBaseValue (0.0f); // Not bypassed

  // Process
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };
  plugin_->process_block (time_nfo, *mock_transport_);

  EXPECT_TRUE (plugin_->process_called_);
  EXPECT_EQ (plugin_->last_time_info_.nframes_, 512);
}

TEST_F (PluginTest, ProcessingWhenBypassed)
{
  // Set configuration
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Test Plugin";
  PluginConfiguration config;
  config.descr_ = std::move (descriptor);
  plugin_->set_configuration (config);

  // Prepare for processing
  plugin_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  // Create bypass parameter and set to bypassed
  auto * bypass = plugin_->bypassParameter ();
  bypass->setBaseValue (1.0f); // Bypassed

  // Process
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };
  plugin_->process_block (time_nfo, *mock_transport_);

  EXPECT_FALSE (plugin_->process_called_);
}

TEST_F (PluginTest, ProcessingWhenInstantiationFailed)
{
  // Set configuration
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Test Plugin";
  PluginConfiguration config;
  config.descr_ = std::move (descriptor);
  plugin_->set_configuration (config);

  // Prepare for processing
  plugin_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  // Simulate instantiation failure
  plugin_->set_instantiation_failed (true);

  // Process
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };
  plugin_->process_block (time_nfo, *mock_transport_);

  EXPECT_FALSE (plugin_->process_called_);
}

TEST_F (PluginTest, CurrentlyEnabled)
{
  // Set configuration
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Test Plugin";
  PluginConfiguration config;
  config.descr_ = std::move (descriptor);
  plugin_->set_configuration (config);

  // Prepare for processing
  plugin_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  // Create bypass parameter
  auto * bypass = plugin_->bypassParameter ();

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  // Test enabled
  bypass->setBaseValue (0.0f);
  plugin_->process_block (time_nfo, *mock_transport_);
  EXPECT_TRUE (plugin_->currently_enabled ());

  // Test bypassed
  bypass->setBaseValue (1.0f);
  plugin_->process_block (time_nfo, *mock_transport_);
  EXPECT_FALSE (plugin_->currently_enabled ());
}

TEST_F (PluginTest, GenerateDefaultParameters)
{
  // Set configuration
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Test Plugin";
  PluginConfiguration config;
  config.descr_ = std::move (descriptor);
  plugin_->set_configuration (config);

  // Prepare for processing
  plugin_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  // Test bypass parameter
  auto * bypass = plugin_->bypassParameter ();
  EXPECT_EQ (bypass->label (), "Bypass");
  EXPECT_EQ (bypass->range ().type_, dsp::ParameterRange::Type::Toggle);

  // Test gain parameter
  auto * gain = plugin_->gainParameter ();
  EXPECT_EQ (gain->label (), "Gain");
  EXPECT_EQ (gain->range ().type_, dsp::ParameterRange::Type::GainAmplitude);
}

TEST_F (PluginTest, GetStateDirectory)
{
  // Set configuration
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Test Plugin";
  PluginConfiguration config;
  config.descr_ = std::move (descriptor);
  plugin_->set_configuration (config);

  auto state_dir = plugin_->get_state_directory ();
  EXPECT_TRUE (
    state_dir.string ().find ("/tmp/test_state") != std::string::npos);
  EXPECT_TRUE (
    state_dir.string ().find (
      type_safe::get (plugin_->get_uuid ())
        .toString (QUuid::WithoutBraces)
        .toStdString ())
    != std::string::npos);
}

TEST_F (PluginTest, SaveAndLoadState)
{
  // Set configuration
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Test Plugin";
  PluginConfiguration config;
  config.descr_ = std::move (descriptor);
  plugin_->set_configuration (config);

  // Test save state
  plugin_->save_state (std::nullopt);
  EXPECT_TRUE (plugin_->save_state_called_);
  EXPECT_FALSE (plugin_->last_save_state_dir_.has_value ());

  // Test save state with custom directory
  fs::path custom_dir{ "/custom/state/dir" };
  plugin_->save_state (custom_dir);
  EXPECT_EQ (plugin_->last_save_state_dir_, custom_dir);

  // Test load state
  plugin_->load_state (std::nullopt);
  EXPECT_TRUE (plugin_->load_state_called_);
  EXPECT_FALSE (plugin_->last_load_state_dir_.has_value ());

  // Test load state with custom directory
  plugin_->load_state (custom_dir);
  EXPECT_EQ (plugin_->last_load_state_dir_, custom_dir);
}

TEST_F (PluginTest, JsonSerializationRoundtrip)
{
  // Set configuration
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Test Plugin";
  descriptor->protocol_ = Protocol::ProtocolType::Internal;
  PluginConfiguration config;
  config.descr_ = std::move (descriptor);
  plugin_->set_configuration (config);

  // Set some state
  plugin_->setProgramIndex (3);
  plugin_->setUiVisible (true);

  // Serialize
  nlohmann::json j = *plugin_;

  // Create new plugin from JSON
  TestPlugin deserialized (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    [] () { return fs::path{ "/tmp/test_state" }; });
  from_json (j, deserialized);

  // Verify state
  EXPECT_EQ (deserialized.programIndex (), 3);
  EXPECT_TRUE (deserialized.uiVisible ());
  EXPECT_EQ (deserialized.get_name (), u8"Test Plugin");
  EXPECT_EQ (deserialized.get_protocol (), Protocol::ProtocolType::Internal);
  EXPECT_EQ (
    deserialized.gainParameter ()->get_uuid (),
    plugin_->gainParameter ()->get_uuid ());
}

TEST_F (PluginTest, ProcessPassthroughImpl)
{
  // Set configuration
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Test Plugin";
  PluginConfiguration config;
  config.descr_ = std::move (descriptor);
  plugin_->set_configuration (config);

  // Create ports for passthrough
  auto audio_in = port_registry_->create_object<dsp::AudioPort> (
    u8"Audio In", dsp::PortFlow::Input, dsp::AudioPort::BusLayout::Mono, 1);
  auto audio_out = port_registry_->create_object<dsp::AudioPort> (
    u8"Audio Out", dsp::PortFlow::Output, dsp::AudioPort::BusLayout::Mono, 1);

  plugin_->add_input_port (audio_in);
  plugin_->add_output_port (audio_out);

  // Prepare for processing
  plugin_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  // Fill input buffer
  auto * in_port = audio_in.get_object_as<dsp::AudioPort> ();
  auto * out_port = audio_out.get_object_as<dsp::AudioPort> ();
  for (size_t i = 0; i < 512; i++)
    {
      in_port->buffers ()->setSample (0, i, static_cast<float> (i) * 0.01f);
    }

  // Process with bypass enabled
  auto * bypass = plugin_->bypassParameter ();
  bypass->setBaseValue (1.0f); // Bypassed

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };
  plugin_->process_block (time_nfo, *mock_transport_);

  // Verify passthrough
  for (size_t i = 0; i < 512; i++)
    {
      EXPECT_FLOAT_EQ (out_port->buffers ()->getSample (0, i), i * 0.01f);
    }
}

TEST_F (PluginTest, InstantiationStatusTransitions)
{
  QSignalSpy spy (plugin_.get (), &Plugin::instantiationStatusChanged);

  // Initial state
  EXPECT_EQ (
    plugin_->instantiationStatus (), Plugin::InstantiationStatus::Pending);

  // Pending -> Successful
  plugin_->trigger_instantiation_finished (true, {});
  EXPECT_EQ (
    plugin_->instantiationStatus (), Plugin::InstantiationStatus::Successful);
  EXPECT_EQ (spy.count (), 1);

  // Successful -> Failed (should not happen in practice, but test the transition)
  plugin_->trigger_instantiation_finished (false, "Forced failure");
  EXPECT_EQ (
    plugin_->instantiationStatus (), Plugin::InstantiationStatus::Failed);
  EXPECT_EQ (spy.count (), 2);

  // Failed -> Successful (recovery scenario)
  plugin_->trigger_instantiation_finished (true, {});
  EXPECT_EQ (
    plugin_->instantiationStatus (), Plugin::InstantiationStatus::Successful);
  EXPECT_EQ (spy.count (), 3);
}

} // namespace zrythm::plugins
