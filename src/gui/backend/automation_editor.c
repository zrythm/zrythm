/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdlib.h>

#include "gui/backend/automation_editor.h"
#include "project.h"
#include "utils/objects.h"

void
automation_editor_init (AutomationEditor * self)
{
  self->schema_version = AUTOMATION_EDITOR_SCHEMA_VERSION;

  editor_settings_init (&self->editor_settings);
}

AutomationEditor *
automation_editor_clone (AutomationEditor * src)
{
  AutomationEditor * self = object_new (AutomationEditor);
  self->schema_version = AUTOMATION_EDITOR_SCHEMA_VERSION;

  self->editor_settings = src->editor_settings;

  return self;
}

AutomationEditor *
automation_editor_new (void)
{
  AutomationEditor * self = object_new (AutomationEditor);
  self->schema_version = AUTOMATION_EDITOR_SCHEMA_VERSION;

  return self;
}

void
automation_editor_free (AutomationEditor * self)
{
  object_zero_and_free (self);
}
