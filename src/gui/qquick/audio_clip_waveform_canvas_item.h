// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <vector>

#include "gui/qquick/waveform_canvas_item.h"
#include "gui/qquick/waveform_canvas_renderer.h"
#include "structure/arrangement/audio_clip.h"

#include <QPointer>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::structure::arrangement
{
class AudioClip;
}

namespace zrythm::gui::qquick
{

/**
 * @brief AudioClip-specific waveform canvas.
 *
 * Handles AudioClip buffer serialization and re-serialization
 * on clip property changes (loop, bounds, fades).
 *
 * Uses a cached snapshot of clip properties to avoid redundant
 * re-serializations when multiple signals fire for the same change.
 */
class AudioClipWaveformCanvasItem : public WaveformCanvasItem
{
  Q_OBJECT
  QML_NAMED_ELEMENT (AudioClipWaveformCanvas)

  Q_PROPERTY (
    zrythm::structure::arrangement::AudioClip * audioClip READ audioClip WRITE
      setAudioClip NOTIFY audioClipChanged)
  Q_PROPERTY (
    QObject * tempoMap READ tempoMap WRITE setTempoMap NOTIFY tempoMapChanged)

public:
  explicit AudioClipWaveformCanvasItem (QQuickItem * parent = nullptr);

  structure::arrangement::AudioClip * audioClip () const { return audio_clip_; }
  void setAudioClip (structure::arrangement::AudioClip * clip);

  QObject * tempoMap () const { return tempo_map_; }
  void      setTempoMap (QObject * tempoMap);

  /// Only wrap when the clip is actually looped. Non-looped clips may still
  /// have default loop positions, which would cause false wrapping.
  bool hasLoop () const
  {
    return audio_clip_ != nullptr && audio_clip_->looped ()
           && last_snapshot_.loop_end_samples > last_snapshot_.loop_start_samples;
  }

  /// Sample-space offset of the loop-start position relative to clip start
  /// (content-space, not buffer-space).
  int64_t loopStartFrame () const { return last_snapshot_.loop_start_samples; }
  int64_t loopEndFrame () const { return last_snapshot_.loop_end_samples; }
  int64_t clipStartFrame () const { return last_snapshot_.clip_start_samples; }

  /// Where the loop region begins in the serialized buffer. The buffer layout
  /// is: [intro: clip_start→loop_end] [loop: loop_start→loop_end] repeated.
  /// So the loop region starts at frame (loop_end - clip_start).
  int64_t loopRegionBufferStart () const
  {
    return last_snapshot_.loop_end_samples - last_snapshot_.clip_start_samples;
  }

  /// Computes a per-pixel frame mapping that accounts for the tempo map's
  /// non-linear tick-to-sample conversion. Called by the renderer during
  /// synchronize() so the waveform visually aligns with what plays at each
  /// timeline position.
  std::vector<int64_t> computeTimelineFrameMapping (
    int    canvas_width,
    double reference_width,
    double reference_x) const;

Q_SIGNALS:
  void audioClipChanged ();
  void tempoMapChanged ();

private:
  /**
   * @brief Snapshot of clip properties that affect the waveform.
   *
   * Compared against the current state to determine what changed and
   * whether an incremental append or full re-serialize is needed.
   */
  struct ClipSnapshot
  {
    int64_t clip_start_samples{};
    int64_t loop_start_samples{};
    int64_t loop_end_samples{};
    int64_t length_samples{};
    int64_t fade_in_samples{};
    int64_t fade_out_samples{};
    float   gain{ 1.0f };

    friend bool
    operator== (const ClipSnapshot &, const ClipSnapshot &) = default;
  };

  ClipSnapshot take_snapshot () const;

private Q_SLOTS:
  void handle_property_change ();

private:
  QPointer<structure::arrangement::AudioClip> audio_clip_;
  std::vector<QMetaObject::Connection>        clip_connections_;
  QPointer<QObject>                           tempo_map_;
  std::vector<QMetaObject::Connection>        tempo_map_connections_;
  ClipSnapshot                                last_snapshot_;
};

} // namespace zrythm::gui::qquick
