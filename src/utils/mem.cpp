// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>
#include <cstring>

#include "utils/mem.h"

void *
z_realloc (void * ptr, size_t size)
{
  if (!ptr)
    {
      return calloc (1, size);
    }
  return realloc (ptr, size);
}

void
z_free_strv (char ** strv)
{
  if (!strv)
    return;

  for (char ** p = strv; *p != NULL; p++)
    {
      free (*p);
    }
  free (strv);
}
