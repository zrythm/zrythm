/*
 * Copyright © 2020 Canonical Ltd.
 * Copyright © 2021 Alexandros Theodotou
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __Z_STRVBUILDER_H__
#define __Z_STRVBUILDER_H__

#include <glib.h>

/**
 * GStrvBuilder:
 *
 * A helper object to build a %NULL-terminated string array
 * by appending. See g_strv_builder_new().
 *
 * Since: 2.68
 */
typedef struct _StrvBuilder StrvBuilder;

StrvBuilder *
strv_builder_new (void);

void
strv_builder_unref (StrvBuilder * builder);

StrvBuilder *
strv_builder_ref (StrvBuilder * builder);

void
strv_builder_add (
  StrvBuilder * builder,
  const char *  value);

void
strv_builder_addv (
  StrvBuilder * builder,
  const char ** value);

void
strv_builder_add_many (StrvBuilder * builder, ...)
  G_GNUC_NULL_TERMINATED;

char **
strv_builder_end (StrvBuilder * builder);

#endif /* __Z_STRVBUILDER_H__ */
