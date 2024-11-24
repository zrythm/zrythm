// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

# include "gui/dsp/router.h"
# include "gui/dsp/track.h"
#include "utils/rt_thread_id.h"
#include "gui/backend/backend/clip_editor.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

void
ClipEditor::init_loaded ()
{
  z_info ("Initializing clip editor backend...");

  piano_roll_.init_loaded ();

  z_info ("Done initializing clip editor backend");
}

void
ClipEditor::set_region (
  std::optional<RegionPtrVariant> region_opt_var,
  bool                            fire_events)
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

#if 0
  if (fire_events && ZRYTHM_HAVE_UI && MAIN_WINDOW && MW_CLIP_EDITOR)
    {
      /* EVENTS_PUSH (EventType::ET_CLIP_EDITOR_REGION_CHANGED, nullptr); */

      /* setting the region potentially changes the active arranger - process
       * now to change the active arranger */
      EVENT_MANAGER->process_now ();
    }
#endif
}

std::optional<RegionPtrVariant>
ClipEditor::get_region () const
{
  return region_;
}

std::optional<TrackPtrVariant>
ClipEditor::get_track ()
{
  if (ROUTER->is_processing_thread ())
    {
      return std::visit (
        [&] (auto &&track) -> TrackPtrVariant { return track; }, *track_);
    }

  if (!has_region_)
    return std::nullopt;

  return std::visit (
    [&] (auto &&region) { return region->get_track (); }, *region_);
}

std::optional<ClipEditorArrangerSelectionsPtrVariant>
ClipEditor::get_arranger_selections ()
{
  if (!has_region_)
    return std::nullopt;

  return std::visit (
    [&] (auto &&region) { return region->get_arranger_selections (); },
    *region_);
}

void
ClipEditor::set_caches ()
{
  if (has_region_)
    {
      region_ = get_region ();
      track_ = get_track ();
    }
  else
    {
      region_.reset ();
      track_.reset ();
    }
}

void
ClipEditor::init ()
{
  piano_roll_.init ();
  chord_editor_.init ();
  // the rest of the editors are initialized in their respective classes
}
