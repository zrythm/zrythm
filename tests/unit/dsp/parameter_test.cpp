// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <numbers>

#include "dsp/cv_port.h"
#include "dsp/parameter.h"
#include "dsp/port_connection.h"

#include <QSignalSpy>

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::dsp
{

class ProcessorParameterTest : public ::testing::Test
{
protected:
  static constexpr auto      SAMPLE_RATE = units::sample_rate (44100);
  static constexpr nframes_t BLOCK_LENGTH = 256;

  void SetUp () override
  {
    // Create parameter with new constructor
    param_ref = param_registry.create_object<ProcessorParameter> (
      port_registry,
      ProcessorParameter::UniqueId (
        utils::Utf8String::from_utf8_encoded_string ("Cutoff")),
      ParameterRange (
        ParameterRange::Type::Logarithmic, 20.f, 20000.f, 20.f, 1000.f),
      utils::Utf8String::from_utf8_encoded_string ("Cutoff"));
    param = param_ref->get_object_as<ProcessorParameter> ();
    param_mod_input =
      param->get_modulation_input_port_ref ().get_object_as<CVPort> ();

    // Create modulation source CV port
    mod_source_ref = port_registry.create_object<CVPort> (
      utils::Utf8String::from_utf8_encoded_string ("LFO"), PortFlow::Output);
    mod_source = mod_source_ref->get_object_as<CVPort> ();

    // Set up processing
    param->prepare_for_processing (nullptr, SAMPLE_RATE, BLOCK_LENGTH);
    mod_source->prepare_for_processing (nullptr, SAMPLE_RATE, BLOCK_LENGTH);

    // Create connection between CV port and parameter
    mod_sources.push_back (mod_source);
    param_mod_input->set_port_sources (mod_sources);

    // Fill modulation source with test signal
    for (nframes_t i = 0; i < BLOCK_LENGTH; i++)
      {
        mod_source->buf_[i] =
          0.5f
          * std::cos (
            2.0f * std::numbers::pi_v<float>
            * static_cast<float> (i) / BLOCK_LENGTH);
      }

    // Set up mock transport
    mock_transport_ = std::make_unique<graph_test::MockTransport> ();
  }

  dsp::PortRegistry               port_registry;
  dsp::ProcessorParameterRegistry param_registry{ port_registry };
  std::optional<ProcessorParameterUuidReference> param_ref;
  ProcessorParameter *                           param;
  CVPort *                                       param_mod_input{};
  std::optional<PortUuidReference>               mod_source_ref;
  CVPort *                                       mod_source{};
  std::vector<CVPort *>                          mod_sources;
  std::unique_ptr<graph_test::MockTransport>     mock_transport_;
};

TEST_F (ProcessorParameterTest, BasicParameterSetup)
{
  EXPECT_EQ (param->baseValue (), param->range ().convertTo0To1 (1000.f));
}

TEST_F (ProcessorParameterTest, AutomationValueApplication)
{
  // disable modulation
  param_mod_input->port_sources ().front ().second->enabled_ = false;

  const float auto_value = 0.8f;
  param->set_automation_provider ([&] (auto) {
    return std::optional{ auto_value };
  });

  param->process_block (
    { .g_start_frame_ = 0,
      .g_start_frame_w_offset_ = 0,
      .local_offset_ = 0,
      .nframes_ = BLOCK_LENGTH },
    *mock_transport_);

  EXPECT_FLOAT_EQ (param->currentValue (), auto_value);
  EXPECT_FLOAT_EQ (param->valueAfterAutomationApplied (), auto_value);
}

TEST_F (ProcessorParameterTest, ModulationApplication)
{
  // Enable modulation
  param_mod_input->port_sources ().front ().second->enabled_ = true;

  // Set base value to 0.5 (normalized)
  param->setBaseValue (0.5f);

  // Set modulation multiplier to 1.0 and bipolar to false
  auto &conn = param_mod_input->port_sources ().front ().second;
  conn->multiplier_ = 1.0f;
  conn->bipolar_ = false;

  // Process block with modulation
  param->process_block (
    { .g_start_frame_ = 0,
      .g_start_frame_w_offset_ = 0,
      .local_offset_ = 0,
      .nframes_ = BLOCK_LENGTH },
    *mock_transport_);

  // Verify modulation applied (0.5 base + 0.5 * 1.0 = 1.0)
  EXPECT_NEAR (param->currentValue (), 1.0f, 1e-5f);
}

TEST_F (ProcessorParameterTest, ModulationWithMultiplier)
{
  param_mod_input->port_sources ().front ().second->enabled_ = true;
  param->setBaseValue (0.5f);

  // Set modulation multiplier to 0.5
  auto &conn = param_mod_input->port_sources ().front ().second;
  conn->multiplier_ = 0.5f;
  conn->bipolar_ = false;

  param->process_block (
    { .g_start_frame_ = 0,
      .g_start_frame_w_offset_ = 0,
      .local_offset_ = 0,
      .nframes_ = BLOCK_LENGTH },
    *mock_transport_);

  // Verify scaled modulation (0.5 base + 0.5 * 0.5 = 0.75)
  EXPECT_NEAR (param->currentValue (), 0.75f, 1e-5f);
}

TEST_F (ProcessorParameterTest, BipolarModulation)
{
  param_mod_input->port_sources ().front ().second->enabled_ = true;
  param->setBaseValue (0.5f);

  // Set bipolar modulation
  auto &conn = param_mod_input->port_sources ().front ().second;
  conn->multiplier_ = 1.0f;
  conn->bipolar_ = true;

  param->process_block (
    { .g_start_frame_ = 0,
      .g_start_frame_w_offset_ = 0,
      .local_offset_ = 0,
      .nframes_ = BLOCK_LENGTH },
    *mock_transport_);

  // 0.5 CV value becomes 0 after bipolar conversion
  // Modulation amount: (0.5 * 2 - 1) * 0.5 = 0
  EXPECT_NEAR (param->currentValue (), 0.5f, 1e-5f);
}

TEST_F (ProcessorParameterTest, ModulationWithAutomation)
{
  // Enable both automation and modulation
  param->set_automation_provider ([&] (auto) {
    return 0.7f; // Automation value
  });
  param_mod_input->port_sources ().front ().second->enabled_ = true;

  auto &conn = param_mod_input->port_sources ().front ().second;
  conn->multiplier_ = 1.0f;
  conn->bipolar_ = false;

  param->process_block (
    { .g_start_frame_ = 0,
      .g_start_frame_w_offset_ = 0,
      .local_offset_ = 0,
      .nframes_ = BLOCK_LENGTH },
    *mock_transport_);

  // Verify automation + modulation (0.7 auto + 0.5 mod = 1.2 -> clamped to 1.0)
  EXPECT_NEAR (param->currentValue (), 1.0f, 1e-5f);
}

TEST_F (ProcessorParameterTest, MultipleModulationSources)
{
  // Create second modulation source
  auto mod_source2_ref = port_registry.create_object<CVPort> (
    utils::Utf8String::from_utf8_encoded_string ("LFO2"), PortFlow::Output);
  auto mod_source2 = mod_source2_ref.get_object_as<CVPort> ();
  mod_source2->prepare_for_processing (nullptr, SAMPLE_RATE, BLOCK_LENGTH);

  // Fill with different signal
  for (nframes_t i = 0; i < BLOCK_LENGTH; i++)
    {
      mod_source2->buf_[i] = 0.25f;
    }

  // Add second connection
  mod_sources.push_back (mod_source2);
  param_mod_input->set_port_sources (mod_sources);
  param_mod_input->port_sources ().back ().second->multiplier_ = 0.5f;

  param->setBaseValue (0.5f);

  // Enable both connections
  param_mod_input->port_sources ()[0].second->enabled_ = true;
  param_mod_input->port_sources ()[1].second->enabled_ = true;

  param->process_block (
    { .g_start_frame_ = 0,
      .g_start_frame_w_offset_ = 0,
      .local_offset_ = 0,
      .nframes_ = BLOCK_LENGTH },
    *mock_transport_);

  // Verify combined modulation:
  // Source1: 0.5 * 1.0 = 0.5
  // Source2: 0.25 * 0.5 = 0.125
  // Total: 0.5 + 0.5 + 0.125 = 1.125 -> clamped to 1.0
  EXPECT_NEAR (param->currentValue (), 1.0f, 1e-5f);
}

TEST_F (ProcessorParameterTest, GestureBlocksAutomation)
{
  param->beginUserGesture ();
  const float initial_value = param->baseValue ();

  param->process_block (
    { .g_start_frame_ = 0,
      .g_start_frame_w_offset_ = 0,
      .local_offset_ = 0,
      .nframes_ = BLOCK_LENGTH },
    *mock_transport_);
  param->endUserGesture ();

  EXPECT_FLOAT_EQ (param->currentValue (), initial_value);
}

TEST_F (ProcessorParameterTest, GestureBlocksModulation)
{
  param->beginUserGesture ();
  param->setBaseValue (0.5f);

  // Enable modulation
  param_mod_input->port_sources ().front ().second->enabled_ = true;

  param->process_block (
    { .g_start_frame_ = 0,
      .g_start_frame_w_offset_ = 0,
      .local_offset_ = 0,
      .nframes_ = BLOCK_LENGTH },
    *mock_transport_);

  // During gesture, modulation should not be applied
  EXPECT_FLOAT_EQ (param->currentValue (), 0.5f);

  param->endUserGesture ();
}

TEST_F (ProcessorParameterTest, SerializationRoundTrip)
{
  // Set non-default value
  const float test_value = 0.75f;
  param->setBaseValue (test_value);

  // Serialize
  nlohmann::json j;
  to_json (j, *param);

  // Create new param from JSON
  auto new_param = std::make_unique<ProcessorParameter> (
    port_registry,
    ProcessorParameter::UniqueId (
      utils::Utf8String::from_utf8_encoded_string ("Cutoff")),
    ParameterRange (
      ParameterRange::Type::Logarithmic, 20.f, 20000.f, 20.f, 1000.f),
    utils::Utf8String::from_utf8_encoded_string ("Cutoff"));
  from_json (j, *new_param);

  EXPECT_FLOAT_EQ (new_param->baseValue (), test_value);
  EXPECT_EQ (
    new_param->get_modulation_input_port_ref ().id (),
    param->get_modulation_input_port_ref ().id ());
}

TEST_F (ProcessorParameterTest, ParameterTypeHandling)
{
  // Test Linear type
  ProcessorParameter linearParam (
    port_registry, ProcessorParameter::UniqueId (u8"linear"),
    ParameterRange (ParameterRange::Type::Linear, 4.f, 8.f), u8"Linear");
  EXPECT_FLOAT_EQ (linearParam.range ().convertFrom0To1 (0.5f), 6.f);

  // Test Logarithmic type
  ProcessorParameter logParam (
    port_registry, ProcessorParameter::UniqueId (u8"log"),
    ParameterRange (ParameterRange::Type::Logarithmic, 20.f, 20000.f), u8"Log");
  EXPECT_NEAR (logParam.range ().convertFrom0To1 (0.8f), 5023.7734f, 0.1f);

  // Test Toggle type
  ProcessorParameter toggleParam (
    port_registry, ProcessorParameter::UniqueId (u8"toggle"),
    ParameterRange (ParameterRange::Type::Toggle, 0.f, 1.f), u8"Toggle");
  EXPECT_FLOAT_EQ (
    toggleParam.range ().convertFrom0To1 (0.5f), 1.f); // Should clamp to 1
}

TEST_F (ProcessorParameterTest, ValueClamping)
{
  param->setBaseValue (1.5f);
  EXPECT_FLOAT_EQ (param->baseValue (), 1.0f);
  param->setBaseValue (-0.5f);
  EXPECT_FLOAT_EQ (param->baseValue (), 0.0f);
}

TEST_F (ProcessorParameterTest, NodeNameVerification)
{
  EXPECT_EQ (param->get_node_name ().view (), "Cutoff");
}

TEST_F (ProcessorParameterTest, PreparationAndRelease)
{
  EXPECT_NO_THROW (
    param->prepare_for_processing (nullptr, SAMPLE_RATE, BLOCK_LENGTH));
  EXPECT_NO_THROW (param->release_resources ());
}

TEST_F (ProcessorParameterTest, SignalEmissions)
{
  QSignalSpy spy (param, &ProcessorParameter::baseValueChanged);
  param->setBaseValue (0.75f);
  EXPECT_EQ (spy.count (), 1);
  EXPECT_FLOAT_EQ (spy.first ()[0].toFloat (), 0.75f);
}

TEST_F (ProcessorParameterTest, RangeClamping)
{
  auto range = ParameterRange (ParameterRange::Type::Linear, 10.f, 20.f);
  EXPECT_FLOAT_EQ (range.clamp_to_range (15.f), 15.f);
  EXPECT_FLOAT_EQ (range.clamp_to_range (25.f), 20.f);
  EXPECT_FLOAT_EQ (range.clamp_to_range (5.f), 10.f);
}

TEST_F (ProcessorParameterTest, UnitConversion)
{
  EXPECT_EQ (
    ParameterRange::unit_to_string (ParameterRange::Unit::Hz).view (), "Hz");
  EXPECT_EQ (
    ParameterRange::unit_to_string (ParameterRange::Unit::Db).view (), "dB");
  EXPECT_EQ (
    ParameterRange::unit_to_string (ParameterRange::Unit::Us).view (), "μs");
}

TEST_F (ProcessorParameterTest, DisabledAutomation)
{
  param->unset_automation_provider ();
  param->process_block ({}, *mock_transport_);
  EXPECT_FLOAT_EQ (param->valueAfterAutomationApplied (), param->baseValue ());
}

TEST_F (ProcessorParameterTest, NoModulationSources)
{
  std::vector<CVPort *> sources;
  param_mod_input->set_port_sources (sources);
  param->setBaseValue (0.5f);
  param->process_block ({}, *mock_transport_);
  EXPECT_FLOAT_EQ (param->currentValue (), 0.5f);
}
TEST_F (ProcessorParameterTest, ParameterRegistryLifecycle)
{
  // Verify initial parameter is registered
  EXPECT_EQ (param_registry.size (), 1);

  // Create second parameter
  auto param2_ref =
    std::make_optional (param_registry.create_object<ProcessorParameter> (
      port_registry, ProcessorParameter::UniqueId (u8"Resonance"),
      ParameterRange (ParameterRange::Type::Linear, 0.f, 1.f), u8"Resonance"));
  EXPECT_EQ (param_registry.size (), 2);

  // Release reference - should deregister
  param2_ref.reset ();
  EXPECT_EQ (param_registry.size (), 1);
}

TEST_F (ProcessorParameterTest, UuidReferenceSerialization)
{
  // Serialize UUID reference
  nlohmann::json j;
  to_json (j, param_ref.value ());

  // Create new reference from JSON
  ProcessorParameterUuidReference new_ref (param_registry);
  from_json (j, new_ref);

  // Verify same object
  EXPECT_EQ (
    new_ref.get_object_as<ProcessorParameter> ()->get_unique_id (),
    param->get_unique_id ());
}

TEST_F (ProcessorParameterTest, RegistryLookupByUniqueId)
{
  // Find by unique ID
  auto * found_param =
    param_registry.find_by_unique_id (param->get_unique_id ());
  EXPECT_NE (found_param, nullptr);
  EXPECT_EQ (found_param, param);

  // Test non-existent ID
  ProcessorParameter::UniqueId non_existent (u8"NonExistent");
  EXPECT_THROW (
    param_registry.find_by_unique_id_or_throw (non_existent),
    std::runtime_error);
}
TEST_F (ProcessorParameterTest, Formatting)
{
  // Test formatting of UniqueId via format_as
  const auto uuid_str = fmt::format ("{}", param->get_unique_id ());
  EXPECT_EQ (uuid_str, "Cutoff");

  // Test formatting of ProcessorParameter using BOOST_DESCRIBE_CLASS
  const auto param_str = fmt::format ("{}", *param);
  EXPECT_NE (
    param_str.find (
      fmt::format (
        "uuid_={}",
        type_safe::get (param->get_uuid ()).toString (QUuid::WithoutBraces))),
    std::string::npos);
  EXPECT_NE (param_str.find ("label_=Cutoff"), std::string::npos);
}
} // namespace zrythm::dsp
