// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <unordered_map>
#include <utility>

#include "dsp/atomic_position_qml_adapter.h"
#include "gui/qquick/chord_region_segmenter.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/bounded_object.h"
#include "structure/arrangement/chord_object.h"
#include "structure/arrangement/chord_region.h"
#include "structure/arrangement/loopable_object.h"

namespace zrythm::gui::qquick
{

ChordRegionSegmenter::ChordRegionSegmenter (QObject * parent) : QObject (parent)
{
  // Initialize with an empty model so segments() is never null.
  segments_model_ =
    std::make_unique<QRangeModel> (std::vector<ChordSegment> (), this);
}

void
ChordRegionSegmenter::disconnectAll ()
{
  for (const auto &connection : connections_)
    QObject::disconnect (connection);
  connections_.clear ();
}

void
ChordRegionSegmenter::connectToRegion ()
{
  if (region_ == nullptr)
    return;

  // Chord objects list changed (add/remove/move/data).
  if (auto * chord_objects = region_->chordObjects ())
    {
      connections_.push_back (
        QObject::connect (
          chord_objects,
          &zrythm::structure::arrangement::ArrangerObjectListModel::contentChanged,
          this, [this] () { recalculate (); }));
    }

  // Loop configuration changes.
  if (auto * loop_range = region_->loopRange ())
    {
      connections_.push_back (
        QObject::connect (
          loop_range,
          &zrythm::structure::arrangement::ArrangerObjectLoopRange::
            loopableObjectPropertiesChanged,
          this, [this] () { recalculate (); }));
    }

  // Region resized.
  if (auto * bounds = region_->bounds ())
    {
      if (auto * length = bounds->length ())
        {
          connections_.push_back (
            QObject::connect (
              length, &zrythm::dsp::AtomicPositionQmlAdapter::positionChanged,
              this, [this] () { recalculate (); }));
        }
    }
}

void
ChordRegionSegmenter::setRegion (
  zrythm::structure::arrangement::ChordRegion * region)
{
  if (region_ == region)
    return;

  disconnectAll ();
  region_ = region;
  connectToRegion ();
  Q_EMIT regionChanged ();
  recalculate ();
}

void
ChordRegionSegmenter::recalculate ()
{
  // Always start from an empty list; moved into a fresh QRangeModel below.
  std::vector<ChordSegment> new_segments;

  auto rebuild_model = [this, &new_segments] () {
    segments_data_ = std::move (new_segments);
    segments_model_ =
      std::make_unique<QRangeModel> (std::move (segments_data_), this);
    Q_EMIT segmentsChanged ();
  };

  if (region_ == nullptr)
    {
      rebuild_model ();
      return;
    }

  auto * bounds = region_->bounds ();
  auto * loop_range = region_->loopRange ();
  if (bounds == nullptr || bounds->length () == nullptr || loop_range == nullptr)
    {
      rebuild_model ();
      return;
    }

  const double region_ticks = bounds->length ()->ticks ();
  if (region_ticks <= 0.0)
    {
      rebuild_model ();
      return;
    }

  const double loop_start_ticks = loop_range->loopStartPosition ()->ticks ();
  const double loop_end_ticks = loop_range->loopEndPosition ()->ticks ();
  const double clip_start_ticks = loop_range->clipStartPosition ()->ticks ();
  const double loop_length_ticks =
    std::max (0.0, loop_end_ticks - loop_start_ticks);

  // Gather chord objects in sorted-by-position order (stable for equal
  // positions via the multi-index ranked index).
  const auto sorted_chords_view = region_->get_sorted_children_view ();
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
  const auto &insertion_order = region_->get_children_vector ();
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
  // midi_region_canvas_renderer.cpp) ----
  //
  // First iteration uses virtual range [clipStart, loopEnd); subsequent
  // iterations use [loopStart, loopEnd). Each iteration's absolute range is
  // projected to region-relative ticks, clamped to region_ticks.

  double virt_start = clip_start_ticks;
  double virt_end = loop_end_ticks;
  double abs_start = 0.0;
  double abs_end = loop_end_ticks - clip_start_ticks;
  if (abs_end > region_ticks)
    {
      const double overflow = abs_end - region_ticks;
      virt_end -= overflow;
      abs_end -= overflow;
    }

  new_segments.reserve (sorted_chords.size () * 4);

  while (abs_start < region_ticks)
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
      if (abs_end > region_ticks)
        {
          const double overflow = abs_end - region_ticks;
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
