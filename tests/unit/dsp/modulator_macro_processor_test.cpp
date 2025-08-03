// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/modulator_macro_processor.h"
#include "dsp/port.h"
#include "dsp/port_connection.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

class ModulatorMacroProcessorTest : public ::testing::Test
{
protected:
  static constexpr sample_rate_t SAMPLE_RATE = 44100;
  static constexpr nframes_t     BLOCK_LENGTH = 256;

  void SetUp () override
  {
    // Create macro processor
    macro_processor = std::make_unique<ModulatorMacroProcessor> (
      ModulatorMacroProcessor::ProcessorBaseDependencies{
        .port_registry_ = port_registry, .param_registry_ = param_registry },
      0, nullptr);

    // Get references to ports
    cv_in = &macro_processor->get_cv_in_port ();
    cv_out = &macro_processor->get_cv_out_port ();
    macro_param = &macro_processor->get_macro_param ();

    // Create modulation source
    mod_source_ref = port_registry.create_object<CVPort> (
      utils::Utf8String::from_utf8_encoded_string ("LFO"), PortFlow::Output);
    mod_source = mod_source_ref->get_object_as<CVPort> ();

    // Prepare for processing
    macro_processor->prepare_for_processing (SAMPLE_RATE, BLOCK_LENGTH);
    mod_source->prepare_for_processing (SAMPLE_RATE, BLOCK_LENGTH);
    macro_param->prepare_for_processing (SAMPLE_RATE, BLOCK_LENGTH);

    // Set up test signal
    for (nframes_t i = 0; i < BLOCK_LENGTH; i++)
      {
        mod_source->buf_[i] = 0.5f;
      }
  }

  dsp::PortRegistry                        port_registry;
  dsp::ProcessorParameterRegistry          param_registry{ port_registry };
  std::unique_ptr<ModulatorMacroProcessor> macro_processor;

  // Port references
  CVPort *             cv_in{};
  CVPort *             cv_out{};
  ProcessorParameter * macro_param{};

  // Modulation source
  std::optional<PortUuidReference> mod_source_ref;
  CVPort *                         mod_source{};
};

TEST_F (ModulatorMacroProcessorTest, BasicConstruction)
{
  EXPECT_EQ (macro_processor->get_name ().view (), "Macro 1");
  EXPECT_EQ (cv_in->get_label ().view (), "Macro 1 CV In");
  EXPECT_EQ (cv_out->get_label ().view (), "Macro 1 CV Out");
  EXPECT_EQ (macro_param->label ().toStdString (), "Macro 1");
}

TEST_F (ModulatorMacroProcessorTest, ProcessBlockNoInput)
{
  // Set macro parameter value
  macro_param->setBaseValue (0.8f);

  // Process block
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = BLOCK_LENGTH
  };
  macro_processor->process_block (time_nfo);

  // Verify output is macro value (0.8f) since no input
  for (nframes_t i = 0; i < BLOCK_LENGTH; i++)
    {
      EXPECT_FLOAT_EQ (cv_out->buf_[i], 0.8f);
    }
}

TEST_F (ModulatorMacroProcessorTest, ProcessBlockWithInput)
{
  // Create connection between mod source and CV input
  auto connection = std::make_unique<PortConnection> (
    mod_source->get_uuid (), cv_in->get_uuid (), 1.0f, false, true);
  cv_in->port_sources_.emplace_back (mod_source, std::move (connection));

  // Set macro parameter value
  macro_param->setBaseValue (0.5f);

  // Process block
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = BLOCK_LENGTH
  };
  cv_in->process_block (time_nfo);
  macro_processor->process_block (time_nfo);

  // Verify output is input * macro (0.5 * 0.5 = 0.25)
  for (nframes_t i = 0; i < BLOCK_LENGTH; i++)
    {
      EXPECT_FLOAT_EQ (cv_out->buf_[i], 0.25f);
    }
}

TEST_F (ModulatorMacroProcessorTest, ProcessBlockMultipleInputs)
{
  // Create first connection
  auto connection1 = std::make_unique<PortConnection> (
    mod_source->get_uuid (), cv_in->get_uuid (), 1.0f, false, true);
  cv_in->port_sources_.emplace_back (mod_source, std::move (connection1));

  // Create second modulation source
  auto mod_source2_ref = port_registry.create_object<CVPort> (
    utils::Utf8String::from_utf8_encoded_string ("LFO2"), PortFlow::Output);
  auto mod_source2 = mod_source2_ref.get_object_as<CVPort> ();
  mod_source2->prepare_for_processing (SAMPLE_RATE, BLOCK_LENGTH);
  for (nframes_t i = 0; i < BLOCK_LENGTH; i++)
    {
      mod_source2->buf_[i] = 0.25f;
    }

  // Create second connection
  auto connection2 = std::make_unique<PortConnection> (
    mod_source2->get_uuid (), cv_in->get_uuid (), 1.0f, false, true);
  cv_in->port_sources_.emplace_back (mod_source2, std::move (connection2));

  // Set macro parameter value
  macro_param->setBaseValue (0.5f);

  // Process block
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = BLOCK_LENGTH
  };
  cv_in->process_block (time_nfo);
  macro_processor->process_block (time_nfo);

  // Verify output is (input1 + input2) * macro
  // (0.5 + 0.25) * 0.5 = 0.375
  for (nframes_t i = 0; i < BLOCK_LENGTH; i++)
    {
      EXPECT_FLOAT_EQ (cv_out->buf_[i], 0.375f);
    }
}

TEST_F (ModulatorMacroProcessorTest, ProcessBlockZeroMacro)
{
  // Create connection
  auto connection = std::make_unique<PortConnection> (
    mod_source->get_uuid (), cv_in->get_uuid (), 1.0f, false, true);
  cv_in->port_sources_.emplace_back (mod_source, std::move (connection));

  // Set macro to zero
  macro_param->setBaseValue (0.0f);

  // Process block
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = BLOCK_LENGTH
  };
  macro_processor->process_block (time_nfo);

  // Verify output is zero
  for (nframes_t i = 0; i < BLOCK_LENGTH; i++)
    {
      EXPECT_FLOAT_EQ (cv_out->buf_[i], 0.0f);
    }
}

TEST_F (ModulatorMacroProcessorTest, SerializationRoundTrip)
{
  // Serialize
  nlohmann::json j;
  to_json (j, *macro_processor);

  // Create new processor from JSON
  ModulatorMacroProcessor new_processor (
    ModulatorMacroProcessor::ProcessorBaseDependencies{
      .port_registry_ = port_registry, .param_registry_ = param_registry },
    3, nullptr);
  from_json (j, new_processor);

  // Verify properties match original
  EXPECT_EQ (
    new_processor.get_cv_in_port ().get_uuid (),
    macro_processor->get_cv_in_port ().get_uuid ());
  EXPECT_EQ (
    new_processor.get_macro_param ().get_uuid (),
    macro_processor->get_macro_param ().get_uuid ());
  EXPECT_EQ (new_processor.get_name ().view (), "Macro 1");
  EXPECT_EQ (
    new_processor.get_cv_in_port ().get_label ().view (), "Macro 1 CV In");
  EXPECT_EQ (
    new_processor.get_macro_param ().label ().toStdString (), "Macro 1");
}

TEST_F (ModulatorMacroProcessorTest, PortDesignationProvider)
{
  auto designation = macro_processor->get_full_designation_for_port (*cv_in);
  EXPECT_EQ (designation.view (), "Modulator Macro Processor/Macro 1 CV In");
}

// TODO
#if 0
TEST_F (ModulatorMacroProcessorTest, InitFromClone)
{
  // Create original processor
  auto orig = std::make_unique<ModulatorMacroProcessor> (
    port_registry, param_registry, 0, nullptr);

  // Create clone
  ModulatorMacroProcessor clone (port_registry, param_registry, 1, nullptr);
  init_from (clone, *orig, utils::ObjectCloneType::NewIdentity);

  // Verify properties match
  EXPECT_EQ (clone.get_name ().view (), orig->get_name ().view ());
  EXPECT_EQ (
    clone.get_cv_in_port ().get_label ().view (),
    orig->get_cv_in_port ().get_label ().view ());
}
#endif

} // namespace zrythm::dsp
