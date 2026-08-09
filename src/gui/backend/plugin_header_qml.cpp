// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/plugin_header_qml.h"
#include "plugins/plugin.h"
#include "plugins/plugin_host_window.h"
#include "utils/logger.h"

#include <QGuiApplication>
#include <QPalette>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickWindow>
#include <QTimer>

#include <rhi/qrhi.h>

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

OffscreenQmlHeader::OffscreenQmlHeader (plugins::Plugin &plugin, QObject * parent)
    : QObject (parent)
{
  auto * engine = shared_qml_engine.data ();
  if (engine == nullptr)
    {
      fallback_engine_ = utils::make_qobject_unique<QQmlEngine> ();
      engine = fallback_engine_.get ();
    }

  render_control_ = std::make_unique<QQuickRenderControl> ();
  quick_window_ = std::make_unique<QQuickWindow> (render_control_.get ());
  quick_window_->setColor (QGuiApplication::palette ().color (QPalette::Window));

  component_ = utils::make_qobject_unique<QQmlComponent> (
    engine, plugin_header_bar_qml_url ());
  auto * root_object = component_->createWithInitialProperties (
    {
      { QStringLiteral ("plugin"), QVariant::fromValue<QObject *> (&plugin) }
  });
  root_item_ = qobject_cast<QQuickItem *> (root_object);
  if (!is_valid ())
    {
      z_warning (
        "Failed to load plugin window header bar QML: {}",
        component_->errors ().isEmpty ()
          ? QStringLiteral ("unknown error")
          : component_->errors ().constFirst ().toString ());
      delete root_object;
      return;
    }
  // C++-owned and parented to the content item (both visually and as
  // QObject): setParentItem() alone only sets the visual parent, and
  // ~QQuickWindow() deletes only its contentItem, so without this the tree
  // would be owned solely by the shared engine's JS heap and rely on GC
  QQmlEngine::setObjectOwnership (root_item_, QQmlEngine::CppOwnership);
  root_item_->setParentItem (quick_window_->contentItem ());
  root_item_->setParent (quick_window_->contentItem ());

  // Repaint when theme-dependent colors change or on any scene change
  // (hover/pressed visuals, binding-driven text changes, ...). The scene
  // only renders on demand. All notifications go through scheduleRepaint(),
  // which defers and coalesces them as required by the
  // QQuickRenderControl::sceneChanged()/renderRequested() docs
  connect (
    root_item_, SIGNAL (colorsChanged ()), this, SLOT (scheduleRepaint ()));
  connect (
    render_control_.get (), &QQuickRenderControl::sceneChanged, this,
    &OffscreenQmlHeader::scheduleRepaint);
  connect (
    render_control_.get (), &QQuickRenderControl::renderRequested, this,
    &OffscreenQmlHeader::scheduleRepaint);
  connect (root_item_, &QQuickItem::implicitHeightChanged, this, [this] {
    Q_EMIT implicitHeightChanged (implicit_height ());
  });
  connect (root_item_, &QQuickItem::implicitWidthChanged, this, [this] {
    Q_EMIT implicitWidthChanged (controls_implicit_width ());
  });
}

OffscreenQmlHeader::~OffscreenQmlHeader () = default;

void
OffscreenQmlHeader::scheduleRepaint ()
{
  if (repaint_pending_)
    return;
  repaint_pending_ = true;
  // Cancelled automatically if this is destroyed before the timer fires
  QTimer::singleShot (0, this, [this] {
    repaint_pending_ = false;
    Q_EMIT repaintNeeded ();
  });
}

bool
OffscreenQmlHeader::is_valid () const
{
  return component_ != nullptr && component_->status () == QQmlComponent::Ready
         && root_item_ != nullptr;
}

bool
OffscreenQmlHeader::ensure_render_target ()
{
  if (!is_valid () || logical_size_.isEmpty () || dpr_ <= 0.)
    return false;

  if (!renderer_initialized_)
    {
      if (!render_control_->initialize ())
        {
          z_warning (
            "OffscreenQmlHeader: QQuickRenderControl::initialize failed");
          return false;
        }
      renderer_initialized_ = true;
    }

  const auto pixel_size = logical_size_ * dpr_;
  if (
    texture_ != nullptr && texture_->pixelSize () == pixel_size
    && render_target_dpr_ == dpr_)
    return true;

  auto * rhi = render_control_->rhi ();
  texture_.reset (rhi->newTexture (
    QRhiTexture::RGBA8, pixel_size, 1,
    QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
  if (!texture_->create ())
    {
      z_warning ("OffscreenQmlHeader: failed to create render texture");
      texture_.reset ();
      return false;
    }
  depth_stencil_.reset (
    rhi->newRenderBuffer (QRhiRenderBuffer::DepthStencil, pixel_size, 1));
  if (!depth_stencil_->create ())
    {
      z_warning ("OffscreenQmlHeader: failed to create depth-stencil buffer");
      texture_.reset ();
      depth_stencil_.reset ();
      return false;
    }

  QRhiTextureRenderTargetDescription rt_desc (
    QRhiColorAttachment (texture_.get ()));
  rt_desc.setDepthStencilBuffer (depth_stencil_.get ());
  render_target_.reset (rhi->newTextureRenderTarget (rt_desc));
  render_pass_.reset (render_target_->newCompatibleRenderPassDescriptor ());
  render_target_->setRenderPassDescriptor (render_pass_.get ());
  if (!render_target_->create ())
    {
      z_warning ("OffscreenQmlHeader: failed to create render target");
      texture_.reset ();
      depth_stencil_.reset ();
      render_target_.reset ();
      render_pass_.reset ();
      return false;
    }

  auto quick_rt =
    QQuickRenderTarget::fromRhiRenderTarget (render_target_.get ());
  quick_rt.setDevicePixelRatio (dpr_);
  quick_window_->setRenderTarget (quick_rt);
  render_target_dpr_ = dpr_;
  return true;
}

QImage
OffscreenQmlHeader::grab_frame ()
{
  if (!ensure_render_target ())
    return {};

  render_control_->polishItems ();
  render_control_->beginFrame ();
  render_control_->sync ();
  render_control_->render ();

  auto *             rhi = render_control_->rhi ();
  QRhiReadbackResult read_result;
  auto *             batch = rhi->nextResourceUpdateBatch ();
  batch->readBackTexture (texture_.get (), &read_result);
  render_control_->commandBuffer ()->resourceUpdate (batch);
  render_control_->endFrame ();

  if (read_result.data.isEmpty ())
    return {};
  const QImage wrapper (
    reinterpret_cast<const uchar *> (read_result.data.constData ()),
    read_result.pixelSize.width (), read_result.pixelSize.height (),
    QImage::Format_RGBA8888_Premultiplied);
  // wrapper references read_result's data - detach
  return rhi->isYUpInFramebuffer () ? wrapper.flipped () : wrapper.copy ();
}

void
OffscreenQmlHeader::resize (int width, int height)
{
  if (!is_valid ())
    return;
  logical_size_ = QSize (std::max (1, width), std::max (1, height));
  quick_window_->setGeometry (
    0, 0, logical_size_.width (), logical_size_.height ());
  quick_window_->contentItem ()->setSize (logical_size_);
  root_item_->setSize (logical_size_);
  ensure_render_target ();
}

void
OffscreenQmlHeader::set_device_pixel_ratio (qreal dpr)
{
  if (dpr <= 0. || dpr == dpr_)
    return;
  dpr_ = dpr;
  ensure_render_target ();
}

int
OffscreenQmlHeader::controls_implicit_width () const
{
  if (!is_valid ())
    return 64;
  return std::max (64, static_cast<int> (root_item_->implicitWidth ()));
}

int
OffscreenQmlHeader::implicit_height () const
{
  if (!is_valid ())
    return plugins::PluginHostWindow::kHeaderHeight;
  return std::max (1, static_cast<int> (root_item_->implicitHeight ()));
}

} // namespace zrythm::gui
