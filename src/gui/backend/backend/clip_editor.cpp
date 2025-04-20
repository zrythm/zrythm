// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/clip_editor.h"
#include "gui/dsp/router.h"
#include "gui/dsp/track_all.h"
#include "utils/rt_thread_id.h"

ClipEditor::ClipEditor (
  ArrangerObjectRegistry &reg,
  TrackResolver           track_resolver,
  QObject *               parent)
    : QObject (parent), object_registry_ (reg),
      track_resolver_ (std::move (track_resolver)),
      piano_roll_ (new PianoRoll (this)),
      audio_clip_editor_ (new AudioClipEditor (this)),
      automation_editor_ (new AutomationEditor (this)),
      chord_editor_ (new ChordEditor (this))
{
  // connect regionChanged so that trackChanged is emitted too
  connect (this, &ClipEditor::regionChanged, this, &ClipEditor::trackChanged);
}

void
ClipEditor::init_loaded ()
{
  z_info ("Initializing clip editor backend...");

  piano_roll_->init_loaded ();

  z_info ("Done initializing clip editor backend");
}

void
ClipEditor::setRegion (QVariant region)
{
  auto r = dynamic_cast<Region *> (region.value<QObject *> ());
  set_region (r->get_uuid ());
}

QVariant
ClipEditor::getTrack () const
{
  if (has_region ())
    {
      auto track = *get_track ();
      return QVariant::fromStdVariant (track);
    }

  return {};
}

void
ClipEditor::unsetRegion ()
{
  if (!region_id_)
    return;

  region_id_.reset ();
  Q_EMIT regionChanged ({});
}

#if 0
void
ClipEditor::set_region (
  std::optional<RegionPtrVariant> region_opt_var)
{
  /*
   * block until current DSP cycle finishes to avoid potentially sending the
   * events to multiple tracks
   */
  bool sem_aquired = false;
  if (gZrythm && ROUTER && AUDIO_ENGINE->run_.load ())
    {
      z_debug (
        "clip editor region changed, waiting for graph access to make the change");
      ROUTER->graph_access_sem_.acquire ();
      sem_aquired = true;
    }

  if (region_opt_var)
    {
      std::visit (
        [&] (auto &&region) {
          /* if first time showing a region, show the event viewer as necessary */

          if (fire_events && !has_region_)
            {
              // /* EVENTS_PUSH (
              //   EventType::ET_CLIP_EDITOR_FIRST_TIME_REGION_SELECTED,
              //   nullptr); */
            }

          has_region_ = true;
          region_id_ = region->id_;
          track_ = region->get_track ();

          /* if audio region, also set it in selections */
          AUDIO_SELECTIONS->region_id_ = region->id_;
        },
        region_opt_var.value ());
    }
  else
    {
      z_debug ("unsetting region from clip editor");
      has_region_ = false;
      region_.reset ();
      track_.reset ();
    }

  if (sem_aquired)
    {
      ROUTER->graph_access_sem_.release ();
      z_debug ("clip editor region successfully changed");
    }

#  if 0
  if (fire_events && ZRYTHM_HAVE_UI && MAIN_WINDOW && MW_CLIP_EDITOR)
    {
      /* EVENTS_PUSH (EventType::ET_CLIP_EDITOR_REGION_CHANGED, nullptr); */

      /* setting the region potentially changes the active arranger - process
       * now to change the active arranger */
      EVENT_MANAGER->process_now ();
    }
#  endif
}
#endif

std::optional<Region::TrackUuid>
ClipEditor::get_track_id () const
{
  if (!has_region ())
    return std::nullopt;

  auto region_var = object_registry_.find_by_id_or_throw (*region_id_);
  return std::visit (
    [&] (auto &&region) { return region->get_track_id (); }, region_var);
}

void
ClipEditor::unset_region_if_belongs_to_track (const Region::TrackUuid &track_id)
{
  if (!region_id_.has_value ())
    {
      return;
    }

  auto region_track_id = get_track_id ();
  if (region_track_id == track_id)
    {
      unsetRegion ();
    }
}

std::optional<RegionPtrVariant>
ClipEditor::get_region () const
{
  auto region_var = std::visit (
    [&] (auto &&obj) -> RegionPtrVariant {
      if constexpr (std::derived_from<base_type<decltype (obj)>, Region>)
        {
          return obj;
        }
      throw std::bad_variant_access ();
    },
    object_registry_.find_by_id_or_throw (*region_id_));
  return region_var;
}

std::optional<TrackPtrVariant>
ClipEditor::get_track () const
{
#if 0
  if (ROUTER->is_processing_thread ())
    {
    assert (track_);
    return std::visit (
      [&] (auto &&track) -> TrackPtrVariant { return track; }, *track_);
    }
#endif

  if (!has_region ())
    return std::nullopt;

  return std::visit (
    [&] (auto &&region) -> OptionalTrackPtrVariant {
      const auto id = region->get_track_id ();
      if (id)
        {
          return track_resolver_ (*id);
        }

      return std::nullopt;
    },
    *get_region ());
}

#if 0
std::optional<ClipEditorArrangerSelectionsPtrVariant>
ClipEditor::get_arranger_selections ()
{
  if (!has_region_)
    return std::nullopt;

  return std::visit (
    [&] (auto &&region) { return region->get_arranger_selections (); },
    *region_);
}
#endif

void
ClipEditor::set_caches ()
{
  if (has_region ())
    {
      // region_ = get_region ();
      track_ = get_track ();
    }
  else
    {
      // region_.reset ();
      track_.reset ();
    }
}

void
ClipEditor::init ()
{
  piano_roll_->init ();
  chord_editor_->init ();
  // the rest of the editors are initialized in their respective classes
}
