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

auto
AudioRegionWaveformCanvasItem::take_snapshot () const -> RegionSnapshot
{
  RegionSnapshot snap;
  if (audio_region_ == nullptr)
    return snap;

  snap.length_samples = audio_region_->bounds ()->length ()->samples ();
  snap.gain = audio_region_->gain ();

  auto * loop_range = audio_region_->loopRange ();
  if (loop_range != nullptr)
    {
      snap.clip_start_samples = loop_range->clipStartPosition ()->samples ();
      snap.loop_start_samples = loop_range->loopStartPosition ()->samples ();
      snap.loop_end_samples = loop_range->loopEndPosition ()->samples ();
    }

  auto * fade_range = audio_region_->fadeRange ();
  if (fade_range != nullptr)
    {
      snap.fade_in_samples = fade_range->startOffset ()->samples ();
      snap.fade_out_samples = fade_range->endOffset ()->samples ();
    }

  return snap;
}

void
AudioRegionWaveformCanvasItem::handle_property_change ()
{
  if (audio_region_ == nullptr)
    return;

  const auto current = take_snapshot ();
  // Skip if nothing changed (multiple signals often fire for the same
  // underlying change — e.g., setSamples() cascades through loop/fade
  // signals even when those properties didn't change).
  if (current == last_snapshot_)
    return;

  const auto prev_samples = audio_buffer_.getNumSamples ();
  const auto new_samples = static_cast<int> (current.length_samples);

  // Fast path: if only the region length grew (no structural changes
  // to loop, fades, or gain), serialize only the new portion and
  // append it to the existing buffer instead of re-reading the
  // entire clip.
  if (
    current.clip_start_samples == last_snapshot_.clip_start_samples
    && current.loop_start_samples == last_snapshot_.loop_start_samples
    && current.loop_end_samples == last_snapshot_.loop_end_samples
    && current.fade_in_samples == last_snapshot_.fade_in_samples
    && current.fade_out_samples == last_snapshot_.fade_out_samples
    && current.gain == last_snapshot_.gain && new_samples > prev_samples)
    {
      const auto  region_start_ticks = audio_region_->position ()->ticks ();
      const auto &tempo_map = audio_region_->get_tempo_map ();
      const auto  prev_ticks = tempo_map.samples_to_tick (
        units::precise_sample_t (units::samples (prev_samples)));
      const auto new_ticks = tempo_map.samples_to_tick (
        units::precise_sample_t (units::samples (new_samples)));

      juce::AudioSampleBuffer new_data;
      structure::arrangement::RegionRenderer::serialize_to_buffer (
        *audio_region_, new_data,
        std::make_pair (
          region_start_ticks + prev_ticks.in (units::ticks),
          region_start_ticks + new_ticks.in (units::ticks)));

      audio_buffer_.setSize (2, new_samples, true, true, false);

      const auto new_data_samples = new_data.getNumSamples ();
      assert (new_data_samples > 0 && new_data_samples <= new_samples);
      for (int ch = 0; ch < 2; ++ch)
        {
          audio_buffer_.copyFrom (
            ch, prev_samples, new_data, ch, 0, new_data_samples);
        }
    }
  else
    {
      // Structural change (loop, fade, gain, or shrink) — full re-serialize.
      structure::arrangement::RegionRenderer::serialize_to_buffer (
        *audio_region_, audio_buffer_);
    }

  last_snapshot_ = current;
  notifyBufferChanged ();
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
      last_snapshot_ = take_snapshot ();
      structure::arrangement::RegionRenderer::serialize_to_buffer (
        *audio_region_, audio_buffer_);
      notifyBufferChanged ();

      // Re-serialize when loop points or bounds change
      auto * loop_range = audio_region_->loopRange ();
      if (loop_range != nullptr)
        {
          region_connections_.push_back (
            QObject::connect (
              loop_range,
              &structure::arrangement::ArrangerObjectLoopRange::
                loopableObjectPropertiesChanged,
              this, &AudioRegionWaveformCanvasItem::handle_property_change,
              Qt::ConnectionType::QueuedConnection));
        }
      auto * bounds = audio_region_->bounds ();
      if (bounds != nullptr)
        {
          region_connections_.push_back (
            QObject::connect (
              bounds->length (), &dsp::AtomicPositionQmlAdapter::positionChanged,
              this, &AudioRegionWaveformCanvasItem::handle_property_change,
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
              this, &AudioRegionWaveformCanvasItem::handle_property_change,
              Qt::ConnectionType::QueuedConnection));
        }
    }
  else
    {
      audio_buffer_ = juce::AudioSampleBuffer ();
      last_snapshot_ = {};
      notifyBufferChanged ();
    }

  Q_EMIT regionChanged ();
}

} // namespace zrythm::gui::qquick
