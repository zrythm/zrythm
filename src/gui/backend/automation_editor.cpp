/*
 * SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include <stdlib.h>

#include "gui/backend/automation_editor.h"
#include "project.h"
#include "utils/objects.h"

void
automation_editor_init (AutomationEditor * self)
{
  editor_settings_init (&self->editor_settings);
}

AutomationEditor *
automation_editor_clone (AutomationEditor * src)
{
  AutomationEditor * self = object_new (AutomationEditor);

  self->editor_settings = src->editor_settings;

  return self;
}

AutomationEditor *
automation_editor_new (void)
{
  AutomationEditor * self = object_new (AutomationEditor);

  return self;
}

void
automation_editor_free (AutomationEditor * self)
{
  object_zero_and_free (self);
}
