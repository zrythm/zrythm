// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "commands/add_track_command.h"
#include "commands/change_timebase_override_command.h"
#include "commands/change_track_color_command.h"
#include "commands/change_track_comment_command.h"
#include "commands/rename_track_command.h"
#include "commands/route_track_command.h"
#include "dsp/port_fwd.h"
#include "dsp/timebase.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "structure/tracks/tracklist.h"
#include "utils/enum_utils.h"
#include "utils/utf8_string.h"

#include "operations/operation_context.h"
#include "operations/operation_registry.h"
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
    OperationKind::Edit, "track.set_name", "Rename a track",
    std::array<ParamSpec, 2>{
      { { "track", "UUID of the track to rename" },
       { "new_name", "New track name (1 to 128 bytes)" } }
  },
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
    OperationKind::Edit, "track.set_color", "Set a track's color",
    std::array<ParamSpec, 2>{
      { { "track", "UUID of the track" },
       { "color", "New color as a hex string, e.g. \"#ff5588\"" } }
  },
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
    OperationKind::Edit, "track.unset_color",
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
    OperationKind::Edit, "track.set_comment", "Set a track's comment",
    std::array<ParamSpec, 2>{
      { { "track", "UUID of the track" },
       { "comment",
          "New comment (0 to 4096 bytes); an empty string clears it" } }
  },
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
    OperationKind::Edit, "track.set_timebase", "Set a track's timebase override",
    std::array<ParamSpec, 2>{
      { { "track", "UUID of the track" },
       { "timebase", "New timebase override" } }
  },
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
    OperationKind::Edit, "track.route", "Route a track to another track",
    std::array<ParamSpec, 2>{
      { { "track", "UUID of the track to route" },
       { "target", "UUID of the routing target" } }
  },
    [] (OperationContext &ctx, TrackParam track, TrackParam target)
      -> OperationReport {
      auto      &routing = ctx.environment ().track_routing ();
      const auto refusal =
        routing.route_refusal_reason (track.value (), target.value ());
      if (refusal.has_value ())
        {
          return ctx.refused (std::string (refusal->view ()));
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
    OperationKind::Edit, "track.unroute", "Remove a track's route",
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
 * Adds a new empty track. Edit; takes the track type. Returns the new
 * track's UUID and name.
 */
inline const auto &
track_add_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit, "track.add", "Add a new empty track",
    std::array<ParamSpec, 1>{ { { "type", "Type of track to add" } } },
    [] (OperationContext &ctx, EnumParam<structure::tracks::Track::Type> type)
      -> OperationReport {
      using Track = structure::tracks::Track;
      if (!Track::type_is_deletable (type.value ()))
        {
          return ctx.refused (
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
        TrackParam::uuid_to_string (type_safe::get (track_ref.id ()));
      const auto routes_to_master =
        track_ref.get ()->output_signal_type () == dsp::PortType::Audio;
      if (routes_to_master && env.singleton_tracks ().masterTrack () == nullptr)
        {
          return ctx.refused ("this session has no master track to route to");
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
 * Lists the project's tracks. Query; takes an optional limit. Returns
 * each track's UUID, name and type, and the total count.
 */
inline const auto &
track_list_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Query, "track.list", "List the project's tracks",
    std::array<ParamSpec, 1>{
      { { "limit", "Maximum number of tracks to return" } } },
    [] (OperationContext &ctx, LimitParam limit) -> OperationReport {
      auto &collection = ctx.environment ().track_collection ();
      auto  tracks = nlohmann::json::array ();
      for (const auto &track_ref : collection.tracks ())
        {
          if (tracks.size () >= static_cast<std::size_t> (limit.value ()))
            {
              break;
            }
          const auto * track = track_ref.get ();
          tracks.push_back (
            nlohmann::json{
              { "uuid",
               TrackParam::uuid_to_string (type_safe::get (track_ref.id ())) },
              { "name",
               std::string (
                  utils::Utf8String::from_qstring (track->name ()).view ())  },
              // Enums serialize as integers in this project
              { "type", ENUM_VALUE_TO_INT (track->type ())                   },
          });
        }
      return ctx.applied (
        nlohmann::json{
          { "tracks", std::move (tracks)        },
          { "total",  collection.track_count () },
      });
    });
  return operation;
}

/** The track.* operations. */
std::vector<Operation>
make_track_operations ();

} // namespace zrythm::operations
