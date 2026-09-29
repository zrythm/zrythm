// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <unordered_map>

#include "dsp/musical_scale.h"
#include "dsp/tick_types.h"
#include "gui/qquick/chord_clip_canvas_item.h"
#include "gui/qquick/chord_clip_canvas_renderer.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/chord_clip.h"
#include "structure/arrangement/chord_object.h"
#include "structure/arrangement/loop_segment_iterator.h"

#include <QFontMetrics>

namespace zrythm::gui::qquick
{

std::vector<ComputedChordCell>
compute_chord_cells (
  const structure::arrangement::ChordClip &clip,
  dsp::ContentTick                         display_end_tick)
{
  std::vector<ComputedChordCell> cells;

  if (clip.length () == nullptr)
    return cells;

  const auto clip_ticks = clip.length ()->asTick ();
  if (clip_ticks <= dsp::ContentTick{})
    return cells;

  const auto loop_start_ticks = clip.loopStartPosition ()->asTick ();
  const auto loop_end_ticks = clip.loopEndPosition ()->asTick ();
  const auto clip_start_ticks = clip.clipStartPosition ()->asTick ();

  // Gather chord objects in sorted-by-position order.
  const auto sorted_chords_view = clip.get_sorted_children_view ();
  const std::vector<structure::arrangement::ChordObject *> sorted_chords (
    sorted_chords_view.begin (), sorted_chords_view.end ());
  if (sorted_chords.empty ())
    return cells;

  // Build insertion-order index lookup.
  std::unordered_map<structure::arrangement::ChordObject *, int> insertion_index;
  const auto &insertion_order = clip.get_children_vector ();
  for (size_t i = 0; i < insertion_order.size (); ++i)
    {
      insertion_index.emplace (
        insertion_order[i].get_object_as<structure::arrangement::ChordObject> (),
        static_cast<int> (i));
    }

  cells.reserve (sorted_chords.size () * 4);

  structure::arrangement::for_each_loop_segment (
    clip_start_ticks, loop_start_ticks, loop_end_ticks, display_end_tick,
    [&] (const structure::arrangement::LoopSegment &seg) {
      for (size_t i = 0; i < sorted_chords.size (); ++i)
        {
          auto *     chord = sorted_chords[i];
          const auto chord_pos = chord->position ()->asTick ();

          if (chord_pos < seg.virt_start || chord_pos >= seg.virt_end)
            continue;

          // Find end: next chord in the same iteration, or iteration end.
          auto chord_end = seg.virt_end;
          for (size_t j = i + 1; j < sorted_chords.size (); ++j)
            {
              const auto other_pos = sorted_chords[j]->position ()->asTick ();
              if (other_pos >= seg.virt_end)
                break; // sorted: no subsequent chord is in range
              if (other_pos > chord_pos)
                {
                  chord_end = other_pos;
                  break;
                }
            }

          cells.push_back (
            {
              .abs_start = seg.abs_start + (chord_pos - seg.virt_start),
              .abs_end = seg.abs_start + (chord_end - seg.virt_start),
              .chord_object = chord,
              .chord_index =
                [&] {
                  const auto it = insertion_index.find (chord);
                  return it != insertion_index.end () ? it->second : -1;
                }(),
            });
        }
    });

  return cells;
}

namespace
{

QColor
rootNoteToColor (int rootNote)
{
  QColor c = QColor::fromHslF (
    static_cast<float> (std::clamp (rootNote, 0, 11) / 12.0), 0.7f, 0.5f);
  c.setAlphaF (0.6f);
  return c;
}

} // anonymous namespace

void
ChordClipCanvasRenderer::synchronize (QCanvasPainterItem * item)
{
  auto * canvas_item = static_cast<ChordClipCanvasItem *> (item);

  text_color_ = canvas_item->textColor ();
  font_ = canvas_item->font ();
  bold_font_ = font_;
  bold_font_.setBold (true);
  const QFontMetrics name_metrics (font_);
  canvas_width_ = static_cast<float> (canvas_item->width ());
  canvas_height_ = static_cast<float> (canvas_item->height ());

  cells_.clear ();

  auto * clip = canvas_item->chordClip ();
  if (clip == nullptr || canvas_width_ <= 0.0f || canvas_height_ <= 0.0f)
    return;

  if (clip->length () == nullptr)
    return;
  const auto clip_ticks = clip->length ()->asTick ();
  if (clip_ticks <= dsp::ContentTick{})
    return;

  const double ref_width = canvas_item->effectiveReferenceWidth ();
  const double ref_x = canvas_item->referenceX ();
  const double px_per_tick =
    static_cast<double> (ref_width) / clip_ticks.asDouble ();

  // Extend display for looped clips or during loop-resize preview.
  const auto display_end_tick =
    (clip->looped () || canvas_item->loopPreview ())
      ? max (
          clip_ticks,
          dsp::ContentTick{ units::ticks (
            (static_cast<double> (canvas_width_) + ref_x) / px_per_tick) })
      : clip_ticks;

  const auto computed = compute_chord_cells (*clip, display_end_tick);

  cells_.reserve (computed.size ());
  for (const auto &cell : computed)
    {
      CachedCell c;
      c.x =
        static_cast<float> (cell.abs_start.asDouble () * px_per_tick - ref_x);
      c.width = static_cast<float> (
        (cell.abs_end - cell.abs_start).asDouble () * px_per_tick);

      // Chord name and root note
      if (cell.chord_object && cell.chord_object->chordDescriptor ())
        {
          auto * desc = cell.chord_object->chordDescriptor ();
          c.full_name = desc->displayName ();
          const int root = static_cast<int> (desc->rootNote ());
          c.root_note = dsp::MusicalScale::noteToString (desc->rootNote ());
          c.bg_color = rootNoteToColor (root);
        }

      c.muted =
        cell.chord_object && cell.chord_object->mute ()
          ? cell.chord_object->mute ()->muted ()
          : false;

      // Measure full name width for LOD (using QCanvasPainter's text bounding)
      // We can't call painter->textBoundingBox here (no painter in
      // synchronize), so estimate with QFontMetrics.
      if (!c.full_name.isEmpty ())
        {
          c.full_name_width = name_metrics.horizontalAdvance (c.full_name) + 8;
        }
      else
        {
          c.full_name_width = 0;
        }

      cells_.push_back (std::move (c));
    }
}

void
ChordClipCanvasRenderer::paint (QCanvasPainter * painter)
{
  if (cells_.empty () || canvas_width_ <= 0.0f || canvas_height_ <= 0.0f)
    return;

  painter->setRenderHint (QCanvasPainter::RenderHint::Antialiasing, false);

  QColor separator_color = text_color_;
  separator_color.setAlphaF (separator_color.alphaF () * 0.4f);

  for (const auto &cell : cells_)
    {
      // Skip fully off-screen cells
      if (cell.x + cell.width < 0.0f || cell.x > canvas_width_)
        continue;

      // LOD: full name → root note → colored block (same as old QML)
      const bool shows_full_name =
        cell.width >= cell.full_name_width && !cell.full_name.isEmpty ();
      const bool shows_root_note =
        !shows_full_name && cell.width >= 15.0f && !cell.root_note.isEmpty ();

      // Colored block — only when cell is too small for any text
      if (!shows_full_name && !shows_root_note)
        {
          QColor bg = cell.bg_color;
          if (cell.muted)
            bg.setAlphaF (bg.alphaF () * 0.3f);
          painter->setFillStyle (bg);
          painter->fillRect (cell.x, 0.0f, cell.width, canvas_height_);
        }

      // Right-edge separator (always drawn)
      painter->setStrokeStyle (separator_color);
      painter->setLineWidth (1.0f);
      painter->beginPath ();
      painter->moveTo (cell.x + cell.width, 0.0f);
      painter->lineTo (cell.x + cell.width, canvas_height_);
      painter->stroke ();

      if (shows_full_name)
        {
          painter->setFont (font_);
          painter->setTextAlign (QCanvasPainter::TextAlign::Left);
          painter->setTextBaseline (QCanvasPainter::TextBaseline::Middle);
          painter->setFillStyle (text_color_);
          painter->fillText (
            cell.full_name, cell.x + 4.0f, canvas_height_ * 0.5f);
        }
      else if (shows_root_note)
        {
          painter->setFont (bold_font_);
          painter->setTextAlign (QCanvasPainter::TextAlign::Center);
          painter->setTextBaseline (QCanvasPainter::TextBaseline::Middle);
          painter->setFillStyle (text_color_);
          painter->fillText (
            cell.root_note, cell.x + cell.width * 0.5f, canvas_height_ * 0.5f);
        }
    }
}

} // namespace zrythm::gui::qquick
