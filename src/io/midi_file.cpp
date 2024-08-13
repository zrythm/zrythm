// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "io/midi_file.h"
#include "utils/logger.h"

#include "gtk_wrapper.h"
#include <ext/midilib/src/midifile.h>

/**
 * Returns whether the given track in the midi file has data.
 */
bool
midi_file_track_has_data (const char * abs_path, int track_idx)
{
  MIDI_FILE * mf = midiFileOpen (abs_path);
  z_return_val_if_fail (mf, false);

  MIDI_MSG msg;
  midiReadInitMessage (&msg);

  int ev;
  z_debug ("reading MIDI Track {}", track_idx);
  bool have_data = false;
  while (midiReadGetNextMessage (mf, track_idx, &msg))
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
midi_file_get_num_tracks (const char * abs_path, bool non_empty_only)
{
  MIDI_FILE * mf = midiFileOpen (abs_path);
  z_return_val_if_fail (mf, -1);

  int actual_num = 0;

  int num = midiReadGetNumTracks (mf);
  z_debug ("{}: num tracks = {}", abs_path, num);

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
