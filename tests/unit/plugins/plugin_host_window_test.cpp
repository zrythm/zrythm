// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "plugins/plugin_host_window.h"
#include "utils/object_registry.h"

#include <QTest>

#include "helpers/mock_plugin_host_window.h"
#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::plugins
{

namespace
{

class CoordinatorTestPlugin : public Plugin
{
public:
  CoordinatorTestPlugin (
    utils::IObjectRegistry &registry,
    QObject *               parent = nullptr)
      : Plugin (registry, parent)
  {
  }

  std::string save_state_impl () const override { return {}; }
  bool        load_state_impl (const std::string &) override { return true; }

  void process_impl (
    dsp::graph::ProcessBlockInfo,
    const dsp::ITransport &,
    const dsp::TempoMap &) noexcept override
  {
  }
};

/**
 * @brief Hook set recording every call, optionally clamping sizes like a
 * plugin constraining an aspect ratio.
 */
struct RecordingHooks
{
  bool gui_active = true;
  bool can_resize = true;
  bool clamp_size = false;

  int adjust_calls = 0;
  int apply_calls = 0;
  int last_apply_width = 0;
  int last_apply_height = 0;

  PluginViewResizeCoordinator::Hooks hooks ()
  {
    return {
      .gui_active = [this] { return gui_active; },
      .can_resize = [this] { return can_resize; },
      .adjust_size =
        [this] (int &width, int &height) {
          ++adjust_calls;
          if (clamp_size)
            {
              width = std::min (width, 640);
              height = std::min (height, 480);
            }
        },
      .apply_size =
        [this] (int width, int height) {
          ++apply_calls;
          last_apply_width = width;
          last_apply_height = height;
        },
    };
  }
};

} // namespace

class PluginViewResizeCoordinatorTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    plugin_ = std::make_unique<CoordinatorTestPlugin> (*registry_);
    window_state_ = std::make_shared<test_helpers::MockPluginHostWindowState> ();
    window_ = std::make_unique<test_helpers::MockPluginHostWindow> (
      *plugin_, window_state_);
  }

  void TearDown () override
  {
    if (plugin_)
      {
        plugin_->release_resources ();
      }
  }

  std::unique_ptr<utils::ObjectRegistry> registry_;
  std::unique_ptr<CoordinatorTestPlugin> plugin_;

  std::shared_ptr<test_helpers::MockPluginHostWindowState> window_state_;
  std::unique_ptr<test_helpers::MockPluginHostWindow>      window_;

  std::unique_ptr<PluginViewResizeCoordinator> coordinator_;
};

TEST_F (PluginViewResizeCoordinatorTest, FixedSizeViewReceivesNoSizes)
{
  RecordingHooks recording;
  recording.can_resize = false;
  coordinator_ = std::make_unique<PluginViewResizeCoordinator> (
    *window_, recording.hooks ());

  window_->emit_embed_size_changed_for_test (866, 674);

  // A fixed-size view keeps the size the plugin gave it: embed area
  // changes must not be forwarded
  EXPECT_EQ (recording.adjust_calls, 0);
  EXPECT_EQ (recording.apply_calls, 0);
}

TEST_F (PluginViewResizeCoordinatorTest, ResizableViewReceivesConstrainedSizes)
{
  RecordingHooks recording;
  recording.clamp_size = true;
  coordinator_ = std::make_unique<PluginViewResizeCoordinator> (
    *window_, recording.hooks ());

  window_->emit_embed_size_changed_for_test (866, 674);

  // With a scale factor of 1, logical and physical sizes are equal
  EXPECT_EQ (recording.adjust_calls, 1);
  EXPECT_EQ (recording.apply_calls, 1);
  EXPECT_EQ (recording.last_apply_width, 640);
  EXPECT_EQ (recording.last_apply_height, 480);

  // The window snaps to the plugin-constrained size once the resize
  // settles
  ASSERT_TRUE (QTest::qWaitFor ([this] {
    return window_state_->set_size_calls > 0;
  }));
  EXPECT_EQ (window_state_->width, 640);
  EXPECT_EQ (window_state_->height, 480);
}

TEST_F (PluginViewResizeCoordinatorTest, InactiveGuiReceivesNoSizes)
{
  RecordingHooks recording;
  recording.gui_active = false;
  coordinator_ = std::make_unique<PluginViewResizeCoordinator> (
    *window_, recording.hooks ());

  window_->emit_embed_size_changed_for_test (866, 674);

  EXPECT_EQ (recording.adjust_calls, 0);
  EXPECT_EQ (recording.apply_calls, 0);
}

TEST_F (PluginViewResizeCoordinatorTest, DegenerateEmbedSizeIgnored)
{
  RecordingHooks recording;
  coordinator_ = std::make_unique<PluginViewResizeCoordinator> (
    *window_, recording.hooks ());

  window_->emit_embed_size_changed_for_test (0, 0);

  EXPECT_EQ (recording.adjust_calls, 0);
  EXPECT_EQ (recording.apply_calls, 0);
}

} // namespace zrythm::plugins
