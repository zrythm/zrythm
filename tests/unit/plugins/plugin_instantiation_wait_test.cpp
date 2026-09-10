// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <chrono>

#include "plugins/faust/faust_plugin.h"
#include "plugins/plugin_instantiation_wait.h"
#include "utils/object_registry.h"

#include <gtest/gtest.h>

namespace zrythm::plugins
{

class PluginInstantiationWaitTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
  }

  void TearDown () override { registry_.reset (); }

  // A plugin that never had a configuration applied stays Pending: its
  // instantiation never finishes
  PluginUuidReference create_pending_plugin ()
  {
    auto plugin_ref = utils::create_object<FaustPlugin> (*registry_, *registry_);
    EXPECT_EQ (
      plugin_ref.get ()->instantiationStatus (),
      Plugin::InstantiationStatus::Pending);
    return plugin_ref;
  }

  std::unique_ptr<utils::ObjectRegistry> registry_;
};

TEST_F (PluginInstantiationWaitTest, WaitForPluginIdsTimesOutOnPendingPlugin)
{
  auto       pending_ref = create_pending_plugin ();
  const auto id = type_safe::get (pending_ref.id ());

  const auto start = std::chrono::steady_clock::now ();
  wait_for_plugin_instantiations (
    *registry_, std::span<const QUuid>{ &id, 1 },
    std::chrono::milliseconds (50));
  const auto elapsed = std::chrono::steady_clock::now () - start;

  // The plugin stays Pending, so the wait can only end at the deadline
  EXPECT_GE (elapsed, std::chrono::milliseconds (50));
  EXPECT_LT (elapsed, std::chrono::seconds (5));
  EXPECT_EQ (
    pending_ref.get ()->instantiationStatus (),
    Plugin::InstantiationStatus::Pending);
}

TEST_F (PluginInstantiationWaitTest, WaitForPluginIdsIgnoresOtherPendingPlugins)
{
  auto pending_ref = create_pending_plugin ();

  const auto start = std::chrono::steady_clock::now ();
  wait_for_plugin_instantiations (
    *registry_, std::span<const QUuid>{}, std::chrono::milliseconds (50));
  const auto elapsed = std::chrono::steady_clock::now () - start;

  // An empty ID set returns immediately even though the registry holds
  // a Pending plugin
  EXPECT_LT (elapsed, std::chrono::seconds (1));
}

TEST_F (PluginInstantiationWaitTest, WaitForPluginIdsIgnoresUnregisteredIds)
{
  auto pending_ref = create_pending_plugin ();

  const auto unknown_id = QUuid::createUuid ();
  const auto start = std::chrono::steady_clock::now ();
  wait_for_plugin_instantiations (
    *registry_, std::span<const QUuid>{ &unknown_id, 1 },
    std::chrono::milliseconds (50));
  const auto elapsed = std::chrono::steady_clock::now () - start;

  EXPECT_LT (elapsed, std::chrono::seconds (1));
}

} // namespace zrythm::plugins
