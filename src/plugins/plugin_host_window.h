// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string>

#include <QObject>
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
  Q_PROPERTY (bool bypassed READ bypassed NOTIFY bypassedChanged)
  Q_PROPERTY (bool abActive READ abActive NOTIFY abStateChanged)
  Q_PROPERTY (QString pluginName READ pluginName CONSTANT)

public:
  /**
   * @brief Constructs the window and wires the behavior shared by all host
   * windows.
   *
   * Close requests hide the plugin UI, the header strip's bypass button
   * toggles the plugin's bypass parameter, and bypass parameter changes are
   * re-emitted as @ref bypassedChanged.
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

  /**
   * @brief Whether the plugin is currently bypassed, based on the bypass
   * parameter's base value.
   */
  bool bypassed () const;

  /**
   * @brief Whether A/B compare slot B is currently active.
   */
  bool abActive () const { return ab_b_active_; }

  /**
   * @brief The hosted plugin's display name.
   */
  QString pluginName () const;

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
   * system requires a handshake (XEmbed on X11).
   */
  virtual void completeNativeEmbedding () { }

  /**
   * @brief Height of the host-chrome header strip above the plugin view,
   * in logical pixels.
   */
  static constexpr int kHeaderHeight = 28;

  /**
   * @brief Sets the plugin view size and centers the window on the screen.
   *
   * Implementations add the header strip height to the total window size
   * and keep the embed area exactly the requested view size.
   */
  virtual void setSizeAndCenter (int width, int height) = 0;

  /**
   * @brief Sets the plugin view size without centering.
   *
   * Used for resizes. The header strip height is added internally.
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

  Q_SIGNAL void titleChanged (const QString &title);

  /**
   * @brief Emitted when the user clicks the header strip's bypass button.
   */
  Q_SIGNAL void bypassToggleRequested ();

  /**
   * @brief Emitted when the plugin's bypass state changed (in either
   * direction).
   */
  Q_SIGNAL void bypassedChanged (bool bypassed);

  /**
   * @brief Emitted when the user clicks the header strip's preset selector
   * button.
   */
  Q_SIGNAL void presetSelectorRequested ();

  /**
   * @brief Emitted when the user clicks the header strip's A/B compare
   * button.
   *
   * The base class handles this by saving the plugin's current state into
   * the active slot and restoring the other slot (see Plugin::save_state()
   * and Plugin::load_state()).
   */
  Q_SIGNAL void abSwitchRequested ();

  /**
   * @brief Emitted when the active A/B slot changed.
   *
   * @param bActive True when slot B is now active, false for slot A.
   */
  Q_SIGNAL void abStateChanged (bool bActive);

private:
  Plugin     &plugin_;
  QString     title_;
  std::string ab_state_a_;
  std::string ab_state_b_;
  bool        ab_b_active_ = false;
};
} // namespace zrythm::plugins
