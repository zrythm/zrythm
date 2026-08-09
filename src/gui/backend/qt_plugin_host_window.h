// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include "plugins/plugin_host_window.h"

namespace zrythm::gui
{

/**
 * @brief PluginHostWindow backed by a Qt Widgets top-level window.
 *
 * Used on platforms where Qt's platform abstraction provides the same
 * windowing system plugin editors embed into: GNU/Linux X11 sessions (xcb),
 * Windows and macOS. On GNU/Linux Wayland sessions X11PluginHostWindow is
 * used instead, since plugin editors need X11 (XWayland) windows while Qt
 * would create Wayland surfaces.
 */
class QtPluginHostWindow final : public plugins::PluginHostWindow
{
public:
  explicit QtPluginHostWindow (plugins::Plugin &plugin);
  ~QtPluginHostWindow () override;
  Q_DISABLE_COPY_MOVE (QtPluginHostWindow)

  plugins::WindowSystem windowSystem () const override
  {
    // Qt windows belong to the native windowing system on each platform
    // this is used on (xcb on GNU/Linux, Win32, Cocoa)
    return currentWindowSystem ();
  }

  void  setSizeAndCenter (int width, int height) override;
  void  setSize (int width, int height) override;
  void  setResizable (bool resizable) override;
  void  setVisible (bool shouldBeVisible) override;
  WId   getEmbedWindowId () const override;
  float contentScaleFactor () const override;

private:
  /** Emits embedSizeChanged() when the embed area resizes. */
  bool eventFilter (QObject * obj, QEvent * ev) override;

private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

} // namespace zrythm::gui
