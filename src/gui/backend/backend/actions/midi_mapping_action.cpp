// SPDX-FileCopyrightText: © 2020-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "engine/session/midi_mapping.h"
#include "engine/session/router.h"
#include "gui/backend/backend/actions/midi_mapping_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/port_identifier.h"

namespace zrythm::gui::actions
{

MidiMappingAction::MidiMappingAction (QObject * parent)
    : QObject (parent), UndoableAction (UndoableAction::Type::MidiMapping)
{
}

void
init_from (
  MidiMappingAction       &obj,
  const MidiMappingAction &other,
  utils::ObjectCloneType   clone_type)
{
  init_from (
    static_cast<UndoableAction &> (obj),
    static_cast<const UndoableAction &> (other), clone_type);
  obj.idx_ = other.idx_;
  obj.type_ = other.type_;
  obj.dest_port_id_ = other.dest_port_id_;
  obj.dev_id_ = other.dev_id_;
  obj.buf_ = other.buf_;
}

MidiMappingAction::MidiMappingAction (int idx_to_enable_or_disable, bool enable)
    : UndoableAction (UndoableAction::Type::MidiMapping),
      idx_ (idx_to_enable_or_disable),
      type_ (enable ? Type::Enable : Type::Disable)
{
}

MidiMappingAction::MidiMappingAction (
  const std::array<midi_byte_t, 3> buf,
  std::optional<utils::Utf8String> device_id,
  const Port                      &dest_port)
    : UndoableAction (UndoableAction::Type::MidiMapping), type_ (Type::Bind),
      dest_port_id_ (dest_port.get_uuid ()), dev_id_ (device_id), buf_ (buf)
{
}

MidiMappingAction::MidiMappingAction (int idx_to_unbind)
    : UndoableAction (UndoableAction::Type::MidiMapping), idx_ (idx_to_unbind),
      type_ (Type::Unbind)
{
}

void
MidiMappingAction::bind_or_unbind (bool bind)
{
  if (bind)
    {
      auto port_var = PROJECT->find_port_by_id (*dest_port_id_);
      z_return_if_fail (port_var);
      std::visit (
        [&] (auto &&port) {
          idx_ = MIDI_MAPPINGS->mappings_.size ();
          MIDI_MAPPINGS->bind_device (buf_, dev_id_, *port, false);
        },
        *port_var);
    }
  else
    {
      auto &mapping = MIDI_MAPPINGS->mappings_[idx_];
      buf_ = mapping->key_;
      dev_id_ = mapping->device_id_;
      dest_port_id_ = mapping->dest_id_;
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

  /* EVENTS_PUSH (EventType::ET_MIDI_BINDINGS_CHANGED, nullptr); */
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

  /* EVENTS_PUSH (EventType::ET_MIDI_BINDINGS_CHANGED, nullptr); */
}

QString
MidiMappingAction::to_string () const
{
  switch (type_)
    {
    case Type::Enable:
      return QObject::tr ("MIDI mapping enable");
    case Type::Disable:
      return QObject::tr ("MIDI mapping disable");
    case Type::Bind:
      return QObject::tr ("MIDI mapping bind");
    case Type::Unbind:
      return QObject::tr ("MIDI mapping unbind");
    default:
      z_return_val_if_reached ({});
    }
}
}
