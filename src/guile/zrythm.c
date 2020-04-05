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

#if !defined(SCM_MAGIC_SNARF_DOCS) && \
  !defined(SCM_MAGIC_SNARFER)
#include "guile/modules.h"
#include "zrythm.h"
#endif

#include <libguile.h>

/**
 * Guile function to get the zrythm pointer.
 */
#if 0
static SCM
get_ptr (void)
{
  return scm_from_pointer (ZRYTHM, NULL);
}
#endif

SCM_DEFINE (
  get_ver, "zrythm-get-ver", 0, 0, 0,
  (),
  "Return the Zrythm version as a string.")
#define FUNC_NAME s_
{
  char ver[1000];
  zrythm_get_version_with_capabilities (ver);
  return
    scm_from_stringn (
      ver, strlen (ver), "UTF8",
      SCM_FAILED_CONVERSION_QUESTION_MARK);
}
#undef FUNC_NAME

static void
init_module (void * data)
{
  scm_c_define_gsubr (
    "zrythm-get-ver", 0, 0, 0, get_ver);
  scm_c_export ("zrythm-get-ver", NULL);
}

void
guile_zrythm_define_module (void)
{
  scm_c_define_module (
    "zrythm", init_module, NULL);
}
