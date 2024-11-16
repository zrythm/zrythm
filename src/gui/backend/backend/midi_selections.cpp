// SPDX-FileCopyrightText: © 2019-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/position.h"
#include "gui/backend/backend/midi_selections.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

MidiSelections::MidiSelections () : ArrangerSelections (Type::Midi) { }

auto mn_compare_func = [] (const auto &a, const auto &b) {
  return std::get<MidiNote *> (a)->val_ < std::get<MidiNote *> (b)->val_;
};

MidiNote *
MidiSelections::get_highest_note ()
{
  if (objects_.empty ())
    return nullptr;

  return std::get<MidiNote *> (
    (*std::max_element (objects_.begin (), objects_.end (), mn_compare_func)));
}

MidiNote *
MidiSelections::get_lowest_note ()
{
  if (objects_.empty ())
    return nullptr;

  return std::get<MidiNote *> (
    (*std::min_element (objects_.begin (), objects_.end (), mn_compare_func)));
}

void
MidiSelections::sort_by_indices (bool desc)
{
  std::sort (
    objects_.begin (), objects_.end (), [desc] (const auto &a, const auto &b) {
      bool ret = false;
      ret = std::get<MidiNote *> (a)->index_ < std::get<MidiNote *> (b)->index_;
      return desc ? !ret : ret;
    });
}

void
MidiSelections::unlisten_note_diff (const MidiSelections &prev)
{
  // Check for notes in prev that are not in objects_ and stop listening to them
  for (const auto &prev_mn : prev.objects_ | type_is<MidiNote> ())
    {
      if (
        std::none_of (
          objects_.begin (), objects_.end (), [&prev_mn] (const auto &obj) {
            auto mn = std::get<MidiNote *> (obj);
            return *mn == *prev_mn;
          }))
        {
          prev_mn->listen (false);
        }
    }
}

bool
MidiSelections::can_be_pasted_at_impl (const Position pos, const int idx) const
{
  auto r_opt = CLIP_EDITOR->get_region ();
  if (!r_opt || !std::holds_alternative<MidiRegion *> (r_opt.value ()))
    return false;

  auto * r = std::get<MidiRegion *> (r_opt.value ());
  if (r->pos_->frames_ + pos.frames_ < 0)
    return false;

  return true;
}

void
MidiSelections::sort_by_pitch (bool desc)
{
  std::sort (
    objects_.begin (), objects_.end (), [desc] (const auto &a, const auto &b) {
      bool ret = false;
      ret = std::get<MidiNote *> (a)->val_ < std::get<MidiNote *> (b)->val_;
      return desc ? !ret : ret;
    });
}
