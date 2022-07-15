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

#ifndef __UTILS_ZRYTHM_OBJECT_H__
#define __UTILS_ZRYTHM_OBJECT_H__

/**
 * @addtogroup utils
 *
 * @{
 */

#define ZRYTHM_OBJECT_MAGIC 4111481
#define IS_ZRYTHM_OBJECT(_p) \
  ((_p) \
   && ((ZrythmObject *) (_p))->magic == ZRYTHM_OBJECT_MAGIC)

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
