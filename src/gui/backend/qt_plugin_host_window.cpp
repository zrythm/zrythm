// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>

#include "utils/format_qt.h"

#include "gui/backend/offscreen_qml_scene.h"
#include "gui/backend/preset_popup_controller.h"
#include "gui/backend/qt_plugin_host_window.h"
#include "utils/logger.h"
#include "utils/qt.h"

#include <QCloseEvent>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>

namespace zrythm::gui
{

namespace
{
/**
 * @brief Top-level widget that reports close attempts instead of closing.
 *
 * The window does not close by itself - listeners hide it (or ignore the
 * request to keep it open).
 */
class PluginHostWindowWidget final : public QWidget
{
public:
  explicit PluginHostWindowWidget (std::function<void ()> close_callback)
      : close_callback_ (std::move (close_callback))
  {
  }

protected:
  void closeEvent (QCloseEvent * event) override
  {
    event->ignore ();
    close_callback_ ();
  }

private:
  std::function<void ()> close_callback_;
};

/**
 * @brief Header strip widget painting the offscreen QML header.
 *
 * A plain QWidget - deliberately not a QQuickWidget, which would mark the
 * whole top-level window for RHI compositing (usesRhiFlush): its GLX/DRI3
 * swap can block indefinitely waiting for a Present event from the X
 * server during resizes under XWayland.
 *
 * Also hosts the preset popup: the strip's QML scene renders offscreen,
 * so an in-scene popup would be clipped by the strip height; the popup is
 * a real popup window transient to the host window instead.
 */
class PluginHeaderWidget final : public QWidget
{
public:
  explicit PluginHeaderWidget (
    plugins::Plugin &plugin,
    QWidget *        parent = nullptr)
      : QWidget (parent),
        header_ (utils::make_qobject_unique<OffscreenQmlScene> (plugin, this))
  {
    // The scene drives hover from plain mouse moves
    setMouseTracking (true);
    setFixedHeight (header_->implicit_height ());
    setMinimumWidth (header_->controls_implicit_width ());
    connect (header_.get (), &OffscreenQmlScene::repaintNeeded, this, [this] {
      update ();
    });
    connect (
      header_.get (), &OffscreenQmlScene::implicitHeightChanged, this,
      [this] (int height) { setFixedHeight (std::max (1, height)); });
    connect (
      header_.get (), &OffscreenQmlScene::implicitWidthChanged, this,
      [this] (int width) { setMinimumWidth (std::max (1, width)); });
    connect (
      header_.get (), &OffscreenQmlScene::presetPopupRequested, this,
      [this] (
        const QRectF &anchor, QObject * model, int current_index,
        const QString &text_role) {
        open_preset_popup (anchor, model, current_index, text_role);
      });
  }

  OffscreenQmlScene &offscreen_header () const { return *header_; }

  /** Closes the preset popup, if open. */
  void close_preset_popup ()
  {
    if (preset_popup_window_ == nullptr)
      return;
    preset_popup_window_->hide ();
    release_preset_popup_item ();
  }

private:
  /**
   * @brief Releases the popup scene item, deferring its destruction.
   *
   * Called from paths that may run inside the item's own QML signal
   * handlers (selection, dismiss); destroying a QML object synchronously
   * while one of its signal handlers is on the stack is fatal, so the
   * item is destroyed via deleteLater() instead.
   */
  void release_preset_popup_item ()
  {
    if (auto * item = preset_popup_item_.release ())
      item->deleteLater ();
  }

protected:
  void paintEvent (QPaintEvent *) override
  {
    sync_render_state ();
    QPainter painter (this);
    painter.fillRect (rect (), palette ().color (QPalette::Window));
    auto frame = header_->grab_frame ();
    if (frame.isNull ())
      return;
    // grab_frame() returns physical pixels (rendered at the DPR synced
    // above)
    frame.setDevicePixelRatio (devicePixelRatioF ());
    painter.drawImage (0, 0, frame);
  }

  void resizeEvent (QResizeEvent *) override { sync_render_state (); }

  void mousePressEvent (QMouseEvent * ev) override { forward_mouse (ev); }
  void mouseReleaseEvent (QMouseEvent * ev) override { forward_mouse (ev); }
  void mouseMoveEvent (QMouseEvent * ev) override { forward_mouse (ev); }

  void leaveEvent (QEvent *) override
  {
    const QPointF outside (-1, -1);
    QHoverEvent leave_ev (QEvent::HoverLeave, outside, outside, last_mouse_pos_);
    last_mouse_pos_ = outside;
    QCoreApplication::sendEvent (header_->quick_window (), &leave_ev);
    schedule_repaint ();
  }

private:
  /** Schedules a repaint, coalescing bursts of input events (e.g. pointer
   * motion) into one grab+paint per interval. */
  void schedule_repaint ()
  {
    if (repaint_pending_)
      return;
    repaint_pending_ = true;
    QTimer::singleShot (std::chrono::milliseconds{ 16 }, this, [this] {
      repaint_pending_ = false;
      update ();
    });
  }

  void sync_render_state ()
  {
    header_->set_device_pixel_ratio (devicePixelRatioF ());
    header_->resize (width (), height ());
  }

  void forward_mouse (QMouseEvent * ev)
  {
    last_mouse_pos_ = ev->position ();
    QMouseEvent forwarded (
      ev->type (), ev->position (), ev->scenePosition (), ev->globalPosition (),
      ev->button (), ev->buttons (), ev->modifiers ());
    QCoreApplication::sendEvent (header_->quick_window (), &forwarded);
    // The scene only renders on demand; input can change hover/pressed
    // visuals without emitting a scene change signal
    schedule_repaint ();
  }

  /** Shows the preset list popup below the given strip rect. */
  void open_preset_popup (
    const QRectF  &anchor_strip_rect,
    QObject *      model,
    int            current_index,
    const QString &text_role)
  {
    if (model == nullptr)
      return;

    const bool on_wayland =
      QGuiApplication::platformName ().startsWith (QLatin1String ("wayland"));

    if (preset_popup_window_ == nullptr)
      {
        preset_popup_window_ = utils::make_qobject_unique<QQuickWindow> ();
        preset_popup_window_->setFlags (
          Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        preset_popup_window_->setColor (QColorConstants::Transparent);
        // The transient parent is baked into the shell surface when the
        // window is first shown, so it must be set up-front
        preset_popup_window_->setTransientParent (window ()->windowHandle ());
        // QtWayland faux API: allow flipping above the anchor when the
        // popup would go off-screen (plain popups only slide by default)
        preset_popup_window_->setProperty (
          "_q_waylandPopupConstraintAdjustment",
          // constraint_adjustment_slide_x | slide_y | flip_y
          QVariant::fromValue (1u | 2u | 8u));

        preset_popup_component_ = utils::make_qobject_unique<QQmlComponent> (
          header_->qml_engine (),
          QUrl (QStringLiteral (
            "qrc:/qt/qml/Zrythm/components/basic/PresetListPopup.qml")));

        preset_popup_controller_ =
          utils::make_qobject_unique<PresetPopupController> (this);
        connect (
          preset_popup_controller_.get (), &PresetPopupController::activated,
          this, [this] (int index) {
            header_->applyPresetSelection (index);
            close_preset_popup ();
          });
        connect (
          preset_popup_controller_.get (), &PresetPopupController::dismissed,
          this, [this] { close_preset_popup (); });

        // Release the scene item when the window system closes the popup
        // (e.g. Wayland popup_done after an outside click)
        connect (
          preset_popup_window_.get (), &QWindow::visibleChanged, this,
          [this] (bool visible) {
            if (!visible)
              release_preset_popup_item ();
          });
        if (!on_wayland)
          {
            // X11 has no popup grab for bare QWindows; close when the
            // popup loses activation (click anywhere else)
            connect (
              preset_popup_window_.get (), &QWindow::activeChanged, this, [this] {
                if (
                  preset_popup_window_ != nullptr
                  && !preset_popup_window_->isActive ()
                  && preset_popup_window_->isVisible ())
                  close_preset_popup ();
              });
          }
      }

    release_preset_popup_item ();
    auto * item = qobject_cast<
      QQuickItem *> (preset_popup_component_->createWithInitialProperties (
      {
        { QStringLiteral ("model"),        QVariant::fromValue (model)    },
        { QStringLiteral ("currentIndex"), current_index                  },
        { QStringLiteral ("textRole"),     text_role                      },
        { QStringLiteral ("popupHost"),
         QVariant::fromValue<QObject *> (preset_popup_controller_.get ()) },
    }));
    if (item == nullptr)
      {
        z_warning (
          "Failed to load preset list popup QML: {}",
          preset_popup_component_->errors ().isEmpty ()
            ? QStringLiteral ("unknown error")
            : preset_popup_component_->errors ().constFirst ().toString ());
        return;
      }
    QQmlEngine::setObjectOwnership (item, QQmlEngine::CppOwnership);
    auto * content_item = preset_popup_window_->contentItem ();
    item->setParentItem (content_item);
    item->setParent (content_item);
    preset_popup_item_.reset (item);

    const auto width = std::max (
      qCeil (anchor_strip_rect.width ()), qCeil (item->implicitWidth ()));
    const auto height = qCeil (item->implicitHeight ());
    preset_popup_window_->resize (width, height);
    item->setSize (QSizeF (width, height));

    const auto anchor_bottom_left = QPoint (
      qFloor (anchor_strip_rect.x ()), qCeil (anchor_strip_rect.bottom ()));
    if (on_wayland)
      {
        // QtWayland positions popups relative to the transient parent
        preset_popup_window_->setPosition (pos () + anchor_bottom_left);
      }
    else
      {
        auto popup_pos = mapToGlobal (anchor_bottom_left);
        if (const auto * screen = window ()->windowHandle ()->screen ())
          {
            const auto avail = screen->availableGeometry ();
            popup_pos.setX (
              std::clamp (
                popup_pos.x (), avail.left (),
                std::max (avail.left (), avail.right () - width + 1)));
            popup_pos.setY (
              std::clamp (
                popup_pos.y (), avail.top (),
                std::max (avail.top (), avail.bottom () - height + 1)));
          }
        preset_popup_window_->setPosition (popup_pos);
      }
    preset_popup_window_->show ();
  }

private:
  utils::QObjectUniquePtr<OffscreenQmlScene>     header_;
  utils::QObjectUniquePtr<QQuickWindow>          preset_popup_window_;
  utils::QObjectUniquePtr<QQmlComponent>         preset_popup_component_;
  utils::QObjectUniquePtr<QQuickItem>            preset_popup_item_;
  utils::QObjectUniquePtr<PresetPopupController> preset_popup_controller_;
  QPointF                                        last_mouse_pos_{ -1, -1 };
  bool                                           repaint_pending_ = false;
};
} // namespace

class QtPluginHostWindow::Impl
{
public:
  Impl (QtPluginHostWindow &owner) : owner_ (owner)
  {
    window_ = utils::make_qobject_unique<PluginHostWindowWidget> ([&owner] {
      Q_EMIT owner.closeRequested ();
    });
    // Keep plugin windows above the main window
    window_->setWindowFlags (window_->windowFlags () | Qt::WindowStaysOnTopHint);

    content_ = utils::make_qobject_unique<QWidget> (window_.get ());
    // The content widget must own a real native window: its handle is
    // handed to plugin editors as their parent window
    content_->setAttribute (Qt::WA_NativeWindow);
    content_->setAttribute (Qt::WA_DontCreateNativeAncestors);

    header_qml_ = utils::make_qobject_unique<PluginHeaderWidget> (
      owner_.plugin (), window_.get ());
    auto &offscreen_header = header_qml_->offscreen_header ();
    header_height_ = std::max (1, offscreen_header.implicit_height ());
    // Follow height changes from the QML scene (e.g., theme font
    // changes or the header's expandable diagnostics row), keeping the
    // plugin view area untouched. Non-resizable windows move their fixed
    // height with the header instead
    QObject::connect (
      &offscreen_header, &OffscreenQmlScene::implicitHeightChanged,
      window_.get (), [this] (int height) {
        const auto new_height = std::max (1, height);
        const auto delta = new_height - header_height_;
        if (delta == 0)
          return;
        header_height_ = new_height;
        if (!resizable_)
          {
            window_->setFixedHeight (window_->height () + delta);
            return;
          }
        // Keep the minimum height in sync with the header (see
        // apply_size())
        window_->setMinimumHeight (new_height);
        window_->resize (window_->width (), window_->height () + delta);
      });
    // Keep the window's minimum width in sync with the header's controls;
    // fixed-size windows cannot grow, so only resizable ones track it
    QObject::connect (
      &offscreen_header, &OffscreenQmlScene::implicitWidthChanged,
      window_.get (), [this] {
        if (!resizable_)
          return;
        window_->setMinimumWidth (header_qml_->minimumWidth ());
      });

    auto * layout = new QVBoxLayout (window_.get ());
    layout->setContentsMargins (0, 0, 0, 0);
    layout->setSpacing (0);
    layout->addWidget (header_qml_.get ());
    layout->addWidget (content_.get (), 1);

    QObject::connect (
      &owner_, &plugins::PluginHostWindow::titleChanged, window_.get (),
      &QWidget::setWindowTitle);
    window_->setWindowTitle (owner_.title ());

    // Force native handle creation so getEmbedWindowId() is valid before
    // the window is first shown
    content_->winId ();

    // Track screen changes so plugins can be re-fed the scale factor
    // (Windows per-monitor DPI, moving between screens)
    last_scale_ = static_cast<float> (window_->devicePixelRatioF ());
    auto * handle = window_->windowHandle ();
    QObject::connect (
      handle, &QWindow::screenChanged, window_.get (), [this] (QScreen * screen) {
        watch_screen (screen);
        emit_scale_if_changed ();
      });
    watch_screen (handle->screen ());
  }

  /**
   * @brief Resizes the top-level window, re-applying the size constraints
   * for the current resizable state.
   *
   * A fixed-size window is pinned to the size it was given, so the
   * constraints have to follow every size change rather than being set
   * once.
   */
  void apply_size (int width, int height)
  {
    if (resizable_)
      {
        // Minimum: the header strip (its width once it has controls)
        window_->setMinimumSize (header_qml_->minimumWidth (), header_height_);
        window_->setMaximumSize (QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        window_->resize (width, height);
      }
    else
      {
        window_->setFixedSize (width, height);
      }
  }

  void emit_scale_if_changed ()
  {
    const auto scale = static_cast<float> (window_->devicePixelRatioF ());
    if (scale != last_scale_)
      {
        last_scale_ = scale;
        Q_EMIT owner_.contentScaleFactorChanged (scale);
      }
  }

  void watch_screen (QScreen * screen)
  {
    QObject::disconnect (physical_dpi_connection_);
    QObject::disconnect (logical_dpi_connection_);
    if (screen != nullptr)
      {
        physical_dpi_connection_ = QObject::connect (
          screen, &QScreen::physicalDotsPerInchChanged, window_.get (),
          [this] { emit_scale_if_changed (); });
        logical_dpi_connection_ = QObject::connect (
          screen, &QScreen::logicalDotsPerInchChanged, window_.get (),
          [this] { emit_scale_if_changed (); });
      }
  }

  QtPluginHostWindow &owner_;

  utils::QObjectUniquePtr<PluginHostWindowWidget> window_;
  utils::QObjectUniquePtr<QWidget>                content_;
  utils::QObjectUniquePtr<PluginHeaderWidget>     header_qml_;
  int header_height_ = plugins::PluginHostWindow::kHeaderHeight;

  /** Whether the user may resize the window (see apply_size()). */
  bool resizable_ = true;

  float                   last_scale_ = 1.f;
  QMetaObject::Connection physical_dpi_connection_;
  QMetaObject::Connection logical_dpi_connection_;
};

QtPluginHostWindow::QtPluginHostWindow (plugins::Plugin &plugin)
    : plugins::PluginHostWindow (plugin), pimpl_ (std::make_unique<Impl> (*this))
{
  pimpl_->content_->installEventFilter (this);
  pimpl_->window_->installEventFilter (this);
}

QtPluginHostWindow::~QtPluginHostWindow ()
{
  // The window's destruction delivers hide events to its children; the
  // filter must not observe content_ while the Impl members are being
  // torn down
  pimpl_->content_->removeEventFilter (this);
  pimpl_->window_->removeEventFilter (this);
}

bool
QtPluginHostWindow::eventFilter (QObject * obj, QEvent * ev)
{
  if (obj == pimpl_->content_.get () && ev->type () == QEvent::Resize)
    {
      const auto size = pimpl_->content_->size ();
      Q_EMIT embedSizeChanged (size.width (), size.height ());
    }
  // Popups don't follow their anchor across hosts/platforms, so close the
  // preset popup when the host window moves, resizes or hides
  if (
    obj == pimpl_->window_.get ()
    && (ev->type () == QEvent::Move || ev->type () == QEvent::Resize || ev->type () == QEvent::Hide))
    {
      pimpl_->header_qml_->close_preset_popup ();
    }
  return QObject::eventFilter (obj, ev);
}

void
QtPluginHostWindow::setSizeAndCenter (int width, int height)
{
  // Plugin view sizes are validated at the plugin boundary
  assert (width > 0 && height > 0);
  const auto total_height = height + pimpl_->header_height_;
  pimpl_->apply_size (width, total_height);
  if (const auto * screen = pimpl_->window_->screen (); screen != nullptr)
    {
      const auto avail = screen->availableGeometry ();
      pimpl_->window_->move (
        avail.x () + (avail.width () - width) / 2,
        avail.y () + (avail.height () - total_height) / 2);
    }
}

void
QtPluginHostWindow::setSize (int width, int height)
{
  // Plugin view sizes are validated at the plugin boundary
  assert (width > 0 && height > 0);
  pimpl_->apply_size (width, height + pimpl_->header_height_);
}

void
QtPluginHostWindow::setResizable (bool resizable)
{
  pimpl_->resizable_ = resizable;
  pimpl_->apply_size (pimpl_->window_->width (), pimpl_->window_->height ());
}

void
QtPluginHostWindow::setVisible (bool shouldBeVisible)
{
  pimpl_->window_->setVisible (shouldBeVisible);
  if (shouldBeVisible)
    {
      // The window may only land on its final screen when shown. Check
      // asynchronously so the show (and the header's first render)
      // completes before plugins are re-fed the scale and may resize the
      // window in response
      pimpl_->watch_screen (pimpl_->window_->windowHandle ()->screen ());
      QMetaObject::invokeMethod (
        pimpl_->window_.get (), [this] { pimpl_->emit_scale_if_changed (); },
        Qt::QueuedConnection);
    }
}

WId
QtPluginHostWindow::getEmbedWindowId () const
{
  return pimpl_->content_->winId ();
}

float
QtPluginHostWindow::contentScaleFactor () const
{
  return static_cast<float> (pimpl_->window_->devicePixelRatioF ());
}

} // namespace zrythm::gui
