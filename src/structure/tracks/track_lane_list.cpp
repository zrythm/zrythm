// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>
#include <set>

#include "utils/format_qt.h"

#include "structure/tracks/track_lane_list.h"
#include "utils/exceptions.h"
#include "utils/registry_utils.h"
#include "utils/views.h"

#include <scn/scan.h>

namespace zrythm::structure::tracks
{
TrackLaneList::TrackLaneList (
  utils::IObjectRegistry &registry,
  dsp::TimebaseProvider * timebase_provider,
  QObject *               parent)
    : QAbstractListModel (parent),
      dependencies_ (
        TrackLane::TrackLaneDependencies{
          .registry_ = registry,
          .timebase_provider_ = timebase_provider })
{
  QObject::connect (
    this, &TrackLaneList::rowsInserted, this,
    [this] (const QModelIndex &, int first, int last) {
      for (int i = first; i <= last; ++i)
        {
          auto * lane = lanes ().at (i).get ();
          QObject::connect (
            lane->arrangement::ArrangerObjectOwner<
              arrangement::MidiClip>::get_model (),
            &arrangement::ArrangerObjectListModel::contentChanged, this,
            &TrackLaneList::laneObjectsNeedRecache, Qt::QueuedConnection);
          QObject::connect (
            lane->arrangement::ArrangerObjectOwner<
              arrangement::AudioClip>::get_model (),
            &arrangement::ArrangerObjectListModel::contentChanged, this,
            &TrackLaneList::laneObjectsNeedRecache, Qt::QueuedConnection);
          QObject::connect (
            lane, &TrackLane::heightChanged, this,
            &TrackLaneList::totalHeightChanged);
        }
      Q_EMIT totalHeightChanged ();
    });
  QObject::connect (
    this, &TrackLaneList::rowsAboutToBeRemoved, this,
    [this] (const QModelIndex &, int first, int last) {
      for (int i = first; i <= last; ++i)
        {
          auto * lane = lanes ().at (i).get ();
          QObject::disconnect (
            lane->arrangement::ArrangerObjectOwner<
              arrangement::MidiClip>::get_model (),
            &arrangement::ArrangerObjectListModel::contentChanged, this,
            &TrackLaneList::laneObjectsNeedRecache);
          QObject::disconnect (
            lane->arrangement::ArrangerObjectOwner<
              arrangement::AudioClip>::get_model (),
            &arrangement::ArrangerObjectListModel::contentChanged, this,
            &TrackLaneList::laneObjectsNeedRecache);
          QObject::disconnect (
            lane, &TrackLane::heightChanged, this,
            &TrackLaneList::totalHeightChanged);
        }
    });
  QObject::connect (
    this, &TrackLaneList::rowsRemoved, this, &TrackLaneList::totalHeightChanged);
  QObject::connect (
    this, &TrackLaneList::lanesVisibleChanged, this,
    &TrackLaneList::totalHeightChanged);
}

// ========================================================================
// QML Interface
// ========================================================================

QHash<int, QByteArray>
TrackLaneList::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[TrackLanePtrRole] = "trackLane";
  return roles;
}

int
TrackLaneList::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;
  return static_cast<int> (lanes_.size ());
}

QVariant
TrackLaneList::data (const QModelIndex &index, int role) const
{
  if (!index.isValid () || index.row () >= static_cast<int> (lanes_.size ()))
    return {};

  const auto &lane = lanes_.at (static_cast<size_t> (index.row ()));

  switch (role)
    {
    case TrackLanePtrRole:
      return QVariant::fromValue (lane.get ());
    case Qt::DisplayRole:
      return lane.get ()->name ();
    default:
      return {};
    }

  return {};
}

TrackLane *
TrackLaneList::insertLane (size_t index)
{
  if (index > size ())
    throw std::out_of_range ("index out of range");

  auto lane_ref =
    utils::create_object<TrackLane> (dependencies_.registry_, dependencies_);
  // the lane's owner pointer is set before the row insert so rowsInserted
  // handlers observe an attached lane
  lane_ref.get ()->set_owner_list (this);

  beginInsertRows (
    QModelIndex (), static_cast<int> (index), static_cast<int> (index));
  lanes_.insert (
    std::ranges::next (std::begin (lanes_), static_cast<int> (index)),
    std::move (lane_ref));
  endInsertRows ();

  auto * lane = lanes_.at (index).get ();
  lane->generate_name (index);
  update_default_lane_names ();
  return lane;
}

void
TrackLaneList::reinsert_lane (size_t index, TrackLaneUuidReference lane_ref)
{
  if (index > size ())
    throw std::out_of_range ("index out of range");
  if (indexOfLane (lane_ref.get ()) != std::nullopt)
    throw std::invalid_argument ("lane is already in the list");
  if (lane_ref.get ()->owner_list () != nullptr)
    throw std::invalid_argument ("lane is still attached to a list");

  // the lane's owner pointer is set before the row insert so rowsInserted
  // handlers observe an attached lane
  lane_ref.get ()->set_owner_list (this);
  lane_ref.get ()->rewire_dependencies (dependencies_);

  beginInsertRows (
    QModelIndex (), static_cast<int> (index), static_cast<int> (index));
  lanes_.insert (
    std::ranges::next (std::begin (lanes_), static_cast<int> (index)),
    std::move (lane_ref));
  endInsertRows ();

  update_default_lane_names ();
}

void
TrackLaneList::removeLane (size_t index)
{
  // A track keeps at least one lane
  if (empty ())
    {
      z_warning ("No lanes to remove");
      return;
    }
  if (size () <= 1)
    {
      z_warning ("Cannot remove the last lane of a track");
      return;
    }
  erase (index);
}

void
TrackLaneList::moveLane (size_t from_index, size_t to_index)
{
  if (from_index == to_index)
    return;
  if (from_index >= size ())
    throw std::out_of_range ("from index out of range");
  if (to_index > size ())
    throw std::out_of_range ("to index out of range");

  beginMoveRows (
    {}, static_cast<int> (from_index), static_cast<int> (from_index), {},
    static_cast<int> (to_index));
  if (from_index < to_index)
    {
      std::ranges::rotate (
        lanes_.begin () + static_cast<int> (from_index),
        lanes_.begin () + static_cast<int> (from_index) + 1,
        lanes_.begin () + static_cast<int> (to_index) + 1);
    }
  else
    {
      std::ranges::rotate (
        lanes_.begin () + static_cast<int> (to_index),
        lanes_.begin () + static_cast<int> (from_index),
        lanes_.begin () + static_cast<int> (from_index) + 1);
    }

  update_default_lane_names ();
  endMoveRows ();
}

// ========================================================================

void
TrackLaneList::create_missing_lanes (size_t index)
{
  while ((index + 2) > lanes_.size ())
    {
      addLane ();
    }
}

void
TrackLaneList::ensure_trailing_empty_lane ()
{
  if (empty () || !at (size () - 1)->is_empty ())
    {
      addLane ();
    }
}

void
TrackLaneList::trim_trailing_empty_lanes ()
{
  // A single trailing empty lane is kept for new clips
  while (
    size () > 1 && at (size () - 1)->is_empty ()
    && at (size () - 2)->is_empty ())
    {
      removeLane (size () - 1);
    }
}

void
TrackLaneList::remove_empty_last_lanes ()
{
  if (size () < 2)
    return;

  const auto empty_pred = [] (const auto &lane) {
    return lane.get ()->is_empty ();
  };
  // Find the last non-matching element from the end
  auto last_non_matching =
    std::ranges::find_if_not (lanes_ | std::views::reverse, empty_pred);

  if (last_non_matching == lanes_.rend ())
    {
      // All elements match, keep only the first one
      remove_lanes (1, lanes_.size () - 1);
      return;
    }

  // Convert reverse iterator to forward iterator
  auto keep_from = last_non_matching.base ();

  // Keep the first matching element (highest index)
  if (keep_from != lanes_.end ())
    {
      ++keep_from; // Move past the last non-matching element
    }

  if (keep_from == lanes_.end ())
    return;

  const auto first_row =
    static_cast<size_t> (std::ranges::distance (lanes_.begin (), keep_from));
  remove_lanes (first_row, lanes_.size () - first_row);
}

void
TrackLaneList::clear ()
{
  if (!empty ())
    // Releasing these references deletes lanes that are not referenced
    // anywhere else (e.g. the default lanes each new track creates
    // before its serialized lanes are attached)
    remove_lanes (0, lanes_.size ());
}

void
TrackLaneList::erase (const size_t pos)
{
  if (pos < lanes_.size ())
    {
      remove_lanes (pos, 1);
      update_default_lane_names ();
    }
  else
    {
      throw std::out_of_range (
        fmt::format ("position {} out of range ({})", pos, lanes_.size ()));
    }
}

void
TrackLaneList::remove_lanes (const size_t first_row, const size_t count)
{
  if (count == 0)
    return;

  beginRemoveRows (
    QModelIndex (), static_cast<int> (first_row),
    static_cast<int> (first_row + count - 1));
  const auto from = std::ranges::next (lanes_.begin (), first_row);
  const auto to = std::ranges::next (from, static_cast<long> (count));
  for (auto it = from; it != to; ++it)
    it->get ()->set_owner_list (nullptr);
  lanes_.erase (from, to);
  endRemoveRows ();
}

void
TrackLaneList::update_default_lane_names ()
{
  for (const auto &[index, lane] : utils::views::enumerate (lanes_view ()))
    {
      if (
        auto scan_result = scn::scan<std::string> (
          utils::Utf8String::from_qstring (lane->name ()).view (),
          scn::runtime_format (
            utils::Utf8String::from_qstring (
              QObject::tr (TrackLane::default_format_str))
              .view ())))
        {
          lane->generate_name (index);
        }
    }
}

void
to_json (nlohmann::json &j, const TrackLaneList &p)
{
  j[TrackLaneList::kLaneIdsKey] = p.lanes_;
  j[TrackLaneList::kLanesVisibleKey] = p.lanes_visible_;
}

void
from_json (const nlohmann::json &j, TrackLaneList &p)
{
  // Dropping the list's references deletes lanes not referenced anywhere
  // else (the default lanes every constructed track creates), so the ids
  // this call attaches must not be ones the list itself already holds
  p.clear ();

  std::vector<QUuid> lane_ids;
  lane_ids.reserve (j.at (TrackLaneList::kLaneIdsKey).size ());
  for (const auto &lane_id_json : j.at (TrackLaneList::kLaneIdsKey))
    lane_ids.emplace_back (lane_id_json.get<QUuid> ());

  // Validate before touching the model or the vector: every laneIds entry
  // must reference a registered track lane exactly once (a lane may be
  // attached by several lists — the last one wins its owner pointer)
  std::set<QUuid> seen_lane_ids;
  for (const auto &lane_id : lane_ids)
    {
      if (!seen_lane_ids.insert (lane_id).second)
        {
          throw ZrythmException (
            fmt::format (
              "{} contains {} more than once", TrackLaneList::kLaneIdsKey,
              lane_id.toString ()));
        }
      const auto * lane = qobject_cast<TrackLane *> (
        p.dependencies_.registry_.find_by_raw_uuid (lane_id));
      if (lane == nullptr)
        {
          throw ZrythmException (
            fmt::format (
              "{} references {} which is not a registered track lane",
              TrackLaneList::kLaneIdsKey, lane_id.toString ()));
        }
    }

  if (!lane_ids.empty ())
    {
      p.beginInsertRows (
        QModelIndex (), 0, static_cast<int> (lane_ids.size () - 1));
      for (const auto &lane_id : lane_ids)
        {
          p.lanes_.emplace_back (
            TrackLane::Uuid{ lane_id }, p.dependencies_.registry_);
          auto * lane = p.lanes_.back ().get ();
          lane->set_owner_list (&p);
          lane->rewire_dependencies (p.dependencies_);
        }
      p.endInsertRows ();
    }

  bool lanes_visible = false;
  j.at (TrackLaneList::kLanesVisibleKey).get_to (lanes_visible);
  p.setLanesVisible (lanes_visible);
}

TrackLaneList::~TrackLaneList () = default;
}
