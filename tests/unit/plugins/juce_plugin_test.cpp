// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <numbers>

#include "dsp/processor_base.h"
#include "plugins/juce_plugin.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "utils/serialization.h"

#include "helpers/scoped_juce_message_thread.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace zrythm::plugins
{

class MockAudioPluginInstance : public juce::AudioPluginInstance
{
public:
  MOCK_METHOD (const juce::String, getName, (), (const override));
  MOCK_METHOD (
    void,
    fillInPluginDescription,
    (juce::PluginDescription &),
    (const override));
  MOCK_METHOD (void, prepareToPlay, (double, int), (override));
  MOCK_METHOD (void, releaseResources, (), (override));
  MOCK_METHOD (
    void,
    processBlock,
    (juce::AudioBuffer<float> &, juce::MidiBuffer &),
    (override));
  MOCK_METHOD (double, getTailLengthSeconds, (), (const override));
  MOCK_METHOD (bool, acceptsMidi, (), (const override));
  MOCK_METHOD (bool, producesMidi, (), (const override));
  MOCK_METHOD (int, getTotalNumInputChannels, (), (const override));
  MOCK_METHOD (int, getTotalNumOutputChannels, (), (const override));
  MOCK_METHOD (void, getStateInformation, (juce::MemoryBlock &), (override));
  MOCK_METHOD (void, setStateInformation, (const void *, int), (override));
  MOCK_METHOD (juce::AudioProcessorEditor *, createEditor, (), (override));
  MOCK_METHOD (bool, hasEditor, (), (const override));
  MOCK_METHOD (int, getNumPrograms, (), (override));
  MOCK_METHOD (int, getCurrentProgram, (), (override));
  MOCK_METHOD (void, setCurrentProgram, (int), (override));
  MOCK_METHOD (const juce::String, getProgramName, (int), (override));
  MOCK_METHOD (void, changeProgramName, (int, const juce::String &), (override));
};

class MockAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
  MockAudioProcessorEditor (juce::AudioProcessor &p)
      : juce::AudioProcessorEditor (p)
  {
  }
  ~MockAudioProcessorEditor () override
  {
    getAudioProcessor ()->editorBeingDeleted (this);
  }
  MOCK_METHOD (void, setVisible, (bool), (override));
};

class MockTopLevelWindow : public juce::Component
{
public:
  MOCK_METHOD (void, setVisible, (bool), (override));
  MOCK_METHOD (void, addToDesktop, (int, void *), (override));
  MOCK_METHOD (int, getDesktopWindowStyleFlags, (), (const override));
};

class JucePluginTest
    : public ::testing::Test,
      public test_helpers::ScopedJuceApplication
{
protected:
  void SetUp () override
  {
    port_registry_ = std::make_unique<dsp::PortRegistry> ();
    param_registry_ =
      std::make_unique<dsp::ProcessorParameterRegistry> (*port_registry_);

    sample_rate_ = 48000;
    buffer_size_ = 1024;
  }

  void TearDown () override
  {
    if (plugin_)
      {
        plugin_->release_resources ();
      }
  }

  JucePlugin::JucePluginTopLevelWindowProvider
  createMockTopLevelWindowProvider ()
  {
    return [] (juce::AudioProcessorEditor &editor, JucePlugin &) {
      auto mock_window = std::make_unique<MockTopLevelWindow> ();
      EXPECT_CALL (*mock_window, setVisible (::testing::_))
        .Times (::testing::AnyNumber ());
      EXPECT_CALL (*mock_window, addToDesktop (::testing::_, ::testing::_))
        .Times (::testing::AnyNumber ());
      return mock_window;
    };
  }

  void createTestDescriptor ()
  {
    descriptor_ = std::make_unique<PluginDescriptor> ();
    descriptor_->name_ = u8"Test JUCE Plugin";
    descriptor_->protocol_ = Protocol::ProtocolType::VST3;
  }

  void createTestConfiguration ()
  {
    createTestDescriptor ();
    config_ = std::make_unique<PluginConfiguration> ();
    config_->descr_ = std::move (descriptor_);
  }

  void setupMockPlugin ()
  {
    mock_plugin_ = std::make_unique<MockAudioPluginInstance> ();

    // Setup mock expectations
    EXPECT_CALL (*mock_plugin_, getName ())
      .WillRepeatedly (::testing::Return ("Test Plugin"));
    EXPECT_CALL (*mock_plugin_, releaseResources ()).Times (1);

    // Setup parameter expectations
    mock_plugin_->addHostedParameter (
      std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID{ juce::String ("cutoff-unique-id") },
        juce::String ("Cutoff"), juce::NormalisableRange<float> (), 0.5f));

    // Tell the plugin it has 2 ins / 2 outs
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add (juce::AudioChannelSet::stereo ());
    layout.outputBuses.add (juce::AudioChannelSet::stereo ());
    mock_plugin_->setBusesLayout (layout);

    // Allow MIDI inputs/outputs
    EXPECT_CALL (*mock_plugin_, acceptsMidi ())
      .WillRepeatedly (::testing::Return (true));
    EXPECT_CALL (*mock_plugin_, producesMidi ())
      .WillRepeatedly (::testing::Return (true));
  }

  void setupJucePlugin (bool with_successful_instantiation)
  {
    setupJucePlugin (
      [with_successful_instantiation, this] (
        const juce::PluginDescription &, double, int,
        std::function<void (
          std::unique_ptr<juce::AudioPluginInstance>, const juce::String &)>
          callback) {
        if (with_successful_instantiation)
          {
            // Simulate successful instantiation
            callback (std::move (mock_plugin_), "");
          }
        else
          {
            // Simulate instantiation failure
            callback (nullptr, "Plugin not found");
          }
      });
  }

  std::unique_ptr<dsp::PortRegistry>               port_registry_;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry_;
  std::unique_ptr<JucePlugin>                      plugin_;
  std::unique_ptr<PluginDescriptor>                descriptor_;
  std::unique_ptr<PluginConfiguration>             config_;
  std::unique_ptr<MockAudioPluginInstance>         mock_plugin_;
  sample_rate_t                                    sample_rate_{};
  nframes_t                                        buffer_size_{};

private:
  void setupJucePlugin (JucePlugin::CreatePluginInstanceAsyncFunc func)
  {
    plugin_ = std::make_unique<JucePlugin> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      [] () { return fs::path{ "/tmp/test_state" }; }, func,
      [this] () { return sample_rate_; }, [this] () { return buffer_size_; },
      createMockTopLevelWindowProvider ());
  }
};

TEST_F (JucePluginTest, AsyncInstantiationSuccess)
{
  createTestConfiguration ();

  setupMockPlugin ();

  // Setup async callback expectation
  setupJucePlugin (true);

  // Wait for instantiation to complete
  bool instantiation_finished = false;
  bool successful = false;
  QObject::connect (
    plugin_.get (), &JucePlugin::instantiationFinished, plugin_.get (),
    [&instantiation_finished, &successful] (bool instantiation_success) {
      instantiation_finished = true;
      successful = instantiation_success;
    });

  plugin_->set_configuration (*config_);

  // Process events until instantiation completes
  while (!instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }

  EXPECT_TRUE (instantiation_finished);
  EXPECT_TRUE (successful);

  const auto &in_ports = plugin_->get_input_ports ();
  EXPECT_EQ (in_ports.size (), 3);
  EXPECT_TRUE (
    std::holds_alternative<dsp::AudioPort *> (in_ports.at (0).get_object ()));
  EXPECT_TRUE (
    std::holds_alternative<dsp::AudioPort *> (in_ports.at (1).get_object ()));
  EXPECT_TRUE (
    std::holds_alternative<dsp::MidiPort *> (in_ports.at (2).get_object ()));

  const auto &out_ports = plugin_->get_output_ports ();
  EXPECT_EQ (out_ports.size (), 3);
  EXPECT_TRUE (
    std::holds_alternative<dsp::AudioPort *> (out_ports.at (0).get_object ()));
  EXPECT_TRUE (
    std::holds_alternative<dsp::AudioPort *> (out_ports.at (1).get_object ()));
  EXPECT_TRUE (
    std::holds_alternative<dsp::MidiPort *> (out_ports.at (2).get_object ()));
}

TEST_F (JucePluginTest, AsyncInstantiationFailure)
{
  createTestConfiguration ();

  // Setup async callback expectation for failure
  setupJucePlugin (false);

  // Wait for instantiation to complete
  bool    instantiation_finished = false;
  bool    successful = true;
  QString error_message;
  QObject::connect (
    plugin_.get (), &JucePlugin::instantiationFinished, plugin_.get (),
    [&instantiation_finished, &error_message,
     &successful] (bool success, const QString &error) {
      instantiation_finished = true;
      error_message = error;
      successful = success;
    });

  plugin_->set_configuration (*config_);

  // Process events until instantiation completes
  while (!instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }

  EXPECT_TRUE (instantiation_finished);
  EXPECT_FALSE (successful);
  EXPECT_EQ (error_message, "Plugin not found");

  // Prepare for processing
  plugin_->prepare_for_processing (sample_rate_, buffer_size_);

  // Test processing without instantiation
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  // This should not crash even if plugin is not instantiated
  plugin_->process_block (time_nfo);
}

TEST_F (JucePluginTest, ProcessingWithInstantiatedPlugin)
{
  createTestConfiguration ();

  setupMockPlugin ();

  // Setup async callback expectation
  setupJucePlugin (true);

  // Setup processing expectations
  EXPECT_CALL (*mock_plugin_, prepareToPlay (sample_rate_, buffer_size_))
    .Times (1);
  EXPECT_CALL (*mock_plugin_, processBlock (::testing::_, ::testing::_))
    .Times (1);

  // Wait for instantiation to complete
  bool instantiation_finished = false;
  QObject::connect (
    plugin_.get (), &JucePlugin::instantiationFinished, plugin_.get (),
    [&instantiation_finished] () { instantiation_finished = true; });

  plugin_->set_configuration (*config_);

  // Process events until instantiation completes
  while (!instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }

  // Prepare for processing
  plugin_->prepare_for_processing (sample_rate_, buffer_size_);

  // Process a block
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  plugin_->process_block (time_nfo);
}

TEST_F (JucePluginTest, ParameterMapping)
{
  createTestConfiguration ();

  setupMockPlugin ();

  // Setup async callback expectation
  setupJucePlugin (true);

  // Wait for instantiation to complete
  bool instantiation_finished = false;
  QObject::connect (
    plugin_.get (), &JucePlugin::instantiationFinished, plugin_.get (),
    [&instantiation_finished] () { instantiation_finished = true; });

  plugin_->set_configuration (*config_);

  // Process events until instantiation completes
  while (!instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }

  // Test parameter mapping
  EXPECT_EQ (plugin_->get_parameters ().size (), 3);
  EXPECT_EQ (
    plugin_->get_parameters ()
      .at (2)
      .get_object_as<dsp::ProcessorParameter> ()
      ->label ()
      .toStdString (),
    "Cutoff");
}

TEST_F (JucePluginTest, StateSavingLoading)
{
  createTestConfiguration ();

  setupMockPlugin ();

  // Setup state expectations
  juce::MemoryBlock test_state;
  test_state.append ("test state data", 15);

  EXPECT_CALL (*mock_plugin_, getStateInformation (::testing::_))
    .WillOnce ([&test_state] (juce::MemoryBlock &block) { block = test_state; });

  EXPECT_CALL (*mock_plugin_, setStateInformation (::testing::_, 15)).Times (1);

  // Setup async callback expectation
  setupJucePlugin (true);

  // Wait for instantiation to complete
  bool instantiation_finished = false;
  QObject::connect (
    plugin_.get (), &JucePlugin::instantiationFinished, plugin_.get (),
    [&instantiation_finished] () { instantiation_finished = true; });

  plugin_->set_configuration (*config_);

  // Process events until instantiation completes
  while (!instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }

  // Test state saving/loading
  plugin_->save_state (std::nullopt);
  plugin_->load_state (std::nullopt);
}

TEST_F (JucePluginTest, EditorManagement)
{
  createTestConfiguration ();

  setupMockPlugin ();

  // Setup editor expectations
  auto * editor = new MockAudioProcessorEditor (*mock_plugin_);
  editor->setSize (64, 64);
  EXPECT_CALL (*mock_plugin_, hasEditor ())
    .WillRepeatedly (::testing::Return (true));
  EXPECT_CALL (*mock_plugin_, createEditor ())
    .WillOnce (::testing::Return (editor));
  {
    ::testing::InSequence seq;
    EXPECT_CALL (*editor, setVisible (true));
    EXPECT_CALL (*editor, setVisible (false));
  }

  // Setup async callback expectation
  setupJucePlugin (true);

  // Wait for instantiation to complete
  bool instantiation_finished = false;
  QObject::connect (
    plugin_.get (), &JucePlugin::instantiationFinished, plugin_.get (),
    [&instantiation_finished] () { instantiation_finished = true; });

  plugin_->set_configuration (*config_);

  // Process events until instantiation completes
  while (!instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }

  // Test editor management
  plugin_->setUiVisible (true);
  plugin_->setUiVisible (false);
}

TEST_F (JucePluginTest, MidiProcessing)
{
  createTestConfiguration ();
  setupMockPlugin ();

  // Setup async callback expectation
  setupJucePlugin (true);

  // Setup MIDI processing expectations
  EXPECT_CALL (*mock_plugin_, processBlock (::testing::_, ::testing::_))
    .WillOnce ([] (juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi) {
      // Simulate processing MIDI messages
      juce::MidiMessage msg = juce::MidiMessage::noteOn (1, 60, 0.8f);
      midi.addEvent (msg, 0);
    });

  bool instantiation_finished = false;
  QObject::connect (
    plugin_.get (), &JucePlugin::instantiationFinished, plugin_.get (),
    [&instantiation_finished] () { instantiation_finished = true; });

  plugin_->set_configuration (*config_);

  while (!instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }

  plugin_->prepare_for_processing (sample_rate_, buffer_size_);

  // Process with MIDI input
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  plugin_->process_block (time_nfo);

  const auto &midi_out =
    plugin_->get_output_ports ().at (2).get_object_as<dsp::MidiPort> ();
  EXPECT_EQ (midi_out->midi_events_.queued_events_.size (), 1);
  const auto &ev = midi_out->midi_events_.queued_events_.front ();
  EXPECT_TRUE (utils::midi::midi_is_note_on (ev.raw_buffer_));
}

TEST_F (JucePluginTest, BidirectionalParameterSync)
{
  createTestConfiguration ();
  setupMockPlugin ();

  // Add additional parameters for testing
  mock_plugin_->addHostedParameter (
    std::make_unique<juce::AudioParameterFloat> (
      juce::ParameterID{ juce::String ("test-param-unique-id") },
      juce::String ("Test Param"), juce::NormalisableRange<float> (0.0f, 1.0f),
      0.5f));
  // mock_plugin_ will be moved so keep its pointer for later use
  auto * mock_plugin = mock_plugin_.get ();

  setupJucePlugin (true);

  bool instantiation_finished = false;
  QObject::connect (
    plugin_.get (), &JucePlugin::instantiationFinished, plugin_.get (),
    [&instantiation_finished] () { instantiation_finished = true; });

  plugin_->set_configuration (*config_);

  while (!instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }

  // Test parameter synchronization from host to plugin
  auto params = plugin_->get_parameters ();
  EXPECT_EQ (params.size (), 4); // 3 pre-existing + 1 parameters

  // Find the gain parameter
  dsp::ProcessorParameter * zrythm_param = nullptr;
  for (const auto &param : params)
    {
      auto * p = param.get_object_as<dsp::ProcessorParameter> ();
      if (p->label () == "Test Param")
        {
          zrythm_param = p;
          break;
        }
    }
  ASSERT_NE (zrythm_param, nullptr);

  // Change parameter value and verify synchronization
  float new_value = 0.75f;
  zrythm_param->setBaseValue (new_value);

  plugin_->prepare_for_processing (sample_rate_, buffer_size_);

  // Process with MIDI input
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  plugin_->process_block (time_nfo);

  // Verify the change is reflected in the plugin
  auto * juce_param = mock_plugin->getHostedParameter (1);
  EXPECT_EQ (
    juce_param->getParameterID (), juce::String ("test-param-unique-id"));
  EXPECT_FLOAT_EQ (juce_param->getValue (), 0.75f);

  // Other way around...
  juce_param->setValueNotifyingHost (0.25f);
  plugin_->process_block (time_nfo);
  EXPECT_FLOAT_EQ (zrythm_param->baseValue (), 0.25f);
}

TEST_F (JucePluginTest, AudioProcessingEdgeCases)
{
  createTestConfiguration ();
  setupMockPlugin ();

  setupJucePlugin (true);

  bool instantiation_finished = false;
  QObject::connect (
    plugin_.get (), &JucePlugin::instantiationFinished, plugin_.get (),
    [&instantiation_finished] () { instantiation_finished = true; });

  plugin_->set_configuration (*config_);

  while (!instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }

  plugin_->prepare_for_processing (sample_rate_, buffer_size_);

  // Test with zero-length buffer
  EngineProcessTimeInfo zero_time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 0
  };
  plugin_->process_block (zero_time_nfo);

  // Test with very large buffer
  EngineProcessTimeInfo large_time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 8192
  };
  plugin_->process_block (large_time_nfo);

  // Test with non-zero local offset
  EngineProcessTimeInfo offset_time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 256,
    .nframes_ = 512
  };
  plugin_->process_block (offset_time_nfo);
}

TEST_F (JucePluginTest, AdvancedParameterTypes)
{
  createTestConfiguration ();
  setupMockPlugin ();

  // Add various parameter types
  mock_plugin_->addHostedParameter (
    std::make_unique<juce::AudioParameterInt> (
      juce::ParameterID{ juce::String ("int-param") },
      juce::String ("IntParam"), 0, 100, 50));

  mock_plugin_->addHostedParameter (
    std::make_unique<juce::AudioParameterBool> (
      juce::ParameterID{ juce::String ("bool-param") },
      juce::String ("BoolParam"), false));

  mock_plugin_->addHostedParameter (
    std::make_unique<juce::AudioParameterChoice> (
      juce::ParameterID{ juce::String ("choice-param") },
      juce::String ("ChoiceParam"),
      juce::StringArray{ "Option1", "Option2", "Option3" }, 0));

  setupJucePlugin (true);

  bool instantiation_finished = false;
  QObject::connect (
    plugin_.get (), &JucePlugin::instantiationFinished, plugin_.get (),
    [&instantiation_finished] () { instantiation_finished = true; });

  plugin_->set_configuration (*config_);

  while (!instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }

  // Verify parameter types are correctly mapped
  auto params = plugin_->get_parameters ();
  EXPECT_GE (params.size (), 6);

  // Find and test each parameter type
  bool found_int = false;
  bool found_bool = false;
  bool found_choice = false;
  for (const auto &param : params)
    {
      auto * p = param.get_object_as<dsp::ProcessorParameter> ();
      auto   label = p->label ().toStdString ();

      if (label == "IntParam")
        {
          found_int = true;
          // Test integer parameter range
          EXPECT_FLOAT_EQ (p->range ().minf_, 0.0f);
          // Currently no way to get this via JUCE host API...
          // EXPECT_FLOAT_EQ (p->range ().maxf_, 100.0f);
        }
      else if (label == "BoolParam")
        {
          found_bool = true;
          // Test boolean parameter (should be 0 or 1)
          EXPECT_FLOAT_EQ (p->range ().minf_, 0.0f);
          EXPECT_FLOAT_EQ (p->range ().maxf_, 1.0f);
        }
      else if (label == "ChoiceParam")
        {
          found_choice = true;
          // Test choice parameter (should have discrete steps)
          EXPECT_FLOAT_EQ (p->range ().minf_, 0.0f);
          EXPECT_FLOAT_EQ (p->range ().maxf_, 2.0f); // 3 options = 0, 1, 2
        }
    }

  EXPECT_TRUE (found_int);
  EXPECT_TRUE (found_bool);
  EXPECT_TRUE (found_choice);
}

TEST_F (JucePluginTest, SerializationPreservesState)
{
  createTestConfiguration ();
  setupMockPlugin ();

  auto * editor = new MockAudioProcessorEditor (*mock_plugin_);
  editor->setSize (64, 64);
  EXPECT_CALL (*mock_plugin_, hasEditor ())
    .WillRepeatedly (::testing::Return (true));
  EXPECT_CALL (*mock_plugin_, createEditor ())
    .WillOnce (::testing::Return (editor));

  // Setup async callback expectation
  setupJucePlugin (true);

  bool instantiation_finished = false;
  QObject::connect (
    plugin_.get (), &JucePlugin::instantiationFinished, plugin_.get (),
    [&instantiation_finished] () { instantiation_finished = true; });

  plugin_->set_configuration (*config_);

  while (!instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }

  // Set some state
  plugin_->setProgramIndex (5);
  plugin_->setUiVisible (true);

  // Set parameter values
  auto * bypass = plugin_->bypassParameter ();
  auto * gain = plugin_->gainParameter ();
  bypass->setBaseValue (0.0f); // Not bypassed
  gain->setBaseValue (0.75f);  // 75% gain

  // Serialize
  nlohmann::json json;
  to_json (json, *plugin_);

  // Create new plugin instance for deserialization
  auto deserialized_plugin = std::make_unique<JucePlugin> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    [] () { return fs::path{ "/tmp/test_state" }; },
    [] (
      const juce::PluginDescription &, double, int,
      std::function<void (
        std::unique_ptr<juce::AudioPluginInstance>, const juce::String &)>
        callback) {
      // Create a mock for deserialization
      auto mock = std::make_unique<MockAudioPluginInstance> ();
      EXPECT_CALL (*mock, getName ())
        .WillRepeatedly (::testing::Return ("Test Plugin"));
      EXPECT_CALL (*mock, releaseResources ()).Times (1);

#if 0
      auto * editor = new MockAudioProcessorEditor (*mock);
      editor->setSize (64, 64);
      EXPECT_CALL (*mock, hasEditor ()).WillRepeatedly (::testing::Return (true));
      EXPECT_CALL (*mock, createEditor ()).WillOnce (::testing::Return (editor));
#endif

      callback (std::move (mock), "");
    },
    [this] () { return sample_rate_; }, [this] () { return buffer_size_; },
    createMockTopLevelWindowProvider ());

  bool deserialized_plugin_instantiation_finished = false;
  QObject::connect (
    deserialized_plugin.get (), &JucePlugin::instantiationFinished,
    deserialized_plugin.get (), [&deserialized_plugin_instantiation_finished] () {
      deserialized_plugin_instantiation_finished = true;
    });

  // Deserialize
  from_json (json, *deserialized_plugin);

  // Wait for instantiation
  while (!deserialized_plugin_instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }

  // Verify state was preserved
  EXPECT_EQ (deserialized_plugin->programIndex (), 5);
  EXPECT_TRUE (deserialized_plugin->uiVisible ());

  // Verify ports were preserved
  const auto &in_ports = deserialized_plugin->get_input_ports ();
  EXPECT_EQ (in_ports.size (), 3);
  EXPECT_TRUE (
    std::holds_alternative<dsp::AudioPort *> (in_ports.at (0).get_object ()));
  EXPECT_TRUE (
    std::holds_alternative<dsp::AudioPort *> (in_ports.at (1).get_object ()));
  EXPECT_TRUE (
    std::holds_alternative<dsp::MidiPort *> (in_ports.at (2).get_object ()));

  const auto &out_ports = deserialized_plugin->get_output_ports ();
  EXPECT_EQ (out_ports.size (), 3);

  // Verify parameters were preserved
  auto params = deserialized_plugin->get_parameters ();
  EXPECT_GE (params.size (), 2);

  // Find and verify parameter values
  bool found_bypass = false;
  bool found_gain = false;
  for (const auto &param : params)
    {
      auto * p = param.get_object_as<dsp::ProcessorParameter> ();
      if (p->label () == "Bypass")
        {
          found_bypass = true;
          EXPECT_FLOAT_EQ (p->baseValue (), 0.0f);
        }
      else if (p->label () == "Gain")
        {
          found_gain = true;
          EXPECT_FLOAT_EQ (p->baseValue (), 0.75f);
        }
    }
  EXPECT_TRUE (found_bypass);
  EXPECT_TRUE (found_gain);

  // Verify UUIDs are preserved
  EXPECT_EQ (
    deserialized_plugin->bypassParameter ()->get_uuid (),
    plugin_->bypassParameter ()->get_uuid ());
  EXPECT_EQ (
    deserialized_plugin->gainParameter ()->get_uuid (),
    plugin_->gainParameter ()->get_uuid ());
}

TEST_F (JucePluginTest, JuceParameterStateSerialization)
{
  createTestConfiguration ();
  setupMockPlugin ();

  // Add various JUCE parameter types with specific values
  mock_plugin_->addHostedParameter (
    std::make_unique<juce::AudioParameterFloat> (
      juce::ParameterID{ juce::String ("test-float") },
      juce::String ("TestFloat"), juce::NormalisableRange<float> (0.0f, 1.0f),
      0.3f));

  mock_plugin_->addHostedParameter (
    std::make_unique<juce::AudioParameterInt> (
      juce::ParameterID{ juce::String ("test-int") }, juce::String ("TestInt"),
      0, 100, 42));

  mock_plugin_->addHostedParameter (
    std::make_unique<juce::AudioParameterBool> (
      juce::ParameterID{ juce::String ("test-bool") },
      juce::String ("TestBool"), true));

  mock_plugin_->addHostedParameter (
    std::make_unique<juce::AudioParameterChoice> (
      juce::ParameterID{ juce::String ("test-choice") },
      juce::String ("TestChoice"),
      juce::StringArray{ "Option1", "Option2", "Option3" }, 2));

  // Setup state expectations for JUCE plugin state
  juce::MemoryBlock test_state;
  test_state.append ("juce plugin state data", 22);
  EXPECT_CALL (*mock_plugin_, getStateInformation (::testing::_))
    .WillOnce ([&test_state] (juce::MemoryBlock &block) { block = test_state; });

  setupJucePlugin (true);

  bool instantiation_finished = false;
  QObject::connect (
    plugin_.get (), &JucePlugin::instantiationFinished, plugin_.get (),
    [&instantiation_finished] () { instantiation_finished = true; });

  plugin_->set_configuration (*config_);

  while (!instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }

  // Set parameter values
  auto params = plugin_->get_parameters ();
  for (const auto &param : params)
    {
      auto * p = param.get_object_as<dsp::ProcessorParameter> ();
      if (p->label () == "TestFloat")
        {
          p->setBaseValue (0.7f);
        }
      else if (p->label () == "TestInt")
        {
          p->setBaseValue (0.75f); // Should map to 75
        }
      else if (p->label () == "TestBool")
        {
          p->setBaseValue (0.0f); // Should map to false
        }
      else if (p->label () == "TestChoice")
        {
          p->setBaseValue (1.0f); // Should map to Option2
        }
    }

  // Serialize
  nlohmann::json json;
  to_json (json, *plugin_);

  // Create a mock for deserialization
  auto mock = std::make_unique<MockAudioPluginInstance> ();
  EXPECT_CALL (*mock, getName ())
    .WillRepeatedly (::testing::Return ("Test Plugin"));
  EXPECT_CALL (*mock, releaseResources ()).Times (1);
  EXPECT_CALL (*mock, setStateInformation (::testing::_, 22)).Times (1);

  // Create new plugin instance for deserialization
  auto deserialized_plugin = std::make_unique<JucePlugin> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    [] () { return fs::path{ "/tmp/test_state" }; },
    [&mock] (
      const juce::PluginDescription &, double, int,
      std::function<void (
        std::unique_ptr<juce::AudioPluginInstance>, const juce::String &)>
        callback) { callback (std::move (mock), ""); },
    [this] () { return sample_rate_; }, [this] () { return buffer_size_; },
    createMockTopLevelWindowProvider ());

  bool deserialized_plugin_instantiation_finished = false;
  QObject::connect (
    deserialized_plugin.get (), &JucePlugin::instantiationFinished,
    deserialized_plugin.get (), [&deserialized_plugin_instantiation_finished] () {
      deserialized_plugin_instantiation_finished = true;
    });

  // Deserialize
  from_json (json, *deserialized_plugin);

  // Verify parameter values were preserved
  auto deserialized_params = deserialized_plugin->get_parameters ();
  bool found_float = false, found_int = false, found_bool = false,
       found_choice = false;

  for (const auto &param : deserialized_params)
    {
      auto * p = param.get_object_as<dsp::ProcessorParameter> ();
      auto   label = p->label ().toStdString ();

      if (label == "TestFloat")
        {
          found_float = true;
          EXPECT_FLOAT_EQ (p->baseValue (), 0.7f);
        }
      else if (label == "TestInt")
        {
          found_int = true;
          EXPECT_FLOAT_EQ (p->baseValue (), 0.75f);
        }
      else if (label == "TestBool")
        {
          found_bool = true;
          EXPECT_FLOAT_EQ (p->baseValue (), 0.0f);
        }
      else if (label == "TestChoice")
        {
          found_choice = true;
          EXPECT_FLOAT_EQ (p->baseValue (), 1.0f);
        }
    }

  EXPECT_TRUE (found_float);
  EXPECT_TRUE (found_int);
  EXPECT_TRUE (found_bool);
  EXPECT_TRUE (found_choice);

  // Wait for instantiation so we can verify the JUCE state is restored
  while (!deserialized_plugin_instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }
}

TEST_F (JucePluginTest, AudioSignalPassThrough)
{
  createTestConfiguration ();
  setupMockPlugin ();

  // Keep track of juce plugin processor to be used later
  auto * mock_plugin = mock_plugin_.get ();

  // Setup async callback expectation
  setupJucePlugin (true);

  bool instantiation_finished = false;
  QObject::connect (
    plugin_.get (), &JucePlugin::instantiationFinished, plugin_.get (),
    [&instantiation_finished] () { instantiation_finished = true; });

  plugin_->set_configuration (*config_);

  while (!instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }

  plugin_->prepare_for_processing (sample_rate_, buffer_size_);

  // Create test audio signals
  const size_t       test_buffer_size = buffer_size_;
  std::vector<float> left_input (test_buffer_size);
  std::vector<float> right_input (test_buffer_size);

  // Fill with test pattern (sine wave on left, cosine on right)
  for (size_t i = 0; i < test_buffer_size; ++i)
    {
      float phase =
        2.0f * std::numbers::pi_v<float>
        * static_cast<float> (i) / static_cast<float> (test_buffer_size);
      left_input[i] = std::sin (phase);
      right_input[i] = std::cos (phase);
    }

  // Get audio ports
  const auto &in_ports = plugin_->get_input_ports ();
  const auto &out_ports = plugin_->get_output_ports ();

  auto * left_in = in_ports.at (0).get_object_as<dsp::AudioPort> ();
  auto * right_in = in_ports.at (1).get_object_as<dsp::AudioPort> ();
  auto * left_out = out_ports.at (0).get_object_as<dsp::AudioPort> ();
  auto * right_out = out_ports.at (1).get_object_as<dsp::AudioPort> ();

  // Copy input signals to ports
  std::ranges::copy (left_input, left_in->buf_.begin ());
  std::ranges::copy (right_input, right_in->buf_.begin ());

  // Setup mock to verify input and simulate pass-through
  EXPECT_CALL (*mock_plugin, processBlock (::testing::_, ::testing::_))
    .WillOnce (
      [&left_input, &right_input, test_buffer_size] (
        juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi) {
        // Verify input signals are correctly passed
        EXPECT_EQ (buffer.getNumChannels (), 2);
        EXPECT_EQ (buffer.getNumSamples (), static_cast<int> (test_buffer_size));

        // Check left channel
        const float * left_channel = buffer.getReadPointer (0);
        for (size_t i = 0; i < test_buffer_size; ++i)
          {
            EXPECT_NEAR (left_channel[i], left_input[i], 1e-6f);
          }

        // Check right channel
        const float * right_channel = buffer.getReadPointer (1);
        for (size_t i = 0; i < test_buffer_size; ++i)
          {
            EXPECT_NEAR (right_channel[i], right_input[i], 1e-6f);
          }

        // Simulate unity gain pass-through
        float * inner_left_out = buffer.getWritePointer (0);
        float * inner_right_out = buffer.getWritePointer (1);
        for (size_t i = 0; i < test_buffer_size; ++i)
          {
            inner_left_out[i] = left_channel[i];
            inner_right_out[i] = right_channel[i];
          }
      });

  // Process the block
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = static_cast<nframes_t> (test_buffer_size)
  };

  plugin_->process_block (time_nfo);

  // Verify output signals match input (unity gain pass-through)
  for (size_t i = 0; i < test_buffer_size; ++i)
    {
      EXPECT_NEAR (left_out->buf_[i], left_input[i], 1e-6f);
      EXPECT_NEAR (right_out->buf_[i], right_input[i], 1e-6f);
    }
}

TEST_F (JucePluginTest, AudioSignalSplitCycles)
{
  createTestConfiguration ();
  setupMockPlugin ();

  // Keep track of juce plugin processor to be used later
  auto * mock_plugin = mock_plugin_.get ();

  // Setup async callback expectation
  setupJucePlugin (true);

  bool instantiation_finished = false;
  QObject::connect (
    plugin_.get (), &JucePlugin::instantiationFinished, plugin_.get (),
    [&instantiation_finished] () { instantiation_finished = true; });

  plugin_->set_configuration (*config_);

  while (!instantiation_finished)
    {
      QCoreApplication::processEvents ();
    }

  plugin_->prepare_for_processing (sample_rate_, buffer_size_);

  // Create test audio signals for the full buffer
  const size_t       full_buffer_size = buffer_size_;
  std::vector<float> left_input (full_buffer_size);
  std::vector<float> right_input (full_buffer_size);

  // Test split cycle processing with offset and smaller frame count
  constexpr size_t split_offset = 256;
  constexpr size_t split_frames = 512;

  // Fill with test pattern (sine wave on left, cosine on right)
  for (size_t i = split_offset; i < split_offset + split_frames; ++i)
    {
      float phase =
        2.0f * std::numbers::pi_v<float>
        * static_cast<float> (i) / static_cast<float> (full_buffer_size);
      left_input[i] = std::sin (phase);
      right_input[i] = std::cos (phase);
    }

  // Get audio ports
  const auto &in_ports = plugin_->get_input_ports ();
  const auto &out_ports = plugin_->get_output_ports ();

  auto * left_in = in_ports.at (0).get_object_as<dsp::AudioPort> ();
  auto * right_in = in_ports.at (1).get_object_as<dsp::AudioPort> ();
  auto * left_out = out_ports.at (0).get_object_as<dsp::AudioPort> ();
  auto * right_out = out_ports.at (1).get_object_as<dsp::AudioPort> ();

  // Copy input signals to ports
  std::ranges::copy (left_input, left_in->buf_.begin ());
  std::ranges::copy (right_input, right_in->buf_.begin ());

  // Setup mock to verify split cycle processing
  EXPECT_CALL (*mock_plugin, processBlock (::testing::_, ::testing::_))
    .WillOnce (
      [&left_input,
       &right_input] (juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi) {
        // Verify the buffer contains the correct subset of samples
        EXPECT_EQ (buffer.getNumChannels (), 2);
        EXPECT_EQ (buffer.getNumSamples (), static_cast<int> (split_frames));

        // Check that the input signals are correctly offset
        const float * left_channel = buffer.getReadPointer (0);
        const float * right_channel = buffer.getReadPointer (1);

        for (size_t i = 0; i < split_frames; ++i)
          {
            size_t input_index = split_offset + i;
            EXPECT_NEAR (left_channel[i], left_input[input_index], 1e-6f);
            EXPECT_NEAR (right_channel[i], right_input[input_index], 1e-6f);
          }

        // Simulate processing with unity gain
        float * inner_left_out = buffer.getWritePointer (0);
        float * inner_right_out = buffer.getWritePointer (1);
        for (size_t i = 0; i < split_frames; ++i)
          {
            inner_left_out[i] = left_channel[i];
            inner_right_out[i] = right_channel[i];
          }
      });

  // Process with split cycle parameters
  EngineProcessTimeInfo split_time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = split_offset,
    .local_offset_ = split_offset,
    .nframes_ = static_cast<nframes_t> (split_frames)
  };

  plugin_->process_block (split_time_nfo);

  // Verify output signals match input for the processed range
  for (size_t i = 0; i < split_frames; ++i)
    {
      size_t output_index = split_offset + i;
      EXPECT_NEAR (
        left_out->buf_[output_index], left_input[output_index], 1e-6f);
      EXPECT_NEAR (
        right_out->buf_[output_index], right_input[output_index], 1e-6f);
    }

  // Verify that unprocessed regions remain unchanged (or zeroed, depending on
  // implementation) The regions before and after the split should not be modified
  for (size_t i = 0; i < split_offset; ++i)
    {
      // These should remain as they were (input values)
      EXPECT_NEAR (left_out->buf_[i], left_input[i], 1e-6f);
      EXPECT_NEAR (right_out->buf_[i], right_input[i], 1e-6f);
    }

  for (size_t i = split_offset + split_frames; i < full_buffer_size; ++i)
    {
      // These should remain as they were (input values)
      EXPECT_NEAR (left_out->buf_[i], left_input[i], 1e-6f);
      EXPECT_NEAR (right_out->buf_[i], right_input[i], 1e-6f);
    }
}

} // namespace zrythm::plugins
