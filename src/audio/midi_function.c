// SPDX-FileCopyrightText: © 2020, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/engine.h"
#include "audio/midi_function.h"
#include "gui/backend/arranger_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/settings.h"
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
    default:
      break;
    }

  /* set last action */
  g_settings_set_int (S_UI, "midi-function", type);

  EVENTS_PUSH (ET_EDITOR_FUNCTION_APPLIED, NULL);

  return 0;
}
