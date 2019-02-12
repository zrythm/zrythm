/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Application settings.
 */

#include <stdlib.h>

#include "settings/settings.h"
#include "zrythm.h"

void
settings_init (Settings * self)
{
  self->root =
    g_settings_new ("org.zrythm");
  self->preferences =
    g_settings_new ("org.zrythm.preferences");
  self->ui =
    g_settings_new ("org.zrythm.ui");
}
