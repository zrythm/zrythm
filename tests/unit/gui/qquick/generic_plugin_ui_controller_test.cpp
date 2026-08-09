// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/qquick/generic_plugin_ui_controller.h"
#include "plugins/internal_plugin_base.h"
#include "plugins/plugin_configuration.h"
#include "utils/object_registry.h"

#include "helpers/scoped_qcoreapplication.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::gui::qquick
{

class GenericPluginUiControllerTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  class PluginWithNativeUi : public plugins::InternalPluginBase
  {
  public:
    using plugins::InternalPluginBase::InternalPluginBase;
    bool hasNativeUi () const override { return true; }
    void force_native_ui_unavailable (bool unavailable)
    {
      set_native_ui_unavailable (unavailable);
    }
  };

  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    controller_ = std::make_unique<GenericPluginUiController> ();
  }

  void TearDown () override
  {
    controller_.reset ();
    registry_.reset ();
  }

  template <typename PluginT = plugins::InternalPluginBase>
  std::unique_ptr<PluginT> make_plugin (bool instantiate = true)
  {
    auto plugin = std::make_unique<PluginT> (*registry_);
    if (instantiate)
      {
        auto config = std::make_unique<plugins::PluginConfiguration> ();
        config->descr_ = std::make_unique<plugins::PluginDescriptor> ();
        plugin->set_configuration (*config);
      }
    return plugin;
  }

  std::unique_ptr<utils::ObjectRegistry>     registry_;
  std::unique_ptr<GenericPluginUiController> controller_;
};

TEST_F (GenericPluginUiControllerTest, DoesNotShowRowForHiddenUi)
{
  auto plugin = make_plugin ();
  controller_->trackPluginUiVisibility (plugin.get ());
  EXPECT_EQ (controller_->rowCount (), 0);
}

TEST_F (GenericPluginUiControllerTest, ShowsRowWhenUiVisible)
{
  auto plugin = make_plugin ();
  plugin->setUiVisible (true);
  controller_->trackPluginUiVisibility (plugin.get ());
  ASSERT_EQ (controller_->rowCount (), 1);
  EXPECT_EQ (
    controller_
      ->data (controller_->index (0, 0), GenericPluginUiController::PluginRole)
      .value<plugins::Plugin *> (),
    plugin.get ());
}

TEST_F (GenericPluginUiControllerTest, RowAddedAndRemovedOnUiToggle)
{
  auto plugin = make_plugin ();
  controller_->trackPluginUiVisibility (plugin.get ());
  plugin->setUiVisible (true);
  EXPECT_EQ (controller_->rowCount (), 1);
  plugin->setUiVisible (false);
  EXPECT_EQ (controller_->rowCount (), 0);
}

TEST_F (GenericPluginUiControllerTest, SkipsPluginsWithNativeUi)
{
  auto plugin = make_plugin<PluginWithNativeUi> ();
  controller_->trackPluginUiVisibility (plugin.get ());
  plugin->setUiVisible (true);
  EXPECT_EQ (controller_->rowCount (), 0);
}

TEST_F (GenericPluginUiControllerTest, ShowsRowWhenNativeUiBecomesUnavailable)
{
  auto plugin = make_plugin<PluginWithNativeUi> ();
  controller_->trackPluginUiVisibility (plugin.get ());
  plugin->setUiVisible (true);
  EXPECT_EQ (controller_->rowCount (), 0);

  plugin->force_native_ui_unavailable (true);
  EXPECT_EQ (controller_->rowCount (), 1);

  plugin->force_native_ui_unavailable (false);
  EXPECT_EQ (controller_->rowCount (), 0);
}

TEST_F (GenericPluginUiControllerTest, WaitsForInstantiation)
{
  auto plugin = make_plugin (false);
  controller_->trackPluginUiVisibility (plugin.get ());
  plugin->setUiVisible (true);
  EXPECT_EQ (controller_->rowCount (), 0);

  auto config = std::make_unique<plugins::PluginConfiguration> ();
  config->descr_ = std::make_unique<plugins::PluginDescriptor> ();
  plugin->set_configuration (*config);
  EXPECT_EQ (controller_->rowCount (), 1);
}

TEST_F (GenericPluginUiControllerTest, TrackingIsIdempotent)
{
  auto plugin = make_plugin ();
  controller_->trackPluginUiVisibility (plugin.get ());
  controller_->trackPluginUiVisibility (plugin.get ());
  plugin->setUiVisible (true);
  EXPECT_EQ (controller_->rowCount (), 1);
}

TEST_F (GenericPluginUiControllerTest, RowRemovedWhenPluginDestroyed)
{
  auto plugin = make_plugin ();
  controller_->trackPluginUiVisibility (plugin.get ());
  plugin->setUiVisible (true);
  ASSERT_EQ (controller_->rowCount (), 1);

  plugin.reset ();
  EXPECT_EQ (controller_->rowCount (), 0);
}

} // namespace zrythm::gui::qquick
