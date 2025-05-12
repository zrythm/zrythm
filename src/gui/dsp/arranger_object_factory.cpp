// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/io/midi_file.h"
#include "gui/dsp/arranger_object_factory.h"
#include "gui/dsp/transport.h"

ArrangerObjectFactory *
ArrangerObjectFactory::get_instance ()
{
  return PROJECT->getArrangerObjectFactory ();
}

MidiRegion *
ArrangerObjectFactory::addMidiRegionFromChordDescriptor (
  MidiLane *                  lane,
  const dsp::ChordDescriptor &descr,
  double                      startTicks)
{
  auto *     mr = addEmptyMidiRegion (lane, startTicks);
  const auto r_len_ticks = snap_grid_timeline_.get_default_ticks ();
  const auto frames_per_tick = frames_per_tick_getter_ ();
  mr->set_end_pos_full_size (
    dsp::Position (static_cast<double> (r_len_ticks), frames_per_tick),
    frames_per_tick);
  const auto mn_len_ticks = snap_grid_editor_.get_default_ticks ();

  /* create midi notes */
  for (const auto i : std::views::iota (0zu, dsp::ChordDescriptor::MAX_NOTES))
    {
      if (descr.notes_.at (i))
        {
          auto mn =
            get_builder<MidiNote> ()
              .with_pitch (i + 36)
              .with_velocity (VELOCITY_DEFAULT)
              .with_start_ticks (0)
              .with_end_ticks (mn_len_ticks)
              .build_in_registry ();
          mr->add_object (mn);
        }
    }

  return mr;
}

MidiRegion *
ArrangerObjectFactory::addMidiRegionFromMidiFile (
  MidiLane *     lane,
  const QString &abs_path,
  double         startTicks,
  int            midi_track_idx)
{
  auto * mr = addEmptyMidiRegion (lane, startTicks);

  try
    {
      MidiFile mf{ utils::Utf8String::from_qstring (abs_path) };
      // FIXME use dependency injection instead of global TRANSPORT
      mf.into_region (*mr, *TRANSPORT, midi_track_idx);
      if (*mr->pos_ >= *mr->end_pos_)
        {
          throw ZrythmException (fmt::format (
            "Invalid positions: start {} end {}", *mr->pos_, *mr->end_pos_));
        }
    }
  catch (const ZrythmException &e)
    {
      z_warning ("Failed to create region from MIDI file: {}", e.what ());
    }

  return mr;
}