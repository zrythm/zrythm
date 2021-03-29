/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/backend/timeline.h"
#include "utils/objects.h"

/**
 * Inits the Timeline after a Project is loaded.
 */
void
timeline_init_loaded (
  Timeline * self)
{
}

/**
 * Inits the Timeline instance.
 */
void
timeline_init (
  Timeline * self)
{
  editor_settings_init (&self->editor_settings);
}

/**
 * Creates a new Timeline instance.
 */
Timeline *
timeline_new (void)
{
  Timeline * self = object_new (Timeline);

  return self;
}

void
timeline_free (
  Timeline * self)
{
  object_zero_and_free (self);
}
