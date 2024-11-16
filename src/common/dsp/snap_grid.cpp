// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/engine.h"
#include "common/dsp/snap_grid.h"
#include "common/dsp/transport.h"
#include "common/utils/gtest_wrapper.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"

#include <fmt/printf.h>

using namespace zrythm;

const char **
note_length_get_strings (void)
{
  static const char * note_length_strings[] = {
    QT_TR_NOOP_UTF8 ("bar"),
    QT_TR_NOOP_UTF8 ("beat"),
    "2/1",
    "1/1",
    "1/2",
    "1/4",
    "1/8",
    "1/16",
    "1/32",
    "1/64",
    "1/128",
  };
  return note_length_strings;
}

const char *
note_length_to_str (NoteLength len)
{
  return note_length_get_strings ()[ENUM_VALUE_TO_INT (len)];
}

const char **
note_type_get_strings (void)
{
  static const char * note_type_strings[] = {
    QT_TR_NOOP_UTF8 ("normal"),
    QT_TR_NOOP_UTF8 ("dotted"),
    QT_TR_NOOP_UTF8 ("triplet"),
  };
  return note_type_strings;
}

const char *
note_type_to_str (NoteType type)
{
  return note_type_get_strings ()[ENUM_VALUE_TO_INT (type)];
}

int
SnapGrid::get_ticks_from_length_and_type (NoteLength length, NoteType type)
{
  int ticks = 0;
  switch (length)
    {
    case NoteLength::NOTE_LENGTH_BAR:
      z_return_val_if_fail (TRANSPORT && TRANSPORT->ticks_per_bar_ > 0, -1);
      ticks = TRANSPORT->ticks_per_bar_;
      break;
    case NoteLength::NOTE_LENGTH_BEAT:
      z_return_val_if_fail (TRANSPORT && TRANSPORT->ticks_per_beat_ > 0, -1);
      ticks = TRANSPORT->ticks_per_beat_;
      break;
    case NoteLength::NOTE_LENGTH_2_1:
      ticks = 8 * TICKS_PER_QUARTER_NOTE;
      break;
    case NoteLength::NOTE_LENGTH_1_1:
      ticks = 4 * TICKS_PER_QUARTER_NOTE;
      break;
    case NoteLength::NOTE_LENGTH_1_2:
      ticks = 2 * TICKS_PER_QUARTER_NOTE;
      break;
    case NoteLength::NOTE_LENGTH_1_4:
      ticks = TICKS_PER_QUARTER_NOTE;
      break;
    case NoteLength::NOTE_LENGTH_1_8:
      ticks = TICKS_PER_QUARTER_NOTE / 2;
      break;
    case NoteLength::NOTE_LENGTH_1_16:
      ticks = TICKS_PER_QUARTER_NOTE / 4;
      break;
    case NoteLength::NOTE_LENGTH_1_32:
      ticks = TICKS_PER_QUARTER_NOTE / 8;
      break;
    case NoteLength::NOTE_LENGTH_1_64:
      ticks = TICKS_PER_QUARTER_NOTE / 16;
      break;
    case NoteLength::NOTE_LENGTH_1_128:
      ticks = TICKS_PER_QUARTER_NOTE / 32;
      break;
    default:
      z_return_val_if_reached (-1);
    }

  switch (type)
    {
    case NoteType::NOTE_TYPE_NORMAL:
      break;
    case NoteType::NOTE_TYPE_DOTTED:
      ticks = 3 * ticks;
      z_return_val_if_fail (ticks % 2 == 0, -1);
      ticks = ticks / 2;
      break;
    case NoteType::NOTE_TYPE_TRIPLET:
      ticks = 2 * ticks;
      z_return_val_if_fail (ticks % 3 == 0, -1);
      ticks = ticks / 3;
      break;
    }

  return ticks;
}

int
SnapGrid::get_snap_ticks () const
{
  if (snap_adaptive_)
    {
      if (!ZRYTHM_HAVE_UI || ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
        {
          z_error ("can only be used with UI");
          return -1;
        }

      return -1;

#if 0
      RulerWidget * ruler = NULL;
      if (type_ == Type::Timeline)
        {
          ruler = MW_RULER;
        }
      else
        {
          ruler = EDITOR_RULER;
        }

      /* get intervals used in drawing */
      int sixteenth_interval = ruler_widget_get_sixteenth_interval (ruler);
      int beat_interval = ruler_widget_get_beat_interval (ruler);
      int bar_interval =
        (int) MAX ((RW_PX_TO_HIDE_BEATS) / (double) ruler->px_per_bar, 1.0);

      /* attempt to snap at smallest interval */
      if (sixteenth_interval > 0)
        {
          return sixteenth_interval
                 * get_ticks_from_length_and_type (
                   NoteLength::NOTE_LENGTH_1_16, snap_note_type_);
        }
      else if (beat_interval > 0)
        {
          return beat_interval
                 * get_ticks_from_length_and_type (
                   NoteLength::NOTE_LENGTH_BEAT, snap_note_type_);
        }
      else
        {
          return bar_interval
                 * get_ticks_from_length_and_type (
                   NoteLength::NOTE_LENGTH_BAR, snap_note_type_);
        }
#endif
    }
  else
    {
      return get_ticks_from_length_and_type (snap_note_length_, snap_note_type_);
    }
}

double
SnapGrid::get_snap_frames () const
{
  int snap_ticks = get_snap_ticks ();
  return AUDIO_ENGINE->frames_per_tick_ * static_cast<double> (snap_ticks);
}

int
SnapGrid::get_default_ticks () const
{
  if (length_type_ == NoteLengthType::NOTE_LENGTH_LINK)
    {
      return get_snap_ticks ();
    }
  else if (length_type_ == NoteLengthType::NOTE_LENGTH_LAST_OBJECT)
    {
      double last_obj_length = 0.0;
      if (type_ == SnapGrid::Type::Timeline)
        {
          last_obj_length =
            gui::SettingsManager::timelineLastCreatedObjectLengthInTicks ();
        }
      else if (type_ == SnapGrid::Type::Editor)
        {
          last_obj_length =
            gui::SettingsManager::editorLastCreatedObjectLengthInTicks ();
        }
      return (int) last_obj_length;
    }
  else
    {
      return get_ticks_from_length_and_type (
        default_note_length_, default_note_type_);
    }
}

static const char *
get_note_type_short_str (NoteType type)
{
  static const char * note_type_short_strings[] = {
    "",
    ".",
    "t",
  };
  return note_type_short_strings[ENUM_VALUE_TO_INT (type)];
}

std::string
SnapGrid::stringize_length_and_type (NoteLength note_length, NoteType note_type)
{
  const char * c = get_note_type_short_str (note_type);
  const char * first_part = note_length_to_str (note_length);

  return fmt::sprintf ("%s%s", first_part, c);
}

std::string
SnapGrid::stringize () const
{
  if (snap_adaptive_)
    {
      return QObject::tr ("Adaptive").toStdString ();
    }
  else
    {
      return stringize_length_and_type (snap_note_length_, snap_note_type_);
    }
}

bool
SnapGrid::get_nearby_snap_point (
  Position       &ret_pos,
  const Position &pos,
  const bool      return_prev)
{
  z_return_val_if_fail (pos.frames_ >= 0 && pos.ticks_ >= 0, false);

  ret_pos = pos;
  double snap_ticks = get_snap_ticks ();
  double ticks_from_prev = fmod (pos.ticks_, snap_ticks);
  if (return_prev)
    {
      ret_pos.add_ticks (-ticks_from_prev);
    }
  else
    {
      ret_pos.add_ticks (snap_ticks - ticks_from_prev);
    }

  return true;
}