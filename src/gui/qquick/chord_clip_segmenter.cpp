// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <unordered_map>
#include <utility>

#include "gui/qquick/chord_clip_segmenter.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/chord_clip.h"
#include "structure/arrangement/chord_object.h"

namespace zrythm::gui::qquick
{

ChordClipSegmenter::ChordClipSegmenter (QObject * parent) : QObject (parent)
{
  // Initialize with an empty model so segments() is never null.
  segments_model_ =
    std::make_unique<QRangeModel> (std::vector<ChordSegment> (), this);
}

void
ChordClipSegmenter::disconnectAll ()
{
  for (const auto &connection : connections_)
    QObject::disconnect (connection);
  connections_.clear ();
}

void
ChordClipSegmenter::connectToClip ()
{
  if (chord_clip_ == nullptr)
    return;

  // Chord objects list changed (add/remove/move/data).
  if (auto * chord_objects = chord_clip_->chordObjects ())
    {
      connections_.push_back (
        QObject::connect (
          chord_objects,
          &zrythm::structure::arrangement::ArrangerObjectListModel::contentChanged,
          this, [this] () { recalculate (); }));
    }

  // Loop configuration changes.
  connections_.push_back (
    QObject::connect (
      chord_clip_,
      &zrythm::structure::arrangement::Clip::loopablePropertiesChanged, this,
      [this] () { recalculate (); }));

  // Clip resized.
  if (auto * length = chord_clip_->length ())
    {
      connections_.push_back (
        QObject::connect (
          length, &zrythm::dsp::Position::positionChanged, this,
          [this] () { recalculate (); }));
    }
}

void
ChordClipSegmenter::setChordClip (
  zrythm::structure::arrangement::ChordClip * clip)
{
  if (chord_clip_ == clip)
    return;

  disconnectAll ();
  chord_clip_ = clip;
  connectToClip ();
  Q_EMIT chordClipChanged ();
  recalculate ();
}

void
ChordClipSegmenter::recalculate ()
{
  // Always start from an empty list; moved into a fresh QRangeModel below.
  std::vector<ChordSegment> new_segments;

  auto rebuild_model = [this, &new_segments] () {
    segments_data_ = std::move (new_segments);
    segments_model_ =
      std::make_unique<QRangeModel> (std::move (segments_data_), this);
    Q_EMIT segmentsChanged ();
  };

  if (chord_clip_ == nullptr)
    {
      rebuild_model ();
      return;
    }

  if (chord_clip_->length () == nullptr)
    {
      rebuild_model ();
      return;
    }

  const double clip_ticks = chord_clip_->length ()->ticks ();
  if (clip_ticks <= 0.0)
    {
      rebuild_model ();
      return;
    }

  const double loop_start_ticks = chord_clip_->loopStartPosition ()->ticks ();
  const double loop_end_ticks = chord_clip_->loopEndPosition ()->ticks ();
  const double clip_start_ticks = chord_clip_->clipStartPosition ()->ticks ();
  const double loop_length_ticks =
    std::max (0.0, loop_end_ticks - loop_start_ticks);

  // Gather chord objects in sorted-by-position order (stable for equal
  // positions via the multi-index ranked index).
  const auto sorted_chords_view = chord_clip_->get_sorted_children_view ();
  const std::vector<zrythm::structure::arrangement::ChordObject *>
    sorted_chords (sorted_chords_view.begin (), sorted_chords_view.end ());
  if (sorted_chords.empty ())
    {
      rebuild_model ();
      return;
    }

  // Build an O(1) lookup from each chord pointer to its insertion-order
  // index, used to populate chord_index for each segment.
  std::unordered_map<zrythm::structure::arrangement::ChordObject *, int>
              insertion_index;
  const auto &insertion_order = chord_clip_->get_children_vector ();
  for (size_t i = 0; i < insertion_order.size (); ++i)
    {
      insertion_index.emplace (
        insertion_order[i]
          .get_object_as<zrythm::structure::arrangement::ChordObject> (),
        static_cast<int> (i));
    }
  auto get_insertion_index =
    [&insertion_index] (zrythm::structure::arrangement::ChordObject * chord)
    -> int {
    const auto it = insertion_index.find (chord);
    return it != insertion_index.end () ? it->second : -1;
  };

  // ---- Loop-expansion algorithm (ported from
  // midi_clip_canvas_renderer.cpp) ----
  //
  // First iteration uses virtual range [clipStart, loopEnd); subsequent
  // iterations use [loopStart, loopEnd). Each iteration's absolute range is
  // projected to clip-relative ticks, clamped to clip_ticks.

  double virt_start = clip_start_ticks;
  double virt_end = loop_end_ticks;
  double abs_start = 0.0;
  double abs_end = loop_end_ticks - clip_start_ticks;
  if (abs_end > clip_ticks)
    {
      const double overflow = abs_end - clip_ticks;
      virt_end -= overflow;
      abs_end -= overflow;
    }

  new_segments.reserve (sorted_chords.size () * 4);

  while (abs_start < clip_ticks)
    {
      for (size_t i = 0; i < sorted_chords.size (); ++i)
        {
          auto *       chord = sorted_chords[i];
          const double chord_pos = chord->position ()->ticks ();

          // Skip chords outside this iteration's virtual range. Half-open
          // [virt_start, virt_end): a chord exactly at virt_end belongs to
          // the next iteration.
          if (chord_pos < virt_start || chord_pos >= virt_end)
            continue;

          // Find end: next chord in the same iteration, or iteration end.
          double chord_end = virt_end;
          for (size_t j = i + 1; j < sorted_chords.size (); ++j)
            {
              const double other_pos = sorted_chords[j]->position ()->ticks ();
              if (other_pos > chord_pos && other_pos < virt_end)
                {
                  chord_end = other_pos;
                  break;
                }
            }

          const double chord_abs_start = abs_start + (chord_pos - virt_start);
          const double chord_abs_end = abs_start + (chord_end - virt_start);

          ChordSegment segment;
          segment.abs_start_ticks = chord_abs_start;
          segment.abs_end_ticks = chord_abs_end;
          segment.chord_object = chord;
          segment.chord_index = get_insertion_index (chord);
          new_segments.push_back (std::move (segment));
        }

      const double current_len = abs_end - abs_start;
      if (current_len <= 0.0)
        break; // safety against infinite loop

      // Advance to next iteration.
      virt_start = loop_start_ticks;
      virt_end = loop_end_ticks;
      abs_start += current_len;
      abs_end += loop_length_ticks;
      if (abs_end > clip_ticks)
        {
          const double overflow = abs_end - clip_ticks;
          virt_end -= overflow;
          abs_end -= overflow;
        }
    }

  segments_data_ = std::move (new_segments);
  segments_model_ =
    std::make_unique<QRangeModel> (std::move (segments_data_), this);
  Q_EMIT segmentsChanged ();
}

} // namespace zrythm::gui::qquick
