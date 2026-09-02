// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/parameter.h"
#include "plugins/plugin_parameter_list_model.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::plugins
{

class FakeParamPlugin : public Plugin
{
public:
  FakeParamPlugin (utils::IObjectRegistry &registry, QObject * parent = nullptr)
      : Plugin (registry, parent)
  {
    auto bypass_ref = generate_default_bypass_param ();
    add_parameter (bypass_ref);
    set_bypass_id (bypass_ref.id ());
    auto gain_ref = generate_default_gain_param ();
    add_parameter (gain_ref);
    gain_id_ = gain_ref.id ();
  }

private:
  void process_impl (
    dsp::graph::ProcessBlockInfo,
    const dsp::ITransport &,
    const dsp::TempoMap &) noexcept override
  {
  }
  std::string save_state_impl () const override { return {}; }
  bool        load_state_impl (const std::string &) override { return true; }
};

class PluginParameterListModelTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    plugin_ = std::make_unique<FakeParamPlugin> (*registry_);
  }

  dsp::ProcessorParameter * add_param (
    const utils::Utf8String       &label,
    std::vector<utils::Utf8String> group_path)
  {
    auto param_ref = utils::create_object<dsp::ProcessorParameter> (
      *registry_, *registry_, dsp::ProcessorParameter::UniqueId (label),
      dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.f, 1.f), label);
    auto * param = param_ref.get_object_as<dsp::ProcessorParameter> ();
    param->set_group_path (std::move (group_path));
    plugin_->add_parameter (param_ref);
    return param;
  }

  dsp::ProcessorParameter * param_at (int row)
  {
    return model_.data (model_.index (row), PluginParameterListModel::ParamRole)
      .value<dsp::ProcessorParameter *> ();
  }

  std::unique_ptr<utils::ObjectRegistry> registry_;
  std::unique_ptr<FakeParamPlugin>       plugin_;
  PluginParameterListModel               model_;
};

TEST_F (
  PluginParameterListModelTest,
  OrdersUngroupedFirstThenGroupsByFirstAppearance)
{
  auto * osc_a = add_param (
    utils::Utf8String::from_utf8_encoded_string ("OscA"),
    { utils::Utf8String::from_utf8_encoded_string ("Osc") });
  auto * ungrouped =
    add_param (utils::Utf8String::from_utf8_encoded_string ("Ungrouped"), {});
  auto * filter = add_param (
    utils::Utf8String::from_utf8_encoded_string ("Filter"),
    { utils::Utf8String::from_utf8_encoded_string ("Osc"),
      utils::Utf8String::from_utf8_encoded_string ("Filter") });
  auto * osc_b = add_param (
    utils::Utf8String::from_utf8_encoded_string ("OscB"),
    { utils::Utf8String::from_utf8_encoded_string ("Osc") });

  model_.setPlugin (plugin_.get ());

  ASSERT_EQ (model_.rowCount (), 4);
  EXPECT_EQ (param_at (0), ungrouped);
  EXPECT_EQ (param_at (1), osc_a);
  EXPECT_EQ (param_at (2), osc_b);
  EXPECT_EQ (param_at (3), filter);
}

TEST_F (PluginParameterListModelTest, ExposesJoinedGroupPath)
{
  add_param (utils::Utf8String::from_utf8_encoded_string ("Ungrouped"), {});
  add_param (
    utils::Utf8String::from_utf8_encoded_string ("Resonance"),
    { utils::Utf8String::from_utf8_encoded_string ("Osc") });
  add_param (
    utils::Utf8String::from_utf8_encoded_string ("Cutoff"),
    { utils::Utf8String::from_utf8_encoded_string ("Osc"),
      utils::Utf8String::from_utf8_encoded_string ("Filter") });

  model_.setPlugin (plugin_.get ());

  const auto roles = model_.roleNames ();
  EXPECT_EQ (roles[PluginParameterListModel::ParamGroupRole], "paramGroup");

  ASSERT_EQ (model_.rowCount (), 3);
  EXPECT_EQ (
    model_.data (model_.index (0), PluginParameterListModel::ParamGroupRole)
      .toString (),
    QString ());
  EXPECT_EQ (
    model_.data (model_.index (1), PluginParameterListModel::ParamGroupRole)
      .toString (),
    QStringLiteral ("Osc"));
  EXPECT_EQ (
    model_.data (model_.index (2), PluginParameterListModel::ParamGroupRole)
      .toString (),
    QStringLiteral ("Osc / Filter"));
}

} // namespace zrythm::plugins
