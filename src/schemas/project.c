// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "project.h"
#include "utils/objects.h"

#include "schemas/project.h"

Project_v4 *
project_upgrade_from_v2_or_v3 (Project_v1 * old)
{
  if (!old)
    return NULL;

  Project_v4 * self = object_new (Project_v4);
  self->schema_version = 4;
  self->title = g_strdup (old->title);
  self->datetime_str = datetime_get_current_as_string ();
  self->version = zrythm_get_version (false);

  /* upgrade */
  self->tracklist = tracklist_upgrade_from_v1 (old->tracklist);
  self->audio_engine = engine_upgrade_from_v1 (old->audio_engine);
  self->tracklist_selections =
    tracklist_selections_upgrade_from_v1 (old->tracklist_selections);

  /* these will leak memory
   * TODO create versioned clone functions */
  self->clip_editor = old->clip_editor;
  self->timeline = old->timeline;
  self->snap_grid_timeline = old->snap_grid_timeline;
  self->snap_grid_editor = old->snap_grid_editor;
  self->quantize_opts_timeline = old->quantize_opts_timeline;
  self->quantize_opts_editor = old->quantize_opts_editor;
  self->region_link_group_manager = old->region_link_group_manager;
  self->port_connections_manager = old->port_connections_manager;
  self->midi_mappings = old->midi_mappings;
  self->last_selection = old->last_selection;
}

Project_v5 *
project_upgrade_from_v4 (Project_v4 * old);

Project *
project_upgrade_from_v5 (Project_v5 * old);
