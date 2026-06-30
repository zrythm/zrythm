// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>

#include "gui/qquick/waveform_canvas_item.h"

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
