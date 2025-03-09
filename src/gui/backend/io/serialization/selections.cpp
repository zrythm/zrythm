// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/audio_selections.h"
#include "gui/backend/backend/automation_selections.h"
#include "gui/backend/backend/chord_selections.h"
#include "gui/backend/backend/midi_selections.h"
#include "gui/backend/backend/timeline_selections.h"

void
ArrangerSelections::define_base_fields (const Context &ctx)
{
  using T = ISerializable<ArrangerSelections>;
  T::serialize_fields (
    ctx, T::make_field ("type", type_), T::make_field ("objects", objects_));

#if 0
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
#endif
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
