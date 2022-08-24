// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdlib.h>
#include <string.h>

#include "gui/backend/event.h"
#include "utils/objects.h"

#include <gtk/gtk.h>

ZEvent *
event_new (void)
{
  ZEvent * self = object_new (ZEvent);

  return self;
}

void
event_free (ZEvent * self)
{
  g_free_and_null (self->backtrace);
  object_zero_and_free (self);
}
