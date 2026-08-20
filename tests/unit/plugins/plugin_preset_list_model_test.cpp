// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_preset_list_model.h"
#include "utils/object_registry.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::plugins
{

class FakePresetPlugin : public Plugin
{
public:
  FakePresetPlugin (utils::IObjectRegistry &registry, QObject * parent = nullptr)
      : Plugin (registry, parent)
  {
  }

  std::span<const PresetEntry> presetEntries () const override
  {
    return presets_;
  }

  void replace_presets (std::vector<PresetEntry> presets)
  {
    presets_ = std::move (presets);
    notify_presets_rebuilt ();
  }

  std::vector<PresetEntry> presets_{
    { QStringLiteral ("Init"),   QStringLiteral ("Factory"), 0 },
    { QStringLiteral ("Bright"), QStringLiteral ("Factory"), 1 },
    { QStringLiteral ("Lead"),   QStringLiteral ("User"),    2 },
  };

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

class PluginPresetListModelTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    plugin_ = std::make_unique<FakePresetPlugin> (*registry_);
  }

  std::unique_ptr<utils::ObjectRegistry> registry_;
  std::unique_ptr<FakePresetPlugin>      plugin_;
  PluginPresetListModel                  model_;
};

TEST_F (PluginPresetListModelTest, EmptyWithoutPlugin)
{
  EXPECT_EQ (model_.rowCount (), 0);
  EXPECT_EQ (model_.property ("count").toInt (), 0);
  EXPECT_TRUE (model_.nameAt (0).isEmpty ());
}

TEST_F (PluginPresetListModelTest, ExposesEntries)
{
  QSignalSpy count_spy (&model_, &PluginPresetListModel::countChanged);

  model_.setPlugin (plugin_.get ());

  EXPECT_EQ (model_.plugin (), plugin_.get ());
  ASSERT_EQ (model_.rowCount (), 3);
  EXPECT_EQ (count_spy.count (), 1);

  const auto roles = model_.roleNames ();
  EXPECT_EQ (roles[PluginPresetListModel::NameRole], "name");
  EXPECT_EQ (roles[PluginPresetListModel::GroupRole], "group");

  const auto first = model_.index (0);
  EXPECT_EQ (
    model_.data (first, PluginPresetListModel::NameRole).toString (),
    QStringLiteral ("Init"));
  EXPECT_EQ (
    model_.data (first, Qt::DisplayRole).toString (), QStringLiteral ("Init"));
  EXPECT_EQ (
    model_.data (first, PluginPresetListModel::GroupRole).toString (),
    QStringLiteral ("Factory"));
  EXPECT_EQ (
    model_.data (model_.index (2), PluginPresetListModel::GroupRole).toString (),
    QStringLiteral ("User"));

  // Out-of-range indices and unknown roles return empty values
  EXPECT_TRUE (
    model_.data (model_.index (3), PluginPresetListModel::NameRole).isNull ());
  EXPECT_TRUE (model_.data (first, Qt::UserRole + 100).isNull ());

  EXPECT_EQ (model_.nameAt (2), QStringLiteral ("Lead"));
  EXPECT_TRUE (model_.nameAt (3).isEmpty ());
  EXPECT_TRUE (model_.nameAt (-1).isEmpty ());
}

TEST_F (PluginPresetListModelTest, FollowsPresetListChanges)
{
  model_.setPlugin (plugin_.get ());
  ASSERT_EQ (model_.rowCount (), 3);

  QSignalSpy reset_spy (&model_, &PluginPresetListModel::modelReset);
  QSignalSpy count_spy (&model_, &PluginPresetListModel::countChanged);

  plugin_->replace_presets (
    {
      { QStringLiteral ("Only"), QStringLiteral ("Factory"), 0 }
  });

  EXPECT_EQ (reset_spy.count (), 1);
  EXPECT_EQ (count_spy.count (), 1);
  ASSERT_EQ (model_.rowCount (), 1);
  EXPECT_EQ (model_.nameAt (0), QStringLiteral ("Only"));
}

TEST_F (PluginPresetListModelTest, HasGroupsIsFalseWhenNoEntryHasAGroup)
{
  EXPECT_FALSE (model_.hasGroups ());

  plugin_->replace_presets (
    {
      { QStringLiteral ("Init"), {}, 0 },
      { QStringLiteral ("More"), {}, 1 },
  });
  model_.setPlugin (plugin_.get ());
  EXPECT_FALSE (model_.hasGroups ());
}

TEST_F (PluginPresetListModelTest, HasGroupsIsTrueWhenAnyEntryHasAGroup)
{
  // Default fixture: "Factory" and "User" groups
  model_.setPlugin (plugin_.get ());
  EXPECT_TRUE (model_.hasGroups ());

  // A single named group still counts
  plugin_->replace_presets (
    {
      { QStringLiteral ("Init"), QStringLiteral ("Factory"), 0 },
      { QStringLiteral ("More"), QStringLiteral ("Factory"), 1 },
  });
  EXPECT_TRUE (model_.hasGroups ());

  // One named entry among ungrouped entries is enough
  plugin_->replace_presets (
    {
      { QStringLiteral ("Init"), {},                         0 },
      { QStringLiteral ("More"), QStringLiteral ("Factory"), 1 },
  });
  EXPECT_TRUE (model_.hasGroups ());
}

TEST_F (PluginPresetListModelTest, HasGroupsChangedEmittedOnTransitions)
{
  plugin_->replace_presets (
    {
      { QStringLiteral ("Init"), {}, 0 },
  });
  model_.setPlugin (plugin_.get ());
  ASSERT_FALSE (model_.hasGroups ());

  QSignalSpy spy (&model_, &PluginPresetListModel::hasGroupsChanged);

  // Ungrouped -> grouped emits
  plugin_->replace_presets (
    {
      { QStringLiteral ("Init"), QStringLiteral ("Factory"), 0 },
  });
  EXPECT_TRUE (model_.hasGroups ());
  EXPECT_EQ (spy.count (), 1);

  // Grouped -> grouped with different names does not emit
  plugin_->replace_presets (
    {
      { QStringLiteral ("Init"), QStringLiteral ("User"), 0 },
  });
  EXPECT_EQ (spy.count (), 1);

  // Grouped -> empty list emits
  plugin_->replace_presets ({});
  EXPECT_FALSE (model_.hasGroups ());
  EXPECT_EQ (spy.count (), 2);
}

TEST_F (PluginPresetListModelTest, HandlesPluginDestruction)
{
  model_.setPlugin (plugin_.get ());
  ASSERT_EQ (model_.rowCount (), 3);

  QSignalSpy plugin_spy (&model_, &PluginPresetListModel::pluginChanged);

  plugin_.reset ();

  EXPECT_EQ (model_.plugin (), nullptr);
  EXPECT_EQ (plugin_spy.count (), 1);

  // The reset is queued: destroyed() is emitted before QML clears its
  // references to the plugin, so the model must not let views call back
  // into the partially destroyed object
  QCoreApplication::processEvents ();
  EXPECT_EQ (model_.rowCount (), 0);
}

TEST_F (PluginPresetListModelTest, ReassigningPluginResets)
{
  auto other_plugin = std::make_unique<FakePresetPlugin> (*registry_);
  other_plugin->presets_.clear ();

  model_.setPlugin (plugin_.get ());
  ASSERT_EQ (model_.rowCount (), 3);

  // No-op when assigning the same plugin
  QSignalSpy reset_spy (&model_, &PluginPresetListModel::modelReset);
  model_.setPlugin (plugin_.get ());
  EXPECT_EQ (reset_spy.count (), 0);

  model_.setPlugin (other_plugin.get ());
  EXPECT_EQ (model_.rowCount (), 0);

  // The old plugin no longer drives the model
  plugin_->replace_presets ({});
  EXPECT_EQ (reset_spy.count (), 1);
}

} // namespace zrythm::plugins
