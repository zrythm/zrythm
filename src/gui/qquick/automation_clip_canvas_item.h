// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "gui/qquick/clip_canvas_item_base.h"
#include "structure/arrangement/automation_clip.h"

#include <QColor>
#include <QItemSelectionModel>
#include <QPointer>

namespace zrythm::gui::qquick
{

class AutomationClipCanvasRenderer;

class AutomationClipCanvasItem : public ClipCanvasItemBase
{
  Q_OBJECT
  QML_NAMED_ELEMENT (AutomationClipCanvas)

  Q_PROPERTY (
    zrythm::structure::arrangement::AutomationClip * automationClip READ
      automationClip WRITE setAutomationClip NOTIFY automationClipChanged)
  Q_PROPERTY (
    QColor curveColor READ curveColor WRITE setCurveColor NOTIFY
      curveColorChanged)
  Q_PROPERTY (
    QItemSelectionModel * selectionModel READ selectionModel WRITE
      setSelectionModel NOTIFY selectionModelChanged)
  Q_PROPERTY (
    bool dragActive READ dragActive WRITE setDragActive NOTIFY dragActiveChanged)
  Q_PROPERTY (
    double dragDeltaPx READ dragDeltaPx WRITE setDragDeltaPx NOTIFY
      dragDeltaPxChanged)
  Q_PROPERTY (
    double dragDeltaY READ dragDeltaY WRITE setDragDeltaY NOTIFY
      dragDeltaYChanged)
  Q_PROPERTY (
    bool drawPoints READ drawPoints WRITE setDrawPoints NOTIFY drawPointsChanged)
  Q_PROPERTY (
    bool applyLoops READ applyLoops WRITE setApplyLoops NOTIFY applyLoopsChanged)
  Q_PROPERTY (
    double hoveredX READ hoveredX WRITE setHoveredX NOTIFY hoveredXChanged)
  Q_PROPERTY (double contentWidth READ contentWidth NOTIFY contentWidthChanged)
  Q_PROPERTY (
    double contentLeftMargin READ contentLeftMargin NOTIFY
      contentLeftMarginChanged)
public:
  explicit AutomationClipCanvasItem (QQuickItem * parent = nullptr);

  QCanvasPainterItemRenderer * createItemRenderer () const override;

  structure::arrangement::AutomationClip * automationClip () const
  {
    return automation_clip_;
  }
  void setAutomationClip (structure::arrangement::AutomationClip * clip);

  QColor curveColor () const { return curve_color_; }
  void   setCurveColor (const QColor &color);

  QItemSelectionModel * selectionModel () const { return selection_model_; }
  void                  setSelectionModel (QItemSelectionModel * model);

  bool   dragActive () const { return drag_active_; }
  void   setDragActive (bool active);
  double dragDeltaPx () const { return drag_delta_px_; }
  void   setDragDeltaPx (double px);
  double dragDeltaY () const { return drag_delta_y_; }
  void   setDragDeltaY (double dy);

  /// When true, small circles are drawn at each automation point. Default
  /// false — the curve alone is usually what's wanted.
  bool drawPoints () const { return draw_points_; }
  void setDrawPoints (bool draw);

  /// When true (default), the renderer applies the clip's loop unwrapping
  /// (timeline clip preview). When false, it renders only the original source
  /// sequence — points at their authored positions with no loop repetition and
  /// no clip-start offset — which is what the automation editor wants so the
  /// curve aligns with the editable point delegates.
  bool applyLoops () const { return apply_loops_; }
  void setApplyLoops (bool apply);

  /// Canvas-local X of the cursor while it is over a draggable curve segment,
  /// or -1 when it is not. The renderer highlights the single emitted segment
  /// (the one actually under the cursor, so looped copies — which are not
  /// draggable — are never highlighted) containing this X.
  double hoveredX () const { return hovered_x_; }
  void   setHoveredX (double x);

  /// The total width needed for the curve canvas to show every point on both
  /// sides of the clip: spans from the leftmost (live, drag-included) point
  /// (or the clip start, whichever is earlier) to the rightmost point (or the
  /// clip end, whichever is later). QML binds the canvas item's width to this.
  double contentWidth () const;

  /// How far left of the clip start (source 0) the canvas must extend, in
  /// pixels, so that points before the clip start remain visible. QML shifts
  /// the canvas x left by this and sets referenceX to its negation.
  double contentLeftMargin () const;

  /// Returns the set of UUIDs of currently selected (dragged) automation
  /// points. Shared between the item's bounds computation and the renderer's
  /// source-only mode to avoid duplicating the selection-model traversal.
  std::unordered_set<structure::arrangement::ArrangerObject::Uuid>
  draggedUuids () const;

  /**
   * @brief Hit-tests the curve for a curviness drag.
   *
   * Maps the pixel position to a content-tick, finds the curve segment under
   * it, samples the curve, and returns the AutomationPoint whose curve options
   * govern that segment when the point is within @p tolerance pixels of the
   * curve line (vertically), otherwise nullptr.
   */
  Q_INVOKABLE structure::arrangement::AutomationPoint *
              segmentHitTest (double x, double y, double tolerance) const;

Q_SIGNALS:
  void automationClipChanged ();
  void curveColorChanged ();
  void selectionModelChanged ();
  void dragActiveChanged ();
  void dragDeltaPxChanged ();
  void dragDeltaYChanged ();
  void drawPointsChanged ();
  void applyLoopsChanged ();
  void hoveredXChanged ();
  void contentWidthChanged ();
  void contentLeftMarginChanged ();

private:
  /// Returns {effective_start_tick, effective_end_tick} covering the clip
  /// range and every live (drag-included) point. effective_start <= 0;
  /// effective_end >= clip_ticks. Result is cached until invalidated by
  /// @ref invalidateBounds.
  std::pair<double, double> computeEffectiveBounds () const;

  /// Clears the cached bounds and emits contentWidthChanged (which cascades
  /// to contentLeftMarginChanged). Call this whenever any input to
  /// computeEffectiveBounds changes.
  void invalidateBounds ();

private:
  QPointer<structure::arrangement::AutomationClip> automation_clip_;
  QColor                                           curve_color_;
  QPointer<QItemSelectionModel>                    selection_model_;
  bool                                             drag_active_ = false;
  double                                           drag_delta_px_ = 0.0;
  double                                           drag_delta_y_ = 0.0;
  bool                                             draw_points_ = false;
  bool                                             apply_loops_ = true;
  /// -1 = not hovering a draggable segment.
  double                                           hovered_x_ = -1.0;
  std::vector<QMetaObject::Connection>             clip_connections_;
  mutable std::optional<std::pair<double, double>> cached_bounds_;
};

} // namespace zrythm::gui::qquick
