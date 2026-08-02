// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/plugin_header_qml.h"
#include "plugins/plugin_host_window.h"
#include "utils/logger.h"

#include <QPointer>
#include <QQmlEngine>
#include <QQuickItem>
#include <QResizeEvent>

namespace zrythm::gui
{

namespace
{
QPointer<QQmlEngine> shared_qml_engine;
}

QUrl
plugin_header_bar_qml_url ()
{
  return QUrl (
    QStringLiteral ("qrc:/qt/qml/Zrythm/components/PluginWindowHeaderBar.qml"));
}

void
set_plugin_header_qml_engine (QQmlEngine * engine)
{
  shared_qml_engine = engine;
}

utils::QObjectUniquePtr<QQuickWidget>
make_plugin_header_widget (plugins::PluginHostWindow &window, QWidget * parent)
{
  auto widget =
    shared_qml_engine != nullptr
      ? utils::make_qobject_unique<QQuickWidget> (shared_qml_engine, parent)
      : utils::make_qobject_unique<QQuickWidget> (parent);
  widget->setAttribute (Qt::WA_DontCreateNativeAncestors);
  widget->setInitialProperties (
    {
      { QStringLiteral ("hostWindow"),
       QVariant::fromValue<QObject *> (&window) }
  });
  widget->setResizeMode (QQuickWidget::SizeRootObjectToView);
  widget->setSource (plugin_header_bar_qml_url ());
  if (!plugin_header_widget_is_valid (*widget))
    {
      z_warning (
        "Failed to load plugin window header bar QML: {}",
        widget->errors ().isEmpty ()
          ? QStringLiteral ("unknown error")
          : widget->errors ().constFirst ().toString ());
    }
  return widget;
}

bool
plugin_header_widget_is_valid (const QQuickWidget &widget)
{
  return widget.status () == QQuickWidget::Ready
         && widget.rootObject () != nullptr;
}

OffscreenQmlHeader::OffscreenQmlHeader (
  plugins::PluginHostWindow &window,
  QObject *                  parent)
    : QObject (parent)
{
  widget_ = make_plugin_header_widget (window);
  if (is_valid ())
    {
      // Repaint when theme-dependent colors change (the scene does not
      // re-render by itself while hidden)
      QObject::connect (
        widget_->rootObject (), SIGNAL (colorsChanged ()), this,
        SIGNAL (repaintNeeded ()));
    }
}

bool
OffscreenQmlHeader::is_valid () const
{
  return widget_ != nullptr && plugin_header_widget_is_valid (*widget_);
}

QImage
OffscreenQmlHeader::grab_frame ()
{
  if (!is_valid ())
    return {};
  return widget_->grabFramebuffer ();
}

void
OffscreenQmlHeader::resize (int width, int height)
{
  const auto old_size = widget_->size ();
  widget_->resize (width, height);
  // Resize events are deferred until a widget is shown, but QQuickWidget
  // initializes its rendering from QResizeEvent. This widget is never shown,
  // so deliver the event ourselves.
  if (!widget_->isVisible ())
    {
      QResizeEvent ev (QSize (width, height), old_size);
      QCoreApplication::sendEvent (widget_.get (), &ev);
    }
}

int
OffscreenQmlHeader::controls_implicit_width () const
{
  if (!is_valid ())
    return 64;
  return std::max (
    64, static_cast<int> (widget_->rootObject ()->implicitWidth ()));
}

} // namespace zrythm::gui
