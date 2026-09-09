// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/timeline_arranger_objects_model.h"
#include "structure/arrangement/tempo_object_manager.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/automation_tracklist.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_lane.h"
#include "structure/tracks/tracklist.h"
#include "utils/logger.h"

namespace zrythm::gui
{

namespace
{

template <typename ChildT>
structure::arrangement::ArrangerObjectListModel *
owner_model_of (const QObject * obj)
{
  const auto * owner = dynamic_cast<
    const structure::arrangement::ArrangerObjectOwner<ChildT> *> (obj);
  return owner != nullptr ? owner->get_model () : nullptr;
}

} // namespace

TimelineArrangerObjectsModel::TimelineArrangerObjectsModel (QObject * parent)
    : UnifiedProxyModel (parent)
{
}

structure::tracks::Tracklist *
TimelineArrangerObjectsModel::tracklist () const
{
  return tracklist_;
}

void
TimelineArrangerObjectsModel::setTracklist (
  structure::tracks::Tracklist * tracklist)
{
  if (tracklist_ == tracklist)
    return;

  detach_from_tracklist ();
  tracklist_ = tracklist;
  Q_EMIT tracklistChanged ();
  if (tracklist_ != nullptr)
    attach_to_tracklist ();
}

structure::arrangement::TempoObjectManager *
TimelineArrangerObjectsModel::tempoObjectManager () const
{
  return tempo_object_manager_;
}

void
TimelineArrangerObjectsModel::setTempoObjectManager (
  structure::arrangement::TempoObjectManager * manager)
{
  if (tempo_object_manager_ == manager)
    return;

  unregister_tempo_sources ();
  tempo_object_manager_ = manager;
  Q_EMIT tempoObjectManagerChanged ();
  register_tempo_sources ();
}

void
TimelineArrangerObjectsModel::attach_to_tracklist ()
{
  auto * collection = tracklist_->collection ();

  collection_rows_inserted_conn_ = QObject::connect (
    collection, &QAbstractItemModel::rowsInserted, this,
    [this, collection] (const QModelIndex &, int first, int last) {
      if (collection->moveInProgress ())
        return;
      for (int row = first; row <= last; ++row)
        {
          register_track (collection->get_track_at_index (row));
        }
    });

  collection_rows_about_to_be_removed_conn_ = QObject::connect (
    collection, &QAbstractItemModel::rowsAboutToBeRemoved, this,
    [this, collection] (const QModelIndex &, int first, int last) {
      if (collection->moveInProgress ())
        return;
      for (int row = first; row <= last; ++row)
        {
          unregister_track (collection->get_track_at_index (row));
        }
    });

  for (const auto &track_ref : collection->tracks ())
    {
      register_track (track_ref.get ());
    }
}

void
TimelineArrangerObjectsModel::detach_from_tracklist ()
{
  if (tracklist_ == nullptr)
    return;

  auto * collection = tracklist_->collection ();
  QObject::disconnect (collection_rows_inserted_conn_);
  QObject::disconnect (collection_rows_about_to_be_removed_conn_);

  for (const auto &track_ref : collection->tracks ())
    {
      unregister_track (track_ref.get ());
    }

  if (!tracklist_sources_.empty ())
    {
      z_warning (
        "TimelineArrangerObjectsModel: {} tracklist sources still "
        "registered after detaching from the tracklist",
        tracklist_sources_.size ());
      const auto leftover = tracklist_sources_;
      for (auto * model : leftover)
        {
          unregister_tracklist_source (model);
        }
    }
  registered_tracks_.clear ();
}

void
TimelineArrangerObjectsModel::register_track (structure::tracks::Track * track)
{
  if (track == nullptr)
    return;

  if (!registered_tracks_.insert (track).second)
    return;

  if (auto * lanes = track->lanes ())
    {
      for (const auto &lane : lanes->lanes ())
        {
          register_lane (lane.get ());
        }

      QObject::connect (
        lanes, &QAbstractItemModel::rowsInserted, this,
        [this, lanes] (const QModelIndex &, int first, int last) {
          for (int row = first; row <= last; ++row)
            {
              register_lane (lanes->at (row));
            }
        });
      QObject::connect (
        lanes, &QAbstractItemModel::rowsAboutToBeRemoved, this,
        [this, lanes] (const QModelIndex &, int first, int last) {
          for (int row = first; row <= last; ++row)
            {
              unregister_lane (lanes->at (row));
            }
        });
    }

  register_tracklist_source (
    owner_model_of<structure::arrangement::ChordClip> (track));
  register_tracklist_source (
    owner_model_of<structure::arrangement::ScaleObject> (track));
  register_tracklist_source (
    owner_model_of<structure::arrangement::Marker> (track));

  if (auto * atl = track->automationTracklist ())
    {
      register_automation_tracklist (atl);
    }
}

void
TimelineArrangerObjectsModel::unregister_track (structure::tracks::Track * track)
{
  if (track == nullptr)
    return;

  registered_tracks_.erase (track);

  if (auto * lanes = track->lanes ())
    {
      QObject::disconnect (lanes, nullptr, this, nullptr);
      for (const auto &lane : lanes->lanes ())
        {
          unregister_lane (lane.get ());
        }
    }

  unregister_tracklist_source (
    owner_model_of<structure::arrangement::ChordClip> (track));
  unregister_tracklist_source (
    owner_model_of<structure::arrangement::ScaleObject> (track));
  unregister_tracklist_source (
    owner_model_of<structure::arrangement::Marker> (track));

  if (auto * atl = track->automationTracklist ())
    {
      unregister_automation_tracklist (atl);
    }
}

void
TimelineArrangerObjectsModel::register_lane (structure::tracks::TrackLane * lane)
{
  if (lane == nullptr)
    return;

  register_tracklist_source (lane->midiClips ());
  register_tracklist_source (lane->audioClips ());
}

void
TimelineArrangerObjectsModel::unregister_lane (
  structure::tracks::TrackLane * lane)
{
  if (lane == nullptr)
    return;

  unregister_tracklist_source (lane->midiClips ());
  unregister_tracklist_source (lane->audioClips ());
}

void
TimelineArrangerObjectsModel::register_automation_tracklist (
  structure::tracks::AutomationTracklist * atl)
{
  for (auto * at : atl->automation_tracks ())
    {
      register_tracklist_source (at->clips ());
    }

  QObject::connect (
    atl, &QAbstractItemModel::rowsInserted, this,
    [this, atl] (const QModelIndex &, int first, int last) {
      for (int row = first; row <= last; ++row)
        {
          register_tracklist_source (atl->automation_track_at (row)->clips ());
        }
    });
  QObject::connect (
    atl, &QAbstractItemModel::rowsAboutToBeRemoved, this,
    [this, atl] (const QModelIndex &, int first, int last) {
      for (int row = first; row <= last; ++row)
        {
          unregister_tracklist_source (atl->automation_track_at (row)->clips ());
        }
    });
  QObject::connect (
    atl, &QAbstractItemModel::modelAboutToBeReset, this,
    [this, atl] () { snapshot_automation_tracklist (atl); });
  QObject::connect (atl, &QAbstractItemModel::modelReset, this, [this, atl] () {
    resync_automation_tracklist (atl);
  });
}

void
TimelineArrangerObjectsModel::unregister_automation_tracklist (
  structure::tracks::AutomationTracklist * atl)
{
  QObject::disconnect (atl, nullptr, this, nullptr);
  automation_tracklist_reset_snapshots_.erase (atl);
  for (auto * at : atl->automation_tracks ())
    {
      unregister_tracklist_source (at->clips ());
    }
}

void
TimelineArrangerObjectsModel::snapshot_automation_tracklist (
  structure::tracks::AutomationTracklist * atl)
{
  auto &snapshot = automation_tracklist_reset_snapshots_[atl];
  snapshot.clear ();
  for (auto * at : atl->automation_tracks ())
    {
      snapshot.insert (at->clips ());
    }
}

void
TimelineArrangerObjectsModel::resync_automation_tracklist (
  structure::tracks::AutomationTracklist * atl)
{
  const auto snapshot_it = automation_tracklist_reset_snapshots_.find (atl);
  if (snapshot_it == automation_tracklist_reset_snapshots_.end ())
    return;
  auto snapshot = std::move (snapshot_it->second);
  automation_tracklist_reset_snapshots_.erase (snapshot_it);

  std::unordered_set<structure::arrangement::ArrangerObjectListModel *> current;
  for (auto * at : atl->automation_tracks ())
    {
      current.insert (at->clips ());
    }

  for (auto * model : snapshot)
    {
      if (!current.contains (model))
        {
          unregister_tracklist_source (model);
        }
    }
  for (auto * model : current)
    {
      register_tracklist_source (model);
    }
}

void
TimelineArrangerObjectsModel::register_tracklist_source (
  structure::arrangement::ArrangerObjectListModel * model)
{
  if (model == nullptr || tracklist_sources_.contains (model))
    return;

  tracklist_sources_.insert (model);
  QObject::connect (
    model, &structure::arrangement::ArrangerObjectListModel::aboutToBeDestroyed,
    this, [this, model] () {
      tracklist_sources_.erase (model);
      removeSourceModel (model);
    });
  addSourceModel (model);
}

void
TimelineArrangerObjectsModel::unregister_tracklist_source (
  structure::arrangement::ArrangerObjectListModel * model)
{
  if (model == nullptr || tracklist_sources_.erase (model) == 0)
    return;

  QObject::disconnect (
    model, &structure::arrangement::ArrangerObjectListModel::aboutToBeDestroyed,
    this, nullptr);
  removeSourceModel (model);
}

void
TimelineArrangerObjectsModel::register_tempo_sources ()
{
  if (tempo_object_manager_ == nullptr)
    return;

  register_tempo_source (tempo_object_manager_->tempoObjects ());
  register_tempo_source (tempo_object_manager_->timeSignatureObjects ());
}

void
TimelineArrangerObjectsModel::unregister_tempo_sources ()
{
  const auto sources = tempo_sources_;
  for (auto * model : sources)
    {
      unregister_tempo_source (model);
    }
}

void
TimelineArrangerObjectsModel::register_tempo_source (
  structure::arrangement::ArrangerObjectListModel * model)
{
  if (model == nullptr || tempo_sources_.contains (model))
    return;

  tempo_sources_.insert (model);
  QObject::connect (
    model, &structure::arrangement::ArrangerObjectListModel::aboutToBeDestroyed,
    this, [this, model] () {
      tempo_sources_.erase (model);
      removeSourceModel (model);
    });
  addSourceModel (model);
}

void
TimelineArrangerObjectsModel::unregister_tempo_source (
  structure::arrangement::ArrangerObjectListModel * model)
{
  if (model == nullptr || tempo_sources_.erase (model) == 0)
    return;

  QObject::disconnect (
    model, &structure::arrangement::ArrangerObjectListModel::aboutToBeDestroyed,
    this, nullptr);
  removeSourceModel (model);
}

} // namespace zrythm::gui
