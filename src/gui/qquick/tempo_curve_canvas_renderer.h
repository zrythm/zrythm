// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include <QtCanvasPainter/qcanvaslineargradient.h>
#include <QtCanvasPainter/qcanvaspainter.h>
#include <QtCanvasPainter/qcanvaspainteritemrenderer.h>

namespace zrythm::gui::qquick
{

class TempoCurveCanvasItem;

/**
 * @brief Renders the tempo automation curve in the background of the tempo
 * lane.
 *
 * The curve is drawn from the project's tempo objects (the same ones rendered
 * as pills): constant segments as steps, linear segments as ramps, stroked and
 * filled with a translucent vertical gradient. The event data is snapshotted
 * in synchronizeData() (render thread blocked) and painted in paint() (render
 * thread).
 */
class TempoCurveCanvasRenderer : public QCanvasPainterItemRenderer
{
public:
  TempoCurveCanvasRenderer () = default;
  Q_DISABLE_COPY_MOVE (TempoCurveCanvasRenderer)

  void synchronizeData (QCanvasPainterItem * item) override;
  void paint (QCanvasPainter * painter) override;

private:
  /// A tempo object snapshotted on the GUI thread.
  struct CachedTempoEvent
  {
    double tick;
    double bpm;
    bool   linear; ///< true if the curve from this event to the next is linear
  };

  QColor curve_color_{ "#009DFF" };
  float  scroll_x_{ 0.0f };
  float  scroll_x_plus_width_{ 0.0f };
  float  px_per_tick_{ 0.0f };
  float  lane_height_{ 0.0f };
  float  min_bpm_{ 100.0f };
  float  max_bpm_{ 160.0f };
  // Base tempo at tick 0 (governs the region from 0 up to the first object).
  double                        base_bpm_{ 120.0 };
  std::vector<CachedTempoEvent> cached_events_;
};

} // namespace zrythm::gui::qquick
