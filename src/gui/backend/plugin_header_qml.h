// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/qt.h"

#include <QObject>
#include <QQuickWidget>

class QQuickItem;
class QQmlEngine;

namespace zrythm::plugins
{
class PluginHostWindow;
}

namespace zrythm::gui
{

/**
 * @brief URL of the QML component rendering the plugin host window header
 * bar.
 *
 * The component expects a `hostWindow` context property holding the
 * plugins::PluginHostWindow.
 */
QUrl
plugin_header_bar_qml_url ();

/**
 * @brief Sets the QML engine shared by all plugin window header bars.
 *
 * Must be the application's main engine: QML singletons are per-engine, so
 * the header bars can only follow runtime theme changes when they share the
 * main engine. Call once at application startup.
 */
void
set_plugin_header_qml_engine (QQmlEngine * engine);

/**
 * @brief Creates a QQuickWidget showing the plugin host window header bar.
 *
 * Uses the shared engine if set (see set_plugin_header_qml_engine()),
 * otherwise a private engine (theme state will not follow the application).
 */
utils::QObjectUniquePtr<QQuickWidget>
make_plugin_header_widget (
  plugins::PluginHostWindow &window,
  QWidget *                  parent = nullptr);

/** Whether the header bar QML component loaded successfully. */
bool
plugin_header_widget_is_valid (const QQuickWidget &widget);

/**
 * @brief Renders the plugin host window header bar QML offscreen.
 *
 * Wraps a hidden QQuickWidget; used by the X11 host window, which blits
 * grab_frame() onto the header strip and forwards X input events to
 * widget().
 */
class OffscreenQmlHeader final : public QObject
{
  Q_OBJECT

public:
  explicit OffscreenQmlHeader (
    plugins::PluginHostWindow &window,
    QObject *                  parent = nullptr);

  /** Whether the QML component loaded successfully. */
  bool is_valid () const;

  /** The widget input events should be sent to (forwarded to the scene). */
  QQuickWidget * widget () const { return widget_.get (); }

  /** Renders the current scene and returns it. */
  QImage grab_frame ();

  void resize (int width, int height);

  /** Implicit width of the header's controls, for minimum size hints. */
  int controls_implicit_width () const;

Q_SIGNALS:
  /** Emitted when the rendered content changed and needs re-blitting. */
  void repaintNeeded ();

private:
  utils::QObjectUniquePtr<QQuickWidget> widget_;
};

} // namespace zrythm::gui
