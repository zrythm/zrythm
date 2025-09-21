// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "utils/format.h"
#include "utils/icloneable.h"
#include "utils/logger.h"
#include "utils/rt_thread_id.h"
#include "utils/serialization.h"
#include "utils/string.h"

#include <QUuid>

#include "type_safe/strong_typedef.hpp"
#include <boost/stl_interfaces/iterator_interface.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <nlohmann/json.hpp>

namespace zrythm::utils
{
/**
 * Base class for objects that need to be uniquely identified by UUID.
 */
template <typename Derived> class UuidIdentifiableObject
{
public:
  struct Uuid final
      : type_safe::strong_typedef<Uuid, QUuid>,
        type_safe::strong_typedef_op::equality_comparison<Uuid>,
        type_safe::strong_typedef_op::relational_comparison<Uuid>
  {
    using UuidTag = void; // used by the fmt formatter below
    using type_safe::strong_typedef<Uuid, QUuid>::strong_typedef;

    explicit Uuid () = default;

    static_assert (StrongTypedef<Uuid>);
    // static_assert (type_safe::is_strong_typedef<Uuid>::value);
    // static_assert (std::is_same_v<type_safe::underlying_type<Uuid>, QUuid>);

    /**
     * @brief Checks if the UUID is null.
     */
    bool is_null () const { return type_safe::get (*this).isNull (); }

    void set_null () { type_safe::get (*this) = QUuid (); }

    std::size_t hash () const { return qHash (type_safe::get (*this)); }
  };
  static_assert (std::regular<Uuid>);
  static_assert (sizeof (Uuid) == sizeof (QUuid));

  UuidIdentifiableObject () : uuid_ (Uuid (QUuid::createUuid ())) { }
  UuidIdentifiableObject (const Uuid &id) : uuid_ (id) { }
  UuidIdentifiableObject (const UuidIdentifiableObject &other) = default;
  UuidIdentifiableObject &
  operator= (const UuidIdentifiableObject &other) = default;
  UuidIdentifiableObject (UuidIdentifiableObject &&other) = default;
  UuidIdentifiableObject &operator= (UuidIdentifiableObject &&other) = default;
  virtual ~UuidIdentifiableObject () = default;

  auto get_uuid () const { return uuid_; }

  friend void init_from (
    UuidIdentifiableObject       &obj,
    const UuidIdentifiableObject &other,
    utils::ObjectCloneType        clone_type)
  {
    if (clone_type == utils::ObjectCloneType::NewIdentity)
      {
        obj.uuid_ = Uuid (QUuid::createUuid ());
      }
    else
      {
        obj.uuid_ = other.uuid_;
      }
  }

  friend void to_json (nlohmann::json &j, const UuidIdentifiableObject &obj)
  {
    j[kUuidKey] = obj.uuid_;
  }
  friend void from_json (const nlohmann::json &j, UuidIdentifiableObject &obj)
  {
    j.at (kUuidKey).get_to (obj.uuid_);
  }

  friend bool operator== (
    const UuidIdentifiableObject &lhs,
    const UuidIdentifiableObject &rhs)
  {
    return lhs.uuid_ == rhs.uuid_;
  }

private:
  static constexpr std::string_view kUuidKey = "id";

  Uuid uuid_;

  BOOST_DESCRIBE_CLASS (UuidIdentifiableObject, (), (), (), (uuid_))
};

template <typename T>
concept UuidIdentifiable = std::derived_from<T, UuidIdentifiableObject<T>>;

template <typename T>
concept UuidIdentifiableQObject =
  UuidIdentifiable<T> && std::derived_from<T, QObject>;

// specialization for std::hash and boost::hash
#define DEFINE_UUID_HASH_SPECIALIZATION(UuidType) \
  namespace std \
  { \
  template <> struct hash<UuidType> \
  { \
    size_t operator() (const UuidType &uuid) const \
    { \
      return uuid.hash (); \
    } \
  }; \
  } \
  namespace boost \
  { \
  template <> struct hash<UuidType> \
  { \
    size_t operator() (const UuidType &uuid) const \
    { \
      return uuid.hash (); \
    } \
  }; \
  }

/**
 * @brief A reference-counted RAII wrapper for a UUID in a registry.
 *
 * Objects that refer to an object's UUID must use this wrapper.
 *
 * TODO: actually use this
 *
 * @tparam RegistryT
 */
template <typename RegistryT> class UuidReference
{
public:
  using UuidType = typename RegistryT::UuidType;
  using VariantType = typename RegistryT::VariantType;

  static constexpr auto kIdKey = "id"sv;

  // unengaged - need to call set_id to acquire a reference
  UuidReference (RegistryT &registry) : registry_ (registry) { }

  UuidReference (const UuidType &id, RegistryT &registry)
      : id_ (id), registry_ (registry)
  {
    acquire_ref ();
  }

  UuidReference (const UuidReference &other)
      : UuidReference (other.get_registry ())
  {
    if (other.id_.has_value ())
      {
        set_id (other.id_.value ());
      }
  }
  UuidReference &operator= (const UuidReference &other)
  {
    if (this != &other)
      {
        id_ = other.id_;
        registry_ = other.registry_;
        acquire_ref ();
      }
    return *this;
  }

  UuidReference (UuidReference &&other)
      : id_ (std::exchange (other.id_, std::nullopt)),
        registry_ (std::exchange (other.registry_, std::nullopt))
  {
    // No need to acquire_ref() here because we're taking over the reference
    // from 'other' which already holds it
  }

  UuidReference &operator= (UuidReference &&other)
  {
    if (this != &other)
      {
        // Release our current reference (if any)
        release_ref ();

        // Take ownership from other
        id_ = std::exchange (other.id_, std::nullopt);
        registry_ = std::exchange (other.registry_, std::nullopt);

        // No need to acquire_ref() - we're taking over the reference
      }
    return *this;
  }

  ~UuidReference () { release_ref (); }

  auto id () const -> UuidType { return *id_; }

  /**
   * @brief To be used when using the Registry-only constructor.
   *
   * This is useful when deserializing UuidReference types: First construct with
   * just the registry, then from_json() should call this to set the ID and
   * acquire a reference.
   */
  void set_id (const UuidType &id)
  {
    if (id_.has_value ())
      {
        throw std::runtime_error (
          "Cannot set id of UuidReference that already has an id");
      }
    id_ = id;
    acquire_ref ();
  }

  VariantType get_object () const
  {
    return get_registry ().find_by_id_or_throw (id ());
  }

  // Convenience getter
  template <typename ObjectT>
  ObjectT * get_object_as () const
    requires IsInVariant<ObjectT *, VariantType>
  {
    return std::get<ObjectT *> (get_object ());
  }

  RegistryT::BaseType * get_object_base () const
  {
    return get_registry ().find_by_id_as_base_or_throw (id ());
  }

  template <typename... Args>
  UuidReference clone_new_identity (Args &&... args) const
  {
    return std::visit (
      [&] (auto &&obj) {
        return get_registry ().clone_object (*obj, std::forward<Args> (args)...);
      },
      get_object ());
  }

  auto get_iterator_in_registry () const
  {
    return get_registry ().get_iterator_for_id (id ());
  }

  auto get_iterator_in_registry ()
  {
    return get_registry ().get_iterator_for_id (id ());
  }

private:
  void acquire_ref ()
  {
    if (id_.has_value ())
      {
        get_registry ().acquire_reference (*id_);
      }
  }
  void release_ref ()
  {
    if (id_.has_value ())
      {
        get_registry ().release_reference (*id_);
      }
  }
  RegistryT &get_registry () const
  {
    return const_cast<RegistryT &> (*registry_);
  }
  RegistryT &get_registry () { return *registry_; }

  friend void to_json (nlohmann::json &j, const UuidReference &ref)
  {
    assert (ref.id_.has_value ());
    j[kIdKey] = *ref.id_;
  }
  friend void from_json (const nlohmann::json &j, UuidReference &ref)
  {
    auto tmp = ref;
    j.at (kIdKey).get_to (ref.id_);
    ref.acquire_ref ();
    tmp.release_ref ();
  }

  friend bool operator== (const UuidReference &lhs, const UuidReference &rhs)
  {
    return lhs.id () == rhs.id ();
  }

private:
  std::optional<UuidType> id_;
  OptionalRef<RegistryT>  registry_;

  BOOST_DESCRIBE_CLASS (UuidReference<RegistryT>, (), (), (), (id_))
};

template <typename ReturnType, typename UuidType>
using UuidIdentifiablObjectResolver =
  std::function<ReturnType (const UuidType &)>;

/**
 * @brief A registry that owns and manages objects identified by a UUID.
 *
 * This class provides methods to register, unregister, and find objects by
 * their UUID. When an object is registered, this class takes ownership of it
 * and sets the parent to `this`. When an object is unregistered, the ownership
 * is released and the caller is responsible for deleting the object.
 *
 * Intended to be used with variants of pointers to QObjects.
 *
 * @note The registry API is not thread-safe. Registry construction, object
 * registration and deregistration, etc., must only be done from the main
 * thread.
 *
 * @tparam VariantT A variant of raw pointers to QObject-derived classes.
 * @tparam BaseT The common base class of the objects in the variant.
 */
template <typename VariantT, UuidIdentifiable BaseT>
class OwningObjectRegistry : public QObject
{
public:
  using UuidType = typename UuidIdentifiableObject<BaseT>::Uuid;
  using VariantType = VariantT;
  using BaseType = BaseT;

  OwningObjectRegistry (QObject * parent = nullptr)
      : QObject (parent), main_thread_id_ (current_thread_id)
  {
  }

  // ========================================================================
  // QML/QObject Interface
  // ========================================================================

  // TODO signals...

  // ========================================================================

  // Factory method that forwards constructor arguments
  template <typename CreateType, typename... Args>
  auto create_object (Args &&... args) -> UuidReference<OwningObjectRegistry>
    requires std::derived_from<CreateType, BaseT>
  {
    assert (is_main_thread ());

    auto * obj = new CreateType (std::forward<Args> (args)...);
    z_trace (
      "created object of type {} with ID {}", typeid (CreateType).name (),
      obj->get_uuid ());
    register_object (obj);
    return { obj->get_uuid (), *this };
  }

  // Creates a clone of the given object and registers it to the registry.
  template <typename CreateType, typename... Args>
  auto clone_object (const CreateType &other, Args &&... args)
    -> UuidReference<OwningObjectRegistry>
    requires std::derived_from<CreateType, BaseT>
  {
    assert (is_main_thread ());

    CreateType * obj = clone_raw_ptr (
      other, ObjectCloneType::NewIdentity, std::forward<Args> (args)...);
    register_object (obj);
    return { obj->get_uuid (), *this };
  }

  [[gnu::hot]] auto get_iterator_for_id (const UuidType &id) const
  {
    // TODO: some failures left to fix
    // assert (is_main_thread ());

    return objects_by_id_.find (id);
  }

  [[gnu::hot]] auto get_iterator_for_id (const UuidType &id)
  {
    // assert (is_main_thread ());

    return objects_by_id_.find (id);
  }

  /**
   * @brief Returns an object by id.
   *
   * @return A non-owning pointer to the object.
   */
  [[gnu::hot]] std::optional<VariantT>
  find_by_id (const UuidType &id) const noexcept [[clang::nonblocking]]
  {
    const auto it = get_iterator_for_id (id);
    if (it == objects_by_id_.end ())
      {
        return std::nullopt;
      }
    return it->second;
  }

  [[gnu::hot]] auto find_by_id_or_throw (const UuidType &id) const
  {
    auto val = find_by_id (id);
    if (!val.has_value ())
      {
        throw std::runtime_error (
          fmt::format ("Object with id {} not found", id));
      }
    return val.value ();
  }

  BaseT * find_by_id_as_base_or_throw (const UuidType &id) const
  {
    auto var = find_by_id_or_throw (id);
    return std::visit ([] (auto &&val) -> BaseT * { return val; }, var);
  }

  bool contains (const UuidType &id) const
  {
    // assert (is_main_thread ());

    return objects_by_id_.contains (id);
  }

  /**
   * @brief Registers an object.
   *
   * This takes ownership of the object.
   */
  void register_object (VariantT obj_ptr)
  {
    assert (is_main_thread ());

    std::visit (
      [&] (auto &&obj) {
        if (contains (obj->get_uuid ()))
          {
            throw std::runtime_error (
              fmt::format ("Object with id {} already exists", obj->get_uuid ()));
          }
        z_trace ("Registering (inserting) object {}", obj->get_uuid ());
        obj->setParent (this);
        objects_by_id_.emplace (obj->get_uuid (), obj);
      },
      obj_ptr);
  }

  template <typename ObjectT>
  void register_object (ObjectT &obj)
    requires std::derived_from<ObjectT, BaseT>
  {
    register_object (&obj);
  }

  /**
   * @brief Returns the reference count of an object.
   *
   * Mainly intended for debugging.
   */
  auto reference_count (const UuidType &id) const
  {
    assert (is_main_thread ());

    return ref_counts_.at (id);
  }

  void acquire_reference (const UuidType &id)
  {
    assert (is_main_thread ());

    ref_counts_[id]++;
  }

  void release_reference (const UuidType &id)
  {
    assert (is_main_thread ());

    if (--ref_counts_[id] <= 0)
      {
        delete_object_by_id (id);
      }
  }

  auto &get_hash_map () const { return objects_by_id_; }

  /**
   * @brief Returns a list of all UUIDs of the objects in the registry.
   */
  auto get_uuids () const
  {
    assert (is_main_thread ());

    return objects_by_id_ | std::views::keys | std::ranges::to<std::vector> ();
  }

  size_t size () const
  {
    assert (is_main_thread ());

    return objects_by_id_.size ();
  }

  friend void to_json (nlohmann::json &j, const OwningObjectRegistry &obj)
  {
    auto objects = nlohmann::json::array ();
    for (const auto &var : obj.objects_by_id_ | std::views::values)
      {
        objects.push_back (var);
      }
    j[kObjectsKey] = objects;
  }
  template <ObjectBuilder BuilderT>
  friend void from_json_with_builder (
    const nlohmann::json &j,
    OwningObjectRegistry &obj,
    const BuilderT       &builder)
  {
    auto objects = j.at (kObjectsKey);
    for (const auto &object_var_json : objects)
      {
        VariantT object_var;
        utils::serialization::variant_from_json_with_builder (
          object_var_json, object_var, builder);
        obj.register_object (object_var);
      }
  }

private:
  static constexpr const char * kObjectsKey = "objectsById";

  /**
   * @brief Unregisters an object.
   *
   * This releases ownership of the object and the caller is responsible for
   * deleting it.
   */
  VariantT unregister_object (const UuidType &id)
  {
    if (!objects_by_id_.contains (id))
      {
        throw std::runtime_error (
          fmt::format ("Object with id {} not found", id));
      }

    z_trace ("Unregistering object with id {}", id);

    auto obj_it = get_iterator_for_id (id);
    auto obj_var = obj_it->second;
    objects_by_id_.erase (obj_it);
    std::visit ([&] (auto &&obj) { obj->setParent (nullptr); }, obj_var);
    ref_counts_.erase (id);
    return obj_var;
  }

  void delete_object_by_id (const UuidType &id)
  {
    auto obj_var = unregister_object (id);
    std::visit ([&] (auto &&obj) { delete obj; }, obj_var);
  }

  bool is_main_thread () const { return main_thread_id_ == current_thread_id; }

private:
  /**
   * @brief Hash map of the objects.
   *
   * @note Must allow realtime-safe (no locks) concurrent reads.
   */
  boost::unordered::unordered_flat_map<UuidType, VariantT> objects_by_id_;

  /**
   * @brief Reference counts for each hash.
   *
   * @note Only the main thread may read or write to this map.
   */
  boost::unordered::unordered_flat_map<UuidType, int> ref_counts_;

  RTThreadId main_thread_id_;
};

#define DEFINE_UUID_IDENTIFIABLE_OBJECT_SELECTION_MANAGER_QML_PROPERTIES( \
  ClassType, FullyQualifiedChildBaseType) \
public: \
  static_assert ( \
    std::is_same_v<FullyQualifiedChildBaseType, RegistryType::BaseType>); \
  Q_SIGNAL void selectionChanged (); \
  Q_SIGNAL void objectSelectionStatusChanged ( \
    RegistryType::BaseType * object, bool selected); \
  Q_SIGNAL void lastSelectedObjectChanged (); \
  Q_PROPERTY (/* NOLINTNEXTLINE */ \
              FullyQualifiedChildBaseType * lastSelectedObject READ \
                lastSelectedObject NOTIFY lastSelectedObjectChanged) \
  Q_INVOKABLE bool isSelected (RegistryType::BaseType * object) \
  { \
    return is_selected (object->get_uuid ()); \
  } \
  Q_INVOKABLE void selectUnique (RegistryType::BaseType * object) \
  { \
    select_unique (object->get_uuid ()); \
  } \
  Q_INVOKABLE void addToSelection (RegistryType::BaseType * object) \
  { \
    append_to_selection (object->get_uuid ()); \
  } \
  Q_INVOKABLE void removeFromSelection (RegistryType::BaseType * object) \
  { \
    remove_from_selection (object->get_uuid ()); \
  }

template <typename RegistryT> class UuidIdentifiableObjectSelectionManager
{
  using UuidType = typename RegistryT::UuidType;

public:
  using RegistryType = RegistryT;
  using UuidSet = std::unordered_set<UuidType>;

  UuidIdentifiableObjectSelectionManager (const RegistryT &registry)
      : registry_ (registry)
  {
  }

  RegistryT::BaseType * lastSelectedObject () const
  {
    if (last_selected_object_.has_value ())
      return get_object_base (last_selected_object_.value ());

    return nullptr;
  }

  void append_to_selection (this auto &&self, const UuidType &id)
  {
    if (!self.is_selected (id))
      {
        self.selected_objects_.insert (id);
        self.last_selected_object_ = id;
        Q_EMIT self.objectSelectionStatusChanged (
          self.get_object_base (id), true);
        Q_EMIT self.lastSelectedObjectChanged ();
        Q_EMIT self.selectionChanged ();
      }
  }
  void remove_from_selection (this auto &&self, const UuidType &id)
  {
    if (self.is_selected (id))
      {
        self.selected_objects_.erase (id);
        if (
          self.last_selected_object_.has_value ()
          && self.last_selected_object_.value () == id)
          {
            self.last_selected_object_.reset ();
            Q_EMIT self.lastSelectedObjectChanged ();
          }

        Q_EMIT self.objectSelectionStatusChanged (
          self.get_object_base (id), false);
        Q_EMIT self.selectionChanged ();
      }
  }
  void select_unique (this auto &&self, const UuidType &id)
  {
    self.clear_selection ();
    self.append_to_selection (id);
  }
  bool is_selected (const UuidType &id) const
  {
    return selected_objects_.contains (id);
  }
  bool is_only_selection (const UuidType &id) const
  {
    return selected_objects_.size () == 1 && is_selected (id);
  }
  bool empty () const { return selected_objects_.empty (); }
  auto size () const { return selected_objects_.size (); }

  void clear_selection (this auto &&self)
  { // Make a copy of the selected tracks to iterate over
    auto selected_objs_copy = self.selected_objects_;
    for (const auto &uuid : selected_objs_copy)
      {
        self.remove_from_selection (uuid);
      }
  }

  template <RangeOf<UuidType> UuidRange>
  void select_only_these (this auto &&self, const UuidRange &uuids)
  {
    self.clear_selection ();
    for (const auto &uuid : uuids)
      {
        self.append_to_selection (uuid);
      }
  }

private:
  RegistryT::BaseType * get_object_base (const UuidType &id) const
  {
    return registry_.find_by_id_as_base_or_throw (id);
  }

private:
  UuidSet                 selected_objects_;
  std::optional<UuidType> last_selected_object_;
  const RegistryT        &registry_;
};

/**
 * @brief A unified view over UUID-identified objects that supports:
 * - Range of VariantType (direct object pointers)
 * - Range of UuidReference
 * - Range of Uuid + Registry
 */
template <typename RegistryT>
class UuidIdentifiableObjectView
    : public std::ranges::view_interface<UuidIdentifiableObjectView<RegistryT>>
{
public:
  using UuidType = typename RegistryT::UuidType;
  using VariantType = typename RegistryT::VariantType;
  using UuidRefType = UuidReference<RegistryT>;

  // Proxy iterator implementation using Boost.STLInterfaces
  class Iterator
      : public boost::stl_interfaces::proxy_iterator_interface<
#if !BOOST_STL_INTERFACES_USE_DEDUCED_THIS
          Iterator,
#endif
          std::random_access_iterator_tag,
          VariantType>
  {
  public:
    using base_type = boost::stl_interfaces::proxy_iterator_interface<
#if !BOOST_STL_INTERFACES_USE_DEDUCED_THIS
      Iterator,
#endif
      std::random_access_iterator_tag,
      VariantType>;
    using difference_type = base_type::difference_type;

    constexpr Iterator () noexcept = default;

    // Constructor for direct object range
    constexpr Iterator (std::span<const VariantType>::iterator it) noexcept
        : it_var_ (it)
    {
    }

    // Constructor for UuidReference range
    constexpr Iterator (std::span<const UuidRefType>::iterator it) noexcept
        : it_var_ (it)
    {
    }

    // Constructor for Uuid + Registry range
    constexpr Iterator (
      std::span<const UuidType>::iterator it,
      const RegistryT *                   registry) noexcept
        : it_var_ (std::pair{ it, registry })
    {
    }

    constexpr VariantType operator* () const
    {
      return std::visit (
        [] (auto &&arg) {
          using T = std::decay_t<decltype (arg)>;
          if constexpr (
            std::is_same_v<T, typename std::span<const VariantType>::iterator>)
            {
              return *arg;
            }
          else if constexpr (
            std::is_same_v<T, typename std::span<const UuidRefType>::iterator>)
            {
              return arg->get_object ();
            }
          else if constexpr (
            std::is_same_v<
              T,
              std::pair<
                typename std::span<const UuidType>::iterator, const RegistryT *>>)
            {
              return arg.second->find_by_id_or_throw (*arg.first);
            }
        },
        it_var_);
    }

    constexpr Iterator &operator+= (difference_type n) noexcept
    {
      std::visit (
        [n] (auto &&arg) {
          using T = std::decay_t<decltype (arg)>;
          if constexpr (
            std::is_same_v<T, typename std::span<const VariantType>::iterator>)
            {
              arg += n;
            }
          else if constexpr (
            std::is_same_v<T, typename std::span<const UuidRefType>::iterator>)
            {
              arg += n;
            }
          else if constexpr (
            std::is_same_v<
              T,
              std::pair<
                typename std::span<const UuidType>::iterator, const RegistryT *>>)
            {
              arg.first += n;
            }
        },
        it_var_);
      return *this;
    }

    constexpr difference_type operator- (Iterator other) const
    {
      return std::visit (
        [] (auto &&arg, auto &&other_arg) -> difference_type {
          using T = std::decay_t<decltype (arg)>;
          using OtherT = std::decay_t<decltype (other_arg)>;
          if constexpr (!std::is_same_v<T, OtherT>)
            {
              throw std::runtime_error (
                "Comparing iterators of different types");
            }
          else if constexpr (
            std::is_same_v<T, typename std::span<const VariantType>::iterator>)
            {
              return arg - other_arg;
            }
          else if constexpr (
            std::is_same_v<T, typename std::span<const UuidRefType>::iterator>)
            {
              return arg - other_arg;
            }
          else if constexpr (
            std::is_same_v<
              T,
              std::pair<
                typename std::span<const UuidType>::iterator, const RegistryT *>>)
            {
              return arg.first - other_arg.first;
            }
          else
            {
              return 0;
            }
        },
        it_var_, other.it_var_);
    }

    constexpr auto operator<=> (const Iterator &other) const
    {
      return std::visit (
        [] (auto &&arg, auto &&other_arg) -> std::strong_ordering {
          using T = std::decay_t<decltype (arg)>;
          using OtherT = std::decay_t<decltype (other_arg)>;
          if constexpr (!std::is_same_v<T, OtherT>)
            {
              throw std::runtime_error (
                "Comparing iterators of different types");
            }
          else if constexpr (
            std::is_same_v<T, typename std::span<const VariantType>::iterator>)
            {
              return arg <=> other_arg;
            }
          else if constexpr (
            std::is_same_v<T, typename std::span<const UuidRefType>::iterator>)
            {
              return arg <=> other_arg;
            }
          else if constexpr (
            std::is_same_v<
              T,
              std::pair<
                typename std::span<const UuidType>::iterator, const RegistryT *>>)
            {
              return arg.first <=> other_arg.first;
            }
          else
            {
              return std::strong_ordering::equal;
            }
        },
        it_var_, other.it_var_);
    }

  private:
    std::variant<
      typename std::span<const VariantType>::iterator,
      typename std::span<const UuidRefType>::iterator,
      std::pair<typename std::span<const UuidType>::iterator, const RegistryT *>>
      it_var_;
  };

  /// Constructor for direct object range
  explicit UuidIdentifiableObjectView (std::span<const VariantType> objects)
      : objects_ (objects)
  {
  }

  /// Constructor for UuidReference range
  explicit UuidIdentifiableObjectView (std::span<const UuidRefType> refs)
      : refs_ (refs)
  {
  }

  /// Constructor for Uuid + Registry
  explicit UuidIdentifiableObjectView (
    const RegistryT          &registry,
    std::span<const UuidType> uuids)
      : uuids_ (uuids), registry_ (&registry)
  {
  }

  /// Single object constructor
  explicit UuidIdentifiableObjectView (const VariantType &obj)
      : objects_ (std::span<const VariantType> (&obj, 1))
  {
  }

  Iterator begin () const
  {
    if (objects_)
      {
        return Iterator (objects_->begin ());
      }
    else if (refs_)
      {
        return Iterator (refs_->begin ());
      }
    else
      {
        return Iterator (uuids_->begin (), registry_);
      }
  }

  Iterator end () const
  {
    if (objects_)
      {
        return Iterator (objects_->end ());
      }
    else if (refs_)
      {
        return Iterator (refs_->end ());
      }
    else
      {
        return Iterator (uuids_->end (), registry_);
      }
  }

  VariantType operator[] (size_t index) const
  {
    if (objects_)
      return (*objects_)[index];
    if (refs_)
      return (*refs_)[index].get_object ();
    return registry_->find_by_id_or_throw ((*uuids_)[index]);
  }

  VariantType front () const { return *begin (); }
  VariantType back () const
  {
    if (empty ())
      throw std::out_of_range ("Cannot get back() of empty view");

    auto it = end ();
    --it;
    return *it;
  }
  VariantType at (size_t index) const
  {
    if (index >= size ())
      throw std::out_of_range (
        "UuidIdentifiableObjectView::at index out of range");

    return (*this)[index];
  }

  size_t size () const
  {
    if (objects_)
      return objects_->size ();
    if (refs_)
      return refs_->size ();
    return uuids_->size ();
  }

  bool empty () const { return size () == 0; }

  // Projections and transformations
  static UuidType uuid_projection (const VariantType &var)
  {
    return std::visit ([] (const auto &obj) { return obj->get_uuid (); }, var);
  }

  template <typename T> auto get_elements_by_type () const
  {
    return *this | std::views::filter ([] (const auto &var) {
      return std::holds_alternative<T *> (var);
    }) | std::views::transform ([] (const auto &var) {
      return std::get<T *> (var);
    });
  }

  static RegistryT::BaseType * base_projection (const VariantType &var)
  {
    return std::visit (
      [] (const auto &ptr) -> RegistryT::BaseType * { return ptr; }, var);
  }

  auto as_base_type () const
  {
    return std::views::transform (*this, base_projection);
  }

  template <typename T> static auto type_projection (const VariantType &var)
  {
    return std::holds_alternative<T *> (var);
  }

  template <typename T> bool contains_type () const
  {
    return std::ranges::any_of (*this, type_projection<T>);
  }

  template <typename BaseType>
  static auto derived_from_type_projection (const VariantType &var)
  {
    return std::visit (
      [] (const auto &ptr) {
        return std::derived_from<base_type<decltype (ptr)>, BaseType>;
      },
      var);
  }

  /**
   * @note Assumes the range has already been filtered to only contain the type
   * T.
   */
  template <typename T> static auto type_transformation (const VariantType &var)
  {
    return std::get<T *> (var);
  }

  template <typename T>
  static auto derived_from_type_transformation (const VariantType &var)
  {
    return std::visit (
      [] (const auto &ptr) -> T * {
        using ElementT = base_type<decltype (ptr)>;
        if constexpr (std::derived_from<ElementT, T>)
          return ptr;
        throw std::runtime_error ("Not derived from type");
      },
      var);
  }

  // note: assumes all objects are of this type
  template <typename T> auto as_type () const
  {
    return std::views::transform (*this, type_transformation<T>);
  }

  template <typename T> auto get_elements_derived_from () const
  {
    return *this | std::views::filter (derived_from_type_projection<T>)
           | std::views::transform (derived_from_type_transformation<T>);
  }

private:
  std::optional<std::span<const VariantType>> objects_;
  std::optional<std::span<const UuidRefType>> refs_;
  std::optional<std::span<const UuidType>>    uuids_;
  const RegistryT *                           registry_ = nullptr;
};

} // namespace zrythm::utils

// Concept to detect UuidIdentifiableObject::Uuid types
template <typename T>
concept UuidType = requires (T t) {
  typename T::UuidTag;
  { type_safe::get (t) } -> std::convertible_to<QUuid>;
};

// Formatter for any UUID type from UuidIdentifiableObject
template <UuidType T>
struct fmt::formatter<T> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const T &uuid, FormatContext &ctx) const
  {
    return fmt::formatter<QString>{}.format (
      type_safe::get (uuid).toString (QUuid::WithoutBraces), ctx);
  }
};
