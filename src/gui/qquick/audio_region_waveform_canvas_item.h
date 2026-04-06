// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/qquick/waveform_canvas_item.h"

#include <QPointer>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::structure::arrangement
{
class AudioRegion;
}

namespace zrythm::gui::qquick
{

/**
 * @brief AudioRegion-specific waveform canvas.
 *
 * Handles AudioRegion buffer serialization and re-serialization
 * on region property changes (loop, bounds, fades).
 */
class AudioRegionWaveformCanvasItem : public WaveformCanvasItem
{
  Q_OBJECT
  QML_NAMED_ELEMENT (AudioRegionWaveformCanvas)

  Q_PROPERTY (QObject * region READ region WRITE setRegion NOTIFY regionChanged)

public:
  explicit AudioRegionWaveformCanvasItem (QQuickItem * parent = nullptr);

  QObject * region () const { return region_; }
  void      setRegion (QObject * region);

Q_SIGNALS:
  void regionChanged ();

private:
  void re_serialize_buffer ();

  QObject *                                     region_ = nullptr;
  QPointer<structure::arrangement::AudioRegion> audio_region_;
  std::vector<QMetaObject::Connection>          region_connections_;
};

} // namespace zrythm::gui::qquick
