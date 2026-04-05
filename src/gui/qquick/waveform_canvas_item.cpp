// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/qquick/waveform_canvas_item.h"
#include "gui/qquick/waveform_canvas_renderer.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/loopable_object.h"
#include "structure/arrangement/region_renderer.h"

namespace zrythm::gui::qquick
{

WaveformCanvasItem::WaveformCanvasItem (QQuickItem * parent)
    : QCanvasPainterItem (parent)
{
  setFillColor (Qt::transparent);
  setAlphaBlending (true);
}

QCanvasPainterItemRenderer *
WaveformCanvasItem::createItemRenderer () const
{
  return new WaveformCanvasRenderer ();
}

void
WaveformCanvasItem::re_serialize_buffer ()
{
  if (audio_region_ != nullptr)
    {
      if (!audio_buffer_.has_value ())
        audio_buffer_.emplace ();
      structure::arrangement::RegionRenderer::serialize_to_buffer (
        *audio_region_, *audio_buffer_);
    }
  update ();
}

void
WaveformCanvasItem::setRegion (QObject * region)
{
  if (region_ == region)
    return;
  region_ = region;

  audio_region_ =
    qobject_cast<zrythm::structure::arrangement::AudioRegion *> (region);

  // Disconnect old signal connections from previous region
  for (const auto &connection : region_connections_)
    QObject::disconnect (connection);
  region_connections_.clear ();

  // Serialize the region's audio into a buffer
  audio_buffer_ = std::nullopt;
  if (audio_region_ != nullptr)
    {
      audio_buffer_.emplace ();
      zrythm::structure::arrangement::RegionRenderer::serialize_to_buffer (
        *audio_region_, *audio_buffer_);

      // Re-serialize when loop points or bounds change
      auto * loop_range = audio_region_->loopRange ();
      if (loop_range != nullptr)
        {
          region_connections_.push_back (
            QObject::connect (
              loop_range,
              &structure::arrangement::ArrangerObjectLoopRange::
                loopableObjectPropertiesChanged,
              this, &WaveformCanvasItem::re_serialize_buffer,
              Qt::ConnectionType::QueuedConnection));
        }
      auto * bounds = audio_region_->bounds ();
      if (bounds != nullptr)
        {
          region_connections_.push_back (
            QObject::connect (
              bounds->length (), &dsp::AtomicPositionQmlAdapter::positionChanged,
              this, &WaveformCanvasItem::re_serialize_buffer,
              Qt::ConnectionType::QueuedConnection));
        }
      auto * fade_range = audio_region_->fadeRange ();
      if (fade_range != nullptr)
        {
          region_connections_.push_back (
            QObject::connect (
              fade_range,
              &structure::arrangement::ArrangerObjectFadeRange::
                fadePropertiesChanged,
              this, &WaveformCanvasItem::re_serialize_buffer,
              Qt::ConnectionType::QueuedConnection));
        }
    }

  Q_EMIT regionChanged ();
  update ();
}

void
WaveformCanvasItem::setPxPerTick (qreal px)
{
  if (qFuzzyCompare (px_per_tick_, px))
    return;
  px_per_tick_ = px;
  Q_EMIT pxPerTickChanged ();
  update ();
}

void
WaveformCanvasItem::setWaveformColor (const QColor &color)
{
  if (waveform_color_ == color)
    return;
  waveform_color_ = color;
  Q_EMIT waveformColorChanged ();
  update ();
}

void
WaveformCanvasItem::setOutlineColor (const QColor &color)
{
  if (outline_color_ == color)
    return;
  outline_color_ = color;
  Q_EMIT outlineColorChanged ();
  update ();
}

} // namespace zrythm::gui::qquick
