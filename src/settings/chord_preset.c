/*
 * Copyright (C) 2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/chord_preset.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

ChordPreset *
chord_preset_new (
  const char * name)
{
  ChordPreset * self = object_new (ChordPreset);

  self->schema_version = CHORD_PRESET_SCHEMA_VERSION;
  self->name = g_strdup (name);

  return self;
}

ChordPreset *
chord_preset_clone (
  const ChordPreset * src)
{
  ChordPreset * self = chord_preset_new (src->name);

  for (int i = 0; i < 12; i++)
    {
      self->descr[i] =
        chord_descriptor_clone (src->descr[i]);
    }

  return self;
}

/**
 * Gets informational text.
 *
 * Must be free'd by caller.
 */
char *
chord_preset_get_info_text (
  const ChordPreset * self)
{
  GString * gstr = g_string_new (_("Chords"));
  g_string_append (gstr, ":\n");
  bool have_any = false;
  for (int i = 0; i < 12; i++)
    {
      ChordDescriptor * descr = self->descr[i];
      if (descr->type == CHORD_TYPE_NONE)
        break;

      char * lbl =
        chord_descriptor_to_new_string (descr);
      g_string_append (gstr, lbl);
      g_free (lbl);
      g_string_append (gstr, ", ");
      have_any = true;
    }

  if (have_any)
    {
      g_string_erase (
        gstr, (gssize) gstr->len - 2, 2);
    }

  return g_string_free (gstr, false);
}

const char *
chord_preset_get_name (
  const ChordPreset * self)
{
  return self->name;
}

void
chord_preset_set_name (
  ChordPreset * self,
  const char *  name)
{
  object_free_w_func_and_null (g_free, self->name);

  self->name = g_strdup (name);

  EVENTS_PUSH (ET_CHORD_PRESET_EDITED, NULL);
}

/**
 * Frees the plugin setting.
 */
void
chord_preset_free (
  ChordPreset * self)
{
  object_free_w_func_and_null (
    g_free, self->name);

  for (int i = 0; i < 12; i++)
    {
      object_free_w_func_and_null (
        chord_descriptor_free, self->descr[i]);
    }

  object_zero_and_free (self);
}
