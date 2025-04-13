// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/dsp/tracklist.h"

using namespace zrythm;

void
gui::backend::Tool::define_fields (const Context &ctx)
{
  using T = ISerializable<Tool>;
  T::serialize_fields (ctx, T::make_field ("toolValue", tool_));
}

void
RegionLinkGroup::define_fields (const Context &ctx)
{
  using T = ISerializable<RegionLinkGroup>;
  T::serialize_fields (
    ctx, T::make_field ("ids", ids_), T::make_field ("groupIdx", group_idx_));
}

void
RegionLinkGroupManager::define_fields (const Context &ctx)
{
  using T = ISerializable<RegionLinkGroupManager>;
  T::serialize_fields (ctx, T::make_field ("groups", groups_));
}

void
MidiMapping::define_fields (const Context &ctx)
{
  using T = ISerializable<MidiMapping>;
  T::serialize_fields (
    ctx, T::make_field ("key", key_),
    T::make_field ("devicePort", device_port_, true),
    T::make_field ("destId", dest_id_), T::make_field ("enabled", enabled_));
}

void
MidiMappings::define_fields (const Context &ctx)
{
  using T = ISerializable<MidiMappings>;
  T::serialize_fields (ctx, T::make_field ("mappings", mappings_));
}

void
gui::actions::UndoManager::define_fields (const Context &ctx)
{
  using T = ISerializable<UndoManager>;
  T::serialize_fields (
    ctx, T::make_field ("undoStack", undo_stack_),
    T::make_field ("redoStack", redo_stack_));
}

void
Project::define_fields (const Context &ctx)
{
  using T = ISerializable<Project>;
  Context new_ctx = ctx;

  // first serialize/deserialize the registries
  T::serialize_fields (
    new_ctx, T::make_field ("portRegistry", port_registry_),
    T::make_field ("pluginRegistry", plugin_registry_),
    T::make_field ("trackRegistry", track_registry_));

  // if deserializing, add the registries to the deserialization context
  if (ctx.is_deserializing ())
    {
      new_ctx.add_dependency (std::ref (get_port_registry ()));
      new_ctx.add_dependency (std::ref (get_track_registry ()));
      new_ctx.add_dependency (std::ref (get_plugin_registry ()));
    }

  // serialize/deserialize the rest
  T::serialize_fields (
    new_ctx, T::make_field ("title", title_),
    T::make_field ("datetime", datetime_str_),
    T::make_field ("version", version_),
    T::make_field ("clipEditor", clip_editor_),
    T::make_field ("timeline", timeline_),
    T::make_field ("snapGridTimeline", snap_grid_timeline_),
    T::make_field ("snapGridEditor", snap_grid_editor_),
    T::make_field ("quantizeOptsTimeline", quantize_opts_timeline_),
    T::make_field ("quantizeOptsEditor", quantize_opts_editor_),
    T::make_field ("transport", transport_),
    T::make_field ("audioEngine", audio_engine_),
    T::make_field ("tracklist", tracklist_),
    T::make_field ("regionLinkGroupManager", region_link_group_manager_),
    T::make_field ("portConnectionsManager", port_connections_manager_),
    T::make_field ("midiMappings", midi_mappings_),
    T::make_field ("undoManager", undo_manager_, true),
    T::make_field ("lastSelection", last_selection_));

// TODO:
/* skip undo history from versions previous to 1.10 because of a change in how
 * stretching changes are stored */
#if 0
  if (
    (self->format_major == 1 && self->format_minor >= 10)
    || self->format_major > 1)
    {
      if (undo_manager_obj)
        {
          self->undo_manager = object_new (UndoManager);
          undo_manager_deserialize_from_json (
            doc, undo_manager_obj, self->undo_manager, error);
        }
    }
#endif
}
