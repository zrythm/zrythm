// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/plugin_header_qml.h"
#include "gui/backend/qt_plugin_host_window.h"
#include "utils/qt.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QQuickItem>
#include <QQuickWidget>
#include <QScreen>
#include <QVBoxLayout>
#include <QWidget>

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

    header_ = utils::make_qobject_unique<QWidget> (window_.get ());
    header_->setFixedHeight (kHeaderHeight);
    // Filled with the Window role of the application palette
    header_->setAutoFillBackground (true);

    header_qml_ = make_plugin_header_widget (owner_, header_.get ());
    if (auto * root_item = header_qml_->rootObject (); root_item != nullptr)
      {
        header_qml_->setMinimumWidth (
          static_cast<int> (root_item->implicitWidth ()));
      }

    auto * header_layout = new QHBoxLayout (header_.get ());
    header_layout->setContentsMargins (0, 0, 0, 0);
    header_layout->addWidget (header_qml_.get ());

    auto * layout = new QVBoxLayout (window_.get ());
    layout->setContentsMargins (0, 0, 0, 0);
    layout->setSpacing (0);
    layout->addWidget (header_.get ());
    layout->addWidget (content_.get (), 1);

    QObject::connect (
      &owner_, &plugins::PluginHostWindow::titleChanged, window_.get (),
      &QWidget::setWindowTitle);
    window_->setWindowTitle (owner_.title ());

    // Force native handle creation so getEmbedWindowId() is valid before
    // the window is first shown
    content_->winId ();
  }

  QtPluginHostWindow &owner_;

  utils::QObjectUniquePtr<PluginHostWindowWidget> window_;
  utils::QObjectUniquePtr<QWidget>                content_;
  utils::QObjectUniquePtr<QWidget>                header_;
  utils::QObjectUniquePtr<QQuickWidget>           header_qml_;
};

QtPluginHostWindow::QtPluginHostWindow (plugins::Plugin &plugin)
    : plugins::PluginHostWindow (plugin), pimpl_ (std::make_unique<Impl> (*this))
{
}

QtPluginHostWindow::~QtPluginHostWindow () = default;

void
QtPluginHostWindow::setSizeAndCenter (int width, int height)
{
  const auto total_height = height + kHeaderHeight;
  pimpl_->window_->resize (width, total_height);
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
  pimpl_->window_->resize (width, height + kHeaderHeight);
}

void
QtPluginHostWindow::setResizable (bool resizable)
{
  if (resizable)
    {
      // Minimum: the header strip (its width once it has controls)
      pimpl_->window_->setMinimumSize (
        pimpl_->header_qml_->minimumWidth (), kHeaderHeight);
      pimpl_->window_->setMaximumSize (QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }
  else
    {
      pimpl_->window_->setFixedSize (pimpl_->window_->size ());
    }
}

void
QtPluginHostWindow::setVisible (bool shouldBeVisible)
{
  pimpl_->window_->setVisible (shouldBeVisible);
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
