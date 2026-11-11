// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "structure/tracks/channel_send_routing.h"

#include <type_safe/strong_typedef.hpp>

namespace zrythm::structure::tracks
{

ChannelSendRouting::ChannelSendRouting (
  TrackCollection &collection,
  QObject *        parent)
    : QObject (parent), collection_ (collection)
{
}

ChannelSendRouting::~ChannelSendRouting () = default;

// ============================================================================
// QML Interface
// ============================================================================

QVariantList
ChannelSendRouting::sendTargetsFor (Track * source) const
{
  QVariantList targets;
  if (source == nullptr || source->channel () == nullptr)
    return targets;

  const auto push_track_target = [&targets] (Track &track) {
    targets.append (
      QVariantMap{
        { QStringLiteral ("key"),
         type_safe::get (track.get_uuid ()).toString (QUuid::WithoutBraces) },
        { QStringLiteral ("display"), track.name ()                         },
        { QStringLiteral ("track"),   QVariant::fromValue (&track)          },
        { QStringLiteral ("port"),    QVariant ()                           },
    });
  };
  const auto push_plugin_target =
    [&targets] (
      const plugins::Plugin &plugin, Track &track, dsp::AudioPort &port) {
      targets.append (
        QVariantMap{
          { QStringLiteral ("key"),
           type_safe::get (port.get_uuid ()).toString (QUuid::WithoutBraces) },
          { QStringLiteral ("display"),
           QObject::tr ("%1 (sidechain) — %2")
              .arg (plugin.get_name ().to_qstring (), track.name ()) },
          // The port is the routing target; the owning track is also
          // provided for display
          { QStringLiteral ("track"), QVariant::fromValue (&track) },
          { QStringLiteral ("port"), QVariant::fromValue (&port) },
      });
    };

  for (const auto &track_ref : collection_.tracks ())
    {
      auto * track = track_ref.get ();
      if (track == nullptr || track == source)
        continue;

      if (can_send_to_track (*source, *track))
        {
          push_track_target (*track);
        }

      // Plugin sidechain inputs on the target's channel
      if (track->channel () != nullptr)
        {
          for (const auto &plugin_ref : track->channel ()->get_plugins ())
            {
              const auto * plugin = plugin_ref.get ();
              if (plugin == nullptr)
                continue;
              for (const auto &port_ref : plugin->get_all_input_ports ())
                {
                  auto * port = port_ref.get_object_as<dsp::AudioPort> ();
                  if (
                    port == nullptr || !port->is_input ()
                    || port->purpose () != dsp::AudioPort::Purpose::Sidechain)
                    continue;
                  if (can_send_to_port (*source, *port))
                    {
                      push_plugin_target (*plugin, *track, *port);
                    }
                }
            }
        }
    }

  return targets;
}

QString
ChannelSendRouting::sendDestinationKey (const ChannelSend * send) const
{
  if (send == nullptr || !send->has_destination ())
    return {};

  const auto owner = resolve_send_destination (*send);
  if (!owner.has_value ())
    return {};

  if (owner->plugin != nullptr)
    {
      return type_safe::get (send->destination_port ()->id ())
        .toString (QUuid::WithoutBraces);
    }

  return type_safe::get (owner->track->get_uuid ())
    .toString (QUuid::WithoutBraces);
}

// ============================================================================

bool
ChannelSendRouting::can_send_to_track (
  const Track &source,
  const Track &destination) const
{
  if (source.channel () == nullptr || destination.channel () == nullptr)
    return false;

  // The master channel routes to the device and cannot feed buses
  if (source.is_master ())
    return false;

  if (&source == &destination)
    return false;

  if (!Track::type_can_be_send_target (destination.type ()))
    return false;

  const auto source_type =
    source.channel ()->is_audio () ? dsp::PortType::Audio : dsp::PortType::Midi;
  const auto dest_type = destination.input_signal_type ();
  return dest_type.has_value () && *dest_type == source_type;
}

bool
ChannelSendRouting::can_send_to_port (
  const Track          &source,
  const dsp::AudioPort &destination) const
{
  if (source.channel () == nullptr || !destination.is_input ())
    return false;

  // The master channel routes to the device and cannot source sends
  if (source.is_master ())
    return false;

  if (destination.purpose () != dsp::AudioPort::Purpose::Sidechain)
    return false;

  // Sidechain ports are audio, so the source must be an audio channel
  if (!source.channel ()->is_audio ())
    return false;

  const auto owner = resolve_port_owner (destination.get_uuid ());
  if (!owner.has_value () || owner->plugin == nullptr)
    return false;

  return owner->track != &source;
}

std::optional<dsp::PortUuidReference>
ChannelSendRouting::destination_port_for_track (const Track &destination) const
{
  const auto * processor = destination.get_track_processor ();
  if (processor == nullptr)
    return std::nullopt;

  dsp::Port * port = nullptr;
  if (processor->is_audio ())
    port = &processor->get_stereo_in_port ();
  else if (processor->is_midi ())
    port = &processor->get_midi_in_port ();
  if (port == nullptr)
    return std::nullopt;

  return dsp::PortUuidReference{
    port->get_uuid (), collection_.get_registry ()
  };
}

std::optional<ChannelSendRouting::SendDestinationOwner>
ChannelSendRouting::resolve_port_owner (const dsp::PortUuid &port_id) const
{
  const auto track_for_port_id =
    [] (const auto &port_refs, const dsp::PortUuid &id) {
      return std::ranges::any_of (port_refs, [&id] (const auto &port_ref) {
        return port_ref.id () == id;
      });
    };

  for (const auto &track_ref : collection_.tracks ())
    {
      auto * track = track_ref.get ();
      if (track == nullptr)
        continue;

      const auto * processor = track->get_track_processor ();
      if (
        processor != nullptr
        && track_for_port_id (processor->get_all_input_ports (), port_id))
        {
          return SendDestinationOwner{ track, nullptr };
        }

      if (track->channel () == nullptr)
        continue;

      for (const auto &plugin_ref : track->channel ()->get_plugins ())
        {
          auto * plugin = plugin_ref.get ();
          if (plugin == nullptr)
            continue;
          if (track_for_port_id (plugin->get_all_input_ports (), port_id))
            {
              return SendDestinationOwner{ track, plugin };
            }
        }
    }

  return std::nullopt;
}

std::optional<ChannelSendRouting::SendDestinationOwner>
ChannelSendRouting::resolve_send_destination (const ChannelSend &send) const
{
  if (!send.has_destination ())
    return std::nullopt;

  if (send.destination_port ()->get () == nullptr)
    return std::nullopt;

  return resolve_port_owner (send.destination_port ()->id ());
}

} // namespace zrythm::structure::tracks
