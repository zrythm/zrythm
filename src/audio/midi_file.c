/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/midi_file.h"

#include <gtk/gtk.h>

#include <ext/midilib/src/midifile.h>

/**
 * Returns whether the given track in the midi file
 * has data.
 */
bool
midi_file_track_has_data (
  const char * abs_path,
  int          track_idx)
{
  MIDI_FILE * mf = midiFileOpen (abs_path);
  g_return_val_if_fail (mf, false);

  MIDI_MSG msg;
  midiReadInitMessage (&msg);

  int ev;
  g_debug ("reading MIDI Track %d", track_idx);
  bool have_data = false;
  while (
    midiReadGetNextMessage (mf, track_idx, &msg))
    {
      if (msg.bImpliedMsg)
        {
          ev = msg.iImpliedMsg;
        }
      else
        {
          ev = msg.iType;
        }

      switch (ev)
        {
        case msgNoteOff:
        case msgMetaEvent:
          break;
        case msgNoteOn:
        case msgNoteKeyPressure:
        case msgSetParameter:
        case msgSetProgram:
        case msgChangePressure:
        case msgSetPitchWheel:
        case msgSysEx1:
        case msgSysEx2:
          have_data = true;
          break;
        }

      if (have_data)
        {
          break;
        }
    }

  midiReadFreeMessage (&msg);
  midiFileClose (mf);

  return have_data;
}

/**
 * Returns the number of tracks in the MIDI file.
 */
int
midi_file_get_num_tracks (
  const char * abs_path,
  bool         non_empty_only)
{
  MIDI_FILE * mf = midiFileOpen (abs_path);
  g_return_val_if_fail (mf, -1);

  int actual_num = 0;

  int num = midiReadGetNumTracks (mf);
  g_debug ("%s: num tracks = %d", abs_path, num);

  if (!non_empty_only)
    {
      midiFileClose (mf);
      return num;
    }

  for (int i = 0; i < num; i++)
    {
      if (midi_file_track_has_data (abs_path, i))
        {
          actual_num++;
        }
    }

  midiFileClose (mf);

  return actual_num;
}
