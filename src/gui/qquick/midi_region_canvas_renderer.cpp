// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>

#include "gui/qquick/midi_region_canvas_item.h"
#include "gui/qquick/midi_region_canvas_renderer.h"
#include "structure/arrangement/arranger_object_all.h"

namespace zrythm::gui::qquick
{

void
MidiRegionCanvasRenderer::synchronize (QCanvasPainterItem * item)
{
  auto * canvas_item = static_cast<MidiRegionCanvasItem *> (item);

  note_color_ = canvas_item->noteColor ();
  canvas_width_ = static_cast<float> (canvas_item->width ());
  canvas_height_ = static_cast<float> (canvas_item->height ());

  note_rects_.clear ();

  dimmed_color_ = note_color_;
  dimmed_color_.setAlpha (dimmed_color_.alpha () / 3);

  auto * region = canvas_item->midiRegion ();
  if (region == nullptr || canvas_width_ <= 0 || canvas_height_ <= 0)
    return;

  const auto children = region->get_children_view ();
  if (children.empty ())
    return;

  if (region->bounds () == nullptr || region->bounds ()->length () == nullptr)
    return;
  const double region_ticks = region->bounds ()->length ()->ticks ();
  if (region_ticks <= 0.0)
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

  auto * loop_range = region->loopRange ();
  if (loop_range == nullptr)
    return;
  const double loop_start_ticks = loop_range->loopStartPosition ()->ticks ();
  const double loop_end_ticks = loop_range->loopEndPosition ()->ticks ();
  const double clip_start_ticks = loop_range->clipStartPosition ()->ticks ();
  const double loop_length_ticks =
    std::max (0.0, loop_end_ticks - loop_start_ticks);

  double loop_seg_virt_start = clip_start_ticks;
  double loop_seg_virt_end = loop_end_ticks;
  double loop_seg_abs_start = 0.0;
  double loop_seg_abs_end = loop_end_ticks - clip_start_ticks;
  if (loop_seg_abs_end > region_ticks)
    {
      const double diff = loop_seg_abs_end - region_ticks;
      loop_seg_virt_end -= diff;
      loop_seg_abs_end -= diff;
    }

  note_rects_.reserve (children.size () * 4);

  while (loop_seg_abs_start < region_ticks)
    {
      for (const auto * note : children)
        {
          const double note_virt_start = note->position ()->ticks ();
          const double note_virt_end =
            note_virt_start + note->bounds ()->length ()->ticks ();

          if (
            note_virt_start >= loop_seg_virt_end
            || note_virt_end <= loop_seg_virt_start)
            continue;

          const double note_abs_start = std::max (
            loop_seg_abs_start,
            loop_seg_abs_start + (note_virt_start - loop_seg_virt_start));
          const double note_abs_end = std::min (
            loop_seg_abs_end,
            loop_seg_abs_start + (note_virt_end - loop_seg_virt_start));

          const double relative_start = note_abs_start / region_ticks;
          const double relative_end = note_abs_end / region_ticks;
          const auto   x = static_cast<float> (relative_start * canvas_width_);
          const auto   w = static_cast<float> (
            (relative_end - relative_start) * canvas_width_);
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

      const double current_len = loop_seg_abs_end - loop_seg_abs_start;
      if (current_len <= 0.0)
        break;

      loop_seg_virt_start = loop_start_ticks;
      loop_seg_virt_end = loop_end_ticks;
      loop_seg_abs_start += current_len;
      loop_seg_abs_end += loop_length_ticks;

      if (loop_seg_abs_end > region_ticks)
        {
          const double diff = loop_seg_abs_end - region_ticks;
          loop_seg_virt_end -= diff;
          loop_seg_abs_end -= diff;
        }
    }
}

void
MidiRegionCanvasRenderer::paint (QCanvasPainter * painter)
{
  if (note_rects_.empty ())
    return;

  painter->setRenderHint (QCanvasPainter::RenderHint::Antialiasing, false);

  for (const auto &rect : note_rects_)
    {
      painter->setFillStyle (rect.muted ? dimmed_color_ : note_color_);
      painter->fillRect (rect.x, rect.y, rect.width, rect.height);
    }
}

} // namespace zrythm::gui::qquick
