// clang-format off
// SPDX-FileCopyrightText: © 2019-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

#include "dsp/engine.h"
#include "dsp/position.h"
#include "dsp/track.h"
#include "dsp/transport.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/midi_note.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/yaml.h"

#include "gtk_wrapper.h"

MidiNote *
midi_arranger_selections_get_highest_note (MidiArrangerSelections * mas)
{
  MidiNote * top_mn = mas->midi_notes[0];
  MidiNote * tmp;
  for (int i = 0; i < mas->num_midi_notes; i++)
    {
      tmp = mas->midi_notes[i];
      if (tmp->val > top_mn->val)
        {
          top_mn = tmp;
        }
    }
  return top_mn;
}

MidiNote *
midi_arranger_selections_get_lowest_note (MidiArrangerSelections * mas)
{

  MidiNote * bot_mn = mas->midi_notes[0];
  MidiNote * tmp;
  for (int i = 0; i < mas->num_midi_notes; i++)
    {
      tmp = mas->midi_notes[i];
      if (tmp->val < bot_mn->val)
        {
          bot_mn = tmp;
        }
    }
  return bot_mn;
}

/**
 * Sets the listen status of notes on and off based
 * on changes in the previous selections and the
 * current selections.
 */
void
midi_arranger_selections_unlisten_note_diff (
  MidiArrangerSelections * prev,
  MidiArrangerSelections * mas)
{
  for (int i = 0; i < prev->num_midi_notes; i++)
    {
      MidiNote * prev_mn = prev->midi_notes[i];
      int        found = 0;
      for (int j = 0; j < mas->num_midi_notes; j++)
        {
          MidiNote * mn = mas->midi_notes[j];
          if (midi_note_is_equal (prev_mn, mn))
            {
              found = 1;
              break;
            }
        }

      if (!found)
        {
          midi_note_listen (prev_mn, 0);
        }
    }
}

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param region Region to paste to.
 */
int
midi_arranger_selections_can_be_pasted (
  MidiArrangerSelections * ts,
  Position *               pos,
  Region *                 r)
{
  if (!r || r->id.type != RegionType::REGION_TYPE_MIDI)
    return 0;

  ArrangerObject * r_obj = (ArrangerObject *) r;
  if (r_obj->pos.frames + pos->frames < 0)
    return 0;

  return 1;
}

static int
sort_by_pitch_func (const void * _a, const void * _b)
{
  MidiNote * a = *(MidiNote * const *) _a;
  MidiNote * b = *(MidiNote * const *) _b;

  return a->pos - b->pos;
}

static int
sort_by_pitch_desc_func (const void * a, const void * b)
{
  return -sort_by_pitch_func (a, b);
}

void
midi_arranger_selections_sort_by_pitch (MidiArrangerSelections * self, bool desc)
{
  qsort (
    self->midi_notes, (size_t) self->num_midi_notes, sizeof (MidiNote *),
    desc ? sort_by_pitch_desc_func : sort_by_pitch_func);
}
