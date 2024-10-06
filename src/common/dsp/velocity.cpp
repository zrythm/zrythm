// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/midi_note.h"
#include "common/dsp/region.h"
#include "common/dsp/velocity.h"
#include "common/utils/string.h"
#include "gui/backend/gtk_widgets/bot_dock_edge.h"
#include "gui/backend/gtk_widgets/center_dock.h"
#include "gui/backend/gtk_widgets/clip_editor.h"
#include "gui/backend/gtk_widgets/clip_editor_inner.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/midi_editor_space.h"
#include "gui/backend/gtk_widgets/midi_modifier_arranger.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

Velocity::Velocity () : ArrangerObject (Type::Velocity) {};

Velocity::Velocity (MidiNote * midi_note, const uint8_t vel)
    : ArrangerObject (Type::Velocity),
      RegionOwnedObjectImpl<MidiRegion> (midi_note->region_id_),
      midi_note_ (midi_note), vel_ (vel)
{
}

void
Velocity::init_after_cloning (const Velocity &other)
{
  vel_ = other.vel_;
  vel_at_start_ = other.vel_at_start_;
  RegionOwnedObject::copy_members_from (other);
  ArrangerObject::copy_members_from (other);
}

void
Velocity::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  RegionOwnedObject::init_loaded_base ();
}

ArrangerWidget *
Velocity::get_arranger () const
{
  return (ArrangerWidget *) (MW_MIDI_MODIFIER_ARRANGER);
}

void
Velocity::set_val (const int val)
{
  vel_ = std::clamp<uint8_t> (val, 0, 127);

  /* re-set the midi note value to set a note off event */
  auto note = get_midi_note ();
  z_return_if_fail (IS_MIDI_NOTE (note));
  note->set_val (note->val_);
}

MidiNote *
Velocity::get_midi_note () const
{
  z_return_val_if_fail (midi_note_, nullptr);

  return midi_note_;
}

const char *
Velocity::setting_enum_to_str (guint index)
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

guint
Velocity::setting_str_to_enum (const char * str)
{
  guint val;
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

Velocity::ArrangerObjectPtr
Velocity::find_in_project () const
{
  const auto mn = get_midi_note ();
  auto prj_mn = std::dynamic_pointer_cast<MidiNote> (mn->find_in_project ());
  z_return_val_if_fail (prj_mn && prj_mn->vel_, nullptr);
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