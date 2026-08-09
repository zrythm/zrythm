// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include "plugins/plugin.h"
#include "plugins/plugin_host_window.h"

#include <QPointer>

namespace zrythm::test_helpers
{

class MockPluginHostWindow;

struct MockPluginHostWindowState
{
  bool                           visible = false;
  int                            width = 0;
  int                            height = 0;
  bool                           resizable = true;
  bool                           destroyed = false;
  float                          content_scale_factor = 1.f;
  int                            set_size_calls = 0;
  int                            complete_native_embedding_calls = 0;
  QPointer<MockPluginHostWindow> window;
};

/**
 * @brief Recording PluginHostWindow double for offscreen editor tests.
 *
 * All interactions are recorded into shared state that outlives the window,
 * so tests can assert on visibility/size after the plugin destroys it.
 */
class MockPluginHostWindow : public plugins::PluginHostWindow
{
public:
  explicit MockPluginHostWindow (
    plugins::Plugin                           &plugin,
    std::shared_ptr<MockPluginHostWindowState> state,
    plugins::WindowSystem window_system = currentWindowSystem ())
      : PluginHostWindow (plugin), state_ (std::move (state)),
        window_system_ (window_system)
  {
    state_->window = this;
  }
  ~MockPluginHostWindow () override { state_->destroyed = true; }

  plugins::WindowSystem windowSystem () const override
  {
    return window_system_;
  }

  void setSizeAndCenter (int w, int h) override
  {
    state_->width = w;
    state_->height = h;
  }
  void setSize (int w, int h) override
  {
    ++state_->set_size_calls;
    state_->width = w;
    state_->height = h;
  }
  void setVisible (bool visible) override { state_->visible = visible; }
  void setResizable (bool resizable) override { state_->resizable = resizable; }
  void completeNativeEmbedding () override
  {
    ++state_->complete_native_embedding_calls;
  }
  WId getEmbedWindowId () const override { return kEmbedWindowId; }

  float contentScaleFactor () const override
  {
    return state_->content_scale_factor;
  }

  /** Sets the scale factor and emits contentScaleFactorChanged. */
  void set_content_scale_factor_for_test (float factor)
  {
    state_->content_scale_factor = factor;
    Q_EMIT contentScaleFactorChanged (factor);
  }

  /** Dummy native window ID returned by getEmbedWindowId(). */
  static constexpr WId kEmbedWindowId = 12345;

private:
  std::shared_ptr<MockPluginHostWindowState> state_;
  plugins::WindowSystem                      window_system_;
};

/**
 * @brief Returns a host window factory creating MockPluginHostWindows bound
 * to @p state.
 */
inline auto
make_mock_plugin_host_window_factory (
  std::shared_ptr<MockPluginHostWindowState> state,
  plugins::WindowSystem                      window_system =
    plugins::PluginHostWindow::currentWindowSystem ())
{
  return
    [held_state = std::move (state), window_system] (plugins::Plugin &plugin) {
      return std::make_unique<MockPluginHostWindow> (
        plugin, held_state, window_system);
    };
}

} // namespace zrythm::test_helpers
