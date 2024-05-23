/*
 * SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include <string.h>

#include "utils/sort.h"

#include <strings.h>

/**
 * Alphabetical sort func.
 *
 * The arguments must be strings (char *).
 */
int
sort_alphabetical_func (const void * a, const void * b)
{
  char * pa = *(char * const *) a;
  char * pb = *(char * const *) b;
  int    r = strcasecmp (pa, pb);
  if (r)
    return r;

  /* if equal ignoring case, use opposite of strcmp() result to get
   * lower before upper */
  return -strcmp (pa, pb); /* aka: return strcmp(b, a); */
}
