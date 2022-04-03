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

#include "guile/modules.h"

#ifndef SNARF_MODE
#  include "audio/port.h"
#  include "project.h"
#endif

SCM_DEFINE (
  s_stereo_ports_get_port,
  "stereo-ports-get-port",
  2,
  0,
  0,
  (SCM stereo_ports, SCM left),
  "Returns the left or right port of the stereo pair.")
#define FUNC_NAME s_
{
  StereoPorts * sp =
    (StereoPorts *) scm_to_pointer (stereo_ports);
  if (scm_to_bool (left))
    {
      return scm_from_pointer (sp->l, NULL);
    }
  else
    {
      return scm_from_pointer (sp->r, NULL);
    }
}
#undef FUNC_NAME

SCM_DEFINE (
  s_port_get_identifier,
  "port-get-identifier",
  1,
  0,
  0,
  (SCM port),
  "Returns the identifier of @var{port}.")
#define FUNC_NAME s_
{
  Port * p = (Port *) scm_to_pointer (port);
  return scm_from_pointer (&p->id, NULL);
}
#undef FUNC_NAME

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "audio_port.x"
#endif
  scm_c_export (
    "port-get-identifier", "stereo-ports-get-port",
    NULL);
}

void
guile_audio_port_define_module (void)
{
  scm_c_define_module (
    "audio port", init_module, NULL);
}
