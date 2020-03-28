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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/tracklist.h"
#include "guile/tracklist.h"

#include <libguile.h>

static SCM
get_tracks (void)
{
  /* TODO */
  /*SCM arr =*/
    /*scm_make_array (*/

}

static void
init_module (void * data)
{
  scm_c_define_gsubr (
    "tracklist-get-tracks", 0, 0, 0, get_tracks);
  scm_c_export ("tracklist-get-tracks", NULL);
}

void
guile_zrythm_define_module (void)
{
  scm_c_define_module (
    "zrythm audio tracklist", init_module, NULL);
}
