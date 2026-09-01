// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/qt.h"

#include <QObject>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QRectF>
#include <QSize>
#include <QVariantMap>

class QQuickItem;
class QQuickWindow;
class QQuickRenderControl;
class QRhiRenderBuffer;
class QRhiRenderPassDescriptor;
class QRhiTexture;
class QRhiTextureRenderTarget;

namespace zrythm::plugins
{
class Plugin;
}

namespace zrythm::gui
{

/**
 * @brief URL of the QML component rendering the plugin host window header
 * bar.
 *
 * The component expects a `plugin` initial property holding the
 * plugins::Plugin.
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
 * @brief Renders a QML scene offscreen.
 *
 * Renders via QQuickRenderControl into an RHI texture with an explicitly
 * controlled device pixel ratio; consumers paint grab_frame() and forward
 * input events to quick_window(). Used for the plugin host window header
 * strips and for windowed popups over the native strips.
 *
 * All sizes are in logical pixels; grab_frame() returns physical pixels
 * (logical size x device pixel ratio).
 */
class OffscreenQmlScene final : public QObject
{
  Q_OBJECT

public:
  /**
   * @brief Header bar scene for a plugin (see plugin_header_bar_qml_url()).
   *
   * Loads the header bar with `plugin` and `sceneController` initial
   * properties; the header calls back into this object via its invokables
   * (requestPresetPopup(), scheduleRepaint()).
   */
  explicit OffscreenQmlScene (
    plugins::Plugin &plugin,
    QObject *        parent = nullptr);
  /**
   * @brief Generic scene: loads the component at @a qml_url with the
   * given initial properties.
   */
  explicit OffscreenQmlScene (
    const QUrl &qml_url,
    QVariantMap initial_properties,
    QObject *   parent = nullptr);
  ~OffscreenQmlScene () override;

  /** Whether the QML component loaded successfully. */
  bool is_valid () const;

  /** The window input events should be sent to (in logical pixels). */
  QQuickWindow * quick_window () const { return quick_window_.get (); }

  /** The engine the scene was created in (the shared application engine
   * when set). */
  QQmlEngine * qml_engine () const { return engine_; }

  /**
   * @brief Renders the current scene and returns it in physical pixels.
   *
   * Returns a null image when the renderer is not usable.
   */
  QImage grab_frame ();

  /** Sets the scene size in logical pixels. */
  void resize (int width, int height);

  /** Sets the render scale (physical pixels per logical pixel). */
  void set_device_pixel_ratio (qreal dpr);

  /** Implicit width of the header's controls in logical pixels, for
   * minimum size hints. */
  int controls_implicit_width () const;

  /**
   * @brief Height the header strip should have in logical pixels, from the
   * QML scene.
   *
   * Falls back to plugins::PluginHostWindow::kHeaderHeight when the QML is
   * not loaded.
   */
  int implicit_height () const;

Q_SIGNALS:
  /**
   * @brief Emitted when the rendered content changed and needs re-blitting.
   *
   * Never emitted synchronously from within scene graph bookkeeping: it is
   * always deferred to the next event loop iteration (bursts are coalesced),
   * so consumers may safely polish/sync/render from their handlers. See
   * scheduleRepaint().
   */
  void repaintNeeded ();

  /** Emitted when the QML scene's implicit height changed. */
  void implicitHeightChanged (int height);

  /** Emitted when the QML scene's implicit width changed. */
  void implicitWidthChanged (int width);

  /**
   * @brief Emitted when a header bar scene asks for the preset browser to
   * be shown in a separate window.
   *
   * @param anchor_strip_rect Name button rect in scene (logical)
   *   coordinates; the popup should open below it.
   * @param model The selector's preset model (a snapshot; ownership stays
   *   with the scene).
   * @param current_index Index to highlight.
   */
  void presetPopupRequested (
    const QRectF &anchor_strip_rect,
    QObject *     model,
    int           current_index);

public Q_SLOTS:
  /**
   * @brief Applies a selection coming back from an externally hosted
   * preset popup.
   *
   * No-op for scenes whose root has no applyPresetSelection() function.
   */
  void applyPresetSelection (int index);

public:
  /**
   * @brief Called by the header bar scene when the preset list should be
   * shown in a separate window.
   *
   * Emits presetPopupRequested(); the host window backend does the actual
   * work. Invoked from QML (the header's signals are QML-declared, which
   * C++ cannot connect to with member-function-pointer connects).
   *
   * @param anchor_strip_rect Name button rect in scene (logical)
   *   coordinates; the popup should open below it.
   * @param model The selector's preset model (a snapshot; ownership stays
   *   with the scene).
   * @param current_index Index to highlight.
   */
  Q_INVOKABLE void requestPresetPopup (
    const QRectF &anchor_strip_rect,
    QObject *     model,
    int           current_index)
  {
    Q_EMIT presetPopupRequested (anchor_strip_rect, model, current_index);
  }

  /**
   * @brief Emits repaintNeeded() on the next event loop iteration,
   * coalescing bursts into a single emission.
   *
   * The Qt docs forbid polishing, syncing or rendering directly from
   * QQuickRenderControl::sceneChanged()/renderRequested() handlers and
   * recommend deferring with a timer: those signals can be emitted from
   * within the scene graph's dirty-list bookkeeping, and re-entering the
   * renderer from there (e.g. a consumer calling grab_frame()) corrupts
   * that bookkeeping and trips QQuickItemPrivate's assertions.
   *
   * Public and invokable so the scene can request repaints itself
   * (e.g. the header bar on theme color changes).
   */
  Q_INVOKABLE void scheduleRepaint ();

private:
  /**
   * @brief Shared scene setup for both constructors.
   *
   * Loads the component, parents the root item into the offscreen window
   * and wires the generic change notifications.
   */
  void init (const QUrl &qml_url, const QVariantMap &initial_properties);

  /**
   * @brief (Re)creates the RHI render target for the current logical size
   * and device pixel ratio, initializing the renderer on first use.
   *
   * @return Whether a usable render target exists.
   */
  bool ensure_render_target ();

private:
  /** Fallback engine when no shared engine was set (theme state will not
   * follow the application). */
  utils::QObjectUniquePtr<QQmlEngine> fallback_engine_;
  /** The engine the scene was created in. */
  QPointer<QQmlEngine>                      engine_;
  std::unique_ptr<QQuickRenderControl>      render_control_;
  utils::QObjectUniquePtr<QQmlComponent>    component_;
  QPointer<QQuickItem>                      root_item_;
  std::unique_ptr<QRhiTexture>              texture_;
  std::unique_ptr<QRhiRenderBuffer>         depth_stencil_;
  std::unique_ptr<QRhiTextureRenderTarget>  render_target_;
  std::unique_ptr<QRhiRenderPassDescriptor> render_pass_;
  /** Declared after the RHI resources so it is destroyed first: the window
   * holds a QQuickRenderTarget wrapping render_target_. */
  std::unique_ptr<QQuickWindow> quick_window_;
  QSize                         logical_size_;
  qreal                         dpr_ = 1.0;
  /** DPR the current render target was created with. */
  qreal render_target_dpr_ = 0.0;
  bool  renderer_initialized_ = false;
  /** Whether a deferred repaintNeeded() emission is already scheduled. */
  bool repaint_pending_ = false;
};

} // namespace zrythm::gui
