// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdexcept>

#include "utils/format_qt.h"

#include "actions/track_collection_operator.h"
#include "commands/add_track_command.h"
#include "commands/delete_tracks_command.h"
#include "commands/move_tracks_command.h"
#include "commands/route_track_command.h"
#include "commands/set_folder_parent_command.h"
#include "plugins/plugin_all.h"
#include "plugins/plugin_instantiation_wait.h"
#include "utils/logger.h"
#include "utils/qt.h"
#include "utils/views.h"

namespace zrythm::actions
{

TrackCollectionOperator::TrackCollectionOperator (
  undo::UndoStack                     &undo_stack,
  structure::project::ProjectRegistry &project_registry,
  controllers::Clipboard              &clipboard,
  structure::tracks::TrackCollection  &collection,
  structure::tracks::TrackRouting     &track_routing,
  structure::tracks::SingletonTracks  &singleton_tracks,
  std::function<QString ()>            project_id_provider,
  QObject *                            parent)
    : QObject (parent), collection_ (&collection), undo_stack_ (&undo_stack),
      project_registry_ (&project_registry), clipboard_ (&clipboard),
      track_routing_ (&track_routing), singleton_tracks_ (&singleton_tracks),
      project_id_provider_ (std::move (project_id_provider))
{
  QObject::connect (
    clipboard_, &controllers::Clipboard::payloadChanged, this,
    [this] () { Q_EMIT canPasteTracksChanged (); });
}

void
TrackCollectionOperator::moveTracks (
  const QList<structure::tracks::Track *> &tracks,
  int                                      targetPosition,
  structure::tracks::Track *               targetFolder)
{
  if ((collection_ == nullptr) || (undo_stack_ == nullptr) || tracks.isEmpty ())
    return;

  // Convert Track* list to TrackUuidReference vector
  auto &registry = collection_->get_registry ();
  std::vector<structure::tracks::TrackUuidReference> track_refs;
  track_refs.reserve (tracks.size ());

  for (auto * track : tracks)
    {
      if (track != nullptr)
        {
          track_refs.emplace_back (track->get_uuid (), registry);
        }
    }

  if (track_refs.empty ())
    return;

  // Expand the move list to include all descendants of any foldable tracks.
  track_refs = expand_with_descendants (track_refs);

  // Determine folder target.
  // targetPosition is a pre-removal index (the raw drop target from QML),
  // so get_enclosing_folder can be called directly on the current collection.
  std::optional<structure::tracks::Track::Uuid> target_folder_uuid;

  if (
    targetFolder != nullptr
    && structure::tracks::Track::type_is_foldable (targetFolder->type ()))
    {
      target_folder_uuid = targetFolder->get_uuid ();
    }
  else if (
    auto enclosing =
      collection_->get_enclosing_folder (static_cast<size_t> (targetPosition)))
    {
      target_folder_uuid = enclosing;
    }

  auto cmd = std::make_unique<commands::MoveTracksCommand> (
    *collection_, std::move (track_refs), targetPosition, target_folder_uuid);
  undo_stack_->push (cmd.release ());
}

void
TrackCollectionOperator::deleteTracks (
  const QList<structure::tracks::Track *> &tracks)
{
  if ((collection_ == nullptr) || (undo_stack_ == nullptr) || tracks.isEmpty ())
    return;

  auto &registry = collection_->get_registry ();
  std::vector<structure::tracks::TrackUuidReference> track_refs;
  track_refs.reserve (tracks.size ());

  for (const auto * track : tracks)
    {
      if (track == nullptr)
        continue;
      if (!structure::tracks::Track::type_is_deletable (track->type ()))
        {
          throw std::invalid_argument (
            fmt::format (
              "Cannot delete non-deletable track: {}",
              zrythm::utils::to_std_string (track->name ())));
        }
      track_refs.emplace_back (track->get_uuid (), registry);
    }

  if (track_refs.empty ())
    return;

  auto expanded_refs = expand_with_descendants (track_refs);

  auto cmd = std::make_unique<commands::DeleteTracksCommand> (
    *collection_, std::move (expanded_refs));
  undo_stack_->push (cmd.release ());
}

bool
TrackCollectionOperator::copyTracks (
  const QList<structure::tracks::Track *> &tracks)
{
  if (!clipboard_dependencies_ready ())
    {
      z_warning ("copyTracks: clipboard dependencies not set");
      return false;
    }
  if (tracks.isEmpty ())
    {
      z_debug ("copyTracks: nothing selected");
      return false;
    }

  auto copy = build_tracks_payload (tracks);
  if (!copy.has_value ())
    {
      refuse_operation (
        QObject::tr ("The tracks could not be copied to the clipboard"));
      return false;
    }

  try
    {
      clipboard_->setPayload (std::move (copy->payload));
    }
  catch (const std::exception &e)
    {
      z_warning ("Failed to store copied tracks: {}", e.what ());
      refuse_operation (
        QObject::tr ("The tracks could not be copied to the clipboard"));
      return false;
    }
  return true;
}

bool
TrackCollectionOperator::cutTracks (
  const QList<structure::tracks::Track *> &tracks)
{
  if (!clipboard_dependencies_ready ())
    {
      z_warning ("cutTracks: clipboard dependencies not set");
      return false;
    }
  if (tracks.isEmpty ())
    {
      z_debug ("cutTracks: nothing selected");
      return false;
    }

  // Copy first: if it fails, nothing is deleted
  auto copy = build_tracks_payload (tracks);
  if (!copy.has_value ())
    {
      refuse_operation (QObject::tr ("The tracks could not be cut"));
      return false;
    }

  try
    {
      clipboard_->setPayload (std::move (copy->payload));
    }
  catch (const std::exception &e)
    {
      z_warning ("Failed to store cut tracks: {}", e.what ());
      refuse_operation (QObject::tr ("The tracks could not be cut"));
      return false;
    }

  auto root_refs =
    copy->root_ids | std::views::transform ([this] (const QUuid &root_id) {
      return structure::tracks::TrackUuidReference{
        structure::tracks::Track::Uuid (root_id), *project_registry_
      };
    })
    | std::ranges::to<std::vector> ();

  undo::UndoStack::ScopedMacro macro (
    *undo_stack_, QObject::tr ("Cut %1 Tracks").arg (root_refs.size ()));
  undo_stack_->push (
    std::make_unique<commands::DeleteTracksCommand> (
      *collection_, std::move (root_refs))
      .release ());
  return true;
}

QVariantList
TrackCollectionOperator::pasteTracks (int target_position)
{
  if (!clipboard_dependencies_ready ())
    {
      z_warning ("pasteTracks: clipboard dependencies not set");
      return {};
    }

  const auto &clipboard_payload = clipboard_->payload ();
  if (!clipboard_payload.has_value ())
    {
      z_debug ("No clipboard payload to paste");
      return {};
    }
  if (
    clipboard_payload->type ()
    != structure::project::ClipboardPayload::Type::Tracks)
    {
      z_warning ("The clipboard does not contain tracks");
      refuse_operation (
        QObject::tr ("The clipboard contents could not be pasted"));
      return {};
    }

  // Captured before the paste: attaching spins a nested event loop (the
  // plugin wait), during which an OS clipboard change can reassign the
  // payload
  const QString payload_source_project_id =
    clipboard_payload->source_project_id ();

  const auto paste = import_payload_for_paste (*clipboard_payload);
  if (!paste.has_value ())
    return {};

  const auto new_ids =
    attach_paste (*paste, target_position, QObject::tr ("Paste %1 Tracks"));
  if (!new_ids.isEmpty ())
    {
      // Surface content modified while pasting (the pasted remainder is
      // intact; what is gone is gone either way)
      if (paste->dropped_audio_objects > 0 || paste->severed_references > 0)
        {
          QStringList parts;
          if (paste->dropped_audio_objects > 0)
            parts << QObject::tr (
              "%n audio item(s) were dropped", nullptr,
              static_cast<int> (paste->dropped_audio_objects));
          if (paste->severed_references > 0)
            parts << QObject::tr (
              "%n external reference(s) were severed", nullptr,
              static_cast<int> (paste->severed_references));
          // The dropped/severed targets were either copied from another
          // project or deleted here after the copy was made
          const auto cross_project =
            payload_source_project_id != project_id_provider_ ();
          const auto summary = parts.join (QStringLiteral (" · "))
                               + (cross_project
                                    ? QObject::tr (
                                      " (content from another project "
                                      "could not be resolved here)")
                                    : QObject::tr (
                                      " (content deleted after the copy "
                                      "could not be restored)"));
          z_warning ("Paste modified content: {}", summary);
          Q_EMIT pasteContentModified (summary);
        }
    }
  return new_ids;
}

QVariantList
TrackCollectionOperator::duplicateTracks (
  const QList<structure::tracks::Track *> &tracks)
{
  if (!clipboard_dependencies_ready ())
    {
      z_warning ("duplicateTracks: clipboard dependencies not set");
      return {};
    }
  if (tracks.isEmpty ())
    {
      z_debug ("duplicateTracks: nothing selected");
      return {};
    }

  // Clone the tracks directly instead of routing through the clipboard:
  // duplicating must not replace the clipboard contents
  const auto copy = build_tracks_payload (tracks);
  if (!copy.has_value ())
    {
      refuse_operation (QObject::tr ("The tracks could not be duplicated"));
      return {};
    }

  // Insert right after the last duplicated track still in the collection,
  // including the descendants a folder pulls in; if none of the sources
  // is in the collection anymore, append. A track nested under a copied
  // folder comes along with it; the duplicate keeps the copied roots'
  // folder when they all share one, and otherwise lands by its position,
  // like a drag of the same selection
  const auto payload_ids =
    copy->root_ids | std::views::transform ([] (const auto &id) {
      return structure::tracks::Track::Uuid (id);
    })
    | std::ranges::to<std::unordered_set> ();
  int                                           insert_position = -1;
  bool                                          have_shared_parent = false;
  bool                                          parents_differ = false;
  std::optional<structure::tracks::Track::Uuid> shared_parent;
  for (const auto &root_id : copy->root_ids)
    {
      const auto root_uuid = structure::tracks::Track::Uuid (root_id);
      if (!collection_->contains (root_uuid))
        continue;
      insert_position = std::max (
        insert_position,
        static_cast<int> (collection_->get_track_index (root_uuid)) + 1);
      const auto parent = collection_->get_folder_parent (root_uuid);
      if (parent.has_value () && payload_ids.contains (parent.value ()))
        continue;
      if (!have_shared_parent)
        {
          shared_parent = parent;
          have_shared_parent = true;
        }
      else if (shared_parent != parent)
        {
          parents_differ = true;
        }
    }
  const std::optional<structure::tracks::Track::Uuid> duplicate_target_folder =
    have_shared_parent && !parents_differ ? shared_parent : std::nullopt;

  const auto paste = import_payload_for_paste (copy->payload);
  if (!paste.has_value ())
    return {};

  return attach_paste (
    *paste, insert_position, QObject::tr ("Duplicate %1 Tracks"),
    duplicate_target_folder);
}

std::optional<TrackCollectionOperator::PreparedTrackCopy>
TrackCollectionOperator::build_tracks_payload (
  const QList<structure::tracks::Track *> &tracks) const
{
  std::vector<structure::tracks::TrackUuidReference> track_refs;
  for (const auto * track : tracks)
    {
      if (track == nullptr)
        continue;
      if (!track->is_copyable ())
        {
          z_warning ("Track {} is not copyable", track->get_uuid ());
          return std::nullopt;
        }
      track_refs.emplace_back (track->get_uuid (), *project_registry_);
    }
  if (track_refs.empty ())
    {
      z_warning ("No copyable tracks selected");
      return std::nullopt;
    }

  // Folders carry their descendants, which are not JSON children of the
  // folder track
  track_refs = expand_with_descendants (track_refs);

  // Canonicalize to collection order so the payload lists each folder
  // directly followed by its children even when the selection is not
  // contiguous (e.g. a folder and a later track selected together).
  // Selection sources cannot contain a track twice, but the same
  // track passed more than once would otherwise paste twice
  std::ranges::sort (track_refs, [this] (const auto &a, const auto &b) {
    return collection_->get_track_index (a.id ())
           < collection_->get_track_index (b.id ());
  });
  const auto unique_range =
    std::ranges::unique (track_refs, [] (const auto &a, const auto &b) {
      return a.id () == b.id ();
    });
  track_refs.erase (unique_range.begin (), unique_range.end ());

  const auto root_ids =
    track_refs | std::views::transform ([] (const auto &ref) {
      return type_safe::get (ref.id ());
    })
    | std::ranges::to<std::vector> ();

  const auto uuid_to_std_string = [] (const QUuid &id) {
    return id.toString (QUuid::WithoutBraces).toStdString ();
  };

  // Routing lives outside track JSON: capture each root's output target
  // as metadata so a paste can restore in-set routes and keep or default
  // out-of-set ones
  nlohmann::json routing_entries = nlohmann::json::array ();
  for (const auto &ref : track_refs)
    {
      if (auto target = track_routing_->get_output_track (ref.id ()))
        {
          routing_entries.push_back (
            {
              { std::string (
                  structure::project::ClipboardPayload::kRoutingSourceMetadataKey),
               uuid_to_std_string (type_safe::get (ref.id ()))     },
              { std::string (
                  structure::project::ClipboardPayload::kRoutingTargetMetadataKey),
               uuid_to_std_string (type_safe::get (target->id ())) }
          });
        }
    }

  // Folder nesting is collection-level state keyed by UUID: capture the
  // parent of each root whose parent is also a root
  nlohmann::json folder_parents = nlohmann::json::object ();
  for (const auto &ref : track_refs)
    {
      const auto parent = collection_->get_folder_parent (ref.id ());
      if (
        parent.has_value ()
        && std::ranges::contains (root_ids, type_safe::get (parent.value ())))
        {
          folder_parents[uuid_to_std_string (type_safe::get (ref.id ()))] =
            uuid_to_std_string (type_safe::get (parent.value ()));
        }
    }

  nlohmann::json metadata = nlohmann::json::object ();
  if (!routing_entries.empty ())
    {
      metadata[std::string (
        structure::project::ClipboardPayload::kRoutingMetadataKey)] =
        std::move (routing_entries);
    }
  if (!folder_parents.empty ())
    {
      metadata[std::string (
        structure::project::ClipboardPayload::kFolderParentsMetadataKey)] =
        std::move (folder_parents);
    }

  try
    {
      auto payload = structure::project::ClipboardPayload::create (
        *project_registry_, structure::project::ClipboardPayload::Type::Tracks,
        root_ids, project_id_provider_ (), std::move (metadata));
      return PreparedTrackCopy{ std::move (payload), std::move (root_ids) };
    }
  catch (const std::exception &e)
    {
      z_warning ("Failed to serialize the tracks: {}", e.what ());
      return std::nullopt;
    }
}

std::optional<TrackCollectionOperator::PreparedTrackPaste>
TrackCollectionOperator::import_payload_for_paste (
  const structure::project::ClipboardPayload &payload)
{
  PreparedTrackPaste prepared;
  try
    {
      auto filtered = payload.filtered_for_target (*project_registry_);
      if (!filtered.payload.has_value ())
        {
          z_warning (
            "The clipboard contents cannot be pasted into this project");
          refuse_operation (
            QObject::tr ("The clipboard contents could not be pasted"));
          return std::nullopt;
        }

      prepared.payload = structure::project::ClipboardPayload::
        with_regenerated_uuids (std::move (*filtered.payload));
      prepared.imported_ids = prepared.payload.import_into (*project_registry_);
      prepared.dropped_audio_objects = filtered.dropped_audio_objects;
      prepared.severed_references = filtered.severed_references;
    }
  catch (const std::exception &e)
    {
      z_warning (
        "Failed to prepare the clipboard contents for pasting: {}", e.what ());
      refuse_operation (
        QObject::tr ("The clipboard contents could not be pasted"));
      return std::nullopt;
    }
  return prepared;
}

QVariantList
TrackCollectionOperator::attach_paste (
  const PreparedTrackPaste                     &paste,
  int                                           target_position,
  const QString                                &label_pattern,
  std::optional<structure::tracks::Track::Uuid> explicit_target_folder)
{
  // The pasted tracks' plugins must finish instantiating before anything
  // is attached. No root references are held yet, so a failed import is
  // swept wholesale; an in-flight creation callback for a deleted plugin
  // is a no-op (guarded by the plugin's weak self-reference)
  std::vector<QUuid> plugin_ids;
  try
    {
      const auto &registry_json = paste.payload.registry_json ();
      const auto  bucket_it = registry_json.find (
        std::string (structure::project::ProjectRegistry::kPluginsKey));
      if (bucket_it != registry_json.end ())
        {
          for (const auto &entry : *bucket_it)
            {
              plugin_ids.push_back (
                QUuid::fromString (
                  QString::fromStdString (entry.at ("id").get<std::string> ())));
            }
        }
    }
  catch (const std::exception &e)
    {
      // Payloads set in-process bypass decode-time schema validation
      paste.payload.cleanup_failed_import (
        *project_registry_, paste.imported_ids);
      z_warning ("Clipboard: malformed plugin entry refused: {}", e.what ());
      refuse_operation (
        QObject::tr ("The clipboard tracks could not be pasted here"));
      return {};
    }
  // The paste works on its own imported copy of the payload, so clipboard
  // changes delivered while the wait processes events only affect future
  // pastes
  plugins::wait_for_plugin_instantiations (*project_registry_, plugin_ids);
  const auto all_instantiated =
    std::ranges::all_of (plugin_ids, [this] (const QUuid &plugin_id) {
      plugins::PluginUuidReference ref{
        plugins::Plugin::Uuid (plugin_id), *project_registry_
      };
      return ref.get () != nullptr
             && ref.get ()->instantiationStatus ()
                  == plugins::Plugin::InstantiationStatus::Successful;
    });
  if (!all_instantiated)
    {
      z_warning (
        "Cannot paste: some pasted tracks' plugins are not instantiated "
        "(failed or timed out)");
      paste.payload.cleanup_failed_import (
        *project_registry_, paste.imported_ids);
      refuse_operation (
        QObject::tr ("A plugin on the clipboard tracks failed to instantiate"));
      return {};
    }

  std::vector<structure::tracks::TrackUuidReference> root_refs;
  std::vector<QUuid>                                 root_ids;
  for (const auto &root_id : paste.payload.roots ())
    {
      structure::tracks::TrackUuidReference root_ref{
        structure::tracks::Track::Uuid (root_id), *project_registry_
      };
      if (root_ref.get () == nullptr)
        {
          z_warning ("Clipboard: imported track {} not found", root_id);
          continue;
        }
      root_refs.push_back (std::move (root_ref));
      root_ids.push_back (root_id);
    }
  if (root_refs.empty ())
    {
      paste.payload.cleanup_failed_import (
        *project_registry_, paste.imported_ids);
      z_warning ("None of the clipboard tracks could be pasted here");
      refuse_operation (
        QObject::tr ("The clipboard tracks could not be pasted here"));
      return {};
    }

  // Validate the payload's folder nesting before anything is attached:
  // each entry's parent and child must be pasted roots, the parent must
  // be foldable, and the parent must come before the child in payload
  // order. The ordering requirement also rules out parent cycles: a
  // cycle cannot satisfy parent-before-child in both directions, so the
  // links restored during the paste are acyclic by construction.
  std::vector<std::pair<QUuid, QUuid>> folder_parent_entries;
  try
    {
      const auto &metadata = paste.payload.metadata ();
      const auto  folder_parents_it = metadata.find (
        std::string (
          structure::project::ClipboardPayload::kFolderParentsMetadataKey));
      if (
        folder_parents_it != metadata.end () && folder_parents_it->is_object ())
        {
          const auto root_position =
            [&root_ids] (const QUuid &id) -> std::optional<std::size_t> {
            const auto it = std::ranges::find (root_ids, id);
            if (it == root_ids.end ())
              return std::nullopt;
            return static_cast<std::size_t> (it - root_ids.begin ());
          };
          for (
            const auto &[child_str, parent_json] : folder_parents_it->items ())
            {
              const auto child_id =
                QUuid::fromString (QString::fromStdString (child_str));
              const auto parent_id = QUuid::fromString (
                QString::fromStdString (parent_json.get<std::string> ()));
              const auto child_pos = root_position (child_id);
              const auto parent_pos = root_position (parent_id);
              const auto parent_ref =
                structure::tracks::TrackUuidReference{
                  structure::tracks::Track::Uuid (parent_id), *project_registry_
                }
                  .get ();
              if (
                !child_pos.has_value () || !parent_pos.has_value ()
                || *parent_pos >= *child_pos || parent_ref == nullptr
                || !structure::tracks::Track::type_is_foldable (
                  parent_ref->type ()))
                {
                  paste.payload.cleanup_failed_import (
                    *project_registry_, paste.imported_ids);
                  z_warning (
                    "Clipboard: invalid folder nesting {} -> {} refused",
                    child_id.toString (QUuid::WithoutBraces).toStdString (),
                    parent_id.toString (QUuid::WithoutBraces).toStdString ());
                  refuse_operation (
                    QObject::tr (
                      "The clipboard tracks contain invalid folder nesting"));
                  return {};
                }
              folder_parent_entries.emplace_back (child_id, parent_id);
            }
        }
    }
  catch (const std::exception &e)
    {
      // Payloads set in-process bypass decode-time schema validation
      paste.payload.cleanup_failed_import (
        *project_registry_, paste.imported_ids);
      z_warning ("Clipboard: malformed folder nesting refused: {}", e.what ());
      refuse_operation (
        QObject::tr ("The clipboard tracks could not be pasted here"));
      return {};
    }

  // Sweep the imports the pasted roots do not need
  paste.payload.discard_imports_except (
    *project_registry_, paste.imported_ids, root_ids);

  QVariantList new_ids;
  try
    {
      undo::UndoStack::ScopedMacro macro (
        *undo_stack_, label_pattern.arg (root_refs.size ()));

      // Append the tracks in payload order, naming each uniquely against
      // the collection (earlier pasted tracks are already in it, so
      // their names count too)
      for (const auto &root_ref : root_refs)
        {
          auto * track = root_ref.get ();
          track->setName (
            collection_
              ->get_unique_name_for_track (root_ref.id (), track->get_name ())
              .to_qstring ());
          undo_stack_->push (
            std::make_unique<commands::AddEmptyTrackCommand> (
              *collection_, root_ref)
              .release ());
          new_ids.push_back (
            type_safe::get (root_ref.id ()).toString (QUuid::WithoutBraces));
        }

      // Move the block to its target position (the adds appended to the
      // end, so a pre-paste target index is unaffected by them). A
      // position inside a folder nests the block into it, like a drag
      // would, unless the caller named the target folder explicitly
      // (duplicating keeps the source's folder). A target exactly at the
      // end of the pre-paste collection only needs the folder link: the
      // block already sits there
      const auto pasted_count = static_cast<int> (root_refs.size ());
      const auto collection_end =
        static_cast<int> (collection_->track_count ()) - pasted_count;
      std::optional<structure::tracks::Track::Uuid> target_folder =
        explicit_target_folder;
      if (!target_folder.has_value () && target_position >= 0)
        {
          target_folder = collection_->get_enclosing_folder (
            static_cast<size_t> (target_position));
        }
      if (
        (target_position >= 0 && target_position < collection_end)
        || (target_position == collection_end && target_folder.has_value ()))
        {
          undo_stack_->push (
            std::make_unique<commands::MoveTracksCommand> (
              *collection_, root_refs, target_position, target_folder)
              .release ());
        }

      // Restore in-set folder nesting. The payload lists the tracks in
      // collection order (each folder directly followed by its
      // children), so restoring is link-setting only: the children
      // already sit at their copied positions relative to their folder
      for (const auto &[child_id, parent_id] : folder_parent_entries)
        {
          undo_stack_->push (
            std::make_unique<commands::SetFolderParentCommand> (
              *collection_, structure::tracks::Track::Uuid (child_id),
              structure::tracks::Track::Uuid (parent_id))
              .release ());
        }

      // Restore output routing: in-set targets were remapped with the
      // payload, same-project out-of-set targets still resolve, anything
      // else falls back to the new-track default (audio tracks to the
      // master track, no route otherwise)
      {
        const auto &metadata = paste.payload.metadata ();
        const auto  routing_it = metadata.find (
          std::string (
            structure::project::ClipboardPayload::kRoutingMetadataKey));
        if (routing_it != metadata.end () && routing_it->is_array ())
          {
            for (const auto &entry : *routing_it)
              {
                const auto source_id = QUuid::fromString (
                  QString::fromStdString (
                    entry
                      .at (
                        std::string (
                          structure::project::ClipboardPayload::
                            kRoutingSourceMetadataKey))
                      .get<std::string> ()));
                if (!std::ranges::contains (root_ids, source_id))
                  {
                    z_warning (
                      "Clipboard: routing entry for unknown track {} skipped",
                      source_id);
                    continue;
                  }

                std::optional<structure::tracks::Track::Uuid> target_id;
                const auto target_it = entry.find (
                  std::string (
                    structure::project::ClipboardPayload::
                      kRoutingTargetMetadataKey));
                if (
                  target_it != entry.end () && !target_it->is_null ()
                  && target_it->is_string ())
                  {
                    const auto stored_target = QUuid::fromString (
                      QString::fromStdString (target_it->get<std::string> ()));
                    const auto resolves_to_track =
                      std::ranges::contains (root_ids, stored_target)
                      || structure::tracks::
                             TrackUuidReference{ structure::tracks::Track::Uuid (
                                                   stored_target),
                                                 *project_registry_ }
                               .get ()
                           != nullptr;
                    if (resolves_to_track)
                      {
                        target_id =
                          structure::tracks::Track::Uuid (stored_target);
                      }
                  }
                if (!target_id.has_value ())
                  {
                    const auto * source_track =
                      structure::tracks::TrackUuidReference{
                        structure::tracks::Track::Uuid (source_id),
                        *project_registry_
                      }
                        .get ();
                    if (
                      source_track != nullptr
                      && source_track->output_signal_type () == dsp::PortType::Audio
                      && singleton_tracks_->masterTrack () != nullptr)
                      {
                        target_id =
                          singleton_tracks_->masterTrack ()->get_uuid ();
                      }
                  }

                if (target_id.has_value ())
                  {
                    undo_stack_->push (
                      std::make_unique<commands::RouteTrackCommand> (
                        *track_routing_,
                        structure::tracks::Track::Uuid (source_id), target_id)
                        .release ());
                  }
              }
          }
      }
    }
  catch (const std::exception &e)
    {
      // The commands above are validated before execution and are not
      // expected to throw; this net keeps an unexpected exception from
      // crossing into QML. Whatever was pushed forms an undoable macro,
      // and the imported objects are referenced by the attached tracks,
      // so they are not cleaned up here
      z_warning ("Pasting the clipboard tracks failed mid-way: {}", e.what ());
      refuse_operation (
        QObject::tr ("The clipboard tracks could not be pasted here"));
      return {};
    }

  return new_ids;
}

void
TrackCollectionOperator::refuse_operation (const QString &reason)
{
  z_warning ("Refusing operation: {}", reason);
  Q_EMIT operationRefused (reason);
}

bool
TrackCollectionOperator::clipboard_dependencies_ready () const
{
  return collection_ != nullptr && undo_stack_ != nullptr
         && project_registry_ != nullptr && clipboard_ != nullptr
         && track_routing_ != nullptr && singleton_tracks_ != nullptr;
}

} // namespace zrythm::actions
