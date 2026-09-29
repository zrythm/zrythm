// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <unordered_map>
#include <unordered_set>

#include "gui/backend/unified_proxy_model.h"
#include "structure/arrangement/arranger_object_list_model.h"
#include "structure/arrangement/tempo_object_manager.h"
#include "structure/tracks/tracklist.h"

#include <QMetaObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui
{

/**
 * @brief Unified model of all arranger objects on the timeline.
 *
 * Aggregates the arranger object list models of every track in a
 * tracklist (lane MIDI/audio clips, chord/scale/marker objects and
 * automation clips) together with the tempo map's tempo and time
 * signature objects into a single UnifiedProxyModel that views and
 * selections are built on.
 *
 * Source registration follows the domain: a source model is registered
 * when its owner (track, lane or automation track) is inserted into the
 * structure and unregistered when the owner is removed. Track moves,
 * which are carried out as removal-and-reinsertion cycles while the
 * track collection's moveInProgress flag is set, are ignored so that
 * unified rows and selections stay stable while a move is in progress.
 * Automation tracklist reorders, which reset the model instead of
 * moving rows, are resynchronized by diffing the source sets captured
 * before and after the reset, which leaves unchanged sources (and the
 * selections on them) untouched.
 */
class TimelineArrangerObjectsModel : public UnifiedProxyModel
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::structure::tracks::Tracklist * tracklist READ tracklist WRITE
      setTracklist NOTIFY tracklistChanged)
  Q_PROPERTY (
    zrythm::structure::arrangement::TempoObjectManager * tempoObjectManager READ
      tempoObjectManager WRITE setTempoObjectManager NOTIFY
        tempoObjectManagerChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  explicit TimelineArrangerObjectsModel (QObject * parent = nullptr);

  zrythm::structure::tracks::Tracklist * tracklist () const;
  void          setTracklist (zrythm::structure::tracks::Tracklist * tracklist);
  Q_SIGNAL void tracklistChanged ();

  zrythm::structure::arrangement::TempoObjectManager *
       tempoObjectManager () const;
  void setTempoObjectManager (
    zrythm::structure::arrangement::TempoObjectManager * manager);
  Q_SIGNAL void tempoObjectManagerChanged ();

private:
  void attach_to_tracklist ();
  void detach_from_tracklist ();

  void register_track (structure::tracks::Track * track);
  void unregister_track (structure::tracks::Track * track);

  void register_lane (structure::tracks::TrackLane * lane);
  void unregister_lane (structure::tracks::TrackLane * lane);

  void
  register_automation_tracklist (structure::tracks::AutomationTracklist * atl);
  void
  unregister_automation_tracklist (structure::tracks::AutomationTracklist * atl);
  void
  snapshot_automation_tracklist (structure::tracks::AutomationTracklist * atl);
  void
  resync_automation_tracklist (structure::tracks::AutomationTracklist * atl);

  void register_tracklist_source (
    structure::arrangement::ArrangerObjectListModel * model);
  void unregister_tracklist_source (
    structure::arrangement::ArrangerObjectListModel * model);

  void register_tempo_sources ();
  void unregister_tempo_sources ();
  void register_tempo_source (
    structure::arrangement::ArrangerObjectListModel * model);
  void unregister_tempo_source (
    structure::arrangement::ArrangerObjectListModel * model);
  structure::tracks::Tracklist *               tracklist_ = nullptr;
  structure::arrangement::TempoObjectManager * tempo_object_manager_ = nullptr;

  /** Tracks whose models are currently registered. */
  std::unordered_set<const structure::tracks::Track *> registered_tracks_;

  std::unordered_set<structure::arrangement::ArrangerObjectListModel *>
    tracklist_sources_;
  std::unordered_set<structure::arrangement::ArrangerObjectListModel *>
    tempo_sources_;

  std::unordered_map<
    const structure::tracks::AutomationTracklist *,
    std::unordered_set<structure::arrangement::ArrangerObjectListModel *>>
    automation_tracklist_reset_snapshots_;

  QMetaObject::Connection collection_rows_inserted_conn_;
  QMetaObject::Connection collection_rows_about_to_be_removed_conn_;
};

} // namespace zrythm::gui
