// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/timeline.h"
#include "utils/objects.h"

/**
 * Inits the Timeline after a Project is loaded.
 */
void
timeline_init_loaded (Timeline * self)
{
}

/**
 * Inits the Timeline instance.
 */
void
timeline_init (Timeline * self)
{
  editor_settings_init (&self->editor_settings);
}

Timeline *
timeline_clone (Timeline * src)
{
  Timeline * self = object_new (Timeline);
  self->schema_version = TIMELINE_SCHEMA_VERSION;

  self->editor_settings = src->editor_settings;

  return self;
}

/**
 * Creates a new Timeline instance.
 */
Timeline *
timeline_new (void)
{
  Timeline * self = object_new (Timeline);
  self->schema_version = TIMELINE_SCHEMA_VERSION;

  return self;
}

void
timeline_free (Timeline * self)
{
  object_zero_and_free (self);
}
