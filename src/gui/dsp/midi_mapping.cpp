// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/midi_event.h"
#include "gui/dsp/midi_mapping.h"

#include "utils/midi.h"
#include "utils/rt_thread_id.h"

MidiMapping::MidiMapping (QObject * parent) : QObject (parent) { }

void
MidiMappings::init_loaded ()
{
  for (auto &mapping : mappings_)
    {
      auto dest_var = PROJECT->find_port_by_id (*mapping->dest_id_);
      z_return_if_fail (dest_var);
      std::visit ([&] (auto &&dest) { mapping->dest_ = dest; }, *dest_var);
    }
}

void
MidiMapping::init_after_cloning (const MidiMapping &other)
{
  key_ = other.key_;
  if (other.device_port_)
    device_port_ = std::make_unique<ExtPort> (*other.device_port_);
  dest_id_ = other.dest_id_->clone_unique ();
  enabled_.store (other.enabled_.load ());
}

void
MidiMappings::bind_at (
  std::array<midi_byte_t, 3> buf,
  ExtPort *                  device_port,
  Port                      &dest_port,
  int                        idx,
  bool                       fire_events)
{
  auto mapping = std::make_unique<MidiMapping> ();
  mapping->key_ = buf;
  ;
  if (device_port)
    {
      mapping->device_port_ = std::make_unique<ExtPort> (*device_port);
    }
  mapping->dest_id_ = dest_port.id_->clone_unique ();
  mapping->dest_ = &dest_port;
  mapping->enabled_.store (true);

  mappings_.insert (mappings_.begin () + idx, std::move (mapping));

  char str[100];
  midi_ctrl_change_get_ch_and_description (buf.data (), str);

  if (
    !(ENUM_BITSET_TEST (
      dsp::PortIdentifier::Flags, dest_port.id_->flags_,
      dsp::PortIdentifier::Flags::MidiAutomatable)))
    {
      z_info ("bounded MIDI mapping from {} to {}", str, dest_port.get_label ());
    }

  if (fire_events && ZRYTHM_HAVE_UI)
    {
      // EVENTS_PUSH (EventType::ET_MIDI_BINDINGS_CHANGED, nullptr);
    }
}

void
MidiMappings::unbind (int idx, bool fire_events)
{
  z_return_if_fail (idx >= 0 && idx < static_cast<int> (mappings_.size ()));

  mappings_.erase (mappings_.begin () + idx);

  if (fire_events && ZRYTHM_HAVE_UI)
    {
      // EVENTS_PUSH (EventType::ET_MIDI_BINDINGS_CHANGED, nullptr);
    }
}

int
MidiMappings::get_mapping_index (const MidiMapping &mapping) const
{
  auto it = std::find_if (
    mappings_.begin (), mappings_.end (),
    [&mapping] (const auto &m) { return m.get () == &mapping; });
  return it != mappings_.end () ? std::distance (mappings_.begin (), it) : -1;
}

void
MidiMapping::apply (std::array<midi_byte_t, 3> buf)
{
  z_return_if_fail (dest_);

  if (dest_->id_->type_ == dsp::PortType::Control)
    {
      auto * dest = dynamic_cast<ControlPort *> (dest_);
      /* if toggle, reverse value */
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, dest->id_->flags_,
          PortIdentifier::Flags::Toggle))
        {
          dest->set_toggled (!dest->is_toggled (), true);
        }
      /* else if not toggle set the control value received */
      else
        {
          float normalized_val = static_cast<float> (buf[2]) / 127.f;
          dest->set_control_value (normalized_val, true, true);
        }
    }
  else if (dest_->id_->type_ == dsp::PortType::Event)
    {
      /* FIXME these are called during processing they should be queued as UI
       * events instead */
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, dest_->id_->flags2_,
          PortIdentifier::Flags2::TransportRoll))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_ROLL_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, dest_->id_->flags2_,
          PortIdentifier::Flags2::TransportStop))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_PAUSE_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, dest_->id_->flags2_,
          PortIdentifier::Flags2::TransportBackward))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_MOVE_BACKWARD_REQUIRED,
          // nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, dest_->id_->flags2_,
          PortIdentifier::Flags2::TransportForward))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_MOVE_FORWARD_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, dest_->id_->flags2_,
          PortIdentifier::Flags2::TransportLoopToggle))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_TOGGLE_LOOP_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, dest_->id_->flags2_,
          PortIdentifier::Flags2::TransportRecToggle))
        {
          /* EVENTS_PUSH (
            EventType::ET_TRANSPORT_TOGGLE_RECORDING_REQUIRED, nullptr); */
        }
    }
}

void
MidiMappings::apply_from_cc_events (MidiEventVector &events)
{
  for (const auto &ev : events)
    {
      if (midi_is_controller (ev.raw_buffer_.data ()))
        {
          auto channel = midi_get_channel_1_to_16 (ev.raw_buffer_.data ());
          auto controller = midi_get_controller_number (ev.raw_buffer_.data ());
          auto &mapping = mappings_[(channel - 1) * 128 + controller];
          mapping->apply (ev.raw_buffer_);
        }
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
  const Port                  &dest_port,
  std::vector<MidiMapping *> * arr) const
{
  int count = 0;
  for (const auto &mapping : mappings_)
    {
      if (mapping->dest_ == &dest_port)
        {
          if (arr)
            {
              arr->push_back (mapping.get ());
            }
          count++;
        }
    }
  return count;
}
