/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/*#include "project.h"*/
/*#include "audio/channel.h"*/
/*#include "audio/engine.h"*/
/*#include "utils/midi.h"*/
/*#include "audio/midi_note.h"*/
/*#include "audio/midi_region.h"*/
/*#include "audio/region.h"*/
/*#include "audio/track.h"*/
/*#include "audio/transport.h"*/
/*#include "audio/velocity.h"*/
/*#include "gui/widgets/velocity.h"*/
/*#include "utils/arrays.h"*/
/*#include "utils/flags.h"*/
/*#include "utils/io.h"*/
/*#include "utils/smf.h"*/

/*#include <gtk/gtk.h>*/

/*#include <smf.h>*/

/**
 * Saves regions into MIDI files (.smf)
 */
/*void*/
/*smf_save_regions ()*/
/*{*/
/*[>int ret;<]*/
/*[>ZRegion * region;<]*/
/*[>io_mkdir (PROJECT->regions_dir);<]*/

/*[>for (int i = 0; i < PROJECT->num_regions; i++)<]*/
/*[>{<]*/
/*[>region =<]*/
/*[>project_get_region (i);<]*/

/*[>if (region->linked_region ||<]*/
/*[>region->track->type != TRACK_TYPE_INSTRUMENT)<]*/
/*[>continue;<]*/

/*[>smf_t *smf;<]*/
/*[>smf_track_t *track;<]*/
/*[>smf_event_t *event;<]*/

/*[>smf = smf_new();<]*/
/*[>if (smf == NULL)<]*/
/*[>{<]*/
/*[>g_warning ("smf_new failed");<]*/
/*[>return;<]*/
/*[>}<]*/
/*[>ret = smf_set_ppqn (smf,<]*/
/*[>TICKS_PER_QUARTER_NOTE);<]*/
/*[>if (ret)<]*/
/*[>{<]*/
/*[>g_warning ("Setting PPQN failed");<]*/
/*[>return;<]*/
/*[>}<]*/

/*[>track = smf_track_new();<]*/
/*[>if (track == NULL)<]*/
/*[>{<]*/
/*[>g_warning ("smf_track_new failed");<]*/
/*[>return;<]*/
/*[>}<]*/

/*[>[> add tempo event <]<]*/
/*[>smf_add_track(smf, track);<]*/
/*[>#ifdef HAVE_JACK<]*/
/*[>jack_midi_event_t ev;<]*/
/* see
       * http://www.mixagesoftware.com/en/midikit/help/HTML/meta_events.html
       */
/*[>ev.size = 6;<]*/
/*[>ev.buffer = calloc (6, sizeof (jack_midi_data_t));<]*/
/*[>ev.buffer[0] = 0xFF;<]*/
/*[>ev.buffer[1]= 0x51;<]*/
/*[>ev.buffer[2] = 0x03;<]*/
/*[>[> convert bpm to tempo value <]<]*/
/*[>int tempo = 60000000 / TRANSPORT->bpm;<]*/
/*[>ev.buffer[3] = (tempo >> 16) & 0xFF;<]*/
/*[>ev.buffer[4] = (tempo >> 8) & 0xFF;<]*/
/*[>ev.buffer[5] = tempo & 0xFF;<]*/
/*[>event = smf_event_new_from_pointer (ev.buffer,<]*/
/*[>ev.size);<]*/
/*[>smf_track_add_event_pulses (<]*/
/*[>track,<]*/
/*[>event,<]*/
/*[>0);<]*/
/*[>free (ev.buffer);<]*/
/*[>#endif<]*/

/*[>MidiEvents * events = calloc (1, sizeof (MidiEvents));<]*/
/*[>Position abs_start_pos;<]*/
/*[>position_init (&abs_start_pos);<]*/
/*[>ZRegion * midi_region = (ZRegion *) region;<]*/
/*[>midi_note_notes_to_events (<]*/
/*[>midi_region->midi_notes,<]*/
/*[>midi_region->num_midi_notes,<]*/
/*[>&abs_start_pos,<]*/
/*[>events);<]*/

/*[>for (int j = 0; j < events->num_events; j++)<]*/
/*[>{<]*/
/*[>#ifdef HAVE_JACK<]*/
/*[>jack_midi_event_t * ev =<]*/
/*[>&events->jack_midi_events[j];<]*/
/*[>event =<]*/
/*[>smf_event_new_from_pointer (<]*/
/*[>ev->buffer, ev->size);<]*/
/*[>if (event == NULL)<]*/
/*[>{<]*/
/*[>g_warning ("smf event is NULL");<]*/
/*[>return;<]*/
/*[>}<]*/

/*[>smf_track_add_event_pulses (<]*/
/*[>track,<]*/
/*[>event,<]*/
/*[>ev->time / AUDIO_ENGINE->frames_per_tick);<]*/
/*[>#endif<]*/
/*[>[>g_message ("event at %d", ev->time / AUDIO_ENGINE->frames_per_tick);<]<]*/
/*[>}<]*/

/*[>free (events);<]*/

/*[>char * region_filename = region_generate_filename (region);<]*/

/*[>char * full_path =<]*/
/*[>g_build_filename (PROJECT->regions_dir,<]*/
/*[>region_filename,<]*/
/*[>NULL);<]*/
/*[>g_message ("Writing region %s", full_path);<]*/

/*[>[> save the midi file <]<]*/
/*[>int ret = smf_save(smf, full_path);<]*/
/*[>g_free (full_path);<]*/
/*[>g_free (region_filename);<]*/
/*[>if (ret)<]*/
/*[>{<]*/
/*[>g_warning ("smf_save failed");<]*/
/*[>return;<]*/
/*[>}<]*/

/*[>smf_delete(smf);<]*/
/*[>}<]*/
/*[>g_message ("smf save succeeded");<]*/
/*}*/

/**
 * Loads midi notes from region MIDI files.
 */
/*void*/
/*smf_load_region (const char *   file,   ///< file to load*/
/*ZRegion *   midi_region)  ///< region to save midi notes in*/
/*{*/
/*[>smf_t *smf;<]*/
/*[>smf_event_t *event;<]*/

/*[>smf = smf_load (file);<]*/
/*[>if (smf == NULL)<]*/
/*[>{<]*/
/*[>g_warning ("Failed loading %s", file);<]*/
/*[>return;<]*/
/*[>}<]*/

/*[>g_message ("Loading smf %s", file);<]*/

/*[>MidiNote notes[400];<]*/
/*[>int      num_notes = 0; [> started notes <]<]*/
/*[>[>float bpm;<]<]*/

/*[>while ((event = smf_get_next_event(smf)) != NULL)<]*/
/*[>{<]*/
/*[>[> get bpm <]<]*/
/*[>[>if (event->midi_buffer_length == 6 &&<]<]*/
/*[>[>event->midi_buffer[0] == 0xFF &&<]<]*/
/*[>[>event->midi_buffer[1] == 0x51 &&<]<]*/
/*[>[>event->midi_buffer[2] == 0x03)<]<]*/
/*[>[>{<]<]*/
/*[>[>int tempo = (event->midi_buffer[3] << 16) |<]<]*/
/*[>[>(event->midi_buffer[4] << 8) |<]<]*/
/*[>[>(event->midi_buffer[5]);<]<]*/
/*[>[>bpm = 60000000.f / tempo;<]<]*/
/*[>[>[>transport_set_bpm (bpm);<]<]<]*/
/*[>[>continue;<]<]*/
/*[>[>}<]<]*/

/*[>if (smf_event_is_metadata(event))<]*/
/*[>continue;<]*/

/*[>if (event->midi_buffer_length != 3)<]*/
/*[>{<]*/
/*[>continue;<]*/
/*[>}<]*/

/*[>uint8_t type = event->midi_buffer[0] & 0xf0;<]*/
/*[>[>uint8_t channel = event->midi_buffer[0] & 0xf;<]<]*/
/*[>int ticks = event->time_pulses;<]*/
/*[>if (type == MIDI_CH1_NOTE_ON) [> note on <]<]*/
/*[>{<]*/
/*[>MidiNote * midi_note = &notes[num_notes];<]*/

/*[>g_message ("note on at %d ticks", ticks);<]*/
/*[>[>position_set_to_pos (&notes[num_notes].start_pos,<]<]*/
/*[>[>region->start_pos)<]<]*/
/*[>position_init (&midi_note->start_pos);<]*/
/*[>[>position_add_frames (&notes[num_notes].start_pos,<]<]*/
/*[>[>frames);<]<]*/
/*[>position_set_tick (&midi_note->start_pos,<]*/
/*[>ticks);<]*/
/*[>midi_note->val =<]*/
/*[>event->midi_buffer[1];<]*/
/*[>midi_note->vel =<]*/
/*[>velocity_new (event->midi_buffer[2]);<]*/
/*[>midi_note->vel->midi_note = midi_note;<]*/
/*[>midi_note->vel->widget =<]*/
/*[>velocity_widget_new (midi_note->vel);<]*/

/*[>num_notes++;<]*/
/*[>}<]*/
/*[>else if (type == MIDI_CH1_NOTE_OFF) [> note off <]<]*/
/*[>{<]*/
/*[>g_message ("note off at %d ticks", ticks);<]*/
/*[>[> find note and set its end pos <]<]*/
/*[>for (int i = 0; i < num_notes; i++)<]*/
/*[>{<]*/
/*[>if (notes[i].val == event->midi_buffer[1])<]*/
/*[>{<]*/
/*[>position_init (&notes[i].end_pos);<]*/
/*[>[>position_add_frames (&notes[i].end_pos,<]<]*/
/*[>[>frames);<]<]*/
/*[>position_set_tick (<]*/
/*[>&notes[i].end_pos,<]*/
/*[>ticks);<]*/

/*[>[> FIXME below doesn't work <]<]*/
/*[>[>array_delete (notes,<]<]*/
/*[>[>num_notes,<]<]*/
/*[>[>&notes[i]);<]<]*/
/*[>break;<]*/
/*[>}<]*/
/*[>}<]*/
/*[>}<]*/
/*[>}<]*/

/*[>for (int i = 0; i < num_notes; i++)<]*/
/*[>{<]*/
/*[>MidiNote * midi_note =<]*/
/*[>midi_note_new (<]*/
/*[>midi_region,<]*/
/*[>&notes[i].start_pos,<]*/
/*[>&notes[i].end_pos,<]*/
/*[>notes[i].val,<]*/
/*[>notes[i].vel,<]*/
/*[>F_ADD_TO_PROJ);<]*/
/*[>midi_region_add_midi_note (midi_region,<]*/
/*[>midi_note);<]*/
/*[>}<]*/

/*[>smf_delete(smf);<]*/

/*}*/
