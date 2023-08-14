// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port_identifier.h"
#include "utils/objects.h"

#include "schemas/audio/port_identifier.h"

PortIdentifier *
port_identifier_upgrade_from_v1 (PortIdentifier_v1 * old)
{
  if (!old)
    return NULL;

  PortIdentifier * self = object_new (PortIdentifier);
  self->schema_version = PORT_IDENTIFIER_SCHEMA_VERSION;
  self->label = old->label;
  self->sym = old->sym;
  self->uri = old->uri;
  self->comment = old->comment;
  self->owner_type = (PortOwnerType) old->owner_type;
  self->type = (PortType) old->type;
  self->flow = (PortFlow) old->flow;
  self->unit = (PortUnit) old->unit;
  self->flags = (PortFlags) old->flags;
  self->flags2 = (PortFlags2) old->flags2;
  self->track_name_hash = old->track_name_hash;
  PluginIdentifier * plugin_id =
    plugin_identifier_upgrade_from_v1 (&old->plugin_id);
  self->plugin_id = *plugin_id;
  self->port_group = old->port_group;
  self->ext_port_id = old->ext_port_id;
  self->port_index = old->port_index;

  return self;
}
