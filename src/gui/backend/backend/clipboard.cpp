// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/audio_selections.h"
#include "gui/backend/backend/automation_selections.h"
#include "gui/backend/backend/chord_selections.h"
#include "gui/backend/backend/clipboard.h"
#include "gui/backend/backend/midi_selections.h"
#include "gui/backend/backend/timeline_selections.h"

void
Clipboard::set_type_from_arranger_selections (const ArrangerSelections &sel)
{
  switch (sel.type_)
    {
    case ArrangerSelections::Type::Automation:
      type_ = Type::AutomationSelections;
      break;
    case ArrangerSelections::Type::Timeline:
      type_ = Type::TimelineSelections;
      break;
    case ArrangerSelections::Type::Midi:
      type_ = Type::MidiSelections;
      break;
    case ArrangerSelections::Type::Chord:
      type_ = Type::ChordSelections;
      break;
    case ArrangerSelections::Type::Audio:
      type_ = Type::AudioSelections;
      break;
    default:
      z_return_if_reached ();
    }
}

Clipboard::Clipboard (const ArrangerSelections &sel)
{
  std::visit (
    [&] (auto &&s) { arranger_sel_ = s->clone_unique (); },
    convert_to_variant<ArrangerSelectionsPtrVariant> (&sel));
  set_type_from_arranger_selections (sel);
}

Clipboard::Clipboard (const MixerSelections &sel)
{
  mixer_sel_ = sel.gen_full_from_this ();
  type_ = Type::MixerSelections;
}

Clipboard::Clipboard (const SimpleTracklistSelections &sel)
{
  tracklist_sel_ = sel.gen_tracklist_selections ();
  type_ = Type::TracklistSelections;
}

ArrangerSelections *
Clipboard::get_selections () const
{
  if (arranger_sel_)
    {
      return arranger_sel_.get ();
    }

  z_return_val_if_reached (nullptr);
}