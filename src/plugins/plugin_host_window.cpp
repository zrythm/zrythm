// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/host_window_units.h"
#include "plugins/plugin.h"
#include "plugins/plugin_host_window.h"
#include "utils/logger.h"

namespace zrythm::plugins
{

PluginHostWindow::PluginHostWindow (Plugin &plugin, QObject * parent)
    : QObject (parent), plugin_ (plugin)
{
  connect (this, &PluginHostWindow::closeRequested, this, [this] () {
    z_debug (
      "close button pressed on '{}' plugin window", plugin_.get_node_name ());
    plugin_.setUiVisible (false);
  });
}

PluginViewResizeCoordinator::PluginViewResizeCoordinator (
  PluginHostWindow &window,
  Hooks             hooks)
    : QObject (&window), window_ (window), hooks_ (std::move (hooks)),
      snap_timer_ (utils::make_qobject_unique<QTimer> (this))
{
  snap_timer_->setSingleShot (true);
  snap_timer_->setInterval (150);
  connect (snap_timer_.get (), &QTimer::timeout, this, [this] {
    // Clear the pending snap even when the GUI is gone: a stale size must
    // not be applied to a future editor instance
    const auto pending_size = std::exchange (pending_snap_size_, std::nullopt);
    if (!hooks_.gui_active ())
      return;
    if (pending_size.has_value ())
      window_.setSize (pending_size->first, pending_size->second);
  });
  connect (
    &window_, &PluginHostWindow::embedSizeChanged, this,
    [this] (int logical_width, int logical_height) {
      if (!hooks_.gui_active ())
        return;
      // Degenerate embed sizes (0x0 while minimized) carry no information
      // for the plugin
      if (logical_width <= 0 || logical_height <= 0)
        return;
      const auto scale = window_.contentScaleFactor ();
      // Plugin view sizes are physical pixels (the conversions are no-ops
      // on macOS, where sizes are logical)
      const auto requested_width =
        host_window_logical_to_physical (logical_width, scale);
      const auto requested_height =
        host_window_logical_to_physical (logical_height, scale);
      auto adjusted_width = requested_width;
      auto adjusted_height = requested_height;
      if (hooks_.can_resize ())
        hooks_.adjust_size (adjusted_width, adjusted_height);
      // The plugin may clamp to a non-positive size: reject it and keep the
      // requested size (already known positive)
      if (adjusted_width <= 0 || adjusted_height <= 0)
        {
          z_warning (
            "Plugin constrained its view to an invalid size {}x{}; ignoring "
            "the constraint",
            adjusted_width, adjusted_height);
          adjusted_width = requested_width;
          adjusted_height = requested_height;
        }
      hooks_.apply_size (adjusted_width, adjusted_height);
      if (
        adjusted_width != requested_width || adjusted_height != requested_height)
        {
          // Resize the window to match the plugin-constrained size once
          // the resize settles (debounced)
          pending_snap_size_ = plugin_view_size_to_host_window_size (
            adjusted_width, adjusted_height, scale);
          snap_timer_->start ();
        }
    });
}

} // namespace zrythm::plugins
