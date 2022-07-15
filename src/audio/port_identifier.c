// SPDX-FileCopyrightText: © 2018-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/port.h"
#include "audio/port_identifier.h"
#include "plugins/plugin_identifier.h"
#include "utils/hash.h"
#include "utils/objects.h"
#include "utils/string.h"

void
port_identifier_init (PortIdentifier * self)
{
  memset (self, 0, sizeof (PortIdentifier));
  self->schema_version = PORT_IDENTIFIER_SCHEMA_VERSION;
  plugin_identifier_init (&self->plugin_id);
}

#if 0
PortIdentifier *
port_identifier_new (void)
{
  PortIdentifier * self =
    object_new (PortIdentifier);
  self->schema_version =
    PORT_IDENTIFIER_SCHEMA_VERSION;

  self->track_pos = -1;
  plugin_identifier_init (&self->plugin_id);

  return self;
}
#endif

/**
 * Port group comparator function where @ref p1 and
 * @ref p2 are pointers to Port.
 */
int
port_identifier_port_group_cmp (const void * p1, const void * p2)
{
  const Port * control1 = *(const Port **) p1;
  const Port * control2 = *(const Port **) p2;

  g_return_val_if_fail (IS_PORT (control1), -1);
  g_return_val_if_fail (IS_PORT (control2), -1);

  /* use index for now - this assumes that ports
   * inside port groups are declared in sequence */
  return control1->id.port_index - control2->id.port_index;

#if 0
  return
    (control1->id.port_group &&
     control2->id.port_group) ?
      strcmp (
        control1->id.port_group,
        control2->id.port_group) : 0;
#endif
}

/**
 * Copy the identifier content from \ref src to
 * \ref dest.
 *
 * @note This frees/allocates memory on \ref dest.
 */
void
port_identifier_copy (
  PortIdentifier *       dest,
  const PortIdentifier * src)
{
  g_return_if_fail (dest != src);
  dest->schema_version = src->schema_version;
  string_copy_w_realloc (&dest->label, src->label);
  string_copy_w_realloc (&dest->sym, src->sym);
  string_copy_w_realloc (&dest->uri, src->uri);
  string_copy_w_realloc (&dest->comment, src->comment);
  dest->owner_type = src->owner_type;
  dest->type = src->type;
  dest->flow = src->flow;
  dest->flags = src->flags;
  dest->flags2 = src->flags2;
  plugin_identifier_copy (&dest->plugin_id, &src->plugin_id);
  string_copy_w_realloc (&dest->ext_port_id, src->ext_port_id);
  string_copy_w_realloc (&dest->port_group, src->port_group);
  dest->track_name_hash = src->track_name_hash;
  dest->port_index = src->port_index;
}

/**
 * Returns if the 2 PortIdentifier's are equal.
 *
 * @note Does not check insignificant data like
 *   comment.
 */
bool
port_identifier_is_equal (
  const PortIdentifier * src,
  const PortIdentifier * dest)
{
  bool eq =
    dest->owner_type == src->owner_type
    && dest->type == src->type && dest->flow == src->flow
    && dest->flags == src->flags && dest->flags2 == src->flags2
    && dest->track_name_hash == src->track_name_hash;
  if (dest->owner_type == PORT_OWNER_TYPE_PLUGIN)
    {
      eq =
        eq
        && plugin_identifier_is_equal (
          &dest->plugin_id, &src->plugin_id);
    }

  /* if LV2 (has symbol) check symbol match,
   * otherwise check index match and label match */
  if (dest->sym)
    {
      eq = eq && string_is_equal (dest->sym, src->sym);
    }
  else
    {
      eq =
        eq && dest->port_index == src->port_index
        && string_is_equal (dest->label, src->label);
    }

  /* do string comparisons at the end */
  eq =
    eq && string_is_equal (dest->uri, src->uri)
    && string_is_equal (dest->port_group, src->port_group)
    && string_is_equal (dest->ext_port_id, src->ext_port_id);

  return eq;
}

/**
 * To be used as GEqualFunc.
 */
int
port_identifier_is_equal_func (const void * a, const void * b)
{
  return port_identifier_is_equal (
    (const PortIdentifier *) a, (const PortIdentifier *) b);
}

void
port_identifier_print_to_str (
  const PortIdentifier * self,
  char *                 buf,
  size_t                 buf_sz)
{
  snprintf (
    buf, buf_sz,
    "[PortIdentifier %p]\nlabel: %s\n"
    "sym: %s\nuri: %s\ncomment: %s\nowner type: %s\n"
    "type: %s\nflow: %s\nflags: %d %d\n"
    "port group: %s\next port id: %s\n"
    "track name hash: %u\nport idx: %d",
    self, self->label, self->sym, self->uri, self->comment,
    port_owner_type_strings[self->owner_type].str,
    port_type_strings[self->type].str,
    port_flow_strings[self->flow].str, self->flags,
    self->flags2, self->port_group, self->ext_port_id,
    self->track_name_hash, self->port_index);
}

void
port_identifier_print (const PortIdentifier * self)
{
  size_t size = 2000;
  char   buf[size];
  port_identifier_print_to_str (self, buf, size);
  g_message ("%s", buf);
}

bool
port_identifier_validate (PortIdentifier * self)
{
  g_return_val_if_fail (
    self->schema_version == PORT_IDENTIFIER_SCHEMA_VERSION
      && plugin_identifier_validate (&self->plugin_id),
    false);
  return true;
}

uint32_t
port_identifier_get_hash (const void * self)
{
  void *                 state = hash_create_state ();
  const PortIdentifier * id = (const PortIdentifier *) self;
  uint32_t               hash = 0;
  if (id->label)
    hash = hash ^ g_str_hash (id->label);
  if (id->sym)
    hash = hash ^ g_str_hash (id->sym);
  if (id->uri)
    hash = hash ^ g_str_hash (id->uri);
  hash = hash ^ g_int_hash (&id->owner_type);
  hash = hash ^ g_int_hash (&id->type);
  hash = hash ^ g_int_hash (&id->flow);
  hash = hash ^ g_int_hash (&id->flags);
  hash = hash ^ g_int_hash (&id->flags2);
  hash = hash ^ g_int_hash (&id->unit);
  hash =
    hash
    ^ hash_get_for_struct_full (
      state, &id->plugin_id, sizeof (PluginIdentifier));
  if (id->port_group)
    hash = hash ^ g_str_hash (id->port_group);
  if (id->ext_port_id)
    hash = hash ^ g_str_hash (id->ext_port_id);
  hash = hash ^ id->track_name_hash;
  hash = hash ^ g_int_hash (&id->port_index);
  hash_free_state (state);
  return hash;
}

PortIdentifier *
port_identifier_clone (const PortIdentifier * src)
{
  PortIdentifier * self = object_new (PortIdentifier);
  self->schema_version = PORT_IDENTIFIER_SCHEMA_VERSION;

  port_identifier_copy (self, src);

  return self;
}

void
port_identifier_free_members (PortIdentifier * self)
{
  g_free_and_null (self->label);
  g_free_and_null (self->sym);
  g_free_and_null (self->uri);
  g_free_and_null (self->comment);
  g_free_and_null (self->port_group);
  g_free_and_null (self->ext_port_id);
  object_set_to_zero (self);
}

void
port_identifier_free (PortIdentifier * self)
{
  port_identifier_free_members (self);
  object_zero_and_free (self);
}

void
port_identifier_free_func (void * self)
{
  port_identifier_free ((PortIdentifier *) self);
}
