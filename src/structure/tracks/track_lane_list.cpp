// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/track_lane_list.h"

#include <scn/scan.h>

namespace zrythm::structure::tracks
{
TrackLaneList::TrackLaneList (
  structure::arrangement::ArrangerObjectRegistry &obj_registry,
  dsp::FileAudioSourceRegistry                   &file_audio_source_registry,
  QObject *                                       parent)
    : QAbstractListModel (parent),
      dependencies_ (
        TrackLane::TrackLaneDependencies{
          .obj_registry_ = obj_registry,
          .file_audio_source_registry_ = file_audio_source_registry,
          .soloed_lanes_exist_func_ = [this] () {
            return std::ranges::any_of (lanes_view (), &TrackLane::soloed);
          } })
{
  QObject::connect (
    this, &TrackLaneList::rowsInserted, this,
    [this] (const QModelIndex &, int first, int last) {
      for (int i = first; i <= last; ++i)
        {
          const auto &lane = lanes ().at (i);
          QObject::connect (
            lane->arrangement::ArrangerObjectOwner<
              arrangement::MidiRegion>::get_model (),
            &arrangement::ArrangerObjectListModel::contentChanged, this,
            &TrackLaneList::laneObjectsNeedRecache, Qt::QueuedConnection);
          QObject::connect (
            lane->arrangement::ArrangerObjectOwner<
              arrangement::AudioRegion>::get_model (),
            &arrangement::ArrangerObjectListModel::contentChanged, this,
            &TrackLaneList::laneObjectsNeedRecache, Qt::QueuedConnection);
        }
    });
  QObject::connect (
    this, &TrackLaneList::rowsAboutToBeRemoved, this,
    [this] (const QModelIndex &, int first, int last) {
      for (int i = first; i <= last; ++i)
        {
          const auto &lane = lanes ().at (i);
          QObject::disconnect (
            lane->arrangement::ArrangerObjectOwner<
              arrangement::MidiRegion>::get_model (),
            &arrangement::ArrangerObjectListModel::contentChanged, this,
            &TrackLaneList::laneObjectsNeedRecache);
          QObject::disconnect (
            lane->arrangement::ArrangerObjectOwner<
              arrangement::AudioRegion>::get_model (),
            &arrangement::ArrangerObjectListModel::contentChanged, this,
            &TrackLaneList::laneObjectsNeedRecache);
        }
    });
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
      return lane->name ();
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

  beginInsertRows (
    QModelIndex (), static_cast<int> (index), static_cast<int> (index));
  lanes_.insert (
    std::next (std::begin (lanes_), static_cast<int> (index)),
    utils::make_qobject_unique<TrackLane> (dependencies_, this));
  endInsertRows ();

  auto * lane = lanes_.at (index).get ();
  lane->generate_name (index);
  update_default_lane_names ();
  return lane;
}

void
TrackLaneList::removeLane (size_t index)
{
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
TrackLaneList::fill_events_callback (
  const dsp::ITransport                        &transport,
  const EngineProcessTimeInfo                  &time_nfo,
  dsp::MidiEventVector *                        midi_events,
  std::optional<TrackProcessor::StereoPortPair> stereo_ports)
{
  for (const auto &lane : lanes ())
    {
      for (
        const auto * r :
        lane->arrangement::ArrangerObjectOwner<
          arrangement::AudioRegion>::get_children_view ())
        {
          TrackProcessor::fill_events_from_region_rt (
            transport, time_nfo, midi_events, stereo_ports, *r);
        }
    }
}

void
TrackLaneList::create_missing_lanes (size_t index)
{
  while ((index + 2) > lanes_.size ())
    {
      addLane ();
    }
}

void
TrackLaneList::remove_empty_last_lanes ()
{
  if (size () < 2)
    return;

  const auto empty_pred = [] (const auto &lane) {
    return lane->midiRegions ()->rowCount () == 0;
  };
  // Find the last non-matching element from the end
  auto last_non_matching =
    std::ranges::find_if_not (lanes_ | std::views::reverse, empty_pred);

  if (last_non_matching == lanes_.rend ())
    {
      // All elements match, keep only the first one
      beginRemoveRows ({}, 1, static_cast<int> (lanes_.size () - 1));
      lanes_.erase (lanes_.begin () + 1, lanes_.end ());
      endRemoveRows ();
      return;
    }

  // Convert reverse iterator to forward iterator
  auto keep_from = last_non_matching.base ();

  // Keep the first matching element (highest index)
  if (keep_from != lanes_.end ())
    {
      ++keep_from; // Move past the last non-matching element
    }

  // Erase from keep_from to end
  beginRemoveRows (
    {}, static_cast<int> (std::ranges::distance (lanes_.begin (), keep_from)),
    static_cast<int> (lanes_.size ()));
  lanes_.erase (keep_from, lanes_.end ());
  endRemoveRows ();
}

void
TrackLaneList::erase (const size_t pos)
{
  if (pos < lanes_.size ())
    {
      beginRemoveRows (
        QModelIndex (), static_cast<int> (pos), static_cast<int> (pos));
      lanes_.erase (
        lanes_.begin () + static_cast<decltype (lanes_)::difference_type> (pos));
      update_default_lane_names ();
      endRemoveRows ();
    }
  else
    {
      throw std::out_of_range (
        fmt::format ("position {} out of range ({})", pos, lanes_.size ()));
    }
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
init_from (
  TrackLaneList         &obj,
  const TrackLaneList   &other,
  utils::ObjectCloneType clone_type)
{
  obj.clear ();
  obj.lanes_.reserve (other.size ());
  // TODO
#if 0
  for (const auto lane_var : other)
    {
      std::visit (
        [&] (auto &&lane) {
          auto * new_lane = lane->clone_raw_ptr ();
          push_back (new_lane);
        },
        lane_var);
    }
#endif
}
}
