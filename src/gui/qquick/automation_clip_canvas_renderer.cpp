// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cmath>
#include <ranges>
#include <unordered_set>
#include <utility>

#include "dsp/tick_types.h"
#include "gui/qquick/automation_clip_canvas_item.h"
#include "gui/qquick/automation_clip_canvas_renderer.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_list_model.h"
#include "structure/arrangement/clip.h"
#include "structure/arrangement/loop_segment_iterator.h"

namespace zrythm::gui::qquick
{

void
AutomationClipCanvasRenderer::synchronizeData (QCanvasPainterItem * item)
{
  auto * canvas_item = static_cast<AutomationClipCanvasItem *> (item);

  curve_color_ = canvas_item->curveColor ();
  canvas_width_ = static_cast<float> (canvas_item->width ());
  canvas_height_ = static_cast<float> (canvas_item->height ());
  reference_width_ = canvas_item->effectiveReferenceWidth ();
  reference_x_ = canvas_item->referenceX ();
  draw_points_ = canvas_item->drawPoints ();
  apply_loops_ = canvas_item->applyLoops ();
  hovered_x_ = canvas_item->hoveredX ();

  points_.clear ();

  auto * clip = canvas_item->automationClip ();
  if (clip == nullptr || canvas_width_ <= 0.0f || canvas_height_ <= 0.0f)
    return;

  const auto sorted_points = clip->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::AutomationPoint>::get_sorted_children_view ();
  if (sorted_points.empty ())
    return;

  if (clip->length () == nullptr)
    return;
  const auto clip_ticks = clip->length ()->asTick ();
  if (clip_ticks <= dsp::ContentTick{})
    return;

  const auto loop_start_ticks = clip->loopStartPosition ()->asTick ();
  const auto loop_end_ticks = clip->loopEndPosition ()->asTick ();
  const auto clip_start_ticks = clip->clipStartPosition ()->asTick ();

  const double clip_ticks_d = clip_ticks.asDouble ();
  const double px_per_tick =
    static_cast<double> (reference_width_) / clip_ticks_d;
  // Extend beyond clip_ticks for looped clips or during loop-resize of a
  // non-looped clip (loopPreview). Source-only mode (editor) never extends.
  const auto display_end_tick =
    apply_loops_ && (clip->looped () || canvas_item->loopPreview ())
      ? max (
          clip_ticks,
          dsp::ContentTick{ units::ticks (
            (static_cast<double> (canvas_width_) + reference_x_) / px_per_tick) })
      : clip_ticks;
  const double display_end_ticks_d = display_end_tick.asDouble ();

  // Find the source segment governing a content tick. Returns (governing point
  // P0, successor P1). P0 is the last real point at or before `tick` (or the
  // first point if `tick` is before it, in which case the value is held flat
  // and the successor is null).
  const auto governing_at = [&sorted_points] (double tick)
    -> std::pair<
      const structure::arrangement::AutomationPoint *,
      const structure::arrangement::AutomationPoint *> {
    const auto tick_of = [] (const structure::arrangement::AutomationPoint * ap) {
      return ap->position ()->asTick ().asDouble ();
    };
    const auto it = std::ranges::upper_bound (sorted_points, tick, {}, tick_of);
    if (it == sorted_points.begin ())
      return { *it, nullptr };
    return {
      *std::ranges::prev (it), (it != sorted_points.end ()) ? *it : nullptr
    };
  };

  // Emit one control point at source tick `src_tick`, whose rendered segment
  // runs to `next_src_tick`. The segment is a (possibly partial) slice of the
  // source automation segment governing `src_tick`, so a loop boundary that
  // cuts a curve mid-segment renders the true non-linear arc up to the cut
  // (instead of treating the cut as a fresh segment).
  const auto emit_point =
    [&] (
      double abs_tick_d, double src_tick, double next_src_tick, bool is_real,
      const structure::arrangement::AutomationPoint * ap) {
      if (abs_tick_d < 0.0 || abs_tick_d > display_end_ticks_d)
        return;

      const auto [gov, succ] = governing_at (src_tick);
      const auto   opts = gov->curveOpts ();
      const double gov_tick = gov->position ()->asTick ().asDouble ();
      const float  seg_value_a = gov->value ();
      const float  seg_value_b = succ ? succ->value () : seg_value_a;

      double t_start = 0.0;
      double t_max = 1.0;
      if (succ != nullptr)
        {
          const double succ_tick = succ->position ()->asTick ().asDouble ();
          const double span = succ_tick - gov_tick;
          if (span > 0.0)
            {
              t_start = std::clamp ((src_tick - gov_tick) / span, 0.0, 1.0);
              t_max = std::clamp ((next_src_tick - gov_tick) / span, 0.0, 1.0);
            }
        }

      const float own_val = static_cast<float> (dsp::evaluate_curve (
        seg_value_a, seg_value_b, opts->algorithm (),
        static_cast<float> (opts->curviness ()), t_start));

      CachedControlPoint cp;
      cp.px = static_cast<float> (abs_tick_d * px_per_tick - reference_x_);
      cp.val = own_val;
      cp.py = canvas_height_ * (1.0f - own_val);
      cp.curve_opts = dsp::CurveOptions (opts->curviness (), opts->algorithm ());
      cp.seg_value_a = seg_value_a;
      cp.seg_value_b = seg_value_b;
      cp.seg_t_start = static_cast<float> (t_start);
      cp.seg_t_max = static_cast<float> (t_max);
      cp.is_real_point = is_real && (ap == gov);
      points_.push_back (std::move (cp));
    };

  // Emit the control points for one leg: source range [vs, ve] mapped to abs
  // starting at @p as (abs_tick = as + (src_tick - vs)). Collects the real
  // points falling inside the leg plus boundary events at vs/ve, so the region
  // before the first point and after the last is emitted flat (governing_at
  // returns a null successor there).
  const auto emit_leg = [&] (double vs, double ve, double as) {
    struct Event
    {
      double                                          tick;
      bool                                            is_real;
      const structure::arrangement::AutomationPoint * ap;
    };
    std::vector<Event> events;
    events.push_back ({ vs, false, nullptr });
    for (const auto * ap : sorted_points)
      {
        const double t = ap->position ()->asTick ().asDouble ();
        if (t >= vs && t <= ve)
          events.push_back ({ t, true, ap });
      }
    events.push_back ({ ve, false, nullptr });

    for (size_t i = 0; i + 1 < events.size (); ++i)
      {
        const double abs_tick = as + (events[i].tick - vs);
        emit_point (
          abs_tick, events[i].tick, events[i + 1].tick, events[i].is_real,
          events[i].ap);
      }
    // Final boundary at ve: provides the px endpoint for the previous segment.
    // Its own segment (the wrap) is zero-width, so paint() skips it.
    const double abs_tick_end = as + (events.back ().tick - vs);
    emit_point (
      abs_tick_end, events.back ().tick, events.back ().tick, false, nullptr);
  };

  if (apply_loops_)
    {
      structure::arrangement::for_each_loop_segment (
        clip_start_ticks, loop_start_ticks, loop_end_ticks, display_end_tick,
        [&] (const structure::arrangement::LoopSegment &seg) {
          emit_leg (
            seg.virt_start.asDouble (), seg.virt_end.asDouble (),
            seg.abs_start.asDouble ());
        });
    }
  else
    {
      // Source-only mode (automation editor): emit the original sequence at
      // the points' authored positions — no loop unwrapping, no clip-start
      // shift — so the curve lines up exactly with the point delegates. The
      // live drag delta is substituted into a re-sorted render-point list
      // *before* segments are derived, so the curve tracks the dragged point
      // in real time (and reorders correctly when it crosses a neighbour);
      // patching px afterwards could never fix the stale neighbour
      // connectivity.

      const bool   drag_active = canvas_item->dragActive ();
      const auto   dragged = canvas_item->draggedUuids ();
      const double pos_delta =
        drag_active ? canvas_item->dragDeltaPx () / px_per_tick : 0.0;
      const float value_delta =
        drag_active
          ? -static_cast<float> (canvas_item->dragDeltaY ()) / canvas_height_
          : 0.0f;

      struct RenderPt
      {
        double            pos;
        float             value;
        dsp::CurveOptions opts;
      };
      std::vector<RenderPt> rpts;
      rpts.reserve (sorted_points.size ());
      for (const auto * ap : sorted_points)
        {
          double pos = ap->position ()->asTick ().asDouble ();
          float  val = ap->value ();
          if (drag_active && dragged.contains (ap->get_uuid ()))
            {
              pos += pos_delta;
              val = std::clamp (val + value_delta, 0.0f, 1.0f);
            }
          const auto opts = ap->curveOpts ();
          rpts.push_back (
            { pos, val,
              dsp::CurveOptions (opts->curviness (), opts->algorithm ()) });
        }
      std::ranges::sort (rpts, {}, &RenderPt::pos);

      // The canvas spans from the leftmost (live) point (or the clip start,
      // whichever is earlier) to the rightmost (live) point (or the clip end,
      // whichever is later), so points dragged past either clip edge are still
      // followed by the curve instead of being truncated.
      const double effective_start = std::min (0.0, rpts.front ().pos);
      const double effective_end = std::max (clip_ticks_d, rpts.back ().pos);

      const auto px_of = [&] (double abs_tick) {
        return static_cast<float> (abs_tick * px_per_tick - reference_x_);
      };
      const auto push =
        [&] (
          double abs_tick, float value, const dsp::CurveOptions &opts,
          float seg_value_b, bool is_real) {
          CachedControlPoint cp;
          cp.px = px_of (abs_tick);
          cp.val = value;
          cp.py = canvas_height_ * (1.0f - value);
          cp.curve_opts = opts;
          cp.seg_value_a = value;
          cp.seg_value_b = seg_value_b;
          cp.seg_t_start = 0.0f;
          cp.seg_t_max = 1.0f;
          cp.is_real_point = is_real;
          points_.push_back (std::move (cp));
        };

      // Flat lead-in from the effective start to the first point (only when
      // the first point is past the effective start).
      if (rpts.front ().pos > effective_start)
        push (
          effective_start, rpts.front ().value, rpts.front ().opts,
          rpts.front ().value, false);

      for (size_t i = 0; i < rpts.size (); ++i)
        {
          const bool  has_next = i + 1 < rpts.size ();
          const float next_value = has_next ? rpts[i + 1].value : rpts[i].value;
          push (rpts[i].pos, rpts[i].value, rpts[i].opts, next_value, true);
        }

      // Flat tail from the last point to the effective end (only when the last
      // point is short of it).
      if (rpts.back ().pos < effective_end)
        push (
          effective_end, rpts.back ().value, rpts.back ().opts,
          rpts.back ().value, false);
    }

  // Note: no post-emit px patching or global sort. Both emission paths produce
  // points_ already in ascending pixel order (the timeline via sequential loop
  // legs; the editor via the explicit sort of its render-point list). A global
  // sort here would be redundant and — being unstable — used to swap same-X leg
  // boundaries, making degenerate wrap points draw flat (loop-cut bug).
}

void
AutomationClipCanvasRenderer::paint (QCanvasPainter * painter)
{
  if (points_.empty () || canvas_width_ <= 0.0f || canvas_height_ <= 0.0f)
    return;

  // ---- Build adaptive sample points from cached control points ----
  std::vector<float> sample_xs;
  std::vector<float> sample_ys;
  sample_xs.reserve (static_cast<size_t> (canvas_width_) + 16);
  sample_ys.reserve (static_cast<size_t> (canvas_width_) + 16);

  sample_xs.push_back (points_.front ().px);
  sample_ys.push_back (points_.front ().py);

  // Sample-index range [hover_lo, hover_hi] of the hovered source segment, if
  // any (identity-matched against CachedControlPoint::source_ap).
  bool   has_hover = false;
  size_t hover_lo = 0;
  size_t hover_hi = 0;

  for (size_t i = 0; i + 1 < points_.size (); ++i)
    {
      const auto &a = points_[i];
      const auto &b = points_[i + 1];

      const float seg_px = b.px - a.px;
      if (seg_px <= 0.0f)
        continue;

      // Index of this segment's start point (already pushed as the last
      // sample of the previous segment, or the initial point for i == 0).
      const size_t seg_start_idx = sample_xs.size () - 1;

      if (std::abs (a.seg_value_b - a.seg_value_a) < 1e-5f)
        {
          // Flat source segment — single lineTo to b.
          sample_xs.push_back (b.px);
          sample_ys.push_back (b.py);
        }
      else if (a.curve_opts.algo_ == dsp::CurveOptions::Algorithm::Pulse)
        {
          // Step function: hold value then jump at the end.
          sample_xs.push_back (b.px);
          sample_ys.push_back (a.py);
          sample_xs.push_back (b.px);
          sample_ys.push_back (b.py);
        }
      else if (std::abs (a.curve_opts.curviness_) < 1e-4)
        {
          // Linear segment — single lineTo (value at t_max == b.val by
          // construction).
          sample_xs.push_back (b.px);
          sample_ys.push_back (b.py);
        }
      else
        {
          // Curved segment — adaptive sampling of the source segment over its
          // t-subrange [seg_t_start, seg_t_max].
          const int steps = std::clamp (static_cast<int> (seg_px * 0.5f), 4, 64);
          for (int s = 1; s <= steps; ++s)
            {
              const float f = static_cast<float> (s) / steps;
              const float t = a.seg_t_start + f * (a.seg_t_max - a.seg_t_start);
              const float val = dsp::evaluate_curve (
                a.seg_value_a, a.seg_value_b, a.curve_opts.algo_,
                static_cast<float> (a.curve_opts.curviness_), t);
              sample_xs.push_back (a.px + f * seg_px);
              sample_ys.push_back (canvas_height_ * (1.0f - val));
            }
        }

      // Highlight the single emitted segment the cursor is actually over. The
      // hit-test (which gates hovered_x_ in QML) only succeeds on the real,
      // draggable segment, so looped copies never match here.
      if (hovered_x_ >= 0.0f && hovered_x_ >= a.px && hovered_x_ <= b.px)
        {
          hover_lo = seg_start_idx;
          hover_hi = sample_xs.size () - 1;
          has_hover = true;
        }
    }

  painter->setAntialias (1.0);

  // ---- Fill below the curve with a vertical gradient ----
  painter->beginPath ();
  painter->moveTo (sample_xs.front (), sample_ys.front ());
  for (size_t i = 1; i < sample_xs.size (); ++i)
    painter->lineTo (sample_xs[i], sample_ys[i]);
  painter->lineTo (sample_xs.back (), canvas_height_);
  painter->lineTo (sample_xs.front (), canvas_height_);
  painter->closePath ();

  QCanvasLinearGradient gradient (0.0f, 0.0f, 0.0f, canvas_height_);
  QColor                top_fill = curve_color_;
  top_fill.setAlphaF (0.30f);
  QColor bottom_fill = curve_color_;
  bottom_fill.setAlphaF (0.05f);
  gradient.setStartColor (top_fill);
  gradient.setEndColor (bottom_fill);
  painter->setFillStyle (gradient);
  painter->fill ();

  // ---- Stroke the open curve ----
  painter->beginPath ();
  painter->moveTo (sample_xs.front (), sample_ys.front ());
  for (size_t i = 1; i < sample_xs.size (); ++i)
    painter->lineTo (sample_xs[i], sample_ys[i]);
  painter->setLineWidth (2.0f);
  painter->setStrokeStyle (curve_color_);
  painter->stroke ();

  // ---- Hovered segment highlight ----
  if (has_hover && hover_hi > hover_lo)
    {
      painter->beginPath ();
      painter->moveTo (sample_xs[hover_lo], sample_ys[hover_lo]);
      for (size_t i = hover_lo + 1; i <= hover_hi; ++i)
        painter->lineTo (sample_xs[i], sample_ys[i]);
      painter->setLineWidth (4.0f);
      painter->setStrokeStyle (curve_color_.lighter (150));
      painter->stroke ();
    }

  // ---- Automation point circles ----
  if (!draw_points_)
    return;

  painter->setFillStyle (curve_color_);
  for (const auto &p : points_)
    {
      if (!p.is_real_point)
        continue;
      painter->beginPath ();
      painter->circle (p.px, p.py, 3.0f);
      painter->fill ();
    }
}

} // namespace zrythm::gui::qquick
