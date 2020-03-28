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

#include "audio/position.h"
#include "guile/audio/position.h"

#include <libguile.h>

SCM
get_position_type (void)
{
  SCM name = scm_from_utf8_symbol ("position");
  SCM slots =
    scm_list_3 (
      scm_from_utf8_symbol ("data"),
      scm_from_utf8_symbol ("total_ticks"),
      scm_from_utf8_symbol ("frames"));
  scm_t_struct_finalize finalizer = NULL;

  return
    scm_make_foreign_object_type (
      name, slots, finalizer);
}

SCM
make_position (
  SCM name,
  SCM s_total_ticks,
  SCM s_frames)
{
  Position * pos;
  double total_ticks = scm_to_double (s_total_ticks);
  long frames = scm_to_long (s_frames);

  /* Allocate the Position.  Because we
     use scm_gc_malloc, this memory block will
     be automatically reclaimed when it becomes
     inaccessible, and its members will be traced
     by the garbage collector.  */
  pos = (Position *)
    scm_gc_malloc (sizeof (Position), "position");

  pos->total_ticks = total_ticks;
  pos->frames = frames;

  /* wrap the Position * in a new foreign object
   * and return that object */
  return
    scm_make_foreign_object_3 (
      get_position_type (), pos,
      s_total_ticks, s_frames);
}

static void
init_module (void * data)
{
  scm_c_define_gsubr (
    "position-new", 3, 0, 0, make_position);
  scm_c_export ("position-new", NULL);


}

void
guile_audio_position_define_module (void)
{
  scm_c_define_module (
    "zrythm audio position", init_module, NULL);
}
