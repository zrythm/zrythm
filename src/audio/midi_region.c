/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
 *      Copyright 2010, Steven Goodwin.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License,or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "audio/channel.h"
#include "audio/exporter.h"
#include "audio/midi.h"
#include "audio/midi_note.h"
#include "audio/midi_region.h"
#include "audio/region.h"
#include "audio/tempo_track.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/math.h"
#include "utils/object_utils.h"
#include "utils/objects.h"
#include "utils/yaml.h"
#include "zrythm_app.h"

#include <ext/midilib/src/midifile.h>
#include <ext/midilib/src/midiutil.h>

ZRegion *
midi_region_new (
  const Position * start_pos,
  const Position * end_pos,
  int              track_pos,
  int              lane_pos,
  int              idx_inside_lane)
{
  ZRegion * self =
    calloc (1, sizeof (MidiRegion));

  self->id.type = REGION_TYPE_MIDI;

  region_init (
    self, start_pos, end_pos, track_pos,
    lane_pos, idx_inside_lane);

   return self;
}

/**
 * Prints the MidiNotes in the Region.
 *
 * Used for debugging.
 */
void
midi_region_print_midi_notes (
  ZRegion * self)
{
  MidiNote * mn;
  for (int i = 0; i < self->num_midi_notes; i++)
    {
      mn = self->midi_notes[i];
      g_message ("Note at %d", i);
      midi_note_print (mn);
    }
}


/**
 * Adds the MidiNote to the given ZRegion.
 *
 * @param pub_events Publish UI events or not.
 */
void
midi_region_add_midi_note (
  ZRegion *  region,
  MidiNote * midi_note,
  int        pub_events)
{
  array_double_size_if_full (
    region->midi_notes, region->num_midi_notes,
    region->midi_notes_size, MidiNote *)
  array_append (
    region->midi_notes, region->num_midi_notes,
    midi_note);
  midi_note_set_region_and_index (
    midi_note, region, region->num_midi_notes - 1);

  if (pub_events)
    {
      EVENTS_PUSH (
        ET_ARRANGER_OBJECT_CREATED, midi_note);
    }
}

/**
 * Returns the midi note with the given pitch from
 * the unended notes.
 *
 * Used when recording.
 *
 * @param pitch The pitch. If -1, it returns any
 *   unended note. This is useful when the loop
 *   point is met and we want to end them all.
 */
MidiNote *
midi_region_pop_unended_note (
  ZRegion * self,
  int          pitch)
{
  MidiNote * match = NULL;
  for (int i = 0; i < self->num_unended_notes; i++)
    {
      MidiNote * mn = self->unended_notes[i];
      if (pitch == -1 ||
          mn->val == pitch)
        {
          match = mn;
          break;
        }
    }

  if (match)
    {
      /* pop it from the array */
      array_delete (
        self->unended_notes,
        self->num_unended_notes,
        match);

      return match;
    }

  return NULL;
}

/**
 * Gets first midi note
 */
MidiNote *
midi_region_get_first_midi_note (
  ZRegion * region)
{
  MidiNote * result = NULL;
  for (int i = 0;
    i < region->num_midi_notes;
    i++)
  {
    if (
      !result  ||
      ((ArrangerObject *) result)->
        end_pos.total_ticks >
      ((ArrangerObject *) region->midi_notes[i])->
        end_pos.total_ticks)
    {
      result = region->midi_notes[i];
    }
  }
  return result;
}
/**
 * Gets last midi note
 */
MidiNote *
midi_region_get_last_midi_note (
  ZRegion * region)
{
  MidiNote * result = NULL;
  for (int i = 0;
    i < region->num_midi_notes;
    i++)
  {
    if (
      !result ||
      ((ArrangerObject *) result)->
        end_pos.total_ticks <
      ((ArrangerObject *) region->midi_notes[i])->
        end_pos.total_ticks)
    {
      result = region->midi_notes[i];
    }
  }
  return result;
}

/**
 * Gets highest midi note
 */
MidiNote *
midi_region_get_highest_midi_note (
  ZRegion * region)
{
  MidiNote * result = NULL;
  for (int i = 0;
    i < region->num_midi_notes;
    i++)
  {
    if (!result ||
        result->val < region->midi_notes[i]->val)
    {
      result = region->midi_notes[i];
    }
  }
  return result;
}

/**
 * Gets lowest midi note
 */
MidiNote *
midi_region_get_lowest_midi_note (
  ZRegion * region)
{
  MidiNote * result = NULL;
  for (int i = 0;
    i < region->num_midi_notes;
    i++)
  {
    if (!result ||
        result->val > region->midi_notes[i]->val)
    {
      result = region->midi_notes[i];
    }
  }

  return result;
}

/**
 * Removes the MIDI note from the Region.
 *
 * @param free Also free the MidiNote.
 * @param pub_event Publish an event.
 */
void
midi_region_remove_midi_note (
  ZRegion *   region,
  MidiNote * midi_note,
  int        free,
  int        pub_event)
{
  if (MA_SELECTIONS)
    {
      arranger_selections_remove_object (
        (ArrangerSelections *) MA_SELECTIONS,
        (ArrangerObject *) midi_note);
    }

  /*ARRANGER_WIDGET_GET_PRIVATE (*/
    /*MW_MIDI_ARRANGER);*/
  /*if (ar_prv->start_object ==*/
        /*(ArrangerObject *) midi_note)*/
    /*{*/
      /*ar_prv->start_object = NULL;*/
    /*}*/

  array_delete (
    region->midi_notes, region->num_midi_notes,
    midi_note);

  for (int i = 0; i < region->num_midi_notes; i++)
    {
      midi_note_set_region_and_index (
        region->midi_notes[i], region, i);
    }

  if (free)
    free_later (midi_note, arranger_object_free);

  if (pub_event)
    {
      EVENTS_PUSH (
        ET_ARRANGER_OBJECT_REMOVED,
        ARRANGER_OBJECT_TYPE_MIDI_NOTE);
    }
}

/**
 * Creates a MIDI region from the given MIDI
 * file path, starting at the given Position.
 *
 * @param idx The index of this track, starting from
 *   0. This will be sequential, ie, if idx 1 is
 *   requested and the MIDI file only has tracks
 *   5 and 7, it will use track 7.
 */
ZRegion *
midi_region_new_from_midi_file (
  const Position * start_pos,
  const char *     abs_path,
  int              track_pos,
  int              lane_pos,
  int              idx_inside_lane,
  int              idx)
{
  g_message (
    "%s: reading from %s...", __func__, abs_path);

  ZRegion * self = object_new (ZRegion);

  self->id.type = REGION_TYPE_MIDI;

  MIDI_FILE * mf =
    midiFileOpen (abs_path);
  g_return_val_if_fail (mf, NULL);

  bool print_details = false;

  char str[128];
  int ev;
  MIDI_MSG msg;
  int i, iNum;
  unsigned int j;
  Position pos;
  MidiNote * mn;
  double ticks;

  Position end_pos;
  position_from_ticks (
    &end_pos, start_pos->total_ticks + 1);
  region_init (
    self, start_pos, &end_pos, track_pos,
    lane_pos, idx_inside_lane);

  midiReadInitMessage (&msg);
  iNum = midiReadGetNumTracks(mf);
  double ppqn = (double) midiFileGetPPQN (mf);
  double transport_ppqn =
    transport_get_ppqn (TRANSPORT);

  for(i = 0; i < iNum; i++)
    {
      if (i != idx)
        continue;

      g_message (
        "%s: reading MIDI Track %d", __func__, i);
      while(midiReadGetNextMessage (mf, i, &msg))
        {
          /* convert time to zrythm time */
          ticks =
            ((double) msg.dwAbsPos * transport_ppqn) /
            ppqn;
          position_from_ticks (&pos, ticks);
          if (print_details)
            {
              g_message (
                "dwAbsPos: %d ", msg.dwAbsPos);
            }
          position_print (&pos);

          if (ZRYTHM_HAVE_UI &&
              pos.bars >
                MW_TIMELINE->last_timeline_obj_bars -8)
            {
              MW_TIMELINE->last_timeline_obj_bars =
                pos.bars + 8;
              EVENTS_PUSH (
                ET_LAST_TIMELINE_OBJECT_CHANGED,
                NULL);
            }

          if (msg.bImpliedMsg)
            {
              ev = msg.iImpliedMsg;
            }
          else
            {
              ev = msg.iType;
            }

          if (muGetMIDIMsgName (str, ev) &&
              print_details)
            {
              g_message("MIDI msg name: %s", str);
            }
          switch(ev)
            {
            case msgNoteOff:
              if (print_details)
                {
                  g_message (
                    "Note off at %d "
                    "[ch %d pitch %d]",
                    msg.dwAbsPos,
                    msg.MsgData.NoteOff.iChannel,
                    msg.MsgData.NoteOff.iNote);
                }
              mn =
                midi_region_pop_unended_note (
                  self, msg.MsgData.NoteOff.iNote);
              if (mn)
                {
                  arranger_object_end_pos_setter (
                    (ArrangerObject *) mn,  &pos);
                }
              else
                {
                  g_message (
                    "Found a Note off event without "
                    "a corresponding Note on. "
                    "Skipping...");
                }
              break;
            case msgNoteOn:
              if (print_details)
                {
                  g_message (
                    "Note on at %d "
                    "[ch %d pitch %d vel %d]",
                    msg.dwAbsPos,
                    msg.MsgData.NoteOn.iChannel,
                    msg.MsgData.NoteOn.iNote,
                    msg.MsgData.NoteOn.iVolume);
                }
              midi_region_start_unended_note (
                self, &pos, NULL,
                msg.MsgData.NoteOn.iNote,
                msg.MsgData.NoteOn.iVolume, 0);
              break;
            case  msgNoteKeyPressure:
              muGetNameFromNote (
                str,
                msg.MsgData.NoteKeyPressure.iNote);
              if (print_details)
                {
                  g_message (
                    "(%.2d) %s %d",
                    msg.MsgData.NoteKeyPressure.
                      iChannel,
                    str,
                    msg.MsgData.NoteKeyPressure.
                      iPressure);
                }
              break;
            case  msgSetParameter:
              muGetControlName (
                str,
                msg.MsgData.NoteParameter.iControl);
              if (print_details)
                {
                  g_message (
                    "(%.2d) %s -> %d",
                    msg.MsgData.NoteParameter.iChannel,
                    str,
                    msg.MsgData.NoteParameter.iParam);
                }
              break;
            case  msgSetProgram:
              muGetInstrumentName (
                str,
                msg.MsgData.ChangeProgram.iProgram);
              if (print_details)
                {
                  g_message (
                    "(%.2d) %s",
                    msg.MsgData.ChangeProgram.iChannel,
                    str);
                }
              break;
            case msgChangePressure:
              muGetControlName (
                str,
                msg.MsgData.ChangePressure.
                  iPressure);
              if (print_details)
                {
                  g_message (
                    "(%.2d) %s",
                    msg.MsgData.ChangePressure.iChannel,
                    str);
                }
              break;
            case msgSetPitchWheel:
              if (print_details)
                {
                  g_message (
                    "(%.2d) %d",
                    msg.MsgData.PitchWheel.iChannel,
                    msg.MsgData.PitchWheel.iPitch);
                }
              break;
            case msgMetaEvent:
              if (print_details)
                {
                  g_message ("---- meta events");
                }
              switch(msg.MsgData.MetaEvent.iType)
                {
                case metaMIDIPort:
                  if (print_details)
                    {
                      g_message (
                        "MIDI Port = %d",
                        msg.MsgData.MetaEvent.Data.
                          iMIDIPort);
                    }
                  break;
                case metaSequenceNumber:
                  if (print_details)
                    {
                      g_message (
                        "Sequence Number = %d",
                        msg.MsgData.MetaEvent.Data.
                          iSequenceNumber);
                    }
                  break;
                case  metaTextEvent:
                  if (print_details)
                    {
                      g_message (
                        "Text = '%s'",
                        msg.MsgData.MetaEvent.Data.Text.
                          pData);
                    }
                  break;
                case  metaCopyright:
                  if (print_details)
                    {
                      g_message (
                        "Copyright = '%s'",
                        msg.MsgData.MetaEvent.Data.Text.
                          pData);
                    }
                  break;
                case  metaTrackName:
                  {
                    char tmp[6000];
                    strncpy (
                      tmp,
                      (char *) msg.MsgData.MetaEvent.Data.
                        Text.pData,
                      msg.iMsgSize - 3);
                    tmp[msg.iMsgSize - 3] = '\0';
                    region_set_name (
                      self, tmp, 0);
                    g_warn_if_fail (self->name);
                    g_message (
                      "[data sz %d] Track name = '%s'",
                      msg.iMsgSize - 3, self->name);
                  }
                  break;
                case  metaInstrument:
                  g_message (
                    "Instrument = '%s'",
                    msg.MsgData.MetaEvent.Data.Text.
                      pData);
                  break;
                case  metaLyric:
                  g_message (
                    "Lyric = '%s'",
                    msg.MsgData.MetaEvent.
                      Data.Text.pData);
                  break;
                case  metaMarker:
                  g_message (
                    "Marker = '%s'",
                    msg.MsgData.MetaEvent.
                      Data.Text.pData);
                  break;
                case  metaCuePoint:
                  g_message (
                    "Cue point = '%s'",
                    msg.MsgData.MetaEvent.
                      Data.Text.pData);
                  break;
                case  metaEndSequence:
                  g_message ("End Sequence");
                  if (position_is_equal (
                        &pos, start_pos))
                    {
                       /* this is an empty track,
                        * so return NULL
                       * instead */
                      return NULL;
                    }
                  arranger_object_end_pos_setter (
                    (ArrangerObject *) self,  &pos);
                  arranger_object_loop_end_pos_setter (
                    (ArrangerObject *) self,  &pos);
                  break;
                case  metaSetTempo:
                  g_message (
                    "tempo %d",
                    msg.MsgData.MetaEvent.Data.
                      Tempo.iBPM);
                  break;
                case  metaSMPTEOffset:
                  g_message (
                    "SMPTE offset = %d:%d:%d.%d %d",
                    msg.MsgData.MetaEvent.Data.
                      SMPTE.iHours,
                    msg.MsgData.MetaEvent.Data.
                      SMPTE.iMins,
                    msg.MsgData.MetaEvent.Data.
                      SMPTE.iSecs,
                    msg.MsgData.MetaEvent.Data.
                      SMPTE.iFrames,
                    msg.MsgData.MetaEvent.Data.
                      SMPTE.iFF);
                  break;
                case metaTimeSig:
                  g_message (
                    "Time sig = %d/%d",
                    msg.MsgData.MetaEvent.Data.
                      TimeSig.iNom,
                    msg.MsgData.MetaEvent.Data.
                      TimeSig.iDenom /
                      MIDI_NOTE_CROCHET);
                  break;
                case metaKeySig:
                  if (muGetKeySigName (
                        str,
                        msg.MsgData.MetaEvent.
                          Data.KeySig.iKey))
                    g_message("Key sig = %s", str);
                  break;
                case  metaSequencerSpecific:
                  g_message("Sequencer specific = ");
                  /*HexList(msg.MsgData.MetaEvent.Data.Sequencer.pData, msg.MsgData.MetaEvent.Data.Sequencer.iSize);*/
                  break;
                }
              break;

            case msgSysEx1:
            case msgSysEx2:
              g_message("Sysex = ");
              /*HexList(msg.MsgData.SysEx.pData, msg.MsgData.SysEx.iSize);*/
              break;
            }

          /* print the hex */
          if (ev == msgSysEx1 ||
              (ev==msgMetaEvent &&
               msg.MsgData.MetaEvent.iType ==
                 metaSequencerSpecific))
            {
            /* Already done a hex dump */
            }
          else if (print_details)
            {
              char txt[600];
              char tmp[100];
              strcpy (txt, "[");
              if (msg.bImpliedMsg)
                {
                  sprintf (
                    tmp, "%.2x!", msg.iImpliedMsg);
                  strcat (txt, tmp);
                }
              for (j = 0; j < msg.iMsgSize; j++)
                {
                  sprintf(
                    tmp, " %.2x ", msg.data[j]);
                  strcat (txt, tmp);
                }
              strcat (txt, "]");
              g_message ("%s", txt);
            }
        }
    }

  midiReadFreeMessage(&msg);
  midiFileClose(mf);

  g_message ("%s: done", __func__);

  return self;
}

/**
 * Starts an unended note with the given pitch and
 * velocity and adds it to \ref ZRegion.midi_notes.
 *
 * If another note exists with the same pitch, this
 * will be ignored.
 *
 * @param end_pos If this is NULL, it will be set to
 *   1 tick after the start_pos.
 */
void
midi_region_start_unended_note (
  ZRegion *  self,
  Position * start_pos,
  Position * _end_pos,
  int        pitch,
  int        vel,
  int        pub_events)
{
  g_return_if_fail (self && start_pos);

  /* set end pos */
  Position end_pos;
  if (_end_pos)
    {
      position_set_to_pos (&end_pos, _end_pos);
    }
  else
    {
      position_set_to_pos (&end_pos, start_pos);
      position_add_ticks (&end_pos, 1);
    }

  MidiNote * mn =
    midi_note_new (
      &self->id, start_pos, &end_pos, pitch, vel);
  midi_region_add_midi_note (self, mn, pub_events);

  /* add to unended notes */
  array_append (
    self->unended_notes,
    self->num_unended_notes, mn);
}

/**
 * Exports the ZRegion to an existing MIDI file
 * instance.
 *
 * @param add_region_start Add the region start
 *   offset to the positions.
 * @param export_full Traverse loops and export the
 *   MIDI file as it would be played inside Zrythm.
 *   If this is 0, only the original region (from
 *   true start to true end) is exported.
 * @param use_track_pos Whether to use the track
 *   position in the MIDI data. The track will be
 *   set to 1 if false.
 */
void
midi_region_write_to_midi_file (
  ZRegion *   self,
  MIDI_FILE * mf,
  const int   add_region_start,
  bool        export_full,
  bool        use_track_pos)
{
  MidiEvents * events =
    midi_region_get_as_events (
      self, add_region_start, export_full);
  MidiEvent * ev;
  int i;
  ArrangerObject * r_obj =
    (ArrangerObject *) self;
  Track * track =
    arranger_object_get_track (r_obj);
  for (i = 0; i < events->num_events; i++)
    {
      ev = &events->events[i];

      BYTE tmp[] =
        { ev->raw_buffer[0],
          ev->raw_buffer[1],
        ev->raw_buffer[2] };
      midiTrackAddRaw (
        mf, use_track_pos ? track->pos : 1, 3, tmp,
        1,
        i == 0 ?
          (int) ev->time :
          (int)
          (ev->time - events->events[i - 1].time));
    }
  midi_events_free (events);
}

/**
 * Exports the ZRegion to a specified MIDI file.
 *
 * @param full_path Absolute path to the MIDI file.
 * @param export_full Traverse loops and export the
 *   MIDI file as it would be played inside Zrythm.
 *   If this is 0, only the original region (from
 *   true start to true end) is exported.
 */
void
midi_region_export_to_midi_file (
  ZRegion * self,
  const char *   full_path,
  int            midi_version,
  const int      export_full)
{
  MIDI_FILE *mf;

  if ((mf = midiFileCreate(full_path, TRUE)))
    {
      /* Write tempo information out to track 1 */
      midiSongAddTempo (
        mf, 1,
        (int)
        tempo_track_get_current_bpm (P_TEMPO_TRACK));

      /* All data is written out to _tracks_ not
       * channels. We therefore
      ** set the current channel before writing
      data out. Channel assignments
      ** can change any number of times during the
      file, and affect all
      ** tracks messages until it is changed. */
      midiFileSetTracksDefaultChannel (
        mf, 1, MIDI_CHANNEL_1);

      midiFileSetPPQN (mf, TICKS_PER_QUARTER_NOTE);

      midiFileSetVersion (mf, midi_version);

      /* common time: 4 crochet beats, per bar */
      midiSongAddSimpleTimeSig (
        mf, 1, TRANSPORT->beats_per_bar,
        math_round_double_to_int (
          TRANSPORT->ticks_per_beat));

      midi_region_write_to_midi_file (
        self, mf, 0, export_full, false);

      midiFileClose(mf);
    }
}

/**
 * Returns the MIDI channel that this region should
 * be played on, starting from 1.
 */
uint8_t
midi_region_get_midi_ch (
  const ZRegion * self)
{
  g_return_val_if_fail (self, 1);
  uint8_t ret;
  TrackLane * lane = region_get_lane (self);
  g_return_val_if_fail (lane, 1);
  if (lane->midi_ch > 0)
    ret = lane->midi_ch;
  else
    {
      Track * track = track_lane_get_track (lane);
      g_return_val_if_fail (track, 1);
      ret = track->midi_ch;
    }

  g_warn_if_fail (ret > 0);

  return ret;
}

/**
 * Returns a newly initialized MidiEvents with
 * the contents of the region converted into
 * events.
 *
 * Must be free'd with midi_events_free ().
 *
 * @param add_region_start Add the region start
 *   offset to the positions.
 * @param export_full Traverse loops and export the
 *   MIDI file as it would be played inside Zrythm.
 *   If this is 0, only the original region (from
 *   true start to true end) is exported.
 */
MidiEvents *
midi_region_get_as_events (
  ZRegion * self,
  const int      add_region_start,
  const int      full)
{
  MidiEvents * events =
    midi_events_new (NULL);

  ArrangerObject * self_obj =
    (ArrangerObject *) self;

  double region_start = 0;
  if (add_region_start)
    region_start = self_obj->pos.total_ticks;

  MidiNote * mn;
  for (int i = 0; i < self->num_midi_notes; i++)
    {
      mn = self->midi_notes[i];
      ArrangerObject * mn_obj =
        (ArrangerObject *) mn;

      midi_events_add_note_on (
        events, 1, mn->val, mn->vel->vel,
        (midi_time_t)
        (position_to_ticks (&mn_obj->pos) +
          region_start),
        0);
      midi_events_add_note_off (
        events, 1, mn->val,
        (midi_time_t)
        (position_to_ticks (&mn_obj->end_pos) +
          region_start),
        0);
    }

  midi_events_sort_by_time (
    events);

  return events;
}

/**
 * Frees members only but not the MidiRegion
 * itself.
 *
 * Regions should be free'd using region_free.
 */
void
midi_region_free_members (ZRegion * self)
{
  g_return_if_fail (IS_REGION (self));

  for (int i = 0; i < self->num_midi_notes; i++)
    {
      arranger_object_free (
        (ArrangerObject *) self->midi_notes[i]);
    }
  for (int i = 0; i < self->num_unended_notes; i++)
    {
      arranger_object_free (
        (ArrangerObject *) self->unended_notes[i]);
    }
}
