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

#include "utils/strv_builder.h"

/**
 * SECTION:gstrvbuilder
 * @title: GStrvBuilder
 * @short_description: Helper to create NULL-terminated string arrays.
 *
 * #GStrvBuilder is a method of easily building dynamically sized
 * NULL-terminated string arrays.
 *
 * The following example shows how to build a two element array:
 *
 * |[<!-- language="C" -->
 *   g_autoptr(GStrvBuilder) builder = strv_builder_new ();
 *   strv_builder_add (builder, "hello");
 *   strv_builder_add (builder, "world");
 *   g_auto(GStrv) array = strv_builder_end (builder);
 * ]|
 *
 * Since: 2.68
 */

struct _StrvBuilder
{
  GPtrArray array;
};

/**
 * strv_builder_new:
 *
 * Creates a new #GStrvBuilder with a reference count of 1.
 * Use strv_builder_unref() on the returned value when no longer needed.
 *
 * Returns: (transfer full): the new #GStrvBuilder
 *
 * Since: 2.68
 */
StrvBuilder *
strv_builder_new (void)
{
  return (StrvBuilder *)
    g_ptr_array_new_with_free_func (g_free);
}

/**
 * strv_builder_unref:
 * @builder: (transfer full): a #GStrvBuilder allocated by strv_builder_new()
 *
 * Decreases the reference count on @builder.
 *
 * In the event that there are no more references, releases all memory
 * associated with the #GStrvBuilder.
 *
 * Since: 2.68
 **/
void
strv_builder_unref (StrvBuilder * builder)
{
  g_ptr_array_unref (&builder->array);
}

/**
 * strv_builder_ref:
 * @builder: (transfer none): a #GStrvBuilder
 *
 * Atomically increments the reference count of @builder by one.
 * This function is thread-safe and may be called from any thread.
 *
 * Returns: (transfer full): The passed in #GStrvBuilder
 *
 * Since: 2.68
 */
StrvBuilder *
strv_builder_ref (StrvBuilder * builder)
{
  return (StrvBuilder *) g_ptr_array_ref (
    &builder->array);
}

/**
 * strv_builder_add:
 * @builder: a #GStrvBuilder
 * @value: a string.
 *
 * Add a string to the end of the array.
 *
 * Since 2.68
 */
void
strv_builder_add (
  StrvBuilder * builder,
  const char *  value)
{
  g_ptr_array_add (
    &builder->array, g_strdup (value));
}

/**
 * strv_builder_addv:
 * @builder: a #GStrvBuilder
 * @value: (array zero-terminated=1): the vector of strings to add
 *
 * Appends all the strings in the given vector to the builder.
 *
 * Since 2.70
 */
void
strv_builder_addv (
  StrvBuilder * builder,
  const char ** value)
{
  gsize i = 0;
  g_return_if_fail (builder != NULL);
  g_return_if_fail (value != NULL);
  for (i = 0; value[i] != NULL; i++)
    strv_builder_add (builder, value[i]);
}

/**
 * strv_builder_add_many:
 * @builder: a #GStrvBuilder
 * @...: one or more strings followed by %NULL
 *
 * Appends all the given strings to the builder.
 *
 * Since 2.70
 */
void
strv_builder_add_many (StrvBuilder * builder, ...)
{
  va_list       var_args;
  const gchar * str;
  g_return_if_fail (builder != NULL);
  va_start (var_args, builder);
  while ((str = va_arg (var_args, gchar *)) != NULL)
    strv_builder_add (builder, str);
  va_end (var_args);
}

/**
 * strv_builder_end:
 * @builder: a #GStrvBuilder
 *
 * Ends the builder process and returns the constructed NULL-terminated string
 * array. The returned value should be freed with g_strfreev() when no longer
 * needed.
 *
 * Returns: (transfer full): the constructed string array.
 *
 * Since 2.68
 */
char **
strv_builder_end (StrvBuilder * builder)
{
  /* Add NULL terminator */
  g_ptr_array_add (&builder->array, NULL);
  return (char **) g_ptr_array_steal (
    &builder->array, NULL);
}
