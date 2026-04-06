// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/qquick/audio_region_waveform_canvas_item.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/loopable_object.h"
#include "structure/arrangement/region_renderer.h"

namespace zrythm::gui::qquick
{

AudioRegionWaveformCanvasItem::AudioRegionWaveformCanvasItem (
  QQuickItem * parent)
    : WaveformCanvasItem (parent)
{
}

void
AudioRegionWaveformCanvasItem::re_serialize_buffer ()
{
  if (audio_region_ != nullptr)
    {
      structure::arrangement::RegionRenderer::serialize_to_buffer (
        *audio_region_, audio_buffer_);
      notifyBufferChanged ();
    }
}

void
AudioRegionWaveformCanvasItem::setRegion (QObject * region)
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

  // Serialize the region's audio into the base buffer
  if (audio_region_ != nullptr)
    {
      re_serialize_buffer ();

      // Re-serialize when loop points or bounds change
      auto * loop_range = audio_region_->loopRange ();
      if (loop_range != nullptr)
        {
          region_connections_.push_back (
            QObject::connect (
              loop_range,
              &structure::arrangement::ArrangerObjectLoopRange::
                loopableObjectPropertiesChanged,
              this, &AudioRegionWaveformCanvasItem::re_serialize_buffer,
              Qt::ConnectionType::QueuedConnection));
        }
      auto * bounds = audio_region_->bounds ();
      if (bounds != nullptr)
        {
          region_connections_.push_back (
            QObject::connect (
              bounds->length (), &dsp::AtomicPositionQmlAdapter::positionChanged,
              this, &AudioRegionWaveformCanvasItem::re_serialize_buffer,
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
              this, &AudioRegionWaveformCanvasItem::re_serialize_buffer,
              Qt::ConnectionType::QueuedConnection));
        }
    }
  else
    {
      audio_buffer_ = juce::AudioSampleBuffer ();
      notifyBufferChanged ();
    }

  Q_EMIT regionChanged ();
}

} // namespace zrythm::gui::qquick
