// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>

#include "dsp/tick_types.h"
#include "gui/qquick/midi_clip_canvas_item.h"
#include "gui/qquick/midi_clip_canvas_renderer.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/loop_segment_iterator.h"

namespace zrythm::gui::qquick
{

void
MidiClipCanvasRenderer::synchronizeData (QCanvasPainterItem * item)
{
  auto * canvas_item = static_cast<MidiClipCanvasItem *> (item);

  note_color_ = canvas_item->noteColor ();
  canvas_width_ = static_cast<float> (canvas_item->width ());
  canvas_height_ = static_cast<float> (canvas_item->height ());
  reference_width_ = canvas_item->effectiveReferenceWidth ();
  reference_x_ = canvas_item->referenceX ();

  note_rects_.clear ();

  dimmed_color_ = note_color_;
  dimmed_color_.setAlpha (dimmed_color_.alpha () / 3);

  auto * clip = canvas_item->midiClip ();
  if (clip == nullptr || canvas_width_ <= 0 || canvas_height_ <= 0)
    return;

  const auto children = clip->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiNote>::get_children_view ();
  if (children.empty ())
    return;

  if (clip->length () == nullptr)
    return;
  const auto clip_ticks = clip->length ()->asTick ();
  if (clip_ticks <= dsp::ContentTick{})
    return;

  auto pitch_range = get_pitch_range (children);
  if (!pitch_range.has_value ())
    return;

  int min_pitch = static_cast<int> (pitch_range->first);
  int max_pitch = static_cast<int> (pitch_range->second);

  constexpr int min_pitch_count = 5;
  if ((max_pitch - min_pitch) < min_pitch_count)
    {
      min_pitch = std::max (0, min_pitch - 2);
      max_pitch = std::min (127, max_pitch + 2);
    }

  const int num_visible_pitches =
    std::max (min_pitch_count, (max_pitch - min_pitch) + 1);
  const double midi_note_height =
    static_cast<double> (canvas_height_)
    / static_cast<double> (num_visible_pitches);

  const auto loop_start_ticks = clip->loopStartPosition ()->asTick ();
  const auto loop_end_ticks = clip->loopEndPosition ()->asTick ();
  const auto clip_start_ticks = clip->clipStartPosition ()->asTick ();

  // Density: use referenceWidth (constant during drag) so notes don't stretch.
  // display_end_tick extends beyond clip_ticks when the canvas is wider than
  // the reference content (drag preview), for looped clips or during
  // loop-resize of a non-looped clip (loopPreview).
  const double px_per_tick =
    static_cast<double> (reference_width_) / clip_ticks.asDouble ();
  const auto display_end_tick =
    (clip->looped () || canvas_item->loopPreview ())
      ? max (
          clip_ticks,
          dsp::ContentTick{ units::ticks (
            (static_cast<double> (canvas_width_) + reference_x_) / px_per_tick) })
      : clip_ticks;

  note_rects_.reserve (children.size () * 4);

  structure::arrangement::for_each_loop_segment (
    clip_start_ticks, loop_start_ticks, loop_end_ticks, display_end_tick,
    [&] (const structure::arrangement::LoopSegment &seg) {
      for (const auto * note : children)
        {
          const auto note_virt_start = note->position ()->asTick ();
          const auto note_virt_end =
            note_virt_start + note->length ()->asTick ();

          if (note_virt_start >= seg.virt_end || note_virt_end <= seg.virt_start)
            continue;

          const auto note_abs_start = max (
            seg.abs_start, seg.abs_start + (note_virt_start - seg.virt_start));
          const auto note_abs_end =
            min (seg.abs_end, seg.abs_start + (note_virt_end - seg.virt_start));

          const auto x = static_cast<float> (
            note_abs_start.asDouble () * px_per_tick - reference_x_);
          const auto w = static_cast<float> (
            (note_abs_end - note_abs_start).asDouble () * px_per_tick);
          const int  relative_pitch = (note->pitch () - min_pitch) + 1;
          const auto y = static_cast<float> (
            canvas_height_ - (relative_pitch * midi_note_height));

          note_rects_.push_back (
            { .x = x,
              .y = y,
              .width = w,
              .height = static_cast<float> (midi_note_height),
              .muted = note->mute ()->muted () });
        }
    });
}

void
MidiClipCanvasRenderer::paint (QCanvasPainter * painter)
{
  if (note_rects_.empty ())
    return;

  painter->setAntialias (0.0);

  for (const auto &rect : note_rects_)
    {
      painter->setFillStyle (rect.muted ? dimmed_color_ : note_color_);
      painter->fillRect (rect.x, rect.y, rect.width, rect.height);
    }
}

} // namespace zrythm::gui::qquick
