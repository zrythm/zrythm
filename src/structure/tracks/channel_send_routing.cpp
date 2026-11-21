// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "utils/format_qt.h"

#include "structure/tracks/channel_send_routing.h"
#include "utils/utf8_string.h"

#include <magic_enum/magic_enum.hpp>
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
  return !send_to_track_refusal_reason (source, destination).has_value ();
}

std::optional<utils::RefusalReason>
ChannelSendRouting::send_to_track_refusal_reason (
  const Track &source,
  const Track &destination) const
{
  // Reasons are formatted on the refusal branches only, so allowed
  // pairings stay free of string conversions
  const auto track_name = [] (const Track &track) {
    return utils::Utf8String::from_qstring (track.name ());
  };

  if (source.channel () == nullptr)
    return utils::RefusalReason{
      utils::RefusalKind::Unsupported,
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("'{}' has no channel to send from", track_name (source)))
    };

  if (destination.channel () == nullptr)
    return utils::RefusalReason{
      utils::RefusalKind::Unsupported,
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format (
          "'{}' has no channel to receive sends", track_name (destination)))
    };

  // The master channel routes to the device and cannot source sends
  if (source.is_master ())
    return utils::RefusalReason{
      utils::RefusalKind::Unsupported,
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format (
          "'{}' is the master track and routes to the audio device",
          track_name (source)))
    };

  if (&source == &destination)
    return utils::RefusalReason{
      utils::RefusalKind::InvalidParameter,
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("'{}' cannot send to itself", track_name (source)))
    };

  if (!Track::type_can_be_send_target (destination.type ()))
    return utils::RefusalReason{
      utils::RefusalKind::Unsupported,
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("'{}' cannot be a send target", track_name (destination)))
    };

  const auto source_type =
    source.channel ()->is_audio () ? dsp::PortType::Audio : dsp::PortType::Midi;
  const auto dest_type = destination.input_signal_type ();
  if (!dest_type.has_value () || *dest_type != source_type)
    {
      return utils::RefusalReason{
        utils::RefusalKind::TypeMismatch,
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format (
            "'{}' outputs {} but '{}' takes {}", track_name (source),
            std::string (magic_enum::enum_name (source_type)),
            track_name (destination),
            dest_type
              .transform ([] (auto type) {
                return std::string (magic_enum::enum_name (type));
              })
              .value_or ("no signal")))
      };
    }

  return std::nullopt;
}

bool
ChannelSendRouting::can_send_to_port (
  const Track          &source,
  const dsp::AudioPort &destination) const
{
  return !send_to_port_refusal_reason (source, destination).has_value ();
}

std::optional<utils::RefusalReason>
ChannelSendRouting::send_to_port_refusal_reason (
  const Track          &source,
  const dsp::AudioPort &destination) const
{
  const auto track_name = [] (const Track &track) {
    return utils::Utf8String::from_qstring (track.name ());
  };

  if (source.channel () == nullptr)
    return utils::RefusalReason{
      utils::RefusalKind::Unsupported,
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("'{}' has no channel to send from", track_name (source)))
    };

  if (!destination.is_input ())
    return utils::RefusalReason{
      utils::RefusalKind::InvalidParameter,
      utils::Utf8String{ u8"the destination port is not an input port" }
    };

  // The master channel routes to the device and cannot source sends
  if (source.is_master ())
    return utils::RefusalReason{
      utils::RefusalKind::Unsupported,
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format (
          "'{}' is the master track and routes to the audio device",
          track_name (source)))
    };

  if (destination.purpose () != dsp::AudioPort::Purpose::Sidechain)
    return utils::RefusalReason{
      utils::RefusalKind::Unsupported,
      utils::Utf8String{ u8"the destination port is not a sidechain input" }
    };

  // Sidechain ports are audio, so the source must be an audio channel
  if (!source.channel ()->is_audio ())
    return utils::RefusalReason{
      utils::RefusalKind::Unsupported,
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format (
          "'{}' has no audio channel to send to a sidechain from",
          track_name (source)))
    };

  const auto owner = resolve_port_owner (destination.get_uuid ());
  if (!owner.has_value () || owner->plugin == nullptr)
    return utils::RefusalReason{
      utils::RefusalKind::Unsupported,
      utils::Utf8String{ u8"the destination port is not a plugin sidechain input" }
    };

  if (owner->track == &source)
    return utils::RefusalReason{
      utils::RefusalKind::CyclicRoute,
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("'{}' cannot send to its own plugins", track_name (source)))
    };

  return std::nullopt;
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
