// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef DSP_TRACK_LANE_LIST_H
#define DSP_TRACK_LANE_LIST_H

#include "gui/dsp/audio_lane.h"
#include "gui/dsp/midi_lane.h"
#include "gui/dsp/track_lane.h"

class TrackLaneList final
    : public QAbstractListModel,
      public zrythm::utils::serialization::ISerializable<TrackLaneList>
{
  Q_OBJECT
  QML_ELEMENT

public:
  enum Roles
  {
    TrackLanePtrRole = Qt::UserRole + 1,
  };

public:
  TrackLaneList (QObject * parent = nullptr);
  ~TrackLaneList () override;

  friend class LanedTrackImpl<AudioLane>;
  friend class LanedTrackImpl<MidiLane>;

  // ========================================================================
  // QML Interface
  // ========================================================================
  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  // ========================================================================

  void
  copy_members_from (const TrackLaneList &other, ObjectCloneType clone_type);

  [[nodiscard]] size_t size () const noexcept { return lanes_.size (); }

  [[nodiscard]] bool empty () const noexcept { return lanes_.empty (); }

  [[nodiscard]] auto at (size_t idx) const { return lanes_.at (idx); }

  void reserve (size_t size) { lanes_.reserve (size); }

  void push_back (const TrackLanePtrVariant lane)
  {
    const int idx = static_cast<int> (lanes_.size ());
    beginInsertRows (QModelIndex (), idx, idx);
    lanes_.push_back (lane);
    std::visit ([&] (auto &&l) { l->setParent (this); }, lane);
    endInsertRows ();
  }

  /** Removes last lane. */
  void pop_back ()
  {
    if (!empty ())
      {
        const int idx = static_cast<int> (lanes_.size () - 1);
        beginRemoveRows (QModelIndex (), idx, idx);
        auto lane = lanes_.back ();
        std::visit (
          [&] (auto &&l) {
            l->setParent (nullptr);
            lanes_.pop_back ();
            l->deleteLater ();
          },
          lane);

        endRemoveRows ();
      }
  }

  void clear ()
  {
    if (!empty ())
      {
        beginRemoveRows (
          QModelIndex (), 0, static_cast<int> (lanes_.size () - 1));
        for (auto &lane : lanes_)
          {
            std::visit (
              [&] (auto &&l) {
                l->setParent (nullptr);
                l->deleteLater ();
              },
              lane);
          }
        lanes_.clear ();
        endRemoveRows ();
      }
  }

  /** Iterator access. */
  [[nodiscard]] auto begin () noexcept { return lanes_.begin (); }

  [[nodiscard]] auto end () noexcept { return lanes_.end (); }

  [[nodiscard]] auto begin () const noexcept { return lanes_.begin (); }

  [[nodiscard]] auto end () const noexcept { return lanes_.end (); }

  void erase (const size_t pos)
  {
    if (pos < lanes_.size ())
      {
        beginRemoveRows (
          QModelIndex (), static_cast<int> (pos), static_cast<int> (pos));
        auto &lane = lanes_.at (pos);
        std::visit (
          [&] (auto &&l) {
            l->setParent (nullptr);
            lanes_.erase (lanes_.begin () + pos);
            l->deleteLater ();
          },
          lane);
        endRemoveRows ();
      }
    else
      {
        z_error ("position {} out of range ({})", pos, lanes_.size ());
      }
  }

  [[nodiscard]] auto front () noexcept { return lanes_.front (); }

  [[nodiscard]] auto back () noexcept { return lanes_.back (); }

  [[nodiscard]] auto front () const noexcept { return lanes_.front (); }

  [[nodiscard]] auto back () const noexcept { return lanes_.back (); }

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  std::vector<TrackLanePtrVariant> lanes_;
};

#endif
