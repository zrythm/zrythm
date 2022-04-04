// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 */

#include "audio/engine.h"
#include "audio/midi_function.h"
#include "gui/backend/arranger_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "zrythm_app.h"

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
  GError **            error)
{
  /* TODO */
  g_message (
    "applying %s...",
    midi_function_type_to_string (type));

  /* set last action */
  g_settings_set_int (S_UI, "midi-function", type);

  EVENTS_PUSH (ET_EDITOR_FUNCTION_APPLIED, NULL);

  return 0;
}
