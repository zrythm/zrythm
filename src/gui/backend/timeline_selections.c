/*
 * actions/timeline_selections.c - timeline
 *
 * Copyright (C) 2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/backend/timeline_selections.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

/**
 * Serializes to XML.
 *
 * MUST be free'd.
 */
char *
timeline_selections_serialize (
  TimelineSelections * ts) ///< TS to serialize
{
  cyaml_err_t err;

  char * output =
    calloc (1200, sizeof (char));
  size_t output_len;
  err =
    cyaml_save_data (
      &output,
      &output_len,
      &config,
      &timeline_selections_schema,
      ts,
      0);
  if (err != CYAML_OK)
    {
      g_warning ("error %s",
                 cyaml_strerror (err));
    }

  yaml_sanitize (&output);

  return output;
}

TimelineSelections *
timeline_selections_deserialize (const char * e)
{
  TimelineSelections * self;

  cyaml_err_t err =
    cyaml_load_data ((const unsigned char *) e,
                     strlen (e),
                     &config,
                     &timeline_selections_schema,
                     (cyaml_data_t **) &self,
                     NULL);
  if (err != CYAML_OK)
    {
      g_error ("error %s",
               cyaml_strerror (err));
    }

  return self;
}
