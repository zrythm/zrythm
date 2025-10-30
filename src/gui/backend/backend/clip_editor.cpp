// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/clip_editor.h"
#include "structure/tracks/track_all.h"

ClipEditor::ClipEditor (
  ArrangerObjectRegistry &reg,
  TrackResolver           track_resolver,
  QObject *               parent)
    : QObject (parent), object_registry_ (reg),
      track_resolver_ (std::move (track_resolver)),
      piano_roll_ (
        utils::make_qobject_unique<structure::arrangement::PianoRoll> (this)),
      audio_clip_editor_ (
        utils::make_qobject_unique<structure::arrangement::AudioClipEditor> (
          this)),
      automation_editor_ (
        utils::make_qobject_unique<structure::arrangement::AutomationEditor> (
          this)),
      chord_editor_ (
        utils::make_qobject_unique<structure::arrangement::ChordEditor> (this))
{
}

void
ClipEditor::init_loaded ()
{
}

void
ClipEditor::setRegion (QVariant region, QVariant track)
{
  auto * r = dynamic_cast<ArrangerObject *> (region.value<QObject *> ());
  auto * t =
    dynamic_cast<structure::tracks::Track *> (track.value<QObject *> ());
  set_region (r->get_uuid (), t->get_uuid ());
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

auto
ClipEditor::get_region_and_track () const
  -> std::optional<std::pair<ArrangerObjectPtrVariant, TrackPtrVariant>>
{
  auto region_var = std::visit (
    [&] (auto &&obj) -> ArrangerObjectPtrVariant {
      if constexpr (
        structure::arrangement::RegionObject<base_type<decltype (obj)>>)
        {
          return obj;
        }
      throw std::bad_variant_access ();
    },
    object_registry_.find_by_id_or_throw (region_id_->first));
  auto track_var = track_resolver_ (region_id_->second);
  return std::make_pair (region_var, track_var);
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
ClipEditor::init ()
{
  piano_roll_->init ();
  chord_editor_->init ();
  // the rest of the editors are initialized in their respective classes
}
