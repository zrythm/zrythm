// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "dsp/curve.h"
#include "structure/arrangement/automation_point.h"

#include <QColor>
#include <QtCanvasPainter/qcanvaslineargradient.h>
#include <QtCanvasPainter/qcanvaspainter.h>
#include <QtCanvasPainter/qcanvaspainteritemrenderer.h>

namespace zrythm::gui::qquick
{

class AutomationClipCanvasItem;

class AutomationClipCanvasRenderer : public QCanvasPainterItemRenderer
{
public:
  AutomationClipCanvasRenderer () = default;
  Q_DISABLE_COPY_MOVE (AutomationClipCanvasRenderer)

  void synchronize (QCanvasPainterItem * item) override;
  void paint (QCanvasPainter * painter) override;

private:
  struct CachedControlPoint
  {
    /** Pixel x coordinate (after loop unwrapping). */
    float px{};
    /** Pixel y coordinate (pre-computed from val). */
    float py{};
    /** Normalized automation value (0..1) at this point's position. */
    float val{};
    /**
     * @brief Curve options governing the segment that starts at this point
     * (taken from the source segment's governing automation point).
     */
    dsp::CurveOptions curve_opts;

    // --- Source-segment sampling parameters ---
    // The segment starting at this point is a (possibly partial) slice of a
    // source automation segment seg_value_a -> seg_value_b, sampled over the
    // t-range [seg_t_start, seg_t_max]. This lets loop boundaries cut a curve
    // mid-segment without distorting non-linear curves: paint() always
    // evaluates the original source segment over the correct t-subrange.
    /** Start value of the source segment (governing point's value). */
    float seg_value_a = 0.0f;
    /** End value of the source segment (successor's value, or seg_value_a if
     * held flat). */
    float seg_value_b = 0.0f;
    /** t-range of the source segment to render for this emitted segment. */
    float seg_t_start = 0.0f;
    float seg_t_max = 1.0f;

    /** True when this point is a real automation point (drawn as a circle). */
    bool is_real_point = false;
  };

  std::vector<CachedControlPoint> points_;
  QColor                          curve_color_;
  float                           canvas_width_ = 0.0f;
  float                           canvas_height_ = 0.0f;
  double                          reference_width_ = 0;
  double                          reference_x_ = 0;
  bool                            draw_points_ = false;
  /// When false, render the original source sequence only (no loop unwrapping,
  /// no clip-start offset) — used by the automation editor.
  bool apply_loops_ = true;
  /// Canvas-local cursor X while hovering a draggable curve segment, or -1.
  /// The renderer highlights the single emitted segment containing this X
  /// (looped copies are skipped because the hit-test only succeeds on the
  /// real, draggable segment).
  double hovered_x_ = -1.0;
};

} // namespace zrythm::gui::qquick
