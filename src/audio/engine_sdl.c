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

#include "config.h"

#ifdef HAVE_SDL

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/engine_sdl.h"
#include "audio/master_track.h"
#include "audio/mixer.h"
#include "audio/port.h"
#include "project.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

/**
 * Set up Port Audio.
 */
int
engine_sdl_setup (
  AudioEngine * self,
  int           loading)
{
  g_message ("Setting up SDL...");

  g_message ("SDL set up");

  return 0;
}

/**
 * Tests if PortAudio is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_sdl_test (
  GtkWindow * win)
{
  return 0;
}

/**
 * Closes Port Audio.
 */
void
engine_sdl_tear_down (
  AudioEngine * engine)
{
}

#endif
