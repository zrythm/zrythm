// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/audio_selections.h"
#include "gui/backend/backend/automation_selections.h"
#include "gui/backend/backend/chord_selections.h"
#include "gui/backend/backend/midi_selections.h"
#include "gui/backend/backend/mixer_selections.h"
#include "gui/backend/backend/timeline_selections.h"
#include "gui/backend/backend/tracklist_selections.h"

void
MixerSelections::define_base_fields (const Context &ctx)
{
  using T = ISerializable<MixerSelections>;
  T::serialize_fields (
    ctx, T::make_field ("type", type_), T::make_field ("slots", slots_),
    T::make_field ("trackNameHash", track_name_hash_));
}

void
MixerSelections::define_fields (const Context &ctx)
{
  using T = ISerializable<MixerSelections>;
  T::call_all_base_define_fields<MixerSelections> (ctx);
}

void
FullMixerSelections::define_fields (const Context &ctx)
{
  using T = ISerializable<FullMixerSelections>;
  const_cast<FullMixerSelections *> (this)
    ->T::call_all_base_define_fields<MixerSelections> (ctx);

  if (ctx.is_serializing ())
    {
      T::serialize_field<decltype (plugins_), zrythm::plugins::PluginPtrVariant> (
        "plugins", plugins_, ctx);
    }
  else
    {
      yyjson_obj_iter it = yyjson_obj_iter_with (ctx.obj_);
      yyjson_val *    arr = yyjson_obj_iter_get (&it, "plugins");
      if (!arr)
        {
          throw ZrythmException ("No plugins array");
        }
      yyjson_arr_iter pl_arr_it = yyjson_arr_iter_with (arr);
      yyjson_val *    pl_obj = NULL;
      while ((pl_obj = yyjson_arr_iter_next (&pl_arr_it)))
        {
          auto          pl_it = yyjson_obj_iter_with (pl_obj);
          auto          setting_obj = yyjson_obj_iter_get (&pl_it, "setting");
          PluginSetting setting;
          setting.deserialize (Context (setting_obj, ctx));
          auto pl = zrythm::plugins::Plugin::create_unique_from_hosting_type (
            setting.hosting_type_);
          std::visit (
            [&] (auto &&p) {
              using PluginT = base_type<decltype (p)>;
              p->ISerializable<PluginT>::deserialize (Context (pl_obj, ctx));
            },
            convert_to_variant<zrythm::plugins::PluginPtrVariant> (pl.get ()));
          plugins_.emplace_back (std::move (pl));
        }
    }
}

void
ArrangerSelections::define_base_fields (const Context &ctx)
{
  using T = ISerializable<ArrangerSelections>;
  T::serialize_fields (ctx, T::make_field ("type", type_));

  if (ctx.is_serializing ())
    {
      T::serialize_field<decltype (objects_), ArrangerObjectPtrVariant> (
        "objects", objects_, ctx);
    }
  else
    {
      auto it = yyjson_obj_iter_with (ctx.obj_);
      deserialize_field (it, "type", type_, ctx);

      yyjson_val * arr = yyjson_obj_iter_get (&it, "objects");
      if (!arr)
        {
          throw ZrythmException ("No objects array");
        }
      yyjson_arr_iter arranger_obj_arr_it = yyjson_arr_iter_with (arr);
      yyjson_val *    arranger_obj_obj = nullptr;
      while ((arranger_obj_obj = yyjson_arr_iter_next (&arranger_obj_arr_it)))
        {
          auto arranger_obj_it = yyjson_obj_iter_with (arranger_obj_obj);
          auto type =
            yyjson_get_int (yyjson_obj_iter_get (&arranger_obj_it, "type"));
          try
            {
              auto create_obj = [&]<typename ObjType> () {
                auto obj = std::make_shared<ObjType> ();
                obj->ISerializable<ObjType>::deserialize (
                  Context (arranger_obj_obj, ctx));
                objects_.emplace_back (std::move (obj));
              };

              auto type_enum = ENUM_INT_TO_VALUE (ArrangerObject::Type, type);
              switch (type_enum)
                {
                case ArrangerObject::Type::AutomationPoint:
                  create_obj.template operator()<AutomationPoint> ();
                  break;
                case ArrangerObject::Type::MidiNote:
                  create_obj.template operator()<MidiNote> ();
                  break;
                case ArrangerObject::Type::Marker:
                  create_obj.template operator()<Marker> ();
                  break;
                case ArrangerObject::Type::ScaleObject:
                  create_obj.template operator()<ScaleObject> ();
                  break;
                case ArrangerObject::Type::Velocity:
                  create_obj.template operator()<Velocity> ();
                  break;
                case ArrangerObject::Type::ChordObject:
                  create_obj.template operator()<ChordObject> ();
                  break;
                case ArrangerObject::Type::Region:
                  {
                    auto region_id =
                      yyjson_obj_iter_get (&arranger_obj_it, "regionId");
                    RegionIdentifier r_id;
                    r_id.deserialize (Context (region_id, ctx));
                    switch (r_id.type_)
                      {
                      case RegionType::Midi:
                        {
                          create_obj.template operator()<MidiRegion> ();
                        }
                        break;
                      case RegionType::Audio:
                        {
                          create_obj.template operator()<AudioRegion> ();
                          break;
                        }
                      case RegionType::Chord:
                        {
                          create_obj.template operator()<ChordRegion> ();
                          break;
                        }
                      case RegionType::Automation:
                        {
                          create_obj.template operator()<AutomationRegion> ();
                          break;
                        }
                      default:
                        throw ZrythmException ("Unknown region type");
                      }
                    break;
                  }
                default:
                  throw ZrythmException ("Unknown arranger object type");
                }
            }
          catch (std::runtime_error &e)
            {
              throw ZrythmException (e.what ());
            }
        }
    }
}

void
TimelineSelections::define_fields (const Context &ctx)
{
  using T = ISerializable<TimelineSelections>;
  T::call_all_base_define_fields<ArrangerSelections> (ctx);
  T::serialize_fields (
    ctx, T::make_field ("regionTrackVisibilityIndex", region_track_vis_index_),
    T::make_field ("chordTrackVisibilityIndex", chord_track_vis_index_),
    T::make_field ("markerTrackVisibilityIndex", marker_track_vis_index_));
}

void
MidiSelections::define_fields (const Context &ctx)
{
  using T = ISerializable<MidiSelections>;
  T::call_all_base_define_fields<ArrangerSelections> (ctx);
}

void
ChordSelections::define_fields (const Context &ctx)
{
  using T = ISerializable<ChordSelections>;
  T::call_all_base_define_fields<ArrangerSelections> (ctx);
}

void
AudioSelections::define_fields (const Context &ctx)
{
  using T = ISerializable<AudioSelections>;
  T::call_all_base_define_fields<ArrangerSelections> (ctx);
  T::serialize_fields (
    ctx, T::make_field ("hasSelection", has_selection_),
    T::make_field ("selStart", sel_start_), T::make_field ("selEnd", sel_end_),
    T::make_field ("poolId", pool_id_), T::make_field ("regionId", region_id_));
}

void
AutomationSelections::define_fields (const Context &ctx)
{
  using T = ISerializable<AutomationSelections>;
  T::call_all_base_define_fields<ArrangerSelections> (ctx);
}

void
TracklistSelections::define_fields (const Context &ctx)
{
  using T = ISerializable<TracklistSelections>;

  if (ctx.is_serializing ())
    {
      T::serialize_field<decltype (tracks_), TrackPtrVariant> (
        "tracks", tracks_, ctx);
    }
  else
    {
      yyjson_obj_iter it = yyjson_obj_iter_with (ctx.obj_);

      yyjson_val * arr = yyjson_obj_iter_get (&it, "tracks");
      if (!arr)
        {
          throw ZrythmException ("No tracks array");
        }
      yyjson_arr_iter track_arr_it = yyjson_arr_iter_with (arr);
      yyjson_val *    track_obj = nullptr;
      while ((track_obj = yyjson_arr_iter_next (&track_arr_it)))
        {
          auto track_it = yyjson_obj_iter_with (track_obj);
          auto type = yyjson_get_int (yyjson_obj_iter_get (&track_it, "type"));
          try
            {
              auto type_enum = ENUM_INT_TO_VALUE (Track::Type, type);
              auto track = Track::create_unique_from_type (type_enum);
              std::visit (
                [&] (auto &&t) {
                  using TrackT = base_type<decltype (t)>;
                  t->ISerializable<TrackT>::deserialize (
                    Context (track_obj, ctx));
                },
                convert_to_variant<TrackPtrVariant> (track.get ()));
              tracks_.emplace_back (std::move (track));
            }
          catch (std::runtime_error &e)
            {
              throw ZrythmException (e.what ());
            }
        }
    }
}

void
SimpleTracklistSelections::define_fields (const Context &ctx)
{
  serialize_fields (ctx, make_field ("trackNames", track_names_));
}