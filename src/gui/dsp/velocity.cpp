// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/midi_note.h"
#include "gui/dsp/region.h"
#include "gui/dsp/velocity.h"

#include "utils/string.h"

Velocity::Velocity (QObject * parent) : QObject (parent) { }

void
Velocity::init_after_cloning (const Velocity &other, ObjectCloneType clone_type)

{
  vel_ = other.vel_;
  vel_at_start_ = other.vel_at_start_;
  // RegionOwnedObject::copy_members_from (other, clone_type);
  // ArrangerObject::copy_members_from (other, clone_type);
}

#if 0
void
Velocity::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  RegionOwnedObject::init_loaded_base ();
}
#endif

void
Velocity::set_val (const int val)
{
  vel_ = std::clamp<uint8_t> (val, 0, 127);

  /* re-set the midi note value to set a note off event */
// TODO
#if 0
  auto * note = get_midi_note ();
  z_return_if_fail (IS_MIDI_NOTE (note));
  note->set_pitch (note->pitch_);
#endif
}

#if 0
MidiNote *
Velocity::get_midi_note () const
{
  z_return_val_if_fail (midi_note_, nullptr);

  return midi_note_;
}
#endif

const char *
Velocity::setting_enum_to_str (size_t index)
{
  switch (index)
    {
    case 0:
      return "last-note";
    case 1:
      return "40";
    case 2:
      return "90";
    case 3:
      return "120";
    default:
      break;
    }

  z_return_val_if_reached (nullptr);
}

size_t
Velocity::setting_str_to_enum (const char * str)
{
  size_t val;
  if (string_is_equal (str, "40"))
    {
      val = 1;
    }
  else if (string_is_equal (str, "90"))
    {
      val = 2;
    }
  else if (string_is_equal (str, "120"))
    {
      val = 3;
    }
  else
    {
      val = 0;
    }

  return val;
}

#if 0
std::optional<ArrangerObjectPtrVariant>
Velocity::find_in_project () const
{
  const auto mn = get_midi_note ();
  auto       mn_var = mn->find_in_project ();
  z_return_val_if_fail (mn_var.has_value (), std::nullopt);
  auto * prj_mn = std::get<MidiNote *> (mn_var.value ());
  z_return_val_if_fail (prj_mn && prj_mn->vel_, std::nullopt);
  return prj_mn->vel_;
}

std::string
Velocity::print_to_str () const
{
  return fmt::format ("Velocity: {}", vel_);
}

bool
Velocity::validate (bool is_project, double frames_per_tick) const
{
  // TODO
  return true;
}
#endif
