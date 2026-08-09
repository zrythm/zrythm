// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include "plugins/plugin_host_window.h"

#include <QtGlobal>

namespace zrythm::gui
{

/**
 * @brief PluginHostWindow implementation backed by a raw X11 window.
 *
 * Used on GNU/Linux, where plugin embedding requires an X11 window while
 * the application itself may run on Wayland. All plugin host windows share
 * a single process-wide X connection that is pumped from Qt's event loop.
 *
 * The window never moves or resizes itself in response to input events;
 * the plugin's child window handles all of its own input.
 *
 * Compiles on all platforms but is only functional on GNU/Linux with a
 * reachable X server (check @ref is_valid).
 */
class X11PluginHostWindow final : public plugins::PluginHostWindow
{
public:
  explicit X11PluginHostWindow (plugins::Plugin &plugin);
  ~X11PluginHostWindow () override;
  Q_DISABLE_COPY_MOVE (X11PluginHostWindow)

  /**
   * @brief Whether a usable X11 window was created.
   */
  bool is_valid () const;

  plugins::WindowSystem windowSystem () const override
  {
    return plugins::WindowSystem::X11;
  }

  /**
   * @brief Completes the XEmbed handshake with the plugin's client window.
   *
   * Discovers the client among this window's children (retrying briefly
   * while it has not appeared yet) and sends XEMBED_EMBEDDED_NOTIFY,
   * XEMBED_WINDOW_ACTIVATE and XEMBED_FOCUS_IN.
   */
  void completeNativeEmbedding () override;

  void  setSizeAndCenter (int width, int height) override;
  void  setSize (int width, int height) override;
  void  setResizable (bool resizable) override;
  void  setVisible (bool shouldBeVisible) override;
  WId   getEmbedWindowId () const override;
  float contentScaleFactor () const override;

private:
  /** Re-reads the Xft.dpi scale factor, resizing the window and emitting
   * contentScaleFactorChanged() when it changed. */
  void refresh_scale_factor ();

  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

} // namespace zrythm::gui
