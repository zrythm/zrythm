// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>

#include "dsp/tick_types.h"
#include "gui/qquick/midi_clip_canvas_item.h"
#include "gui/qquick/midi_clip_canvas_renderer.h"
#include "structure/arrangement/arranger_object_all.h"

namespace zrythm::gui::qquick
{

void
MidiClipCanvasRenderer::synchronize (QCanvasPainterItem * item)
{
  auto * canvas_item = static_cast<MidiClipCanvasItem *> (item);

  note_color_ = canvas_item->noteColor ();
  canvas_width_ = static_cast<float> (canvas_item->width ());
  canvas_height_ = static_cast<float> (canvas_item->height ());

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
  if (clip_ticks.asDouble () <= 0.0)
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
  const auto loop_length_ticks = max (
    dsp::ContentTick{ units::ticks (0.0) }, loop_end_ticks - loop_start_ticks);

  auto loop_seg_virt_start = clip_start_ticks;
  auto loop_seg_virt_end = loop_end_ticks;
  auto loop_seg_abs_start = dsp::ContentTick{ units::ticks (0.0) };
  auto loop_seg_abs_end = loop_end_ticks - clip_start_ticks;
  if (loop_seg_abs_end > clip_ticks)
    {
      const auto diff = loop_seg_abs_end - clip_ticks;
      loop_seg_virt_end = loop_seg_virt_end - diff;
      loop_seg_abs_end = loop_seg_abs_end - diff;
    }

  note_rects_.reserve (children.size () * 4);

  while (loop_seg_abs_start < clip_ticks)
    {
      for (const auto * note : children)
        {
          const auto note_virt_start = note->position ()->asTick ();
          const auto note_virt_end =
            note_virt_start + note->length ()->asTick ();

          if (
            note_virt_start >= loop_seg_virt_end
            || note_virt_end <= loop_seg_virt_start)
            continue;

          const auto note_abs_start = max (
            loop_seg_abs_start,
            loop_seg_abs_start + (note_virt_start - loop_seg_virt_start));
          const auto note_abs_end = min (
            loop_seg_abs_end,
            loop_seg_abs_start + (note_virt_end - loop_seg_virt_start));

          const double relative_start =
            note_abs_start.asDouble () / clip_ticks.asDouble ();
          const double relative_end =
            note_abs_end.asDouble () / clip_ticks.asDouble ();
          const auto x = static_cast<float> (relative_start * canvas_width_);
          const auto w = static_cast<float> (
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

      const auto current_len = loop_seg_abs_end - loop_seg_abs_start;
      if (current_len.asDouble () <= 0.0)
        break;

      loop_seg_virt_start = loop_start_ticks;
      loop_seg_virt_end = loop_end_ticks;
      loop_seg_abs_start = loop_seg_abs_start + current_len;
      loop_seg_abs_end = loop_seg_abs_end + loop_length_ticks;

      if (loop_seg_abs_end > clip_ticks)
        {
          const auto diff = loop_seg_abs_end - clip_ticks;
          loop_seg_virt_end = loop_seg_virt_end - diff;
          loop_seg_abs_end = loop_seg_abs_end - diff;
        }
    }
}

void
MidiClipCanvasRenderer::paint (QCanvasPainter * painter)
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
