// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/channel.h"
#include "common/dsp/midi_mapping.h"
#include "common/dsp/port_identifier.h"
#include "common/dsp/router.h"
#include "gui/backend/backend/actions/midi_mapping_action.h"
#include "gui/backend/backend/event.h"
#include "gui/backend/backend/event_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

void
MidiMappingAction::init_after_cloning (const MidiMappingAction &other)
{
  UndoableAction::copy_members_from (other);
  idx_ = other.idx_;
  type_ = other.type_;
  dest_port_id_ =
    other.dest_port_id_
      ? std::make_unique<PortIdentifier> (*other.dest_port_id_)
      : nullptr;
  dev_port_ =
    other.dev_port_ ? std::make_unique<ExtPort> (*other.dev_port_) : nullptr;
  buf_ = other.buf_;
}

MidiMappingAction::MidiMappingAction (int idx_to_enable_or_disable, bool enable)
    : idx_ (idx_to_enable_or_disable),
      type_ (enable ? Type::Enable : Type::Disable)
{
  MidiMappingAction ();
}

MidiMappingAction::MidiMappingAction (
  const std::array<midi_byte_t, 3> buf,
  const ExtPort *                  device_port,
  const Port                      &dest_port)
    : type_ (Type::Bind),
      dest_port_id_ (std::make_unique<PortIdentifier> (dest_port.id_)),
      dev_port_ (device_port ? std::make_unique<ExtPort> (*device_port) : nullptr),
      buf_ (buf)
{
  MidiMappingAction ();
}

MidiMappingAction::MidiMappingAction (int idx_to_unbind)
    : idx_ (idx_to_unbind), type_ (Type::Unbind)
{
  MidiMappingAction ();
}

void
MidiMappingAction::bind_or_unbind (bool bind)
{
  if (bind)
    {
      auto port = Port::find_from_identifier (*dest_port_id_);
      idx_ = MIDI_MAPPINGS->mappings_.size ();
      MIDI_MAPPINGS->bind_device (buf_, dev_port_.get (), *port, false);
    }
  else
    {
      auto &mapping = MIDI_MAPPINGS->mappings_[idx_];
      buf_ = mapping->key_;
      dev_port_ =
        mapping->device_port_
          ? std::make_unique<ExtPort> (*mapping->device_port_)
          : nullptr;
      dest_port_id_ = std::make_unique<PortIdentifier> (mapping->dest_id_);
      MIDI_MAPPINGS->unbind (idx_, false);
    }
}

void
MidiMappingAction::perform_impl ()
{
  switch (type_)
    {
    case Type::Enable:
      MIDI_MAPPINGS->mappings_[idx_]->enabled_ = true;
      break;
    case Type::Disable:
      MIDI_MAPPINGS->mappings_[idx_]->enabled_ = false;
      break;
    case Type::Bind:
      bind_or_unbind (true);
      break;
    case Type::Unbind:
      bind_or_unbind (false);
      break;
    }

  EVENTS_PUSH (EventType::ET_MIDI_BINDINGS_CHANGED, nullptr);
}

void
MidiMappingAction::undo_impl ()
{
  switch (type_)
    {
    case Type::Enable:
      MIDI_MAPPINGS->mappings_[idx_]->enabled_ = false;
      break;
    case Type::Disable:
      MIDI_MAPPINGS->mappings_[idx_]->enabled_ = true;
      break;
    case Type::Bind:
      bind_or_unbind (false);
      break;
    case Type::Unbind:
      bind_or_unbind (true);
      break;
    }

  EVENTS_PUSH (EventType::ET_MIDI_BINDINGS_CHANGED, nullptr);
}

std::string
MidiMappingAction::to_string () const
{
  switch (type_)
    {
    case Type::Enable:
      return _ ("MIDI mapping enable");
    case Type::Disable:
      return _ ("MIDI mapping disable");
    case Type::Bind:
      return _ ("MIDI mapping bind");
    case Type::Unbind:
      return _ ("MIDI mapping unbind");
    default:
      z_return_val_if_reached ("");
    }
}