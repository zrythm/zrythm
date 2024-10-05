// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/router.h"
#include "common/dsp/track.h"
#include "common/utils/rt_thread_id.h"
#include "gui/cpp/backend/clip_editor.h"
#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/arranger_object.h"
#include "gui/cpp/gtk_widgets/bot_dock_edge.h"
#include "gui/cpp/gtk_widgets/center_dock.h"
#include "gui/cpp/gtk_widgets/clip_editor.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

void
ClipEditor::init_loaded ()
{
  z_info ("Initializing clip editor backend...");

  piano_roll_.init_loaded ();

  z_info ("Done initializing clip editor backend");
}

void
ClipEditor::set_region (Region * region, bool fire_events)
{
  if (region)
    {
      z_return_if_fail (IS_REGION (region));
    }

  z_debug (
    "clip editor: setting region to {} ({})", fmt::ptr (region),
    region ? region->name_ : "");

  /* if first time showing a region, show the
   * event viewer as necessary */
  if (fire_events && !has_region_ && region)
    {
      EVENTS_PUSH (
        EventType::ET_CLIP_EDITOR_FIRST_TIME_REGION_SELECTED, nullptr);
    }

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

  if (region)
    {
      has_region_ = true;
      region_id_ = region->id_;
      track_ = region->get_track ();

      /* if audio region, also set it in selections */
      AUDIO_SELECTIONS->region_id_ = region->id_;
    }
  else
    {
      has_region_ = false;
      track_ = nullptr;
    }

  region_changed_ = true;

  if (sem_aquired)
    {
      ROUTER->graph_access_sem_.release ();
      z_debug ("clip editor region successfully changed");
    }

  if (fire_events && ZRYTHM_HAVE_UI && MAIN_WINDOW && MW_CLIP_EDITOR)
    {
      EVENTS_PUSH (EventType::ET_CLIP_EDITOR_REGION_CHANGED, nullptr);

      /* setting the region potentially changes the active arranger - process
       * now to change the active arranger */
      EVENT_MANAGER->process_now ();
    }
}

template <FinalRegionSubclass T>
T *
ClipEditor::get_region () const
{
  if (ROUTER->is_processing_thread ())
    {
      return dynamic_cast<T *> (region_);
    }

  if (!has_region_)
    return nullptr;

  return T::find (region_id_).get ();
}

Region *
ClipEditor::get_region () const
{
  if (has_region_)
    {
      switch (region_id_.type_)
        {
        case RegionType::Midi:
          return get_region<MidiRegion> ();
        case RegionType::Audio:
          return get_region<AudioRegion> ();
        case RegionType::Chord:
          return get_region<ChordRegion> ();
        case RegionType::Automation:
          return get_region<AutomationRegion> ();
        default:
          z_return_val_if_reached (nullptr);
        }
    }
  else
    {
      return nullptr;
    }
}

Track *
ClipEditor::get_track ()
{
  if (ROUTER->is_processing_thread ())
    {
      return track_;
    }

  if (!has_region_)
    return nullptr;

  Region * region = get_region ();
  z_return_val_if_fail (region, nullptr);

  return region->get_track ();
}

ArrangerSelections *
ClipEditor::get_arranger_selections ()
{
  Region * region = get_region ();
  if (!region)
    {
      return nullptr;
    }

  return region->get_arranger_selections ();
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
      region_ = nullptr;
      track_ = nullptr;
    }
}

void
ClipEditor::init ()
{
  piano_roll_.init ();
  chord_editor_.init ();
  // the rest of the editors are initialized in their respective classes
}

template MidiRegion *
ClipEditor::get_region<MidiRegion> () const;
template AudioRegion *
ClipEditor::get_region<AudioRegion> () const;
template AutomationRegion *
ClipEditor::get_region<AutomationRegion> () const;
template ChordRegion *
ClipEditor::get_region<ChordRegion> () const;