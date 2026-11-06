// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/modulators/engines/macro_engine.h"
#include "plugins/modulators/modulator.h"
#include "plugins/plugin_configuration.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/scoped_qcoreapplication.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::plugins::modulators
{

namespace
{

constexpr auto kSampleRate = units::sample_rate (44100);
constexpr auto kBlockLength = units::samples (256);

std::unique_ptr<PluginConfiguration>
macro_configuration ()
{
  return PluginConfiguration::create_new_for_descriptor (
    *Modulator::make_descriptor (MacroEngine::kDescriptorId));
}

dsp::ProcessorParameter *
find_parameter (dsp::ProcessorBase &processor, std::string_view unique_id)
{
  const auto found = std::ranges::find_if (
    processor.get_parameters (), [unique_id] (const auto &param_ref) {
      auto * param = param_ref.get ();
      return param != nullptr
             && param->get_unique_id ()
                  == dsp::ProcessorParameter::UniqueId (
                    utils::Utf8String::from_utf8_encoded_string (unique_id));
    });
  return found != processor.get_parameters ().end () ? found->get () : nullptr;
}

dsp::CVPort *
find_cv_port (
  dsp::ProcessorBase &processor,
  dsp::PortFlow       flow,
  std::string_view    symbol)
{
  auto &ports =
    flow == dsp::PortFlow::Input
      ? processor.get_all_input_ports ()
      : processor.get_all_output_ports ();
  const auto found =
    std::ranges::find_if (ports, [symbol] (const auto &port_ref) {
      auto * port = port_ref.get ();
      return port != nullptr
             && port->get_symbol ()
                  == utils::Utf8String::from_utf8_encoded_string (symbol);
    });
  return found != ports.end () ? qobject_cast<dsp::CVPort *> (found->get ()) : nullptr;
}

} // namespace

class ModulatorMacroTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    plugin_ = std::make_unique<Modulator> (*registry_);
    plugin_->set_configuration (*macro_configuration ());

    cv_in_ =
      find_cv_port (*plugin_, dsp::PortFlow::Input, MacroEngine::kInputSymbol);
    cv_out_ = find_cv_port (
      *plugin_, dsp::PortFlow::Output, MacroEngine::kOutputSymbol);
    macro_param_ = find_parameter (*plugin_, MacroEngine::kParameterId);
    ASSERT_NE (cv_in_, nullptr);
    ASSERT_NE (cv_out_, nullptr);
    ASSERT_NE (macro_param_, nullptr);

    mod_source_ref_ = utils::create_object<dsp::CVPort> (
      *registry_, u8"LFO", dsp::PortFlow::Output);
    mod_source_ = mod_source_ref_->get ();
    plugin_->prepare_for_processing (nullptr, kSampleRate, kBlockLength);
    mod_source_->prepare_for_processing (nullptr, kSampleRate, kBlockLength);
    std::ranges::fill (mod_source_->buf_, 0.5f);

    transport_ = std::make_unique<dsp::graph_test::MockTransport> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (kSampleRate);
  }

  void TearDown () override
  {
    plugin_->release_resources ();
    mod_source_->release_resources ();
    plugin_.reset ();
    mod_source_ref_.reset ();
    registry_.reset ();
  }

  dsp::graph::ProcessBlockInfo block_info () const
  {
    return dsp::graph::ProcessBlockInfo::from_position_and_nframes (
      units::samples (0), kBlockLength);
  }

  void process_with_cv_input ()
  {
    const auto time_info = block_info ();
    cv_in_->process_block (time_info, *transport_, *tempo_map_);
    plugin_->process_block (time_info, *transport_, *tempo_map_);
  }

  void expect_output (float expected) const
  {
    for (
      const auto i :
      std::views::iota (size_t{ 0 }, kBlockLength.in<size_t> (units::samples)))
      {
        EXPECT_FLOAT_EQ (cv_out_->buf_[i], expected);
      }
  }

  std::unique_ptr<utils::ObjectRegistry>                registry_;
  std::unique_ptr<Modulator>                            plugin_;
  dsp::CVPort *                                         cv_in_{};
  dsp::CVPort *                                         cv_out_{};
  dsp::ProcessorParameter *                             macro_param_{};
  std::optional<utils::TypedUuidReference<dsp::CVPort>> mod_source_ref_;
  dsp::CVPort *                                         mod_source_{};
  std::unique_ptr<dsp::graph_test::MockTransport>       transport_;
  std::unique_ptr<dsp::TempoMap>                        tempo_map_;
};

TEST_F (ModulatorMacroTest, DescriptorIdentifiesNativeMacroModulator)
{
  const auto descriptor =
    Modulator::make_descriptor (MacroEngine::kDescriptorId);
  ASSERT_NE (descriptor, nullptr);
  EXPECT_EQ (descriptor->protocol_, Protocol::ProtocolType::Internal);
  EXPECT_EQ (
    std::get<utils::Utf8String> (descriptor->path_or_id_).view (),
    MacroEngine::kDescriptorId);
  EXPECT_EQ (descriptor->num_cv_ins_, 1);
  EXPECT_EQ (descriptor->num_cv_outs_, 1);
  EXPECT_TRUE (descriptor->isModulator ());
  EXPECT_TRUE (Modulator::descriptor_is_native_modulator (*descriptor));
  EXPECT_EQ (Modulator::make_descriptor ("not-a-modulator"), nullptr);

  const auto ids = Modulator::available_descriptor_ids ();
  EXPECT_EQ (std::ranges::count (ids, MacroEngine::kDescriptorId), 1);
}

TEST_F (ModulatorMacroTest, CreatesMacroLayout)
{
  EXPECT_EQ (plugin_->get_name (), u8"Macro");
  EXPECT_EQ (plugin_->get_all_input_ports ().size (), 1u);
  EXPECT_EQ (plugin_->get_all_output_ports ().size (), 1u);
  EXPECT_EQ (plugin_->get_parameters ().size (), 3u);
  EXPECT_EQ (cv_in_->get_label (), u8"CV In");
  EXPECT_EQ (cv_out_->get_label (), u8"CV Out");
  EXPECT_EQ (macro_param_->label (), u8"Macro");
  EXPECT_FLOAT_EQ (macro_param_->baseValue (), 0.75f);
}

TEST_F (ModulatorMacroTest, EmitsMacroValueWithoutInput)
{
  macro_param_->setBaseValue (0.8f);
  plugin_->process_block (block_info (), *transport_, *tempo_map_);
  expect_output (0.8f);
}

TEST_F (ModulatorMacroTest, ScalesConnectedInput)
{
  const std::array sources{
    dsp::graph::PortSourceConfig<dsp::CVPort>{ mod_source_, std::nullopt }
  };
  cv_in_->set_port_sources (sources);
  macro_param_->setBaseValue (0.5f);

  process_with_cv_input ();

  expect_output (0.25f);
}

TEST_F (ModulatorMacroTest, ScalesSummedInputs)
{
  std::vector<dsp::graph::PortSourceConfig<dsp::CVPort>> sources{
    { mod_source_, std::nullopt }
  };
  auto second_source_ref = utils::create_object<dsp::CVPort> (
    *registry_, u8"LFO 2", dsp::PortFlow::Output);
  auto * second_source = second_source_ref.get ();
  second_source->prepare_for_processing (nullptr, kSampleRate, kBlockLength);
  std::ranges::fill (second_source->buf_, 0.25f);
  sources.emplace_back (second_source, std::nullopt);
  cv_in_->set_port_sources (sources);
  macro_param_->setBaseValue (0.5f);

  process_with_cv_input ();

  expect_output (0.375f);
  second_source->release_resources ();
}

TEST_F (ModulatorMacroTest, ZeroMacroSilencesConnectedInput)
{
  const std::array sources{
    dsp::graph::PortSourceConfig<dsp::CVPort>{ mod_source_, std::nullopt }
  };
  cv_in_->set_port_sources (sources);
  macro_param_->setBaseValue (0.f);

  process_with_cv_input ();

  expect_output (0.f);
}

TEST_F (ModulatorMacroTest, BasePluginSerializationRestoresMacro)
{
  macro_param_->setBaseValue (0.42f);
  nlohmann::json json;
  to_json (json, static_cast<const Plugin &> (*plugin_));

  auto restored = std::make_unique<Modulator> (*registry_);
  from_json (json, static_cast<Plugin &> (*restored));

  EXPECT_EQ (restored->get_uuid (), plugin_->get_uuid ());
  EXPECT_EQ (restored->get_all_input_ports ().size (), 1u);
  EXPECT_EQ (restored->get_all_output_ports ().size (), 1u);
  EXPECT_EQ (restored->get_parameters ().size (), 3u);
  auto * restored_param = find_parameter (*restored, MacroEngine::kParameterId);
  ASSERT_NE (restored_param, nullptr);
  EXPECT_FLOAT_EQ (restored_param->baseValue (), 0.42f);

  auto * restored_output =
    find_cv_port (*restored, dsp::PortFlow::Output, MacroEngine::kOutputSymbol);
  ASSERT_NE (restored_output, nullptr);
  restored->prepare_for_processing (nullptr, kSampleRate, kBlockLength);
  restored->process_block (block_info (), *transport_, *tempo_map_);
  for (
    const auto i :
    std::views::iota (size_t{ 0 }, kBlockLength.in<size_t> (units::samples)))
    {
      EXPECT_FLOAT_EQ (restored_output->buf_[i], 0.42f);
    }
  restored->release_resources ();
}

// A saved layout that no longer matches its descriptor must leave the
// restored modulator inert: preparation and processing become no-ops
// instead of running an engine that was never prepared
TEST_F (ModulatorMacroTest, LayoutMismatchOnRestoreLeavesModulatorInert)
{
  nlohmann::json json;
  to_json (json, static_cast<const Plugin &> (*plugin_));
  json["parameters"] = nlohmann::json::array ();

  auto restored = std::make_unique<Modulator> (*registry_);
  from_json (json, static_cast<Plugin &> (*restored));

  restored->prepare_for_processing (nullptr, kSampleRate, kBlockLength);
  auto * restored_output =
    find_cv_port (*restored, dsp::PortFlow::Output, MacroEngine::kOutputSymbol);
  ASSERT_NE (restored_output, nullptr);
  restored->process_block (block_info (), *transport_, *tempo_map_);
  for (
    const auto i :
    std::views::iota (size_t{ 0 }, kBlockLength.in<size_t> (units::samples)))
    {
      EXPECT_FLOAT_EQ (restored_output->buf_[i], 0.f);
    }
  restored->release_resources ();
}

} // namespace zrythm::plugins::modulators
