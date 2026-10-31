// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <limits>
#include <unordered_set>

#include "gui/qquick/tempo_curve_canvas_item.h"
#include "gui/qquick/tempo_curve_canvas_renderer.h"
#include "structure/arrangement/arranger_object_list_model.h"
#include "structure/arrangement/tempo_object.h"
#include "structure/arrangement/tempo_object_manager.h"

namespace zrythm::gui::qquick
{

void
TempoCurveCanvasRenderer::synchronizeData (QCanvasPainterItem * item)
{
  auto * tempo_item = static_cast<TempoCurveCanvasItem *> (item);

  curve_color_ = tempo_item->curveColor ();
  scroll_x_ = static_cast<float> (tempo_item->scrollX ());
  scroll_x_plus_width_ = static_cast<float> (tempo_item->scrollXPlusWidth ());
  px_per_tick_ = static_cast<float> (tempo_item->pxPerTick ());
  lane_height_ = static_cast<float> (tempo_item->height ());
  base_bpm_ = tempo_item->baseBpm ();

  cached_events_.clear ();

  auto * manager = tempo_item->tempoObjectManager ();
  if (manager == nullptr || px_per_tick_ <= 0.0f || lane_height_ <= 0.0f)
    return;

  // During a move drag, the dragged objects' positions are not committed until
  // release (the loader shows them via a GPU transform). Apply the live delta
  // to the selected objects so the curve follows the drag in real time.
  std::unordered_set<structure::arrangement::ArrangerObjectUuid> dragged;
  double drag_delta_ticks = 0.0;
  if (tempo_item->dragActive () && tempo_item->selectionModel () != nullptr)
    {
      drag_delta_ticks = tempo_item->dragDeltaPx () / px_per_tick_;
      const int ptr_role = static_cast<int> (
        structure::arrangement::ArrangerObjectListModel::ArrangerObjectPtrRole);
      for (const auto &idx : tempo_item->selectionModel ()->selectedIndexes ())
        dragged.insert (
          idx.data (ptr_role)
            .value<structure::arrangement::ArrangerObject *> ()
            ->get_uuid ());
    }

  for (
    const auto * obj :
    manager->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::TempoObject>::get_sorted_children_view ())
    {
      double tick = obj->position ()->ticks ();
      if (!dragged.empty () && dragged.contains (obj->get_uuid ()))
        tick += drag_delta_ticks;
      cached_events_.push_back (
        {
          tick,
          obj->tempo (),
          obj->curve () == structure::arrangement::TempoObject::CurveType::Linear,
        });
    }

  // Offsetting selected events may break tick ordering; keep the array sorted
  // so the per-pixel interpolation (binary search) stays valid.
  std::ranges::sort (cached_events_, {}, &CachedTempoEvent::tick);

  // Prepend the base tempo at tick 0 unless an inserted object already sits
  // there (in which case the object governs and shadows the base).
  if (cached_events_.empty () || cached_events_.front ().tick > 0.0)
    cached_events_.insert (
      cached_events_.begin (), CachedTempoEvent{ 0.0, base_bpm_, false });

  // Project-wide min/max bpm with padding and a minimum spread so a single
  // tempo object (or several identical tempos) do not collapse to a flat line.
  if (cached_events_.empty ())
    return;

  min_bpm_ = std::numeric_limits<float>::max ();
  max_bpm_ = std::numeric_limits<float>::lowest ();
  for (const auto &e : cached_events_)
    {
      min_bpm_ = std::min (min_bpm_, static_cast<float> (e.bpm));
      max_bpm_ = std::max (max_bpm_, static_cast<float> (e.bpm));
    }

  constexpr float min_spread = 20.0f;
  constexpr float padding_ratio = 0.1f;

  const float spread = max_bpm_ - min_bpm_;
  if (spread < min_spread)
    {
      const float mid = (max_bpm_ + min_bpm_) * 0.5f;
      min_bpm_ = mid - min_spread * 0.5f;
      max_bpm_ = mid + min_spread * 0.5f;
    }
  else
    {
      const float pad = spread * padding_ratio;
      min_bpm_ -= pad;
      max_bpm_ += pad;
    }
}

void
TempoCurveCanvasRenderer::paint (QCanvasPainter * painter)
{
  if (
    lane_height_ <= 0.0f || px_per_tick_ <= 0.0f || cached_events_.empty ()
    || scroll_x_plus_width_ <= scroll_x_)
    return;

  const float bpm_range = max_bpm_ - min_bpm_;
  if (bpm_range <= 0.0f)
    return;

  // Tempo (bpm) at the given tick, interpolated across cached events: constant
  // segments hold the previous event's bpm, linear segments ramp to the next.
  const auto bpm_at_tick = [this] (double tick) -> double {
    if (tick <= cached_events_.front ().tick)
      return cached_events_.front ().bpm;
    if (tick >= cached_events_.back ().tick)
      return cached_events_.back ().bpm;
    const auto it = std::lower_bound (
      cached_events_.begin (), cached_events_.end (), tick,
      [] (const CachedTempoEvent &e, double t) { return e.tick < t; });
    const auto  idx = static_cast<size_t> (it - cached_events_.begin ());
    const auto &prev = cached_events_[idx - 1];
    const auto &next = cached_events_[idx];
    if (prev.linear)
      {
        const double frac = (tick - prev.tick) / (next.tick - prev.tick);
        return prev.bpm + (next.bpm - prev.bpm) * frac;
      }
    return prev.bpm;
  };

  // Map a bpm value to a y coordinate within the lane (higher bpm -> smaller y).
  const auto y_for_bpm = [this, bpm_range] (double bpm) {
    const float normalized = static_cast<float> ((bpm - min_bpm_) / bpm_range);
    return (1.0f - normalized) * lane_height_;
  };

  painter->setAntialias (1.0);
  painter->setLineWidth (2.0f);

  // The canvas is viewport-sized; translate so content-space x maps to the
  // visible portion (same convention as ArrangerGridCanvasRenderer).
  painter->save ();
  painter->translate (-scroll_x_, 0.0f);

  const double visible_start_tick = scroll_x_ / px_per_tick_;
  const double visible_end_tick = scroll_x_plus_width_ / px_per_tick_;
  const int viewport_px = static_cast<int> (scroll_x_plus_width_ - scroll_x_);

  // Sample the tempo once per pixel column across the visible range.
  std::vector<float> ys (static_cast<size_t> (std::max (0, viewport_px)) + 1);
  for (int p = 0; p <= viewport_px; ++p)
    {
      const double tick =
        visible_start_tick
        + (visible_end_tick - visible_start_tick) * p / viewport_px;
      ys[p] = y_for_bpm (bpm_at_tick (tick));
    }

  // The per-pixel samples are computed once (above); the stroke and fill use
  // separate paths because they need different geometry (open top curve vs.
  // closed polygon) and continuing a path across stroke() is unreliable.
  painter->beginPath ();
  painter->moveTo (scroll_x_, ys[0]);
  for (size_t p = 1; p < ys.size (); ++p)
    painter->lineTo (scroll_x_ + static_cast<float> (p), ys[p]);
  painter->setStrokeStyle (curve_color_);
  painter->stroke ();

  painter->beginPath ();
  painter->moveTo (scroll_x_, ys[0]);
  for (size_t p = 1; p < ys.size (); ++p)
    painter->lineTo (scroll_x_ + static_cast<float> (p), ys[p]);
  painter->lineTo (scroll_x_plus_width_, lane_height_);
  painter->lineTo (scroll_x_, lane_height_);
  painter->closePath ();

  QCanvasLinearGradient gradient (0.0f, 0.0f, 0.0f, lane_height_);
  QColor                top_fill = curve_color_;
  top_fill.setAlphaF (0.30f);
  QColor bottom_fill = curve_color_;
  bottom_fill.setAlphaF (0.05f);
  gradient.setStartColor (top_fill);
  gradient.setEndColor (bottom_fill);
  painter->setFillStyle (gradient);
  painter->fill ();

  painter->restore ();
}

} // namespace zrythm::gui::qquick
