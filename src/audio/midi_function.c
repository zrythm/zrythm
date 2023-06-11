// SPDX-FileCopyrightText: © 2020, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/engine.h"
#include "audio/midi_function.h"
#include "gui/backend/arranger_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "zrythm_app.h"

/**
 * Returns a string identifier for the type.
 */
char *
midi_function_type_to_string_id (MidiFunctionType type)
{
  const char * type_str = midi_function_type_to_string (type);
  char *       ret = g_strdup (type_str);
  for (size_t i = 0; i < strlen (ret); i++)
    {
      ret[i] = g_ascii_tolower (ret[i]);
      if (ret[i] == ' ')
        ret[i] = '-';
    }

  return ret;
}

/**
 * Returns a string identifier for the type.
 */
MidiFunctionType
midi_function_string_id_to_type (const char * id)
{
  for (MidiFunctionType i = MIDI_FUNCTION_CRESCENDO;
       i <= MIDI_FUNCTION_STRUM; i++)
    {
      char * str = midi_function_type_to_string_id (i);
      bool   eq = string_is_equal (str, id);
      g_free (str);
      if (eq)
        return i;
    }
  g_return_val_if_reached (MIDI_FUNCTION_CRESCENDO);
}

/**
 * Applies the given action to the given selections.
 *
 * @param sel Selections to edit.
 * @param type Function type.
 */
int
midi_function_apply (
  ArrangerSelections * sel,
  MidiFunctionType     type,
  MidiFunctionOpts     opts,
  GError **            error)
{
  /* TODO */
  g_message (
    "applying %s...", midi_function_type_to_string (type));

  MidiArrangerSelections * mas =
    (MidiArrangerSelections *) sel;

  switch (type)
    {
    case MIDI_FUNCTION_CRESCENDO:
      {
        CurveOptions curve_opts;
        curve_opts_init (&curve_opts);
        curve_opts.algo = opts.curve_algo;
        curve_opts.curviness = opts.curviness;
        int vel_interval = abs (opts.end_vel - opts.start_vel);
        MidiNote * first_note = (MidiNote *)
          arranger_selections_get_first_object (sel);
        MidiNote * last_note = (MidiNote *)
          arranger_selections_get_last_object (sel, false);
        if (first_note == last_note)
          {
            first_note->vel->vel = opts.start_vel;
            break;
          }

        double total_ticks =
          last_note->base.pos.ticks
          - first_note->base.pos.ticks;
        for (int i = 0; i < mas->num_midi_notes; i++)
          {
            MidiNote * mn = mas->midi_notes[i];
            double     mn_ticks_from_start =
              mn->base.pos.ticks - first_note->base.pos.ticks;
            double vel_multiplier = curve_get_normalized_y (
              mn_ticks_from_start / total_ticks, &curve_opts,
              opts.start_vel > opts.end_vel);
            mn->vel->vel =
              opts.start_vel
              + (midi_byte_t) (vel_interval * vel_multiplier);
          }
      }
      break;
    case MIDI_FUNCTION_FLAM:
      {
        /* currently MIDI functions assume no new notes are
         * added so currently disabled */
        break;
        MidiNote * new_midi_notes[mas->num_midi_notes];
        for (int i = 0; i < mas->num_midi_notes; i++)
          {
            MidiNote *       mn = mas->midi_notes[i];
            ArrangerObject * mn_obj = (ArrangerObject *) mn;
            double           len =
              arranger_object_get_length_in_ticks (mn_obj);
            MidiNote * new_mn =
              (MidiNote *) arranger_object_clone (
                mn_obj); // midi_note_new (&mn_obj->region_id, &mn->base.pos, &mn->base.end_pos, mn->val, mn->vel->vel);
            ArrangerObject * new_mn_obj =
              (ArrangerObject *) new_mn;
            new_midi_notes[i] = new_mn;
            double opt_ticks =
              position_ms_to_ticks ((signed_ms_t) opts.time);
            arranger_object_move (new_mn_obj, opt_ticks);
            if (opts.time >= 0)
              {
                /* make new note as long as existing note was
                 * and make existing note up to the new note */
                position_add_ticks (
                  &new_mn_obj->end_pos, len - opt_ticks);
                position_add_ticks (
                  &mn_obj->end_pos,
                  (-(
                    new_mn_obj->end_pos.ticks
                    - new_mn_obj->pos.ticks))
                    + 1);
              }
            else
              {
                /* make new note up to the existing note */
                position_set_to_pos (
                  &new_mn_obj->end_pos, &new_mn_obj->pos);
                position_add_ticks (
                  &new_mn_obj->end_pos,
                  ((mn_obj->end_pos.ticks - mn_obj->pos.ticks)
                   - opt_ticks)
                    - 1);
              }
          }
        int prev_num_midi_notes = mas->num_midi_notes;
        for (int i = 0; i < prev_num_midi_notes; i++)
          {
            MidiNote *       mn = new_midi_notes[i];
            ArrangerObject * mn_obj = (ArrangerObject *) mn;
            ZRegion * r = region_find (&mn_obj->region_id);
            midi_region_add_midi_note (
              r, mn, F_NO_PUBLISH_EVENTS);
            /* clone again because the selections are supposed to hold clones */
            ArrangerObject * mn_clone =
              arranger_object_clone (mn_obj);
            arranger_selections_add_object (sel, mn_clone);
          }
      }
      break;
    case MIDI_FUNCTION_FLIP_VERTICAL:
      {
        MidiNote * highest_note =
          midi_arranger_selections_get_highest_note (mas);
        MidiNote * lowest_note =
          midi_arranger_selections_get_lowest_note (mas);
        g_return_val_if_fail (highest_note && lowest_note, -1);
        int highest_pitch = highest_note->val;
        int lowest_pitch = lowest_note->val;
        int diff = highest_pitch - lowest_pitch;
        for (int i = 0; i < mas->num_midi_notes; i++)
          {
            MidiNote * mn = mas->midi_notes[i];
            uint8_t new_val = diff - (mn->val - lowest_pitch);
            new_val += lowest_pitch;
            mn->val = new_val;
          }
      }
      break;
    case MIDI_FUNCTION_FLIP_HORIZONTAL:
      {
        arranger_selections_sort_by_positions (sel, false);
        Position poses[mas->num_midi_notes];
        for (int i = 0; i < mas->num_midi_notes; i++)
          {
            MidiNote * mn = mas->midi_notes[i];
            position_set_to_pos (&poses[i], &mn->base.pos);
            position_print (&poses[i]);
          }
        for (int i = 0; i < mas->num_midi_notes; i++)
          {
            MidiNote *       mn = mas->midi_notes[i];
            ArrangerObject * obj = (ArrangerObject *) mn;
            double           ticks =
              arranger_object_get_length_in_ticks (obj);
            position_set_to_pos (
              &obj->pos,
              &poses[(mas->num_midi_notes - i) - 1]);
            position_set_to_pos (&obj->end_pos, &obj->pos);
            position_print (&obj->pos);
            position_add_ticks (&obj->end_pos, ticks);
          }
      }
      break;
    case MIDI_FUNCTION_LEGATO:
      {
        arranger_selections_sort_by_positions (sel, false);
        for (int i = 0; i < mas->num_midi_notes - 1; i++)
          {
            MidiNote *       mn = mas->midi_notes[i];
            ArrangerObject * mn_obj = (ArrangerObject *) mn;
            MidiNote *       next_mn = mas->midi_notes[i + 1];
            position_set_to_pos (
              &mn_obj->end_pos, &next_mn->base.pos);
            /* make sure the note has a length */
            if (mn_obj->end_pos.ticks - mn_obj->pos.ticks < 1.0)
              {
                position_add_ms (&mn_obj->end_pos, 40.0);
              }
          }
      }
      break;
    case MIDI_FUNCTION_PORTATO:
      {
        /* do the same as legato but leave some space between
         * the notes */
        arranger_selections_sort_by_positions (sel, false);
        for (int i = 0; i < mas->num_midi_notes - 1; i++)
          {
            MidiNote *       mn = mas->midi_notes[i];
            ArrangerObject * mn_obj = (ArrangerObject *) mn;
            MidiNote *       next_mn = mas->midi_notes[i + 1];
            position_set_to_pos (
              &mn_obj->end_pos, &next_mn->base.pos);
            position_add_ms (&mn_obj->end_pos, -80.0);
            /* make sure the note has a length */
            if (mn_obj->end_pos.ticks - mn_obj->pos.ticks < 1.0)
              {
                position_set_to_pos (
                  &mn_obj->end_pos, &next_mn->base.pos);
                position_add_ms (&mn_obj->end_pos, 40.0);
              }
          }
      }
      break;
    case MIDI_FUNCTION_STACCATO:
      {
        for (int i = 0; i < mas->num_midi_notes - 1; i++)
          {
            MidiNote *       mn = mas->midi_notes[i];
            ArrangerObject * mn_obj = (ArrangerObject *) mn;
            position_set_to_pos (
              &mn_obj->end_pos, &mn_obj->pos);
            position_add_ms (&mn_obj->end_pos, 140.0);
          }
      }
      break;
    case MIDI_FUNCTION_STRUM:
      {
        CurveOptions curve_opts;
        curve_opts_init (&curve_opts);
        curve_opts.algo = opts.curve_algo;
        curve_opts.curviness = opts.curviness;

        midi_arranger_selections_sort_by_pitch (
          mas, !opts.ascending);
        for (int i = 0; i < mas->num_midi_notes; i++)
          {
            MidiNote * first_mn = mas->midi_notes[0];
            MidiNote * mn = mas->midi_notes[i];
            double ms_multiplier = curve_get_normalized_y (
              (double) i / (double) mas->num_midi_notes,
              &curve_opts, !opts.ascending);
            double ms_to_add = ms_multiplier * opts.time;
            g_message (
              "multi %f, ms %f", ms_multiplier, ms_to_add);
            ArrangerObject * mn_obj = (ArrangerObject *) mn;
            double           len_ticks =
              arranger_object_get_length_in_ticks (mn_obj);
            position_set_to_pos (
              &mn_obj->pos, &first_mn->base.pos);
            position_add_ms (&mn_obj->pos, ms_to_add);
            position_set_to_pos (
              &mn_obj->end_pos, &mn_obj->pos);
            position_add_ticks (&mn_obj->end_pos, len_ticks);
          }
      }
      break;
    default:
      break;
    }

  /* set last action */
  g_settings_set_int (S_UI, "midi-function", type);

  EVENTS_PUSH (ET_EDITOR_FUNCTION_APPLIED, NULL);

  return 0;
}
