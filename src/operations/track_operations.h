// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <expected>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "commands/add_track_command.h"
#include "commands/change_timebase_override_command.h"
#include "commands/change_track_color_command.h"
#include "commands/change_track_comment_command.h"
#include "commands/change_uuid_identifiable_object_property_command.h"
#include "commands/channel_send_commands.h"
#include "commands/rename_track_command.h"
#include "commands/route_channel_send_command.h"
#include "commands/route_track_command.h"
#include "dsp/port_fwd.h"
#include "dsp/timebase.h"
#include "structure/tracks/channel_send_routing.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "structure/tracks/tracklist.h"
#include "utils/enum_utils.h"
#include "utils/refusal.h"
#include "utils/utf8_string.h"

#include "operations/operation_context.h"
#include "operations/operation_registry.h"
#include "operations/pagination.h"
#include "operations/param_types.h"
#include <type_safe/strong_typedef.hpp>

namespace zrythm::operations
{

/**
 * Renames a track. Edit; takes the track and the new name (1 to 128
 * bytes).
 */
inline const auto &
track_set_name_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit,
    OperationId{
      "track", "set_name"
  },
    "Rename a track",
    std::array<ParamSpec, 2>{
      { { "track", "UUID of the track to rename" },
        { "new_name", "New track name (1 to 128 bytes)" } } },
    [] (OperationContext &ctx, TrackParam track, StringParam<1, 128> new_name)
      -> OperationReport {
      ctx.push_command (
        std::make_unique<commands::RenameTrackCommand> (
          *track.value (), new_name.value ().to_qstring ()));
      ctx.note_affected (track.uuid_string (), "track");
      return ctx.applied ();
    });
  return operation;
}

/**
 * Sets a track's color. Edit; takes the track and the color as a hex
 * string.
 */
inline const auto &
track_set_color_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit,
    OperationId{
      "track", "set_color"
  },
    "Set a track's color",
    std::array<ParamSpec, 2>{
      { { "track", "UUID of the track" },
        { "color", "New color as a hex string, e.g. \"#ff5588\"" } } },
    [] (OperationContext &ctx, TrackParam track, HexColorParam color)
      -> OperationReport {
      const utils::Color current (track.value ()->color ());
      if (track.value ()->useColor () && current.is_same (color.value ()))
        {
          return ctx.applied ();
        }
      ctx.push_command (
        std::make_unique<commands::ChangeTrackColorCommand> (
          *track.value (), std::optional{ color.value () }));
      ctx.note_affected (track.uuid_string (), "track");
      return ctx.applied ();
    });
  return operation;
}

/**
 * Resets a track's color to its type-derived default. Edit; takes the
 * track.
 */
inline const auto &
track_unset_color_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit, { "track", "unset_color" },
    "Reset a track's color to its type-derived default",
    std::array<ParamSpec, 1>{ { { "track", "UUID of the track" } } },
    [] (OperationContext &ctx, TrackParam track) -> OperationReport {
      if (!track.value ()->useColor ())
        {
          return ctx.applied ();
        }
      ctx.push_command (
        std::make_unique<commands::ChangeTrackColorCommand> (
          *track.value (), std::nullopt));
      ctx.note_affected (track.uuid_string (), "track");
      return ctx.applied ();
    });
  return operation;
}

/** Longest comment accepted by track.set_comment, in bytes. */
inline constexpr std::size_t kMaxTrackCommentBytes = 4096;

/**
 * Sets a track's comment. Edit; takes the track and the comment (an
 * empty comment clears it).
 */
inline const auto &
track_set_comment_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit,
    OperationId{
      "track", "set_comment"
  },
    "Set a track's comment",
    std::array<ParamSpec, 2>{
      { { "track", "UUID of the track" },
        { "comment",
          "New comment (0 to 4096 bytes); an empty string clears it" } } },
    [] (
      OperationContext &ctx, TrackParam track,
      StringParam<0, kMaxTrackCommentBytes> comment) -> OperationReport {
      if (
        utils::Utf8String::from_qstring (track.value ()->comment ())
        == comment.value ())
        {
          return ctx.applied ();
        }
      ctx.push_command (
        std::make_unique<commands::ChangeTrackCommentCommand> (
          *track.value (), comment.value ().to_qstring ()));
      ctx.note_affected (track.uuid_string (), "track");
      return ctx.applied ();
    });
  return operation;
}

/**
 * Sets a track's timebase override. Edit; takes the track and the new
 * timebase.
 */
inline const auto &
track_set_timebase_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit,
    OperationId{
      "track", "set_timebase"
  },
    "Set a track's timebase override",
    std::array<ParamSpec, 2>{
      { { "track", "UUID of the track" },
        { "timebase", "New timebase override" } } },
    [] (
      OperationContext &ctx, TrackParam track,
      EnumParam<dsp::Timebase> timebase) -> OperationReport {
      auto * provider = track.value ()->timebaseProvider ();
      if (provider->overrideValue () == timebase.value ())
        {
          return ctx.applied ();
        }
      ctx.push_command (
        std::make_unique<commands::ChangeTimebaseOverrideCommand> (
          *provider, timebase.value ()));
      ctx.note_affected (track.uuid_string (), "track");
      return ctx.applied ();
    });
  return operation;
}

/**
 * Routes a track to another track. Edit; takes the track and the
 * routing target. Refuses a target that would create a routing cycle
 * or that takes a different signal type.
 */
inline const auto &
track_route_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit,
    OperationId{
      "track", "route"
  },
    "Route a track to another track",
    std::array<ParamSpec, 2>{
      { { "track", "UUID of the track to route" },
        { "target", "UUID of the routing target" } } },
    [] (OperationContext &ctx, TrackParam track, TrackParam target)
      -> OperationReport {
      auto      &routing = ctx.environment ().track_routing ();
      const auto refusal =
        routing.route_refusal_reason (track.value (), target.value ());
      if (refusal.has_value ())
        {
          return ctx.refused (*refusal);
        }
      const auto current_target =
        routing.get_output_track (track.value ()->get_uuid ());
      if (
        current_target.has_value ()
        && current_target->id () == target.value ()->get_uuid ())
        {
          return ctx.applied ();
        }
      ctx.push_command (
        std::make_unique<commands::RouteTrackCommand> (
          routing, track.value ()->get_uuid (), target.value ()->get_uuid ()));
      ctx.note_affected (track.uuid_string (), "track");
      ctx.note_affected (target.uuid_string (), "track");
      return ctx.applied ();
    });
  return operation;
}

/**
 * Removes a track's route, leaving it unrouted. Edit; takes the
 * track.
 */
inline const auto &
track_unroute_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit, { "track", "unroute" }, "Remove a track's route",
    std::array<ParamSpec, 1>{ { { "track", "UUID of the track to unroute" } } },
    [] (OperationContext &ctx, TrackParam track) -> OperationReport {
      auto &routing = ctx.environment ().track_routing ();
      if (!routing.get_output_track (track.value ()->get_uuid ()).has_value ())
        {
          return ctx.applied ();
        }
      ctx.push_command (
        std::make_unique<commands::RouteTrackCommand> (
          routing, track.value ()->get_uuid (), std::nullopt));
      ctx.note_affected (track.uuid_string (), "track");
      return ctx.applied ();
    });
  return operation;
}

/**
 * Returns the send group of the track's channel when the send belongs
 * to it, or the refusal reason otherwise.
 */
inline std::expected<structure::tracks::ChannelSendGroup *, utils::RefusalReason>
send_group_owning (TrackParam track, SendParam send)
{
  if (track.value ()->channel () == nullptr)
    {
      return std::unexpected (
        utils::RefusalReason{
          utils::RefusalKind::Unsupported,
          utils::Utf8String::from_utf8_encoded_string (
            "this track has no channel") });
    }
  auto       &group = *track.value ()->channel ()->sends ();
  const auto &sends = group.get_sends ();
  const auto  owned = std::ranges::find (
    sends, send.value ()->get_uuid (),
    &structure::tracks::ChannelSendUuidReference::id);
  if (owned == sends.end ())
    {
      return std::unexpected (
        utils::RefusalReason{
          utils::RefusalKind::InvalidState,
          utils::Utf8String::from_utf8_encoded_string (
            "the send does not belong to this track") });
    }
  return &group;
}

/**
 * Appends a send to a track's channel. Edit; takes the track and
 * whether the send taps before the fader. Returns the new send's
 * UUID.
 */
inline const auto &
track_add_send_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit,
    OperationId{
      "track", "add_send"
  },
    "Add a send to a track's channel",
    std::array<ParamSpec, 2>{
      { { "track", "UUID of the track" },
        { "is_prefader", "Whether the send taps before the fader" } } },
    [] (OperationContext &ctx, TrackParam track, BoolParam is_prefader)
      -> OperationReport {
      if (track.value ()->channel () == nullptr)
        {
          return ctx.refused (
            utils::RefusalKind::Unsupported,
            "this track has no channel to add a send to");
        }
      auto &group = *track.value ()->channel ()->sends ();
      ctx.push_command (
        std::make_unique<commands::AddChannelSendCommand> (
          group, is_prefader.value ()));
      ctx.note_affected (track.uuid_string (), "track");
      if (ctx.failed_push_count () > 0)
        {
          // the send was not appended, so there is no UUID to report
          return ctx.applied ();
        }
      const auto send_uuid =
        uuid_to_string (type_safe::get (group.get_sends ().back ().id ()));
      ctx.note_affected (send_uuid, "send");
      return ctx.applied (
        nlohmann::json{
          { "send", send_uuid }
      });
    });
  return operation;
}

/**
 * Removes a send from a track's channel. Edit; takes the track and
 * the send; the send must belong to the track.
 */
inline const auto &
track_remove_send_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit,
    OperationId{
      "track", "remove_send"
  },
    "Remove a send from a track's channel",
    std::array<ParamSpec, 2>{
      { { "track", "UUID of the track that owns the send" },
        { "send", "UUID of the send" } } },
    [] (OperationContext &ctx, TrackParam track, SendParam send)
      -> OperationReport {
      const auto group = send_group_owning (track, send);
      if (!group.has_value ())
        {
          return ctx.refused (group.error ());
        }
      ctx.push_command (
        std::make_unique<commands::RemoveChannelSendCommand> (
          *group.value (), send.value ()->get_uuid ()));
      ctx.note_affected (track.uuid_string (), "track");
      ctx.note_affected (send.uuid_string (), "send");
      return ctx.applied ();
    });
  return operation;
}

/**
 * Moves a send's tap point before or after the channel fader. Edit;
 * takes the send and the new tap point.
 */
inline const auto &
track_set_send_prefader_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit,
    OperationId{
      "track", "set_send_prefader"
  },
    "Move a send's tap point before or after the channel fader",
    std::array<ParamSpec, 2>{
      { { "send", "UUID of the send" },
        { "is_prefader", "Whether the send taps before the fader" } } },
    [] (OperationContext &ctx, SendParam send, BoolParam is_prefader)
      -> OperationReport {
      if (send.value ()->isPrefader () == is_prefader.value ())
        {
          return ctx.applied ();
        }
      ctx.push_command (
        std::make_unique<commands::ChangeUuidIdentifiableObjectPropertyCommand> (
          *send.value (), send.value ()->registry (),
          QStringLiteral ("isPrefader"), is_prefader.value (),
          commands::Command::EngineRequirement::RecalculateGraph));
      ctx.note_affected (send.uuid_string (), "send");
      return ctx.applied ();
    });
  return operation;
}

/**
 * Points a send at a track's input. Edit; takes the track that owns
 * the send, the send and the target track. Refuses a target that
 * takes a different signal type; pointing at the destination the send
 * already has elides.
 */
inline const auto &
track_set_send_target_track_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit,
    OperationId{
      "track", "set_send_target_track"
  },
    "Point a channel send at a track",
    std::array<ParamSpec, 3>{
      { { "track", "UUID of the track that owns the send" },
        { "send", "UUID of the send" },
        { "target_track", "UUID of the destination track" } } },
    [] (
      OperationContext &ctx, TrackParam track, SendParam send,
      TrackParam target_track) -> OperationReport {
      const auto group = send_group_owning (track, send);
      if (!group.has_value ())
        {
          return ctx.refused (group.error ());
        }
      auto      &routing = ctx.environment ().channel_send_routing ();
      const auto refusal = routing.send_to_track_refusal_reason (
        *track.value (), *target_track.value ());
      if (refusal.has_value ())
        {
          return ctx.refused (*refusal);
        }
      const auto dest_port =
        routing.destination_port_for_track (*target_track.value ());
      if (!dest_port.has_value ())
        {
          return ctx.refused (
            utils::RefusalKind::Unsupported,
            "the destination track has no input port to send to");
        }
      const auto current = send.value ()->destination_port ();
      if (current.has_value () && current->id () == dest_port->id ())
        {
          return ctx.applied ();
        }
      const auto wiring_refusal =
        ctx.environment ().send_destination_refusal_reason (
          *send.value (), *dest_port->get ());
      if (wiring_refusal.has_value ())
        {
          return ctx.refused (*wiring_refusal);
        }
      ctx.push_command (
        std::make_unique<commands::RouteChannelSendCommand> (
          structure::tracks::ChannelSendUuidReference{
            send.value ()->get_uuid (), send.value ()->registry () },
          dest_port->id ()));
      ctx.note_affected (track.uuid_string (), "track");
      ctx.note_affected (send.uuid_string (), "send");
      return ctx.applied ();
    });
  return operation;
}

/**
 * Points a send at a plugin sidechain input. Edit; takes the track
 * that owns the send, the send and the target port. Pointing at the
 * destination the send already has elides.
 */
inline const auto &
track_set_send_target_port_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit,
    OperationId{
      "track", "set_send_target_port"
  },
    "Point a channel send at a plugin sidechain input",
    std::array<ParamSpec, 3>{
      { { "track", "UUID of the track that owns the send" },
        { "send", "UUID of the send" },
        { "target_port", "UUID of the destination sidechain port" } } },
    [] (
      OperationContext &ctx, TrackParam track, SendParam send,
      AudioPortParam target_port) -> OperationReport {
      const auto group = send_group_owning (track, send);
      if (!group.has_value ())
        {
          return ctx.refused (group.error ());
        }
      auto      &routing = ctx.environment ().channel_send_routing ();
      const auto refusal = routing.send_to_port_refusal_reason (
        *track.value (), *target_port.value ());
      if (refusal.has_value ())
        {
          return ctx.refused (*refusal);
        }
      const auto current = send.value ()->destination_port ();
      if (
        current.has_value ()
        && current->id () == target_port.value ()->get_uuid ())
        {
          return ctx.applied ();
        }
      const auto wiring_refusal =
        ctx.environment ().send_destination_refusal_reason (
          *send.value (), *target_port.value ());
      if (wiring_refusal.has_value ())
        {
          return ctx.refused (*wiring_refusal);
        }
      ctx.push_command (
        std::make_unique<commands::RouteChannelSendCommand> (
          structure::tracks::ChannelSendUuidReference{
            send.value ()->get_uuid (), send.value ()->registry () },
          target_port.value ()->get_uuid ()));
      ctx.note_affected (track.uuid_string (), "track");
      ctx.note_affected (send.uuid_string (), "send");
      return ctx.applied ();
    });
  return operation;
}

/**
 * Clears a send's destination. Edit; takes the track that owns the
 * send and the send. Clearing a send with no destination elides.
 */
inline const auto &
track_clear_send_target_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit,
    OperationId{
      "track", "clear_send_target"
  },
    "Clear a channel send's destination",
    std::array<ParamSpec, 2>{
      { { "track", "UUID of the track that owns the send" },
        { "send", "UUID of the send" } } },
    [] (OperationContext &ctx, TrackParam track, SendParam send)
      -> OperationReport {
      const auto group = send_group_owning (track, send);
      if (!group.has_value ())
        {
          return ctx.refused (group.error ());
        }
      if (!send.value ()->has_destination ())
        {
          return ctx.applied ();
        }
      ctx.push_command (
        std::make_unique<commands::RouteChannelSendCommand> (
          structure::tracks::ChannelSendUuidReference{
            send.value ()->get_uuid (), send.value ()->registry () },
          std::nullopt));
      ctx.note_affected (track.uuid_string (), "track");
      ctx.note_affected (send.uuid_string (), "send");
      return ctx.applied ();
    });
  return operation;
}

/**
 * Adds a new empty track. Edit; takes the track type. Returns the new
 * track's UUID and name.
 */
inline const auto &
track_add_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit, { "track", "add" }, "Add a new empty track",
    std::array<ParamSpec, 1>{ { { "type", "Type of track to add" } } },
    [] (OperationContext &ctx, EnumParam<structure::tracks::Track::Type> type)
      -> OperationReport {
      using Track = structure::tracks::Track;
      if (!Track::type_is_deletable (type.value ()))
        {
          return ctx.refused (
            utils::RefusalKind::Unsupported,
            fmt::format (
              "track type '{}' is a singleton and cannot be added",
              ENUM_NAME (type.value ())));
        }
      auto &env = ctx.environment ();
      auto  track_ref = env.track_factory ().create_empty_track (type.value ());
      track_ref.get ()->setName (
        env.track_collection ()
          .get_unique_name_for_track (
            track_ref.id (), track_ref.get ()->get_name ())
          .to_qstring ());
      const auto track_uuid_string =
        uuid_to_string (type_safe::get (track_ref.id ()));
      const auto routes_to_master =
        track_ref.get ()->output_signal_type () == dsp::PortType::Audio;
      if (routes_to_master && env.singleton_tracks ().masterTrack () == nullptr)
        {
          return ctx.refused (
            utils::RefusalKind::Unavailable,
            "this session has no master track to route to");
        }
      {
        undo::UndoStack::ScopedMacro macro (
          env.undo_stack (), QObject::tr ("Add Track"));
        ctx.push_command (
          std::make_unique<commands::AddEmptyTrackCommand> (
            env.track_collection (), track_ref));
        if (routes_to_master)
          {
            ctx.push_command (
              std::make_unique<commands::RouteTrackCommand> (
                env.track_routing (), track_ref.id (),
                env.singleton_tracks ().masterTrack ()->get_uuid ()));
          }
      }
      ctx.note_affected (track_uuid_string, "track");
      return ctx.applied (
        nlohmann::json{
          { "track", track_uuid_string },
          { "name",
           std::string (
              utils::Utf8String::from_qstring (track_ref.get ()->name ())
                .view ())              },
      });
    });
  return operation;
}

/**
 * Lists the project's tracks. Query; takes an optional page limit and
 * an opaque cursor from a previous response's next_cursor. Returns one
 * page of track summaries (UUID, name, type) in position order, with
 * the total track count, the cursor to the next page, and whether the
 * project revision moved since the cursor was minted.
 */
inline const auto &
track_list_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Query,
    OperationId{
      "track", "list"
  },
    "List the project's tracks",
    std::array<ParamSpec, 2>{
      { { "limit", "Maximum number of tracks to return" },
        { "cursor",
          "Opaque pagination cursor from a previous response's next_cursor" } } },
    [] (OperationContext &ctx, LimitParam limit, CursorParam cursor)
      -> OperationReport {
      auto       &collection = ctx.environment ().track_collection ();
      PageBuilder page (
        limit, cursor, ctx.environment ().undo_stack ().revision ());
      const auto &tracks = collection.tracks ();
      for (const auto index : std::views::iota (0uz, tracks.size ()))
        {
          const auto  &track_ref = tracks[index];
          const auto * track = track_ref.get ();
          const auto   uuid = uuid_to_string (type_safe::get (track_ref.id ()));
          page.consider (index, uuid, [&uuid, track] {
            return nlohmann::json{
              { "uuid", uuid                                                },
              { "name",
               std::string (
                  utils::Utf8String::from_qstring (track->name ()).view ()) },
              { "type", track->type ()                                      },
            };
          });
        }
      return ctx.applied (page.finish (collection.track_count ()));
    });
  return operation;
}

/** The track.* operations. */
std::vector<Operation>
make_track_operations ();

} // namespace zrythm::operations
