// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/port_identifier.h"
#include "structure/arrangement/arranger_object.h"
#include "structure/arrangement/arranger_object_list_model.h"

#define DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES( \
  ClassType, QPropertyName, ChildType) \
public: \
  /* ================================================================ */ \
  /* arrangerObjects */ \
  /* ================================================================ */ \
  Q_PROPERTY ( \
    structure::arrangement::ArrangerObjectListModel * QPropertyName READ \
      get##QPropertyName CONSTANT) \
  structure::arrangement::ArrangerObjectListModel * get##QPropertyName () const \
  { \
    return ArrangerObjectOwner<ChildType>::get_model (); \
  }

namespace zrythm::structure::arrangement
{
class LaneOwnedObject;
class Region;

template <FinalArrangerObjectSubclass ChildT> class ArrangerObjectOwner
{
public:
  using PortUuid = dsp::PortIdentifier::PortUuid;
  using ArrangerObjectChildType = ChildT;
  using TrackUuid = structure::tracks::TrackUuid;
  using ArrangerObjectListModel =
    structure::arrangement::ArrangerObjectListModel;

  ArrangerObjectOwner () = default;
  virtual ~ArrangerObjectOwner () { delete list_model_; }
  Z_DISABLE_COPY_MOVE (ArrangerObjectOwner)

  /**
   * @brief Location info of this class.
   */
  struct Location
  {
    std::optional<TrackUuid> track_id_;

    // Either lane index, region ID or control port ID (for automation regions)
    std::optional<std::variant<ArrangerObjectUuid, int, PortUuid>> owner_;
  };

  auto &get_children_vector () { return children_; }

  auto &get_children_vector () const { return children_; }

  auto get_children_view () const
  {
    const auto &vec_ref = get_children_vector ();
    return vec_ref | std::views::transform ([&] (const auto &id) {
             return std::get<ChildT *> (id.get_object ());
           });
  }

  auto &get_children_snapshots_vector () { return snapshots_; }
  auto &get_children_snapshots_vector () const { return snapshots_; }

  auto get_children_snapshots_view () const
  {
    const auto &vec_ref = get_children_snapshots_vector ();
    return vec_ref | std::views::transform ([&] (const auto &id) {
             return std::get<ChildT *> (id.get_object ());
           });
  }

  void add_ticks_to_children (double ticks, dsp::FramesPerTick frames_per_tick)
  {
    for (auto * child : get_children_view ())
      {
        child->move (ticks, frames_per_tick);
      }
  }

  ArrangerObjectListModel * get_model () const { return list_model_; }

  template <typename SelfT>
  ArrangerObjectUuidReference
  remove_object (this SelfT &self, const ArrangerObject::Uuid &id)
  {
    auto it_to_remove =
      std::ranges::find (self.children_, id, &ArrangerObjectUuidReference::id);
    if (it_to_remove == self.children_.end ())
      {
        throw std::runtime_error (
          fmt::format ("object to remove not found: {}", id));
      }
    z_trace ("removing object: {}", id);

    auto obj_ref = *it_to_remove;
    auto obj = std::get<ChildT *> (obj_ref.get_object ());
    if constexpr (std::is_same_v<ChildT, AutomationPoint>)
      {
        if (self.last_recorded_ap_ == obj)
          {
            self.last_recorded_ap_ = nullptr;
          }
      }

    const auto remove_idx =
      std::distance (self.children_.begin (), it_to_remove);
    self.list_model_->begin_remove_rows (remove_idx, remove_idx);
    self.children_.erase (it_to_remove);
    self.list_model_->end_remove_rows ();
    return obj_ref;
  }

  template <typename SelfT>
  void insert_object (
    this SelfT                        &self,
    const ArrangerObjectUuidReference &obj_ref,
    int                                idx)
  {
    assert (
      idx >= 0
      && static_cast<size_t> (idx)
           <= self.ArrangerObjectOwner<ChildT>::children_.size ());

    auto * obj = std::get<ChildT *> (obj_ref.get_object ());

    self.ArrangerObjectOwner<ChildT>::list_model_->begin_insert_rows (idx, idx);
    self.ArrangerObjectOwner<ChildT>::children_.insert (
      self.ArrangerObjectOwner<ChildT>::children_.begin () + idx, obj_ref);

    if constexpr (std::is_same_v<ChildT, AutomationRegion>)
      {
        obj->set_automation_track (self);
      }
    else if constexpr (std::derived_from<ChildT, LaneOwnedObject>)
      {
        obj->set_lane (&self);
      }

    obj->set_track_id (self.get_location (*obj).track_id_);

    self.ArrangerObjectOwner<ChildT>::list_model_->end_insert_rows ();
  }

  template <typename SelfT>
  void add_object (this SelfT &self, const ArrangerObjectUuidReference &obj_ref)
  {
    self.ArrangerObjectOwner<ChildT>::insert_object (
      obj_ref, self.ArrangerObjectOwner<ChildT>::children_.size ());
  }

  void clear_objects ()
  {
    list_model_->begin_reset_model ();
    {
      clearing_ = true;
      children_.clear ();
      snapshots_.clear ();
      clearing_ = false;
    }
    list_model_->end_reset_model ();
  }

  /**
   * @brief Returns the current location of this owner.
   *
   * To be used by e.g. undoable actions where we need to know where to put
   * back the object.
   *
   * @note The parameter is just used to disambiguate when this base class is
   * used twice or more in the same derived class.
   */
  virtual Location get_location (const ChildT &) const = 0;

  /**
   * @brief Get the children field name to be used during
   * serialization/deserialization.
   *
   * This is used because a class may derive from this multiple times so make
   * sure the field name is unique.
   *
   * @return std::string
   */
  virtual std::string
  get_field_name_for_serialization (const ChildT *) const = 0;

  void
  copy_members_from (const ArrangerObjectOwner &other, ObjectCloneType clone_type)
  {
    children_.reserve (other.children_.size ());
    // TODO
#if 0
  for (const auto &ap : other.get_children_view ())
    {
      const auto * clone =
        ap->clone_and_register (get_arranger_object_registry ());
      aps_.push_back (clone->get_uuid ());
    }
#endif
  }

  void copy_children (const ArrangerObjectOwner &other)
  {
    // TODO
  }

private:
  friend void to_json (nlohmann::json &j, const ArrangerObjectOwner &obj)
  {
    const auto children_field_name =
      obj.get_field_name_for_serialization (static_cast<ChildT *> (nullptr));
    j[children_field_name] = obj.children_;
  }
  friend void from_json (const nlohmann::json &j, ArrangerObjectOwner &obj)
  {
    const auto children_field_name =
      obj.get_field_name_for_serialization (static_cast<ChildT *> (nullptr));
    j.at (children_field_name).get_to (obj.children_);
  }

private:
  std::vector<ArrangerObjectUuidReference> children_;
  std::vector<ArrangerObjectUuidReference> snapshots_;
  ArrangerObjectListModel *                list_model_ =
    new ArrangerObjectListModel{ children_ };

protected:
  // TODO: remove if not needed - currently used by TrackLane
  bool clearing_{};

  BOOST_DESCRIBE_CLASS (ArrangerObjectOwner<ChildT>, (), (), (), (children_))
};
} // namespace zrythm::structure::arrangement
