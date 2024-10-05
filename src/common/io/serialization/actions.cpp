// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/actions/arranger_selections.h"
#include "gui/cpp/backend/actions/channel_send_action.h"
#include "gui/cpp/backend/actions/chord_action.h"
#include "gui/cpp/backend/actions/midi_mapping_action.h"
#include "gui/cpp/backend/actions/mixer_selections_action.h"
#include "gui/cpp/backend/actions/port_action.h"
#include "gui/cpp/backend/actions/port_connection_action.h"
#include "gui/cpp/backend/actions/range_action.h"
#include "gui/cpp/backend/actions/tracklist_selections.h"
#include "gui/cpp/backend/actions/transport_action.h"
#include "gui/cpp/backend/actions/undo_stack.h"

void
UndoStack::define_fields (const Context &ctx)
{
  using T = ISerializable<UndoStack>;
  T::serialize_fields (ctx, T::make_field ("maxLength", max_size_));

  if (ctx.is_serializing ())
    {
      T::serialize_field<decltype (actions_), UndoableActionPtrVariant> (
        "actions", actions_, ctx);
    }
  else
    {
      yyjson_obj_iter it = yyjson_obj_iter_with (ctx.obj_);

      yyjson_val * arr = yyjson_obj_iter_get (&it, "actions");
      size_t       arr_size = yyjson_arr_size (arr);
      actions_.clear ();
      for (size_t i = 0; i < arr_size; ++i)
        {
          yyjson_val * val = yyjson_arr_get (arr, i);
          if (yyjson_is_obj (val))
            {
              auto elem_it = yyjson_obj_iter_with (val);
              auto type_id_int =
                yyjson_obj_iter_get (&elem_it, "undoableActionType");
              if (yyjson_is_int (type_id_int))
                {
                  try
                    {
                      auto type_id = ENUM_INT_TO_VALUE (
                        UndoableAction::Type, yyjson_get_int (type_id_int));
                      auto action =
                        UndoableAction::create_unique_from_type (type_id);
                      std::visit (
                        [&] (auto &&_action) {
                          _action->ISerializable<base_type<decltype (_action)>>::
                            deserialize (Context (val, ctx));
                        },
                        convert_to_variant<UndoableActionPtrVariant> (
                          action.get ()));
                      actions_.push_back (std::move (action));
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
UndoableAction::define_base_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("undoableActionType", undoable_action_type_),
    make_field ("framesPerTick", frames_per_tick_),
    make_field ("sampleRate", sample_rate_),
    make_field ("numActions", num_actions_),
    make_field ("portConnectionsBefore", port_connections_before_, true),
    make_field ("portConnectionsAfter", port_connections_after_, true));
}

void
ArrangerSelectionsAction::define_fields (const Context &ctx)
{
  UndoableAction::define_base_fields (ctx);

  using T = ISerializable<ArrangerSelectionsAction>;
  T::serialize_fields (
    ctx, T::make_field ("type", type_), T::make_field ("editType", edit_type_),
    T::make_field ("resizeType", resize_type_), T::make_field ("ticks", ticks_),
    T::make_field ("deltaTracks", delta_tracks_),
    T::make_field ("deltaLanes", delta_lanes_),
    T::make_field ("deltaChords", delta_chords_),
    T::make_field ("deltaPitch", delta_pitch_),
    T::make_field ("deltaVel", delta_vel_),
    T::make_field ("deltaNormalizedAmount", delta_normalized_amount_),
    T::make_field ("targetPort", target_port_, true),
    T::make_field ("string", str_, true), T::make_field ("position", pos_),
    T::make_field ("quantizeOptions", opts_, true));

  if (ctx.is_serializing ())
    {
      T::serialize_field<decltype (sel_), ArrangerSelectionsPtrVariant> (
        "selections", sel_, ctx, true);
      T::serialize_field<decltype (sel_after_), ArrangerSelectionsPtrVariant> (
        "selectionsAfter", sel_after_, ctx, true);
      T::serialize_field<decltype (region_before_), RegionPtrVariant> (
        "regionBefore", region_before_, ctx, true);
      T::serialize_field<decltype (region_after_), RegionPtrVariant> (
        "regionAfter", region_after_, ctx, true);
      T::serialize_field<decltype (r1_), LengthableObjectPtrVariant> (
        "r1", r1_, ctx, true);
      T::serialize_field<decltype (r2_), LengthableObjectPtrVariant> (
        "r2", r2_, ctx, true);
    }
  else
    {
      yyjson_obj_iter it = yyjson_obj_iter_with (ctx.obj_);

      auto handle_selections = [&] (yyjson_val * sel_obj, auto &sel) {
        if (sel_obj == nullptr || yyjson_is_null (sel_obj))
          {
            sel = nullptr;
            return;
          }

        auto sel_obj_it = yyjson_obj_iter_with (sel_obj);
        yyjson_val * sel_type_id_int = yyjson_obj_iter_get (&sel_obj_it, "type");
        if (yyjson_is_int (sel_type_id_int))
          {
            try
              {
                auto sel_type_id = ENUM_INT_TO_VALUE (
                  ArrangerSelections::Type, yyjson_get_int (sel_type_id_int));
                auto new_sel = ArrangerSelections::new_from_type (sel_type_id);
                std::visit (
                  [&] (auto &&new_sel_casted) {
                    new_sel_casted
                      ->ISerializable<base_type<decltype (new_sel_casted)>>::
                        deserialize (Context (sel_obj, ctx));
                  },
                  convert_to_variant<ArrangerSelectionsPtrVariant> (
                    new_sel.get ()));
                sel = std::move (new_sel);
              }
            catch (std::runtime_error &e)
              {
                throw ZrythmException (
                  "Invalid type id: "
                  + std::to_string (yyjson_get_uint (sel_type_id_int)));
              }
          }
      };

      auto obj = yyjson_obj_iter_get (&it, "selections");
      handle_selections (obj, sel_);
      obj = yyjson_obj_iter_get (&it, "selectionsAfter");
      handle_selections (obj, sel_after_);

      auto create_region =
        [&] (yyjson_val * region_obj) -> std::unique_ptr<Region> {
        if (region_obj == nullptr || yyjson_is_null (region_obj))
          {
            return nullptr;
          }

        auto r_obj_it = yyjson_obj_iter_with (region_obj);
        yyjson_val * region_id_obj = yyjson_obj_iter_get (&r_obj_it, "regionId");
        RegionIdentifier region_id;
        region_id.ISerializable<RegionIdentifier>::deserialize (
          Context (region_id_obj, ctx));
        std::unique_ptr<Region> region;
        switch (region_id.type_)
          {
          case RegionType::Audio:
            region = std::make_unique<AudioRegion> ();
            break;
          case RegionType::Midi:
            region = std::make_unique<MidiRegion> ();
            break;
          case RegionType::Automation:
            region = std::make_unique<AutomationRegion> ();
            break;
          case RegionType::Chord:
            region = std::make_unique<ChordRegion> ();
          }
        std::visit (
          [&] (auto &&region_casted) {
            region_casted->ISerializable<base_type<decltype (region_casted)>>::
              deserialize (Context (region_obj, ctx));
          },
          convert_to_variant<RegionPtrVariant> (region.get ()));
        return region;
      };

      obj = yyjson_obj_iter_get (&it, "regionBefore");
      region_before_ = create_region (obj);
      obj = yyjson_obj_iter_get (&it, "regionAfter");
      region_after_ = create_region (obj);

      auto create_lengthable_obj =
        [&] (yyjson_val * lo_obj) -> std::unique_ptr<LengthableObject> {
        if (lo_obj == nullptr || yyjson_is_null (lo_obj))
          {
            return nullptr;
          }

        auto         lo_obj_it = yyjson_obj_iter_with (lo_obj);
        yyjson_val * lo_type_id_int = yyjson_obj_iter_get (&lo_obj_it, "type");
        if (yyjson_is_int (lo_type_id_int))
          {
            try
              {
                auto lo_type_id = ENUM_INT_TO_VALUE (
                  ArrangerObject::Type, yyjson_get_int (lo_type_id_int));
                switch (lo_type_id)
                  {
                  case ArrangerObject::Type::MidiNote:
                    {
                      auto lo_obj_ptr = std::make_unique<MidiNote> ();
                      lo_obj_ptr->ISerializable<MidiNote>::deserialize (
                        Context (lo_obj, ctx));
                      return lo_obj_ptr;
                    }
                  case ArrangerObject::Type::Region:
                    return create_region (lo_obj);
                  default:
                    throw ZrythmException (
                      "Invalid type id: "
                      + std::to_string (yyjson_get_uint (lo_type_id_int)));
                  }
              }
            catch (std::runtime_error &e)
              {
                throw ZrythmException (
                  "Invalid type id: "
                  + std::to_string (yyjson_get_uint (lo_type_id_int)));
              }
          }
        else
          {
            throw ZrythmException (
              "Invalid type id: "
              + std::to_string (yyjson_get_uint (lo_type_id_int)));
          }
      };

      auto handle_lo_obj_array = [&] (const auto &key, auto &lo_objs) {
        yyjson_val * arr = yyjson_obj_iter_get (&it, key);
        if (!arr)
          {
            throw ZrythmException ("No LengthableObject array");
          }
        yyjson_arr_iter lo_arr_it = yyjson_arr_iter_with (arr);
        yyjson_val *    lo_obj = nullptr;
        auto            size = yyjson_arr_size (arr);
        for (size_t i = 0; i < size; ++i)
          {
            lo_obj = yyjson_arr_iter_next (&lo_arr_it);
            lo_objs.emplace_back (create_lengthable_obj (lo_obj));
          }
      };

      handle_lo_obj_array ("r1", r1_);
      handle_lo_obj_array ("r2", r2_);

      num_split_objs_ = r1_.size ();
    }
}

void
MixerSelectionsAction::define_fields (const Context &ctx)
{
  UndoableAction::define_base_fields (ctx);

  using T = ISerializable<MixerSelectionsAction>;
  T::serialize_fields (
    ctx, T::make_field ("type", mixer_selections_action_type_),
    T::make_field ("slotType", slot_type_), T::make_field ("toSlot", to_slot_),
    T::make_field ("toTrackNameHash", to_track_name_hash_),
    T::make_field ("newChannel", new_channel_),
    T::make_field ("numPlugins", num_plugins_),
    T::make_field ("newVal", new_val_),
    T::make_field ("newBridgeMode", new_bridge_mode_),
    T::make_field ("setting", setting_, true),
    T::make_field ("mixerSelectionsBefore", ms_before_, true),
    T::make_field ("mixerSelectionsBeforeSimple", ms_before_simple_, true),
    T::make_field ("deletedSelections", deleted_ms_, true),
    T::make_field ("automationTracks", ats_, true),
    T::make_field ("deletedAutomationTracks", deleted_ats_, true));
}

void
TracklistSelectionsAction::define_fields (const Context &ctx)
{
  UndoableAction::define_base_fields (ctx);

  using T = ISerializable<TracklistSelectionsAction>;
  T::serialize_fields (
    ctx, T::make_field ("type", tracklist_selections_action_type_),
    T::make_field ("trackType", track_type_),
    T::make_field ("editType", edit_type_),
    T::make_field ("pluginSetting", pl_setting_, true),
    T::make_field ("isEmpty", is_empty_), T::make_field ("trackPos", track_pos_),
    T::make_field ("lanePos", lane_pos_), T::make_field ("havePos", have_pos_),
    T::make_field ("pos", pos_, true),
    T::make_field ("tracksBefore", track_positions_before_, true),
    T::make_field ("tracksAfter", track_positions_after_, true),
    T::make_field ("numTracks", num_tracks_),
    T::make_field ("fileBasename", file_basename_, true),
    T::make_field ("base64Midi", base64_midi_, true),
    T::make_field ("poolId", pool_id_),
    T::make_field ("tracklistSelectionsBefore", tls_before_, true),
    T::make_field ("tracklistSelectionsAfter", tls_after_, true),
    T::make_field (
      "foldableTracklistSelectionsBefore", foldable_tls_before_, true),
    T::make_field ("outTracks", out_track_hashes_, true),
    T::make_field ("srcSends", src_sends_, true),
    T::make_field ("iValBefore", ival_before_, true),
    T::make_field ("iValAfter", ival_after_, true),
    T::make_field ("colorsBefore", colors_before_, true),
    T::make_field ("newColor", new_color_, true),
    T::make_field ("newTxt", new_txt_, true),
    T::make_field ("valBefore", val_before_),
    T::make_field ("valAfter", val_after_),
    T::make_field ("numFoldChangeTracks", num_fold_change_tracks_));
}

void
ChannelSendAction::define_fields (const Context &ctx)
{
  UndoableAction::define_base_fields (ctx);

  using T = ISerializable<ChannelSendAction>;
  T::serialize_fields (
    ctx, T::make_field ("type", send_action_type_),
    T::make_field ("sendBefore", send_before_),
    T::make_field ("lId", l_id_, true), T::make_field ("rId", r_id_, true),
    T::make_field ("midiId", midi_id_, true));
}

void
PortConnectionAction::define_fields (const Context &ctx)
{
  UndoableAction::define_base_fields (ctx);

  using T = ISerializable<PortConnectionAction>;
  T::serialize_fields (
    ctx, T::make_field ("type", type_),
    T::make_field ("connection", connection_, true),
    T::make_field ("val", val_));
}

void
PortAction::define_fields (const Context &ctx)
{
  UndoableAction::define_base_fields (ctx);

  using T = ISerializable<PortAction>;
  T::serialize_fields (
    ctx, T::make_field ("type", type_),
    T::make_field ("portId", port_id_, true), T::make_field ("val", val_));
}

void
MidiMappingAction::define_fields (const Context &ctx)
{
  UndoableAction::define_base_fields (ctx);

  using T = ISerializable<MidiMappingAction>;
  T::serialize_fields (
    ctx, T::make_field ("type", type_), T::make_field ("index", idx_),
    T::make_field ("destPortId", dest_port_id_, true),
    T::make_field ("devPortId", dev_port_, true),
    T::make_field ("buf", buf_, true));
}

void
RangeAction::define_fields (const Context &ctx)
{
  UndoableAction::define_base_fields (ctx);

  using T = ISerializable<RangeAction>;
  T::serialize_fields (
    ctx, T::make_field ("type", type_), T::make_field ("startPos", start_pos_),
    T::make_field ("endPos", end_pos_),
    T::make_field ("selectionsBefore", sel_before_, true),
    T::make_field ("selectionsAfter", sel_after_, true),
    // TODO check: is this needed?
    // T::make_field ("firstRun", first_run_));
    T::make_field ("transport", transport_));
}

void
TransportAction::define_fields (const Context &ctx)
{
  UndoableAction::define_base_fields (ctx);

  using T = ISerializable<TransportAction>;
  T::serialize_fields (
    ctx, T::make_field ("type", type_), T::make_field ("bpmBefore", bpm_before_),
    T::make_field ("bpmAfter", bpm_after_),
    T::make_field ("intBefore", int_before_),
    T::make_field ("intAfter", int_after_),
    T::make_field ("musicalMode", musical_mode_));
}

void
ChordAction::define_fields (const Context &ctx)
{
  UndoableAction::define_base_fields (ctx);

  using T = ISerializable<ChordAction>;
  T::serialize_fields (
    ctx, T::make_field ("type", type_),
    T::make_field ("chordBefore", chord_before_, true),
    T::make_field ("chordAfter", chord_after_, true),
    T::make_field ("chordIndex", chord_idx_),
    T::make_field ("chordsBefore", chords_before_, true),
    T::make_field ("chordsAfter", chords_after_, true));
}