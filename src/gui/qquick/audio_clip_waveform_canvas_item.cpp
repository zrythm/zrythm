// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "gui/qquick/audio_clip_waveform_canvas_item.h"
#include "structure/arrangement/audio_clip.h"
#include "structure/arrangement/clip_renderer.h"

namespace zrythm::gui::qquick
{

AudioClipWaveformCanvasItem::AudioClipWaveformCanvasItem (QQuickItem * parent)
    : WaveformCanvasItem (parent)
{
}

auto
AudioClipWaveformCanvasItem::take_snapshot () const -> ClipSnapshot
{
  ClipSnapshot snap;
  if (audio_clip_ == nullptr)
    return snap;

  const auto &tempo_map = audio_clip_->get_tempo_map ();
  const auto  clip_start_samples =
    tempo_map.tick_to_samples_rounded (audio_clip_->position ()->asTick ());

  // Loop positions are content-space — convert via ContentTimeWarp (handles
  // arbitrary warp markers). Returns samples relative to the clip start.
  auto *     warp = audio_clip_->contentWarp ();
  const auto content_offset_samples =
    [&] (dsp::ContentPosition * pos) -> int64_t {
    if (pos == nullptr || warp == nullptr)
      return 0;
    const auto abs_samples = warp->contentToTimelineSamples (pos->asTick ());
    return (abs_samples - clip_start_samples).in (units::samples);
  };

  // Fade offsets are plain project ticks from the clip start — NOT warped.
  // The caller passes the actual tick offset; for end_offset (which is stored
  // backwards from the clip end), the caller computes length - end_offset.
  const auto unwarped_offset_samples = [&] (double ticks_from_start) -> int64_t {
    const auto abs_samples = tempo_map.tick_to_samples_rounded (
      audio_clip_->position ()->asTick ()
      + dsp::TimelineTick{ units::ticks (ticks_from_start) });
    return (abs_samples - clip_start_samples).in (units::samples);
  };

  snap.length_samples =
    (audio_clip_->get_end_position_samples (true) - clip_start_samples)
      .in (units::samples);
  snap.gain = audio_clip_->gain ();

  snap.clip_start_samples =
    content_offset_samples (audio_clip_->clipStartPosition ());
  snap.loop_start_samples =
    content_offset_samples (audio_clip_->loopStartPosition ());
  snap.loop_end_samples =
    content_offset_samples (audio_clip_->loopEndPosition ());

  auto * fade_range = audio_clip_->fadeRange ();
  if (fade_range != nullptr)
    {
      snap.fade_in_samples =
        unwarped_offset_samples (fade_range->startOffset ()->ticks ());
      const auto length_ticks = audio_clip_->length ()->ticks ();
      snap.fade_out_samples = unwarped_offset_samples (
        length_ticks - fade_range->endOffset ()->ticks ());
    }

  return snap;
}

void
AudioClipWaveformCanvasItem::handle_property_change ()
{
  if (audio_clip_ == nullptr)
    return;

  const auto current = take_snapshot ();
  // Skip if nothing changed (multiple signals often fire for the same
  // underlying change — e.g., setSamples() cascades through loop/fade
  // signals even when those properties didn't change).
  if (current == last_snapshot_)
    return;

  const auto prev_samples = audio_buffer_.getNumSamples ();
  const auto new_samples = static_cast<int> (current.length_samples);

  // Fast path: if only the clip length grew (no structural changes
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
      // Compute the absolute timeline tick positions at the old and new
      // buffer ends. prev_samples/new_samples are relative to the clip
      // start, so we must add the clip's absolute sample position before
      // converting to ticks.
      const auto &tempo_map = audio_clip_->get_tempo_map ();
      const auto  clip_start_samples =
        tempo_map.tick_to_samples_rounded (audio_clip_->position ()->asTick ())
          .in (units::samples);
      const auto prev_end_tick = tempo_map.samples_to_tick (
        units::precise_sample_t (
          units::samples (clip_start_samples + prev_samples)));
      const auto new_end_tick = tempo_map.samples_to_tick (
        units::precise_sample_t (
          units::samples (clip_start_samples + new_samples)));

      juce::AudioSampleBuffer new_data;
      structure::arrangement::ClipRenderer::serialize_to_buffer (
        *audio_clip_, new_data, std::make_pair (prev_end_tick, new_end_tick));

      audio_buffer_.setSize (2, new_samples, true, true, false);

      const auto new_data_samples = new_data.getNumSamples ();
      // The delta window above is derived via a samples→tick→samples
      // round-trip, which can drift by ±1 sample per leg and make
      // new_data_samples exceed the room left in the buffer. Clamp the copy
      // length to the available space to avoid overrunning audio_buffer_.
      const auto to_copy =
        std::clamp (new_data_samples, 0, new_samples - prev_samples);
      for (int ch = 0; ch < 2; ++ch)
        {
          audio_buffer_.copyFrom (ch, prev_samples, new_data, ch, 0, to_copy);
        }
    }
  else
    {
      // Structural change (loop, fade, gain, or shrink) — full re-serialize.
      structure::arrangement::ClipRenderer::serialize_to_buffer (
        *audio_clip_, audio_buffer_);
    }

  last_snapshot_ = current;
  notifyBufferChanged ();
}

void
AudioClipWaveformCanvasItem::setAudioClip (
  structure::arrangement::AudioClip * clip)
{
  if (audio_clip_ == clip)
    return;
  audio_clip_ = clip;

  // Disconnect old signal connections from previous clip
  for (const auto &connection : clip_connections_)
    QObject::disconnect (connection);
  clip_connections_.clear ();

  // Serialize the clip's audio into the base buffer
  if (audio_clip_ != nullptr)
    {
      last_snapshot_ = take_snapshot ();
      structure::arrangement::ClipRenderer::serialize_to_buffer (
        *audio_clip_, audio_buffer_);
      notifyBufferChanged ();

      // Re-serialize when loop points or bounds change
      clip_connections_.push_back (
        QObject::connect (
          audio_clip_, &structure::arrangement::Clip::loopablePropertiesChanged,
          this, &AudioClipWaveformCanvasItem::handle_property_change,
          Qt::ConnectionType::QueuedConnection));
      // Re-serialize when warp map changes (e.g. switching to/from absolute mode)
      clip_connections_.push_back (
        QObject::connect (
          audio_clip_, &structure::arrangement::Clip::timelineLengthTicksChanged,
          this, &AudioClipWaveformCanvasItem::handle_property_change,
          Qt::ConnectionType::QueuedConnection));
      clip_connections_.push_back (
        QObject::connect (
          audio_clip_->length (), &dsp::Position::positionChanged, this,
          &AudioClipWaveformCanvasItem::handle_property_change,
          Qt::ConnectionType::QueuedConnection));
      auto * fade_range = audio_clip_->fadeRange ();
      if (fade_range != nullptr)
        {
          clip_connections_.push_back (
            QObject::connect (
              fade_range,
              &structure::arrangement::ArrangerObjectFadeRange::
                fadePropertiesChanged,
              this, &AudioClipWaveformCanvasItem::handle_property_change,
              Qt::ConnectionType::QueuedConnection));
        }
    }
  else
    {
      audio_buffer_ = juce::AudioSampleBuffer ();
      last_snapshot_ = {};
      notifyBufferChanged ();
    }

  Q_EMIT audioClipChanged ();
}

void
AudioClipWaveformCanvasItem::setTempoMap (QObject * tempoMap)
{
  if (tempo_map_ == tempoMap)
    return;

  for (const auto &connection : tempo_map_connections_)
    QObject::disconnect (connection);
  tempo_map_connections_.clear ();

  tempo_map_ = tempoMap;

  if (tempo_map_ != nullptr)
    {
      tempo_map_connections_.push_back (
        QObject::connect (
          tempo_map_, SIGNAL (tempoEventsChanged ()), this,
          SLOT (handle_property_change ()),
          Qt::ConnectionType::QueuedConnection));
    }

  Q_EMIT tempoMapChanged ();
}

} // namespace zrythm::gui::qquick
