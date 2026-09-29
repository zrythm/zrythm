// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <limits>
#include <unordered_set>

#include "dsp/position.h"
#include "dsp/tick_types.h"
#include "gui/qquick/automation_clip_canvas_item.h"
#include "gui/qquick/automation_clip_canvas_renderer.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_list_model.h"
#include "structure/arrangement/clip.h"

namespace zrythm::gui::qquick
{

AutomationClipCanvasItem::AutomationClipCanvasItem (QQuickItem * parent)
    : ClipCanvasItemBase (parent)
{
  setFillColor (Qt::transparent);
  setAlphaBlending (true);
  // contentWidth depends on the (clip-length-based) reference width.
  connect (this, &ClipCanvasItemBase::referenceWidthChanged, this, [this] () {
    invalidateBounds ();
  });
  // contentWidth and contentLeftMargin derive from the same inputs and change
  // together, so the latter follows the former.
  connect (this, &AutomationClipCanvasItem::contentWidthChanged, this, [this] () {
    Q_EMIT contentLeftMarginChanged ();
  });
}

QCanvasPainterItemRenderer *
AutomationClipCanvasItem::createItemRenderer () const
{
  return new AutomationClipCanvasRenderer ();
}

void
AutomationClipCanvasItem::setAutomationClip (
  structure::arrangement::AutomationClip * clip)
{
  if (automation_clip_ == clip)
    return;
  automation_clip_ = clip;

  for (const auto &connection : clip_connections_)
    QObject::disconnect (connection);
  clip_connections_.clear ();

  if (automation_clip_ != nullptr)
    {
      clip_connections_.push_back (
        QObject::connect (
          automation_clip_, &structure::arrangement::Clip::contentChanged, this,
          [this] () {
            update ();
            invalidateBounds ();
          },
          Qt::ConnectionType::QueuedConnection));

      clip_connections_.push_back (
        QObject::connect (
          automation_clip_,
          &structure::arrangement::Clip::loopablePropertiesChanged, this,
          [this] () { update (); }, Qt::ConnectionType::QueuedConnection));

      if (automation_clip_->length () != nullptr)
        clip_connections_.push_back (
          QObject::connect (
            automation_clip_->length (), &dsp::Position::positionChanged, this,
            [this] () {
              update ();
              invalidateBounds ();
            },
            Qt::ConnectionType::QueuedConnection));
    }

  invalidateBounds ();
  Q_EMIT automationClipChanged ();
  update ();
}

void
AutomationClipCanvasItem::setCurveColor (const QColor &color)
{
  if (curve_color_ == color)
    return;
  curve_color_ = color;
  Q_EMIT curveColorChanged ();
  update ();
}

void
AutomationClipCanvasItem::setSelectionModel (QItemSelectionModel * model)
{
  if (selection_model_ == model)
    return;
  if (selection_model_ != nullptr)
    QObject::disconnect (
      selection_model_, &QItemSelectionModel::selectionChanged, this, nullptr);
  selection_model_ = model;
  if (selection_model_ != nullptr)
    QObject::connect (
      selection_model_, &QItemSelectionModel::selectionChanged, this,
      [this] () {
        update ();
        invalidateBounds ();
      },
      Qt::ConnectionType::QueuedConnection);
  Q_EMIT selectionModelChanged ();
  invalidateBounds ();
  update ();
}

void
AutomationClipCanvasItem::setDragActive (bool active)
{
  if (drag_active_ == active)
    return;
  drag_active_ = active;
  Q_EMIT dragActiveChanged ();
  invalidateBounds ();
  update ();
}

void
AutomationClipCanvasItem::setDragDeltaPx (double px)
{
  if (qFuzzyCompare (drag_delta_px_, px))
    return;
  drag_delta_px_ = px;
  Q_EMIT dragDeltaPxChanged ();
  if (drag_active_)
    {
      invalidateBounds ();
      update ();
    }
}

void
AutomationClipCanvasItem::setDragDeltaY (double dy)
{
  if (qFuzzyCompare (drag_delta_y_, dy))
    return;
  drag_delta_y_ = dy;
  Q_EMIT dragDeltaYChanged ();
  if (drag_active_)
    update ();
}

void
AutomationClipCanvasItem::setDrawPoints (bool draw)
{
  if (draw_points_ == draw)
    return;
  draw_points_ = draw;
  Q_EMIT drawPointsChanged ();
  update ();
}

void
AutomationClipCanvasItem::setApplyLoops (bool apply)
{
  if (apply_loops_ == apply)
    return;
  apply_loops_ = apply;
  Q_EMIT applyLoopsChanged ();
  update ();
}

void
AutomationClipCanvasItem::setHoveredX (double x)
{
  if (qFuzzyCompare (hovered_x_, x))
    return;
  hovered_x_ = x;
  Q_EMIT hoveredXChanged ();
  update ();
}

structure::arrangement::AutomationPoint *
AutomationClipCanvasItem::segmentHitTest (double x, double y, double tolerance)
  const
{
  if (
    automation_clip_ == nullptr || width () <= 0
    || effectiveReferenceWidth () <= 0)
    return nullptr;

  const auto * length = automation_clip_->length ();
  if (length == nullptr)
    return nullptr;
  const auto clip_ticks = length->asTick ();
  if (clip_ticks <= dsp::ContentTick{})
    return nullptr;

  const double px_per_tick = effectiveReferenceWidth () / clip_ticks.asDouble ();
  const double virt_tick_d = (x + referenceX ()) / px_per_tick;

  // Only the region strictly between the first and last real point contains an
  // editable curve segment. The lead-in (before the first point) and tail
  // (after the last) just hold a flat value — not editable.
  const auto sorted_points =
    automation_clip_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::AutomationPoint>::get_sorted_children_view ();
  if (sorted_points.empty ())
    return nullptr;
  const double first_tick =
    sorted_points.front ()->position ()->asTick ().asDouble ();
  const double last_tick =
    sorted_points.back ()->position ()->asTick ().asDouble ();
  if (virt_tick_d <= first_tick || virt_tick_d >= last_tick)
    return nullptr;

  auto [val, ap] = automation_clip_->get_value_at_virt_tick (
    dsp::ContentTick{ units::ticks (virt_tick_d) });
  if (ap == nullptr)
    return nullptr;

  const float curve_y = static_cast<float> (height ()) * (1.0f - val);
  if (
    std::abs (static_cast<float> (y) - curve_y)
    <= static_cast<float> (tolerance))
    return ap;
  return nullptr;
}

std::unordered_set<structure::arrangement::ArrangerObject::Uuid>
AutomationClipCanvasItem::draggedUuids () const
{
  std::unordered_set<structure::arrangement::ArrangerObject::Uuid> dragged;
  if (!drag_active_ || selection_model_ == nullptr)
    return dragged;

  const int ptr_role = static_cast<int> (
    structure::arrangement::ArrangerObjectListModel::ArrangerObjectPtrRole);
  for (const auto &idx : selection_model_->selectedIndexes ())
    {
      auto * obj =
        idx.data (ptr_role).value<structure::arrangement::ArrangerObject *> ();
      if (obj != nullptr)
        dragged.insert (obj->get_uuid ());
    }
  return dragged;
}

void
AutomationClipCanvasItem::invalidateBounds ()
{
  cached_bounds_.reset ();
  Q_EMIT contentWidthChanged ();
}

std::pair<double, double>
AutomationClipCanvasItem::computeEffectiveBounds () const
{
  if (cached_bounds_.has_value ())
    return *cached_bounds_;

  const double clip_ticks_d =
    (automation_clip_ != nullptr && automation_clip_->length () != nullptr)
      ? automation_clip_->length ()->asTick ().asDouble ()
      : 0.0;

  if (
    automation_clip_ == nullptr || effectiveReferenceWidth () <= 0
    || clip_ticks_d <= 0.0)
    {
      cached_bounds_ = { 0.0, clip_ticks_d };
      return *cached_bounds_;
    }

  const double px_per_tick = effectiveReferenceWidth () / clip_ticks_d;

  const auto sorted_points =
    automation_clip_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::AutomationPoint>::get_sorted_children_view ();
  if (sorted_points.empty ())
    {
      cached_bounds_ = { 0.0, clip_ticks_d };
      return *cached_bounds_;
    }

  // Apply the live horizontal drag delta to selected points so the extent
  // follows points dragged past either clip edge.
  const auto   dragged = draggedUuids ();
  const double pos_delta =
    (drag_active_ && selection_model_ != nullptr)
      ? drag_delta_px_ / px_per_tick
      : 0.0;

  double min_pos = std::numeric_limits<double>::max ();
  double max_pos = std::numeric_limits<double>::lowest ();
  for (const auto * ap : sorted_points)
    {
      double pos = ap->position ()->asTick ().asDouble ();
      if (dragged.contains (ap->get_uuid ()))
        pos += pos_delta;
      min_pos = std::min (min_pos, pos);
      max_pos = std::max (max_pos, pos);
    }

  cached_bounds_ = { std::min (0.0, min_pos), std::max (clip_ticks_d, max_pos) };
  return *cached_bounds_;
}

double
AutomationClipCanvasItem::contentWidth () const
{
  const double base = effectiveReferenceWidth ();
  if (automation_clip_ == nullptr || base <= 0)
    return base;
  const auto [eff_start, eff_end] = computeEffectiveBounds ();
  const double clip_ticks_d =
    (automation_clip_->length () != nullptr)
      ? automation_clip_->length ()->asTick ().asDouble ()
      : 0.0;
  if (clip_ticks_d <= 0.0)
    return base;
  const double px_per_tick = base / clip_ticks_d;
  return double ((eff_end - eff_start) * px_per_tick);
}

double
AutomationClipCanvasItem::contentLeftMargin () const
{
  if (
    automation_clip_ == nullptr || effectiveReferenceWidth () <= 0
    || automation_clip_->length () == nullptr)
    return 0.0;
  const auto [eff_start, eff_end] = computeEffectiveBounds ();
  const double clip_ticks_d = automation_clip_->length ()->asTick ().asDouble ();
  if (clip_ticks_d <= 0.0)
    return 0.0;
  const double px_per_tick = effectiveReferenceWidth () / clip_ticks_d;
  // eff_start <= 0, so the left margin is positive.
  return double (-eff_start * px_per_tick);
}

} // namespace zrythm::gui::qquick
