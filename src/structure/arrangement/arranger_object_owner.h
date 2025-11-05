// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/file_audio_source.h"
#include "structure/arrangement/arranger_object.h"
#include "structure/arrangement/arranger_object_list_model.h"

#define DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES( \
  ClassType, QPropertyName, ChildType) \
public: \
  Q_PROPERTY ( \
    zrythm::structure::arrangement::ArrangerObjectListModel * QPropertyName \
      READ QPropertyName CONSTANT) \
  structure::arrangement::ArrangerObjectListModel * QPropertyName () const \
  { \
    return ArrangerObjectOwner<ChildType>::get_model (); \
  }

namespace zrythm::structure::arrangement
{
template <FinalArrangerObjectSubclass ChildT> class ArrangerObjectOwner
{
public:
  using ArrangerObjectChildType = ChildT;
  using ArrangerObjectListModel =
    structure::arrangement::ArrangerObjectListModel;

  template <typename T>
  explicit ArrangerObjectOwner (
    ArrangerObjectRegistry       &registry,
    dsp::FileAudioSourceRegistry &file_audio_source_registry,
    T                            &derived)
    requires std::derived_from<T, QObject>
      : registry_ (registry),
        file_audio_source_registry_ (file_audio_source_registry)
  {
    if constexpr (std::derived_from<T, ArrangerObject>)
      {
        list_model_ = utils::make_qobject_unique<ArrangerObjectListModel> (
          children_, derived);
      }
    else
      {
        list_model_ = utils::make_qobject_unique<ArrangerObjectListModel> (
          children_, &derived);
      }
  }
  virtual ~ArrangerObjectOwner () = default;
  Z_DISABLE_COPY_MOVE (ArrangerObjectOwner)

  auto &get_children_vector () const
  {
    return children_.get<random_access_index> ();
  }

  auto get_children_view () const
  {
    const auto &vec_ref = children_.get<random_access_index> ();
    return vec_ref
           | std::views::transform (
             &ArrangerObjectUuidReference::get_object_as<ChildT>);
  }

  auto get_sorted_children_view () const
  {
    const auto &vec_ref = children_.get<sorted_index> ();
    return vec_ref
           | std::views::transform (
             &ArrangerObjectUuidReference::get_object_as<ChildT>);
  }

  void add_ticks_to_children (double ticks)
  {
    for (auto * child : get_children_view ())
      {
        child->position ()->setTicks (child->position ()->ticks () + ticks);
      }
  }

  /**
   * @brief O(1) lookup if an object with the given ID exists.
   */
  bool contains_object (const ArrangerObject::Uuid &id) const
  {
    return children_.get<uuid_hash_index> ().contains (id);
  }

  ArrangerObjectListModel * get_model () const { return list_model_.get (); }

  template <typename SelfT>
  ArrangerObjectUuidReference
  remove_object (this SelfT &self, const ArrangerObject::Uuid &id)
  {
    auto &container = self.ArrangerObjectOwner<ChildT>::children_;
    auto &random_access_idx = container.template get<random_access_index> ();
    auto  it_to_remove = std::ranges::find (
      random_access_idx, id, &ArrangerObjectUuidReference::id);
    if (it_to_remove == random_access_idx.end ())
      {
        throw std::runtime_error (
          fmt::format ("object to remove not found: {}", id));
      }
    z_trace ("removing object: {}", id);

    auto       obj_ref = *it_to_remove;
    const auto remove_idx =
      std::distance (random_access_idx.begin (), it_to_remove);

    // Remove from model first to handle Qt signals properly
    self.ArrangerObjectOwner<ChildT>::list_model_->removeRows (
      static_cast<int> (remove_idx), 1);

    return obj_ref;
  }

  template <typename SelfT>
  void insert_object (
    this SelfT                        &self,
    const ArrangerObjectUuidReference &obj_ref,
    int                                idx)
  {
    self.ArrangerObjectOwner<ChildT>::list_model_->insertObject (obj_ref, idx);
  }

  template <typename SelfT>
  void add_object (this SelfT &self, const ArrangerObjectUuidReference &obj_ref)
  {
    self.ArrangerObjectOwner<ChildT>::insert_object (
      obj_ref,
      static_cast<int> (self.ArrangerObjectOwner<ChildT>::children_.size ()));
  }

  void clear_objects () { list_model_->clear (); }

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

  friend void init_from (
    ArrangerObjectOwner       &obj,
    const ArrangerObjectOwner &other,
    utils::ObjectCloneType     clone_type)
  {
    if (clone_type == utils::ObjectCloneType::NewIdentity)
      {
        obj.children_.reserve (other.children_.size ());
        for (const auto &child : other.get_children_view ())
          {
            // FIXME: not ideal to have child-specific logic here...
            std::optional<ArrangerObjectUuidReference> clone_ref;
            if constexpr (std::is_same_v<ChildT, AudioSourceObject>)
              {
                auto dummy_file_source_ref =
                  obj.file_audio_source_registry_.create_object<
                    dsp::FileAudioSource> (
                    utils::audio::AudioBuffer (2, 16),
                    utils::audio::BitDepth::BIT_DEPTH_32, 44100, 120.0,
                    u8"Unused dummy Audio Source");
                clone_ref = obj.registry_.clone_object (
                  *child, child->get_tempo_map (),
                  obj.file_audio_source_registry_, dummy_file_source_ref);
                obj.children_.get<sequenced_index> ().emplace_back (
                  std::move (*clone_ref));
              }
            else if constexpr (RegionObject<ChildT>)
              {
                // TODO
              }
            else if constexpr (std::is_same_v<ChildT, Marker>)
              {
                clone_ref = obj.registry_.clone_object (
                  *child, child->get_tempo_map (), child->markerType ());
                obj.children_.get<sequenced_index> ().emplace_back (
                  std::move (*clone_ref));
              }
            else
              {
                clone_ref =
                  obj.registry_.clone_object (*child, child->get_tempo_map ());
                obj.children_.get<sequenced_index> ().emplace_back (
                  std::move (*clone_ref));
              }
          }
      }
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
    for (const auto &child_json : j.at (children_field_name))
      {
        const auto uuid =
          child_json.at (ArrangerObjectUuidReference::kIdKey)
            .template get<ArrangerObjectUuid> ();
        ArrangerObjectUuidReference obj_ref{ uuid, obj.registry_ };
        obj.children_.get<sequenced_index> ().emplace_back (std::move (obj_ref));
      }
  }

private:
  ArrangerObjectRegistry                          &registry_;
  dsp::FileAudioSourceRegistry                    &file_audio_source_registry_;
  ArrangerObjectRefMultiIndexContainer             children_;
  utils::QObjectUniquePtr<ArrangerObjectListModel> list_model_;

  BOOST_DESCRIBE_CLASS (ArrangerObjectOwner<ChildT>, (), (), (), (children_))
};
} // namespace zrythm::structure::arrangement
