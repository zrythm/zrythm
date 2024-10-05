// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/project.h"

#include "common/dsp/tracklist.h"

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
UndoManager::define_fields (const Context &ctx)
{
  using T = ISerializable<UndoManager>;
  T::serialize_fields (
    ctx, T::make_field ("undoStack", undo_stack_),
    T::make_field ("redoStack", redo_stack_));
}

void
Tracklist::define_fields (const Context &ctx)
{
  using T = ISerializable<Tracklist>;
  T::serialize_fields (
    ctx, T::make_field ("pinnedTracksCutoff", pinned_tracks_cutoff_));

  if (ctx.is_serializing ())
    {
      T::serialize_field<decltype (tracks_), TrackPtrVariant> (
        "tracks", tracks_, ctx);
    }
  else
    {
      yyjson_obj_iter it = yyjson_obj_iter_with (ctx.obj_);

      yyjson_val * arr = yyjson_obj_iter_get (&it, "tracks");
      size_t       arr_size = yyjson_arr_size (arr);
      tracks_.clear ();
      for (size_t i = 0; i < arr_size; ++i)
        {
          yyjson_val * val = yyjson_arr_get (arr, i);
          if (yyjson_is_obj (val))
            {
              auto elem_it = yyjson_obj_iter_with (val);
              auto type_id_int = yyjson_obj_iter_get (&elem_it, "type");
              if (yyjson_is_int (type_id_int))
                {
                  try
                    {
                      auto type_id = ENUM_INT_TO_VALUE (
                        Track::Type, yyjson_get_int (type_id_int));
                      auto track = Track::create_unique_from_type (type_id);
                      std::visit (
                        [&] (auto &&t) {
                          t->ISerializable<base_type<decltype (t)>>::deserialize (
                            Context (val, ctx));
                        },
                        convert_to_variant<TrackPtrVariant> (track.get ()));
                      tracks_.push_back (std::move (track));
                    }
                  catch (std::runtime_error &e)
                    {
                      throw ZrythmException (
                        "Invalid type id: "
                        + std::to_string (yyjson_get_uint (type_id_int)));
                    }
                }
            }
        }
    }
}

void
Project::define_fields (const Context &ctx)
{
  using T = ISerializable<Project>;
  T::serialize_fields (
    ctx, T::make_field ("title", title_),
    T::make_field ("datetime", datetime_str_),
    T::make_field ("version", version_),
    T::make_field ("clipEditor", clip_editor_),
    T::make_field ("timeline", timeline_),
    T::make_field ("snapGridTimeline", snap_grid_timeline_),
    T::make_field ("snapGridEditor", snap_grid_editor_),
    T::make_field ("quantizeOptsTimeline", quantize_opts_timeline_),
    T::make_field ("quantizeOptsEditor", quantize_opts_editor_),
    T::make_field ("audioEngine", audio_engine_),
    T::make_field ("tracklist", tracklist_),
    T::make_field ("mixerSelections", mixer_selections_, true),
    T::make_field ("timelineSelections", timeline_selections_, true),
    T::make_field ("midiArrangerSelections", midi_selections_, true),
    T::make_field ("chordSelections", chord_selections_, true),
    T::make_field ("automationSelections", automation_selections_, true),
    T::make_field ("audioSelections", audio_selections_, true),
    T::make_field ("tracklistSelections", tracklist_selections_, true),
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