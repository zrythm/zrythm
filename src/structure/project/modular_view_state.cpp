// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/modular_view_state.h"
#include "utils/logger.h"
#include "utils/registry_utils.h"
#include "utils/utf8_string.h"

#include <QUuid>

namespace zrythm::structure::project
{

namespace
{

/** Length of the "track:" ID prefix of @p key, or -1 when the key does not
 * carry it. */
qsizetype
track_id_prefix_length (const QString &key)
{
  if (key.startsWith (u8"track:"))
    return static_cast<qsizetype> (std::u8string_view (u8"track:").size ());
  return -1;
}

} // namespace

ModularViewState::ModularViewState (
  utils::IObjectRegistry &registry,
  QObject *               parent)
    : QObject (parent), registry_ (registry)
{
}

void
ModularViewState::set_node_position (
  const QString &node_id,
  const QPointF &position)
{
  node_positions_.insert_or_assign (node_id, position);
}

std::optional<QPointF>
ModularViewState::find_node_position (const QString &node_id) const
{
  const auto it = node_positions_.find (node_id);
  if (it == node_positions_.end ())
    return std::nullopt;
  return it->second;
}

void
ModularViewState::clear_node_position (const QString &node_id)
{
  node_positions_.erase (node_id);
}

void
ModularViewState::set_viewport (const QString &key, const Viewport &viewport)
{
  viewports_.insert_or_assign (key, viewport);
}

std::optional<ModularViewState::Viewport>
ModularViewState::find_viewport (const QString &key) const
{
  const auto it = viewports_.find (key);
  if (it == viewports_.end ())
    return std::nullopt;
  return it->second;
}

ModularViewState::Scope
ModularViewState::scope () const
{
  return scope_;
}

void
ModularViewState::set_scope (Scope scope)
{
  scope_ = scope;
}

std::optional<ModularViewState::TrackUuid>
ModularViewState::selected_track () const
{
  return selected_track_;
}

void
ModularViewState::set_selected_track (std::optional<TrackUuid> track)
{
  selected_track_ = track;
}

bool
ModularViewState::key_references_removed_track (const QString &key) const
{
  const qsizetype prefix_length = track_id_prefix_length (key);
  if (prefix_length < 0)
    return false;
  const QUuid uuid (QUuid::fromString (key.sliced (prefix_length)));
  return uuid.isNull () || !registry_.contains (uuid);
}

void
to_json (nlohmann::json &j, const ModularViewState &state)
{
  // nlohmann does not store a const char* as a string, so the scope
  // identifier is wrapped in std::string.
  j = nlohmann::json{
    { ModularViewState::kScopeKey,
     std::string (
        state.scope_ == ModularViewState::Scope::Global
          ? ModularViewState::kScopeGlobalString
          : ModularViewState::kScopeTrackString) },
  };

  nlohmann::json positions_json = nlohmann::json::array ();
  for (const auto &[node_id, position] : state.node_positions_)
    {
      if (state.key_references_removed_track (node_id))
        {
          z_debug (
            "pruning Modular node position of removed track: {}",
            utils::Utf8String::from_qstring (node_id));
          continue;
        }
      positions_json.push_back (
        nlohmann::json::array (
          {
            node_id,
            nlohmann::json{
                           { "x", position.x () },
                           { "y", position.y () },
                           }
      }));
    }
  if (!positions_json.empty ())
    j[ModularViewState::kNodePositionsKey] = positions_json;

  nlohmann::json viewports_json = nlohmann::json::array ();
  for (const auto &[key, viewport] : state.viewports_)
    {
      if (state.key_references_removed_track (key))
        {
          z_debug (
            "pruning Modular viewport of removed track: {}",
            utils::Utf8String::from_qstring (key));
          continue;
        }
      viewports_json.push_back (
        nlohmann::json::array (
          {
            key,
            nlohmann::json{
                           { "panX", viewport.pan_x },
                           { "panY", viewport.pan_y },
                           { "zoom", viewport.zoom },
                           }
      }));
    }
  if (!viewports_json.empty ())
    j[ModularViewState::kViewportsKey] = viewports_json;

  if (state.selected_track_.has_value ())
    {
      if (utils::contains (state.registry_, *state.selected_track_))
        j[ModularViewState::kSelectedTrackKey] = *state.selected_track_;
      else
        z_debug ("pruning Modular selected track: {}", *state.selected_track_);
    }
}

void
from_json (const nlohmann::json &j, ModularViewState &state)
{
  if (j.contains (ModularViewState::kNodePositionsKey))
    {
      state.node_positions_.clear ();
      for (const auto &entry : j.at (ModularViewState::kNodePositionsKey))
        {
          if (
            !entry.is_array () || entry.size () < 2 || !entry[0].is_string ()
            || !entry[1].is_object () || !entry[1].contains ("x")
            || !entry[1].contains ("y") || !entry[1]["x"].is_number ()
            || !entry[1]["y"].is_number ())
            {
              z_warning ("Skipping malformed Modular node position entry");
              continue;
            }
          QString node_id;
          entry[0].get_to (node_id);
          state.node_positions_.insert_or_assign (
            node_id,
            QPointF (entry[1]["x"].get<double> (), entry[1]["y"].get<double> ()));
        }
    }
  if (j.contains (ModularViewState::kViewportsKey))
    {
      state.viewports_.clear ();
      for (const auto &entry : j.at (ModularViewState::kViewportsKey))
        {
          if (
            !entry.is_array () || entry.size () < 2 || !entry[0].is_string ()
            || !entry[1].is_object () || !entry[1].contains ("panX")
            || !entry[1].contains ("panY") || !entry[1].contains ("zoom")
            || !entry[1]["panX"].is_number () || !entry[1]["panY"].is_number ()
            || !entry[1]["zoom"].is_number ()
            || entry[1]["zoom"].get<double> () <= 0.0)
            {
              z_warning ("Skipping malformed Modular viewport entry");
              continue;
            }
          QString key;
          entry[0].get_to (key);
          ModularViewState::Viewport viewport;
          viewport.pan_x = entry[1]["panX"].get<double> ();
          viewport.pan_y = entry[1]["panY"].get<double> ();
          viewport.zoom = entry[1]["zoom"].get<double> ();
          state.viewports_.insert_or_assign (key, viewport);
        }
    }
  if (j.contains (ModularViewState::kScopeKey))
    {
      if (j.at (ModularViewState::kScopeKey).is_string ())
        {
          const auto scope =
            j.at (ModularViewState::kScopeKey).get<std::string> ();
          if (scope == ModularViewState::kScopeGlobalString)
            state.scope_ = ModularViewState::Scope::Global;
          else if (scope == ModularViewState::kScopeTrackString)
            state.scope_ = ModularViewState::Scope::Track;
          else
            z_warning ("Ignoring unknown Modular scope: {}", scope);
        }
      else
        {
          z_warning ("Ignoring non-string Modular scope");
        }
    }
  if (j.contains (ModularViewState::kSelectedTrackKey))
    {
      if (j.at (ModularViewState::kSelectedTrackKey).is_string ())
        {
          ModularViewState::TrackUuid track;
          j.at (ModularViewState::kSelectedTrackKey).get_to (track);
          state.selected_track_ = track;
        }
      else
        {
          z_warning ("Ignoring non-string Modular selected track");
        }
    }
}

} // namespace zrythm::structure::project
