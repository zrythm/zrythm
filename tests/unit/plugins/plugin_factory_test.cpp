// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_factory.h"

#include "helpers/scoped_juce_message_thread.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::plugins
{

class PluginFactoryTest
    : public ::testing::Test,
      public test_helpers::ScopedJuceApplication
{
protected:
  void SetUp () override
  {
    // Create registries
    plugin_registry_ = std::make_unique<PluginRegistry> ();
    port_registry_ = std::make_unique<dsp::PortRegistry> ();
    param_registry_ =
      std::make_unique<dsp::ProcessorParameterRegistry> (*port_registry_);

    // Create factory
    factory_ = create_factory ();
  }

  void TearDown () override
  {
    factory_.reset ();
    plugin_registry_.reset ();
    param_registry_.reset ();
    port_registry_.reset ();
  }

  // Helper to create a mock async plugin instance function
  JucePlugin::CreatePluginInstanceAsyncFunc create_mock_async_func ()
  {
    return
      [] (
        const juce::PluginDescription &, double, int,
        std::function<void (
          std::unique_ptr<juce::AudioPluginInstance>, const juce::String &)>
          callback) {
        // Simulate async failure for now - can be customized in tests
        callback (nullptr, "Mock plugin not found");
      };
  }

  // Helper to create a mock window provider
  PluginHostWindowFactory create_mock_window_provider ()
  {
    return [] (Plugin &) {
      class MockWindow : public IPluginHostWindow
      {
      public:
        void setJuceComponentContentNonOwned (juce::Component *) override { }
        void setSizeAndCenter (int, int) override { }
        void setSize (int, int) override { }
        void setVisible (bool) override { }
        WId  getEmbedWindowId () const override { return 0; }
      };
      return std::make_unique<MockWindow> ();
    };
  }

  // Helper to create a test plugin descriptor
  std::unique_ptr<PluginDescriptor> create_test_descriptor (
    Protocol::ProtocolType protocol = Protocol::ProtocolType::Internal)
  {
    auto descriptor = std::make_unique<PluginDescriptor> ();
    descriptor->name_ = u8"Test Plugin";
    descriptor->author_ = u8"Test Author";
    descriptor->protocol_ = protocol;
    descriptor->num_audio_ins_ = 2;
    descriptor->num_audio_outs_ = 2;
    descriptor->num_midi_ins_ = 1;
    descriptor->num_midi_outs_ = 1;
    return descriptor;
  }

  // Helper to create a test plugin configuration
  std::unique_ptr<PluginConfiguration> create_test_configuration (
    Protocol::ProtocolType protocol = Protocol::ProtocolType::Internal)
  {
    auto descriptor = create_test_descriptor (protocol);
    auto config = std::make_unique<PluginConfiguration> ();
    config->descr_ = std::move (descriptor);
    return config;
  }

  // Helper to create a factory with default dependencies
  std::unique_ptr<PluginFactory> create_factory ()
  {
    auto factory_deps = PluginFactory::CommonFactoryDependencies{
      .plugin_registry_ = *plugin_registry_,
      .processor_base_dependencies_{
                                    .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      .state_dir_path_provider_ = [] () { return fs::path{ "/tmp/test_state" }; },
      .create_plugin_instance_async_func_ = create_mock_async_func (),
      .sample_rate_provider_ = [this] () { return sample_rate_; },
      .buffer_size_provider_ = [this] () { return buffer_size_; },
      .top_level_window_provider_ = create_mock_window_provider (),
      .audio_thread_checker_ = [] () { return false; }
    };

    return std::make_unique<PluginFactory> (std::move (factory_deps));
  }

  std::unique_ptr<PluginRegistry>                  plugin_registry_;
  std::unique_ptr<dsp::PortRegistry>               port_registry_;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry_;
  std::unique_ptr<PluginFactory>                   factory_;

  sample_rate_t sample_rate_{ 48000 };
  nframes_t     buffer_size_{ 1024 };
};

// Test basic factory construction
TEST_F (PluginFactoryTest, Construction)
{
  EXPECT_NE (factory_, nullptr);
}

// Test plugin creation for different types using public API
TEST_F (PluginFactoryTest, CreateDifferentPluginTypes)
{
  // Test InternalPluginBase creation
  auto internal_config =
    create_test_configuration (Protocol::ProtocolType::Internal);
  auto internal_finish_options = PluginFactory::InstantiationFinishOptions{
    .handler_ = [] (plugins::PluginUuidReference) { }, .handler_context_ = nullptr
  };

  auto internal_plugin_ref = factory_->create_plugin_from_setting (
    *internal_config, internal_finish_options);
  auto * internal_plugin =
    std::get<InternalPluginBase *> (internal_plugin_ref.get_object ());
  EXPECT_NE (internal_plugin, nullptr);
  EXPECT_EQ (internal_plugin->get_protocol (), Protocol::ProtocolType::Internal);
  EXPECT_TRUE (plugin_registry_->contains (internal_plugin->get_uuid ()));

  // Test ClapPlugin creation
  auto clap_config = create_test_configuration (Protocol::ProtocolType::CLAP);
  auto clap_finish_options = PluginFactory::InstantiationFinishOptions{
    .handler_ = [] (plugins::PluginUuidReference) { }, .handler_context_ = nullptr
  };

  auto clap_plugin_ref =
    factory_->create_plugin_from_setting (*clap_config, clap_finish_options);
  auto * clap_plugin = std::get<ClapPlugin *> (clap_plugin_ref.get_object ());
  EXPECT_NE (clap_plugin, nullptr);
  EXPECT_EQ (clap_plugin->get_protocol (), Protocol::ProtocolType::CLAP);
  EXPECT_TRUE (plugin_registry_->contains (clap_plugin->get_uuid ()));

  // Test JucePlugin creation
  auto juce_config = create_test_configuration (Protocol::ProtocolType::VST3);
  auto juce_finish_options = PluginFactory::InstantiationFinishOptions{
    .handler_ = [] (plugins::PluginUuidReference) { }, .handler_context_ = nullptr
  };

  auto juce_plugin_ref =
    factory_->create_plugin_from_setting (*juce_config, juce_finish_options);
  auto * juce_plugin = std::get<JucePlugin *> (juce_plugin_ref.get_object ());
  EXPECT_NE (juce_plugin, nullptr);
  EXPECT_EQ (juce_plugin->get_protocol (), Protocol::ProtocolType::VST3);
  EXPECT_TRUE (plugin_registry_->contains (juce_plugin->get_uuid ()));
}

// Test create_plugin_from_setting with different protocols
TEST_F (PluginFactoryTest, CreatePluginFromSetting)
{
  // Test Internal protocol
  auto internal_config =
    create_test_configuration (Protocol::ProtocolType::Internal);
  auto internal_finish_options = PluginFactory::InstantiationFinishOptions{
    .handler_ = [] (plugins::PluginUuidReference) { }, .handler_context_ = nullptr
  };

  auto internal_plugin_ref = factory_->create_plugin_from_setting (
    *internal_config, internal_finish_options);
  auto * internal_plugin =
    std::get<InternalPluginBase *> (internal_plugin_ref.get_object ());
  EXPECT_NE (internal_plugin, nullptr);
  EXPECT_EQ (internal_plugin->get_protocol (), Protocol::ProtocolType::Internal);

  // Test CLAP protocol
  auto clap_config = create_test_configuration (Protocol::ProtocolType::CLAP);
  auto clap_finish_options = PluginFactory::InstantiationFinishOptions{
    .handler_ = [] (plugins::PluginUuidReference) { }, .handler_context_ = nullptr
  };

  auto clap_plugin_ref =
    factory_->create_plugin_from_setting (*clap_config, clap_finish_options);
  auto * clap_plugin = std::get<ClapPlugin *> (clap_plugin_ref.get_object ());
  EXPECT_NE (clap_plugin, nullptr);
  EXPECT_EQ (clap_plugin->get_protocol (), Protocol::ProtocolType::CLAP);

  // Test VST3 protocol (should use JucePlugin)
  auto vst3_config = create_test_configuration (Protocol::ProtocolType::VST3);
  auto vst3_finish_options = PluginFactory::InstantiationFinishOptions{
    .handler_ = [] (plugins::PluginUuidReference) { }, .handler_context_ = nullptr
  };

  auto vst3_plugin_ref =
    factory_->create_plugin_from_setting (*vst3_config, vst3_finish_options);
  auto * vst3_plugin = std::get<JucePlugin *> (vst3_plugin_ref.get_object ());
  EXPECT_NE (vst3_plugin, nullptr);
  EXPECT_EQ (vst3_plugin->get_protocol (), Protocol::ProtocolType::VST3);
}

// Test plugin registration in registry
TEST_F (PluginFactoryTest, PluginRegistration)
{
  // Create multiple plugins and verify they're all registered
  const int                                 num_plugins = 5;
  std::vector<plugins::PluginUuidReference> plugin_refs;

  for (int i = 0; i < num_plugins; ++i)
    {
      auto config = create_test_configuration (Protocol::ProtocolType::Internal);
      config->descr_->name_ = utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("Test Plugin {}", i));

      auto finish_options = PluginFactory::InstantiationFinishOptions{
        .handler_ = [] (plugins::PluginUuidReference) { },
        .handler_context_ = nullptr
      };

      auto ref = factory_->create_plugin_from_setting (*config, finish_options);
      plugin_refs.push_back (ref);
    }

  // Verify all plugins are registered
  EXPECT_EQ (plugin_registry_->size (), num_plugins);

  for (const auto &ref : plugin_refs)
    {
      EXPECT_TRUE (plugin_registry_->contains (ref.id ()));

      // Verify we can retrieve the plugin
      auto plugin_var = plugin_registry_->find_by_id (ref.id ());
      EXPECT_TRUE (plugin_var.has_value ());
    }
}

// Test that newly created plugins have proper configuration
TEST_F (PluginFactoryTest, PluginConfigurationApplied)
{
  auto config = create_test_configuration (Protocol::ProtocolType::Internal);
  config->descr_->name_ = u8"Custom Test Plugin";
  config->descr_->author_ = u8"Custom Author";

  // Use create_plugin_from_setting to apply configuration
  auto finish_options = PluginFactory::InstantiationFinishOptions{
    .handler_ = [] (plugins::PluginUuidReference) { }, .handler_context_ = nullptr
  };

  auto plugin_ref =
    factory_->create_plugin_from_setting (*config, finish_options);
  auto * plugin = std::get<InternalPluginBase *> (plugin_ref.get_object ());

  EXPECT_NE (plugin, nullptr);
  EXPECT_EQ (plugin->get_name (), u8"Custom Test Plugin");
  EXPECT_EQ (plugin->get_protocol (), Protocol::ProtocolType::Internal);
}

// Test instantiation finished handler with async behavior
TEST_F (PluginFactoryTest, InstantiationFinishedHandlerAsync)
{
  bool                                        handler_called = false;
  std::optional<plugins::PluginUuidReference> received_ref;

  auto finish_options = PluginFactory::InstantiationFinishOptions{
    .handler_ =
      [&received_ref, &handler_called] (plugins::PluginUuidReference ref) {
        handler_called = true;
        received_ref = ref;
      },
    .handler_context_ = factory_.get ()
  };

  // Test with JucePlugin (async instantiation)
  auto juce_config = create_test_configuration (Protocol::ProtocolType::VST3);
  auto juce_plugin_ref =
    factory_->create_plugin_from_setting (*juce_config, finish_options);
  auto * juce_plugin = std::get<JucePlugin *> (juce_plugin_ref.get_object ());

  EXPECT_NE (juce_plugin, nullptr);
  EXPECT_TRUE (plugin_registry_->contains (juce_plugin->get_uuid ()));

  // Process events to handle async instantiation
  QCoreApplication::processEvents ();

  // Handler should be called even for failed instantiation
  EXPECT_TRUE (handler_called);
  EXPECT_EQ (received_ref->id (), juce_plugin_ref.id ());
}

// Test factory dependencies are properly used
TEST_F (PluginFactoryTest, FactoryDependenciesUsed)
{
  auto config = create_test_configuration (Protocol::ProtocolType::Internal);
  auto finish_options = PluginFactory::InstantiationFinishOptions{
    .handler_ = [] (plugins::PluginUuidReference) { }, .handler_context_ = nullptr
  };

  auto plugin_ref =
    factory_->create_plugin_from_setting (*config, finish_options);
  auto * plugin = std::get<InternalPluginBase *> (plugin_ref.get_object ());

  EXPECT_NE (plugin, nullptr);

  // Verify plugin has expected state directory path provider
  auto state_dir = plugin->get_state_directory ();
  EXPECT_TRUE (
    state_dir.string ().find ("/tmp/test_state") != std::string::npos);

  // Verify plugin is in registry
  EXPECT_TRUE (plugin_registry_->contains (plugin->get_uuid ()));
}

// Test multiple plugins can be created independently
TEST_F (PluginFactoryTest, MultiplePluginsIndependent)
{
  // Create configs for different plugin types
  auto internal_config =
    create_test_configuration (Protocol::ProtocolType::Internal);
  auto clap_config = create_test_configuration (Protocol::ProtocolType::CLAP);
  auto juce_config = create_test_configuration (Protocol::ProtocolType::VST3);

  auto finish_options = PluginFactory::InstantiationFinishOptions{
    .handler_ = [] (plugins::PluginUuidReference) { }, .handler_context_ = nullptr
  };

  // Create plugins
  auto ref1 =
    factory_->create_plugin_from_setting (*internal_config, finish_options);
  auto ref2 =
    factory_->create_plugin_from_setting (*clap_config, finish_options);
  auto ref3 =
    factory_->create_plugin_from_setting (*juce_config, finish_options);

  // Verify each plugin is independent
  auto * plugin1 = std::get<InternalPluginBase *> (ref1.get_object ());
  auto * plugin2 = std::get<ClapPlugin *> (ref2.get_object ());
  auto * plugin3 = std::get<JucePlugin *> (ref3.get_object ());

  EXPECT_NE (plugin1, nullptr);
  EXPECT_NE (plugin2, nullptr);
  EXPECT_NE (plugin3, nullptr);

  EXPECT_EQ (plugin1->get_protocol (), Protocol::ProtocolType::Internal);
  EXPECT_EQ (plugin2->get_protocol (), Protocol::ProtocolType::CLAP);
  EXPECT_EQ (plugin3->get_protocol (), Protocol::ProtocolType::VST3);

  // All should be registered
  EXPECT_TRUE (plugin_registry_->contains (plugin1->get_uuid ()));
  EXPECT_TRUE (plugin_registry_->contains (plugin2->get_uuid ()));
  EXPECT_TRUE (plugin_registry_->contains (plugin3->get_uuid ()));

  // Verify they have different UUIDs
  EXPECT_NE (plugin1->get_uuid (), plugin2->get_uuid ());
  EXPECT_NE (plugin2->get_uuid (), plugin3->get_uuid ());
  EXPECT_NE (plugin1->get_uuid (), plugin3->get_uuid ());
}

// Test sample rate and buffer size providers
TEST_F (PluginFactoryTest, SampleRateAndBufferSizeProviders)
{
  // Create custom providers
  auto custom_sample_rate = 44100.0;
  auto custom_buffer_size = 512;

  auto factory_deps = PluginFactory::CommonFactoryDependencies{
    .plugin_registry_ = *plugin_registry_,
    .processor_base_dependencies_{
                                  .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    .state_dir_path_provider_ = [] () { return fs::path{ "/tmp/test_state" }; },
    .create_plugin_instance_async_func_ = create_mock_async_func (),
    .sample_rate_provider_ =
      [custom_sample_rate] () { return custom_sample_rate; },
    .buffer_size_provider_ =
      [custom_buffer_size] () { return custom_buffer_size; },
    .top_level_window_provider_ = create_mock_window_provider (),
    .audio_thread_checker_ = [] () { return false; }
  };

  auto custom_factory =
    std::make_unique<PluginFactory> (std::move (factory_deps));

  auto juce_config = create_test_configuration (Protocol::ProtocolType::VST3);
  auto finish_options = PluginFactory::InstantiationFinishOptions{
    .handler_ = [] (plugins::PluginUuidReference) { }, .handler_context_ = nullptr
  };

  auto plugin_ref =
    custom_factory->create_plugin_from_setting (*juce_config, finish_options);
  auto * plugin = std::get<JucePlugin *> (plugin_ref.get_object ());

  EXPECT_NE (plugin, nullptr);
  EXPECT_TRUE (plugin_registry_->contains (plugin->get_uuid ()));
}

// Test error handling for invalid configurations
TEST_F (PluginFactoryTest, ErrorHandling)
{
  // Test with null descriptor (should not crash)
  auto invalid_config = std::make_unique<PluginConfiguration> ();
  // invalid_config->descr_ is null

  auto finish_options = PluginFactory::InstantiationFinishOptions{
    .handler_ = [] (plugins::PluginUuidReference) { }, .handler_context_ = nullptr
  };

  EXPECT_THROW (
    factory_->create_plugin_from_setting (*invalid_config, finish_options),
    std::logic_error);
}

} // namespace zrythm::plugins
