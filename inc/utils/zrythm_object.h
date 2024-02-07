/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#ifndef __UTILS_ZRYTHM_OBJECT_H__
#define __UTILS_ZRYTHM_OBJECT_H__

/**
 * @addtogroup utils
 *
 * @{
 */

#define ZRYTHM_OBJECT_MAGIC 4111481
#define IS_ZRYTHM_OBJECT(_p) \
  ((_p) && ((ZrythmObject *) (_p))->magic == ZRYTHM_OBJECT_MAGIC)

/**
 * Base object for debugging.
 */
typedef struct ZrythmObject
{
  const char * file;
  const char * func;
  int          line;
  int          magic;
} ZrythmObject;

#define zrythm_object_init(x) \
  ((ZrythmObject *) x)->file = __FILE__; \
  ((ZrythmObject *) x)->func = __func__; \
  ((ZrythmObject *) x)->line = __LINE__; \
  ((ZrythmObject *) x)->magic = ZRYTHM_OBJECT_MAGIC

/**
 * @}
 */

#endif
