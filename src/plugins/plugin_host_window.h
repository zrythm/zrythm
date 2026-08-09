// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <utility>

#include "utils/qt.h"

#include <QObject>
#include <QTimer>
#include <QtGui/qwindowdefs.h>

namespace zrythm::plugins
{

class Plugin;

/**
 * @brief Native windowing system a plugin host window belongs to.
 */
enum class WindowSystem
{
  X11,
  Wayland,
  Win32,
  Cocoa,
};

/**
 * @brief Abstract base class for top-level plugin hosting windows.
 */
class PluginHostWindow : public QObject
{
  Q_OBJECT

  Q_PROPERTY (QString title READ title WRITE setTitle NOTIFY titleChanged)
  Q_PROPERTY (
    float contentScaleFactor READ contentScaleFactor NOTIFY
      contentScaleFactorChanged)

public:
  /**
   * @brief Constructs the window and wires the behavior shared by all host
   * windows.
   *
   * Close requests hide the plugin UI.
   *
   * @param plugin The plugin this window hosts. Must outlive the window
   * (the plugin owns its editor window).
   */
  explicit PluginHostWindow (Plugin &plugin, QObject * parent = nullptr);
  ~PluginHostWindow () override = default;

  /**
   * @brief The plugin this window hosts.
   */
  Plugin &plugin () const { return plugin_; }

  QString title () const { return title_; }

  void setTitle (const QString &title)
  {
    if (title_ == title)
      return;
    title_ = title;
    Q_EMIT titleChanged (title);
  }

  /**
   * @brief Returns the window system of the platform this binary was built
   * for.
   */
  static constexpr WindowSystem currentWindowSystem ()
  {
#if defined(Q_OS_LINUX)
    return WindowSystem::X11;
#elif defined(Q_OS_MACOS)
    return WindowSystem::Cocoa;
#elif defined(Q_OS_WIN)
    return WindowSystem::Win32;
#else
#  error "unsupported platform"
#endif
  }

  /**
   * @brief The native windowing system this window belongs to.
   *
   * Plugins use this to decide which platform type to offer to native
   * editors.
   */
  virtual WindowSystem windowSystem () const = 0;

  /**
   * @brief Runs any platform-specific embedding handshake.
   *
   * Plugins must call this after a native editor view has been attached to
   * the window from @ref getEmbedWindowId. Does nothing unless the window
   * system requires a handshake (XEmbed on X11); it is a no-op on the Qt
   * window path by design, since plain reparenting is sufficient for
   * xcb/Win32/Cocoa.
   */
  virtual void completeNativeEmbedding () { }

  /**
   * @brief Fallback height of the host-chrome header strip above the plugin
   * view, in logical pixels.
   *
   * The actual height is driven by the header bar QML's implicitHeight;
   * this is only used when the QML failed to load.
   */
  static constexpr int kHeaderHeight = 28;

  /**
   * @brief Sets the plugin view size and centers the window on the screen.
   *
   * Sizes are in logical pixels; implementations convert to physical
   * pixels internally using the current scale factor. Implementations add
   * the header strip height to the total window size and keep the embed
   * area exactly the requested view size.
   */
  virtual void setSizeAndCenter (int width, int height) = 0;

  /**
   * @brief Sets the plugin view size without centering.
   *
   * Used for resizes. Sizes are in logical pixels (see setSizeAndCenter).
   * The header strip height is added internally.
   */
  virtual void setSize (int width, int height) = 0;

  /**
   * @brief Sets whether the user can resize the window.
   *
   * When false, the window is fixed to the size most recently set via
   * setSize()/setSizeAndCenter(). Call after the plugin's resize
   * capabilities and initial view size are known.
   */
  virtual void setResizable (bool resizable) = 0;

  virtual void setVisible (bool shouldBeVisible) = 0;

  /**
   * @brief Gets a native window handle to embed in.
   *
   * To be used in natively hosted plugins.
   */
  virtual WId getEmbedWindowId () const = 0;

  /**
   * @brief Returns the display scale factor of the window (1.0 = no scaling).
   *
   * Used to feed VST3's IPlugViewContentScaleSupport and to convert between
   * physical-pixel view sizes and this window's logical-pixel sizes.
   */
  virtual float contentScaleFactor () const { return 1.f; }

  /**
   * @brief Emitted when the user clicks the window's close button.
   *
   * The window does not close by itself - listeners should hide it (or
   * ignore this to keep it open).
   */
  Q_SIGNAL void closeRequested ();

  /**
   * @brief Emitted when native embedding of the plugin's window failed
   * after exhausting all retries.
   *
   * The window hides itself before emitting. Listeners should tear down
   * the plugin's native presentation and fall back to the generic UI;
   * teardown must be deferred (e.g., QTimer::singleShot(0)) because the
   * emission comes from within the window's own call stack — destroying
   * the window synchronously would be use-after-free.
   */
  Q_SIGNAL void embeddingFailed ();

  Q_SIGNAL void titleChanged (const QString &title);

  /**
   * @brief Emitted when the scale factor of the window's screen changed
   * (e.g., the window moved to a screen with different scaling).
   */
  Q_SIGNAL void contentScaleFactorChanged (float factor);

  /**
   * @brief Emitted when the embed area's size changed outside of a
   * plugin-initiated resize (e.g., the user resized the window via the
   * window manager), in logical pixels.
   *
   * Plugin formats with a host-to-plugin size API (e.g., VST3's
   * IPlugView::onSize) should forward the new size to the plugin.
   */
  Q_SIGNAL void embedSizeChanged (int width, int height);

private:
  Plugin &plugin_;
  QString title_;
};

/**
 * @brief Coordinates host-side embed area resizes between a
 * PluginHostWindow and a plugin GUI.
 *
 * Forwards embed area resizes (e.g., the user dragging the window corner)
 * to the plugin, letting the plugin constrain the size first (CLAP
 * clap_plugin_gui.adjust_size, VST3 IPlugView.checkSizeConstraint), then
 * applying it (CLAP set_size, VST3 onSize). When the plugin constrains
 * the size to something different, the host window is resized to match
 * once the resize settles (debounced).
 *
 * Sizes passed to the hooks are physical pixels (CLAP clap_gui and VST3
 * ViewRect sizes are physical on Windows/GNU/Linux and logical on macOS;
 * the conversions are no-ops on macOS).
 *
 * The coordinator is parented to the window and destroyed with it.
 */
class PluginViewResizeCoordinator : public QObject
{
  Q_OBJECT
public:
  struct Hooks
  {
    /** Whether the plugin GUI currently accepts size calls. */
    std::function<bool ()> gui_active;

    /** Whether the plugin view is user-resizable. */
    std::function<bool ()> can_resize;

    /** Lets the plugin constrain a size in place (e.g., aspect ratio). */
    std::function<void (int &width, int &height)> adjust_size;

    /** Applies a size to the plugin view. */
    std::function<void (int width, int height)> apply_size;
  };

  PluginViewResizeCoordinator (PluginHostWindow &window, Hooks hooks);

private:
  PluginHostWindow               &window_;
  Hooks                           hooks_;
  utils::QObjectUniquePtr<QTimer> snap_timer_;

  /** Last plugin-constrained size (logical px) pending a deferred host
   * window snap. */
  std::optional<std::pair<int, int>> pending_snap_size_;
};
} // namespace zrythm::plugins
