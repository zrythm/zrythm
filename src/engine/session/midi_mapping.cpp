// SPDX-FileCopyrightText: © 2019-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "utils/format_qt.h"
#include <fmt/std.h>

#include "dsp/midi_event.h"
#include "engine/session/midi_mapping.h"
#include "utils/logger.h"
#include "utils/midi.h"
#include "utils/serialization.h"

namespace zrythm::engine::session
{
MidiMapping::MidiMapping (utils::IObjectRegistry &registry, QObject * parent)
    : QObject (parent), registry_ (registry)
{
}

void
init_from (
  MidiMapping           &obj,
  const MidiMapping     &other,
  utils::ObjectCloneType clone_type)
{
  obj.key_ = other.key_;
  obj.device_id_ = other.device_id_;
  obj.dest_id_ = other.dest_id_;
  obj.enabled_.store (other.enabled_.load ());
}

MidiMappings::MidiMappings (utils::IObjectRegistry &registry)
    : registry_ (registry)
{
}

void
MidiMappings::bind_at (
  std::array<midi_byte_t, 3>           buf,
  std::optional<utils::Utf8String>     device_id,
  dsp::ProcessorParameterUuidReference dest_port,
  int                                  idx,
  bool                                 fire_events)
{
  auto mapping = std::make_unique<MidiMapping> (registry_);
  mapping->key_ = buf;
  mapping->device_id_ = device_id;
  mapping->dest_id_ = dest_port;
  mapping->enabled_.store (true);

  mappings_.insert (mappings_.begin () + idx, std::move (mapping));

  auto str = utils::midi::midi_ctrl_change_get_description (buf);
  z_info ("bounded MIDI mapping from {} to {}", str, dest_port.get ()->label ());
}

void
MidiMappings::unbind (int idx, bool fire_events)
{
  z_return_if_fail (idx >= 0 && idx < static_cast<int> (mappings_.size ()));

  mappings_.erase (mappings_.begin () + idx);
}

int
MidiMappings::get_mapping_index (const MidiMapping &mapping) const
{
  auto it = std::ranges::find_if (mappings_, [&mapping] (const auto &m) {
    return m.get () == &mapping;
  });
  return it != mappings_.end () ? std::distance (mappings_.begin (), it) : -1;
}

void
MidiMapping::apply (std::array<midi_byte_t, 3> buf)
{
  auto * dest = dest_id_->get ();
  /* if toggle, reverse value */
  if (dest->range ().type_ == dsp::ParameterRange::Type::Toggle)
    {
      dest->setBaseValue (
        dest->range ().isToggled (dest->baseValue ()) ? 0.f : 1.f);
    }
  /* else if not toggle set the control value received */
  else
    {
      float normalized_val = static_cast<float> (buf[2]) / 127.f;
      dest->setBaseValue (normalized_val);
    }
    // TODO port these from MidiPort's to parameters
#if 0
  else if (dest_->id_->type_ == dsp::PortType::Midi)
    {
      /* FIXME these are called during processing they should be queued as UI
       * events instead */
      if (ENUM_BITSET_TEST (
            dest_->id_->flags_, PortIdentifier::Flags::TransportRoll))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_ROLL_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          dest_->id_->flags_, PortIdentifier::Flags::TransportStop))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_PAUSE_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          dest_->id_->flags_, PortIdentifier::Flags::TransportBackward))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_MOVE_BACKWARD_REQUIRED,
          // nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          dest_->id_->flags_, PortIdentifier::Flags::TransportForward))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_MOVE_FORWARD_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          dest_->id_->flags_, PortIdentifier::Flags::TransportLoopToggle))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_TOGGLE_LOOP_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          dest_->id_->flags_, PortIdentifier::Flags::TransportRecToggle))
        {
          /* EVENTS_PUSH (
            EventType::ET_TRANSPORT_TOGGLE_RECORDING_REQUIRED, nullptr); */
        }
    }
#endif
}

void
MidiMappings::apply_from_cc_events (
  std::span<const dsp::RealtimeMidiEvent> events)
{
  for (const auto &ev : events)
    {
      auto d = ev.data ();
      if (d.size () < 3 || !utils::midi::midi_is_controller (d))
        continue;
      auto  channel = utils::midi::midi_get_channel_1_to_16 (d);
      auto  controller = utils::midi::midi_get_controller_number (d);
      auto &mapping = mappings_[(channel - 1) * 128 + controller];
      std::array<midi_byte_t, 3> buf = { d[0], d[1], d[2] };
      mapping->apply (buf);
    }
}

void
MidiMappings::apply (const midi_byte_t * buf)
{
  for (auto &mapping : mappings_)
    {
      if (
        mapping->enabled_.load () && mapping->key_[0] == buf[0]
        && mapping->key_[1] == buf[1])
        {
          std::array<midi_byte_t, 3> arr = { buf[0], buf[1], buf[2] };
          mapping->apply (arr);
        }
    }
}

int
MidiMappings::get_for_port (
  const dsp::ProcessorParameter &dest_port,
  std::vector<MidiMapping *> *   arr) const
{
  int count = 0;
  for (const auto &mapping : mappings_)
    {
      if (mapping->dest_id_->id () == dest_port.get_uuid ())
        {
          if (arr != nullptr)
            {
              arr->push_back (mapping.get ());
            }
          count++;
        }
    }
  return count;
}

void
to_json (nlohmann::json &j, const MidiMapping &mapping)
{
  j[MidiMapping::kKeyKey] = mapping.key_;
  j[MidiMapping::kDeviceIdKey] = mapping.device_id_;
  if (mapping.dest_id_)
    {
      j[MidiMapping::kDestIdKey] = *mapping.dest_id_;
    }
  j[MidiMapping::kEnabledKey] = mapping.enabled_.load ();
}

void
from_json (const nlohmann::json &j, MidiMapping &mapping)
{
  j.at (MidiMapping::kKeyKey).get_to (mapping.key_);
  j.at (MidiMapping::kDeviceIdKey).get_to (mapping.device_id_);
  if (j.contains (MidiMapping::kDestIdKey))
    {
      mapping.dest_id_.emplace (mapping.registry_);
      j.at (MidiMapping::kDestIdKey).get_to (*mapping.dest_id_);
    }
  mapping.enabled_.store (j.at (MidiMapping::kEnabledKey).get<bool> ());
}

void
to_json (nlohmann::json &j, const MidiMappings &mappings)
{
  j[MidiMappings::kMappingsKey] = mappings.mappings_;
}

void
from_json (const nlohmann::json &j, MidiMappings &mappings)
{
  for (const auto &mapping_json : j.at (MidiMappings::kMappingsKey))
    {
      auto mapping = std::make_unique<MidiMapping> (mappings.registry_);
      from_json (mapping_json, *mapping);
      mappings.mappings_.push_back (std::move (mapping));
    }
}
}
