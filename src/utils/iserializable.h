// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * API for ISerializable interface.
 */

#ifndef __IO_SERIALIZATION_ISERIALIZABLE_H__
#define __IO_SERIALIZATION_ISERIALIZABLE_H__

#include <any>
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "utils/dependency_holder.h"
#include "utils/exceptions.h"
#include "utils/json.h"
#include "utils/logger.h"
#include "utils/object_factory.h"
#include "utils/string.h"
#include "utils/traits.h"

#include <QUuid>

#include <yyjson.h>

namespace zrythm::utils::serialization
{

// Concept for a container with signed integral value types
template <typename T>
concept SignedIntegralContainer =
  ContainerType<T> && std::is_integral_v<typename T::value_type>
  && std::is_signed_v<typename T::value_type>;

template <typename T>
concept UnsignedIntegralContainer =
  ContainerType<T> && std::is_integral_v<typename T::value_type>
  && std::is_unsigned_v<typename T::value_type>;

template <typename T>
concept QHashType = requires {
  typename T::key_type;
  typename T::mapped_type;
  requires std::derived_from<typename T::key_type, QUuid>
             || std::is_same_v<
               type_safe::underlying_type<typename T::key_type>, QUuid>;
};

class ISerializableBase
{
public:
  virtual ~ISerializableBase () = default;

  struct Context
  {
    Context () = default;

    /**
     * @brief Used for serialization.
     */
    Context (
      yyjson_mut_doc *   mut_doc,
      yyjson_mut_val *   mut_obj,
      const std::string &document_type,
      int                format_major_version,
      int                format_minor_version)
        : Context ()
    {
      mut_doc_ = mut_doc;
      mut_obj_ = mut_obj;
      document_type_ = document_type;
      format_major_version_ = format_major_version;
      format_minor_version_ = format_minor_version;
    }

    /**
     * @brief Used for deserialization.
     */
    Context (
      yyjson_val *       obj,
      const std::string &document_type,
      int                format_major_version,
      int                format_minor_version)
        : Context ()
    {
      obj_ = obj;
      document_type_ = document_type;
      format_major_version_ = format_major_version;
      format_minor_version_ = format_minor_version;
    }

    Context (
      yyjson_mut_doc * mut_doc,
      yyjson_mut_val * mut_obj,
      const Context   &other)
        : Context ()
    {
      // FIXME use parameters?
      mut_doc_ = other.mut_doc_;
      mut_obj_ = other.mut_obj_;
      document_type_ = other.document_type_;
      format_major_version_ = other.format_major_version_;
      format_minor_version_ = other.format_minor_version_;
    }

    Context (yyjson_val * obj, const Context &other) : Context ()
    {
      // FIXME use parameters?
      obj_ = obj;
      document_type_ = other.document_type_;
      format_major_version_ = other.format_major_version_;
      format_minor_version_ = other.format_minor_version_;
      dependency_holder_ = other.dependency_holder_;
    }

    template <typename T> void add_dependency (T &&dependency)
    {
      dependency_holder_.put (std::forward<T> (dependency));
    }

    constexpr bool is_serializing () const { return mut_doc_ != nullptr; }
    constexpr bool is_deserializing () const { return obj_ != nullptr; }

    /**
     * @brief Type of document being (de)serialized.
     *
     * If this causes performance issues, use Symap and store an int instead.
     */
    std::string document_type_;

    /** Major version of the document format. */
    int format_major_version_ = 0;

    /** Minor version of the document format. */
    int format_minor_version_ = 0;

    /* used during serialization */
    yyjson_mut_doc * mut_doc_ = nullptr;
    yyjson_mut_val * mut_obj_ = nullptr;

    /* used during deserialization */
    yyjson_val * obj_ = nullptr;

    /** Dependency storage */
    DeserializationDependencyHolder dependency_holder_;
  };
};

template <typename T>
concept VariantOfSerializablePointers = requires {
  []<typename... Ts> (std::variant<Ts...> *) {
    static_assert (
      (((IsRawPointer<Ts>
         && std::derived_from<std::remove_pointer_t<Ts>, ISerializableBase>) )
       && ...),
      "All types in the variant must be pointers to classes deriving from ISerializable");
  }(static_cast<T *> (nullptr));
};

template <typename T>
concept OptionalVariantOfSerializablePointers = requires {
  typename T::value_type;
  requires VariantOfSerializablePointers<typename T::value_type>;
  requires std::same_as<T, std::optional<typename T::value_type>>;
};

/**
 * @brief Interface for serializable objects using CRTP.
 *
 * This class provides a simplified approach to serialization and
 * deserialization using the Curiously Recurring Template Pattern (CRTP). It
 * allows derived classes to define their serializable fields in a single
 * method, reducing code duplication and improving maintainability.
 *
 * Custom logic can be implemented in the define_fields method of derived
 * classes. This allows for pre-processing before serialization or
 * post-processing after deserialization.
 *
 * Example usage:
 * @code
 * class CustomObject : public
 * zrythm::utils::serialization::ISerializable<CustomObject> { private: int
 * value_; std::string processed_data_;
 *
 * public:
 *     void define_fields(const Context& ctx) {
 *         if (ctx.mut_doc_) {
 *             // Custom logic for serialization
 *             process_data_for_serialization();
 *         }
 *
 *         serialize_fields(ctx,
 *             make_field("value", value_),
 *             make_field("processed_data", processed_data_)
 *         );
 *
 *         if (!ctx.mut_doc_) {
 *             // Custom logic for deserialization
 *             post_process_deserialized_data();
 *         }
 *     }
 *
 * private:
 *     void process_data_for_serialization() {
 *         // Custom processing before serialization
 *     }
 *
 *     void post_process_deserialized_data() {
 *         // Custom processing after deserialization
 *     }
 * };
 * @endcode
 *
 * @tparam Derived The derived class implementing this interface
 */
template <typename Derived>
class ISerializable : virtual public ISerializableBase
{
public:
  ~ISerializable () override = default;

  template <typename T>
  using is_serializable = std::is_base_of<ISerializable<T>, T>;

#if 0
  template <typename T, typename = void> struct is_container : std::false_type
  {
  };

  template <typename T>
  struct is_container<
    T,
    std::void_t<
      typename T::value_type,
      typename T::iterator,
      typename T::const_iterator,
      decltype (std::declval<T> ().begin ()),
      decltype (std::declval<T> ().end ()),
      decltype (std::declval<T> ().size ())>>
      : std::negation<std::is_same<T, std::string>>
  {
  };

  template <typename T>
   static constexpr bool is_container_v = is_container<T>::value;
#endif

  template <typename T> struct is_serializable_pointer : std::false_type
  {
  };
  template <typename T>
  struct is_serializable_pointer<std::unique_ptr<T>> : is_serializable<T>
  {
  };
  template <typename T>
  struct is_serializable_pointer<std::shared_ptr<T>> : is_serializable<T>
  {
  };
  template <typename T> struct is_serializable_pointer<T *> : is_serializable<T>
  {
  };
  template <typename T>
  static constexpr bool is_serializable_pointer_v =
    is_serializable_pointer<T>::value;

  template <typename T, std::size_t N> struct is_array_of_serializable_pointers
  {
  private:
    template <typename U, std::size_t M> static constexpr bool check_array ()
    {
      if constexpr (StdArray<U> && std::rank_v<U> == 1 && std::extent_v<U> == M)
        {
          return is_serializable_pointer<std::remove_extent_t<U>>::value;
        }
      else
        {
          return false;
        }
    }

  public:
    static constexpr bool value =
      std::is_same_v<T, std::array<typename T::value_type, N>>
      && check_array<typename T::value_type[N], N> ();
  };

  // note: doesn't work, needs testing
  template <typename T, std::size_t N>
  static constexpr bool is_array_of_serializable_pointers_v =
    is_array_of_serializable_pointers<T, N>::value;

  template <typename T, typename = void>
  struct is_container_of_serializable_objects : std::false_type
  {
  };

  template <typename T>
  struct is_container_of_serializable_objects<
    T,
    std::enable_if_t<ContainerType<T>>>
      : std::is_base_of<ISerializable<typename T::value_type>, typename T::value_type>
  {
  };

  template <typename T>
  static constexpr bool is_container_of_serializable_objects_v =
    is_container_of_serializable_objects<T>::value;

  template <typename T, typename = void>
  struct is_container_of_optional_serializable_objects : std::false_type
  {
  };

  template <typename T>
  struct is_container_of_optional_serializable_objects<
    T,
    std::void_t<
      typename T::value_type,
      typename T::value_type::value_type,
      decltype (std::declval<typename T::value_type> ().has_value ())>>
      : std::is_base_of<
          ISerializable<typename T::value_type::value_type>,
          typename T::value_type::value_type>
  {
  };

  template <typename T>
  static constexpr bool is_container_of_optional_serializable_objects_v =
    is_container_of_optional_serializable_objects<T>::value;

  template <typename T, typename = void>
  struct is_container_of_serializable_pointers : std::false_type
  {
  };

  template <typename T>
  struct is_container_of_serializable_pointers<
    T,
    std::enable_if_t<ContainerType<T>>>
      : is_serializable_pointer<typename T::value_type>
  {
  };

  template <typename T>
  static constexpr bool is_container_of_serializable_pointers_v =
    is_container_of_serializable_pointers<T>::value;

  template <typename FieldT, typename VariantT, typename = void>
  struct is_convertible_pointer : std::false_type
  {
  };

  template <typename FieldT, typename VariantT>
  struct is_convertible_pointer<
    FieldT,
    VariantT,
    std::enable_if_t<SmartPtr<FieldT> && !std::is_void_v<VariantT>>>
  {
    static constexpr bool value =
      std::is_pointer_v<std::variant_alternative_t<0, VariantT>>
      && std::is_base_of_v<
        ISerializable<
          std::remove_pointer_t<std::variant_alternative_t<0, VariantT>>>,
        std::remove_pointer_t<std::variant_alternative_t<0, VariantT>>>;
  };

  template <typename FieldT, typename VariantT>
  static constexpr bool is_convertible_pointer_v =
    is_convertible_pointer<FieldT, VariantT>::value;

  template <typename FieldT, typename VariantT, typename = void>
  struct is_container_of_convertible_pointers : std::false_type
  {
  };

  template <typename FieldT, typename VariantT>
  struct is_container_of_convertible_pointers<
    FieldT,
    VariantT,
    std::enable_if_t<ContainerType<FieldT> && !std::is_void_v<VariantT>>>
  {
    static constexpr bool value =
      is_convertible_pointer_v<typename FieldT::value_type, VariantT>;
  };

  template <typename FieldT, typename VariantT>
  static constexpr bool is_container_of_convertible_pointers_v =
    is_container_of_convertible_pointers<FieldT, VariantT>::value;

  template <typename FieldT, typename VariantT = void>
  static constexpr bool check_serializability ()
  {
    if constexpr (std::is_same_v<VariantT, void>)
      {
        if constexpr (OptionalType<FieldT>)
          {
            return check_serializability<typename FieldT::value_type> ();
          }
        else if constexpr (SmartPtrToContainer<FieldT>)
          {
            return check_serializability<typename FieldT::element_type> ();
          }
        else
          {
            return static_cast<bool> (
              std::derived_from<FieldT, ISerializableBase> || QHashType<FieldT>
              || StrongTypedef<FieldT> || std::is_same_v<FieldT, QUuid>
              || is_serializable_pointer_v<FieldT> || std::is_integral_v<FieldT>
              || std::is_floating_point_v<FieldT> || std::is_same_v<FieldT, bool>
              || std::is_same_v<FieldT, std::string>
              || std::is_same_v<FieldT, QString>
              || std::is_same_v<FieldT, fs::path>
              || std::is_same_v<FieldT, std::vector<bool>>
              || SignedIntegralContainer<FieldT>
              || UnsignedIntegralContainer<FieldT>
              || std::is_same_v<FieldT, std::vector<int>>
              || std::is_same_v<FieldT, std::vector<unsigned int>>
              || std::is_same_v<FieldT, std::vector<std::string>>
              || VectorOfVariantPointers<FieldT>
              || is_container_of_serializable_objects_v<FieldT>
              || is_container_of_optional_serializable_objects_v<FieldT>
              || is_container_of_serializable_pointers_v<FieldT>
              || VariantOfSerializablePointers<FieldT> || std::is_enum_v<FieldT>
              || OptionalVariantOfSerializablePointers<FieldT> // note: probably
                                                               // not needed
                                                               // anymore since
                                                               // the else if
                                                               // added below
              || FloatingPointContainer<FieldT>
              || is_atomic_serializable_v<FieldT>);
          }
      }
    else
      {
        return is_container_of_convertible_pointers_v<FieldT, VariantT>
               || is_convertible_pointer_v<FieldT, VariantT>;
      }
  }

  template <typename Owner, typename T, typename VariantT = void>
  struct is_field_serializable
  {
    static constexpr bool value =
      ISerializable<Owner>::template check_serializability<T, VariantT> ();
  };

  template <typename Owner, typename T, typename VariantT = void>
  static constexpr bool is_field_serializable_v =
    is_field_serializable<Owner, T, VariantT>::value;

  template <typename T> struct is_atomic_serializable : std::false_type
  {
  };

  template <typename T>
  struct is_atomic_serializable<std::atomic<T>>
      : is_field_serializable<Derived, T>
  {
  };

  template <typename T>
  static constexpr bool is_atomic_serializable_v =
    is_atomic_serializable<T>::value;

  /**
   * @brief Returns the type of the document to be (de)serialized.
   *
   * Only needed to be implemented by root objects.
   */
  virtual std::string get_document_type () const
  {
    const std::string doc_type = typeid (Derived).name ();
    z_warning (
      "Please implement ISerializable::get_document_type() for {}", doc_type);
    return doc_type;
  }

  /**
   * @brief Returns the major version of the document format.
   *
   * Only needed to be implemented by root objects.
   */
  virtual int get_format_major_version () const { return 1; }

  /**
   * @brief Returns the minor version of the document format.
   *
   * Only needed to be implemented by root objects.
   */
  virtual int get_format_minor_version () const { return 0; }

  /**
   * @brief Upgrades the object from the given version to the latest version.
   *
   * @return Whether successful.
   */
  virtual bool upgrade (const int format_major, const int format_minor)
  {
    return false;
  }

#if 0
  template <typename Base> void call_base_serialization (const Context &ctx)
  {
    static_assert (std::derived_from<Base, ISerializableBase>);
    if (ctx.mut_doc_)
      {
        dynamic_cast<Base *> (this)->serialize_base (ctx);
      }
    else
      {
        dynamic_cast<Base *> (this)->deserialize_base (ctx);
      }
  }

  template <typename... Bases>
  void call_all_base_serializations (const Context &ctx)
  {
    (call_base_serialization<Bases> (ctx), ...);
  }
#endif

  template <std::derived_from<ISerializableBase> Base>
  void call_base_define_fields (const Context &ctx)
  {
    dynamic_cast<Base *> (this)->Base::define_base_fields (ctx);
  }

  template <typename... Bases>
  void call_all_base_define_fields (const Context &ctx)
  {
    (call_base_define_fields<Bases> (ctx), ...);
  }

  template <typename SerializeFunc>
  void serialize_with_custom (const Context &ctx, SerializeFunc &&func) const
  {
#if 0
    if (ctx.is_serializing ())
      {
#endif
    func (ctx);
#if 0
      }
    else
      {
        // FIXME: const cast is a hack
        const_cast<Derived *> (dynamic_cast<const Derived *> (this))
          ->define_fields (ctx);
      }
#endif
  }

  template <typename DeserializeFunc>
  void deserialize_with_custom (const Context &ctx, DeserializeFunc &&func)
  {
#if 0
    if (ctx.is_deserializing ())
      {
#endif
    func (ctx);
#if 0
      }
    else
      {
        static_cast<Derived *> (this)->define_fields (ctx);
      }
#endif
  }

  void serialize (const Context &ctx) const
  {
    serialize_with_custom (ctx, [this] (const Context &c) {
      // FIXME: const cast is a hack
      const_cast<Derived *> (dynamic_cast<const Derived *> (this))
        ->define_fields (c);
    });
  }

  void deserialize (const Context &ctx)
  {
    deserialize_with_custom (ctx, [this] (const Context &c) {
      dynamic_cast<Derived *> (this)->define_fields (c);
    });
  }

  /**
   * Serializes the object to a JSON string.
   *
   * Note that we are using a CStringRAII wrapper here because std::string would
   * allocate more memory than a C-style string and we expect huge strings here.
   *
   * @param ctx The context of the serialization.
   * @throw ZrythmException if an error occurred.
   */
  string::CStringRAII serialize_to_json_string () const
  {
    yyjson_mut_doc * doc = yyjson_mut_doc_new (nullptr);
    yyjson_mut_val * root = yyjson_mut_obj (doc);
    if (!root)
      {
        throw ZrythmException ("Failed to create root obj");
      }
    yyjson_mut_doc_set_root (doc, root);

    auto document_type = get_document_type ();
    auto format_major_version = get_format_major_version ();
    auto format_minor_version = get_format_minor_version ();
    z_debug (
      "serializing '{}' v{}.{} to JSON...", document_type, format_major_version,
      format_minor_version);
    yyjson_mut_obj_add_strncpy (
      doc, root, "documentType", document_type.data (), document_type.size ());
    yyjson_mut_obj_add_int (doc, root, "formatMajor", format_major_version);
    yyjson_mut_obj_add_int (doc, root, "formatMinor", format_minor_version);

    Context ctx (
      doc, root, document_type, format_major_version, format_minor_version);
    serialize (ctx);
    yyjson_write_err write_err;
    char *           json = yyjson_mut_write_opts (
      doc, YYJSON_WRITE_PRETTY_TWO_SPACES, nullptr, nullptr, &write_err);
#if ZRYTHM_DEV_BUILD
    yyjson_mut_write_file (
      "/tmp/zrythm.json", doc,
      YYJSON_WRITE_PRETTY_TWO_SPACES | YYJSON_WRITE_ALLOW_INVALID_UNICODE,
      nullptr, nullptr);
#endif
    z_debug ("done serializing to json");
    yyjson_mut_doc_free (doc);

    if (!json)
      {
        throw ZrythmException (
          fmt::format ("Failed to serialize to JSON:\n{}", write_err.msg));
      }

    return { json };
  }

  void deserialize_from_json_string (const char * json)
  {
    yyjson_doc * doc =
      yyjson_read_opts ((char *) json, strlen (json), 0, nullptr, nullptr);
    yyjson_val * root = yyjson_doc_get_root (doc);
    if (!root)
      {
        throw ZrythmException ("Failed to create root JSON object");
      }

    auto document_type = get_document_type ();
    auto format_major_version = get_format_major_version ();
    auto format_minor_version = get_format_minor_version ();
    z_debug (
      "deserializing '{}' v{}.{} from JSON...", document_type,
      format_major_version, format_minor_version);
    yyjson_obj_iter it = yyjson_obj_iter_with (root);
    if (!yyjson_equals_str (
          yyjson_obj_iter_get (&it, "documentType"), document_type.c_str ()))
      {
        throw ZrythmException ("Invalid document type");
      }
    int deserialized_format_major_version =
      yyjson_get_int (yyjson_obj_iter_get (&it, "formatMajor"));
    int deserialized_format_minor_version =
      yyjson_get_int (yyjson_obj_iter_get (&it, "formatMinor"));

    /* abort if read version is newer */
    if (
      deserialized_format_major_version > format_major_version
      || (deserialized_format_major_version == format_major_version && deserialized_format_minor_version > format_minor_version))
      {
        throw ZrythmException (fmt::format (
          "Cannot deserialize newer '{}' version {}.{}", document_type,
          deserialized_format_major_version, deserialized_format_minor_version));
      }
    Context ctx (
      root, document_type, deserialized_format_major_version,
      deserialized_format_minor_version);
    deserialize (ctx);
    z_debug ("done deserializing from json");

    yyjson_doc_free (doc);
  }

  template <IsVariant T>
  void serialize_variant_pointer (
    const T         &variant_ptr,
    yyjson_mut_doc * doc,
    yyjson_mut_val * obj,
    Context          ctx) const
  {
    // Store variant index
    yyjson_mut_obj_add_uint (doc, obj, "variantIndex", variant_ptr.index ());

    // Store the actual data
    yyjson_mut_val * data_obj = yyjson_mut_obj_add_obj (doc, obj, "data");
    ctx.mut_obj_ = data_obj;
    std::visit (
      [&] (auto &&ptr) {
        using PtrType = base_type<decltype (ptr)>;
        ptr->ISerializable<PtrType>::serialize (ctx);
      },
      variant_ptr);
  }

  template <IsVariant T>
  T deserialize_variant_pointer (yyjson_val * val, Context ctx)
  {
    size_t variant_index =
      yyjson_get_uint (yyjson_obj_get (val, "variantIndex"));
    yyjson_val * data_obj = yyjson_obj_get (val, "data");
    ctx.obj_ = data_obj;
    auto result = create_object_at_variant_index<T> (variant_index, ctx);
    std::visit (
      [&] (auto &&ptr) {
        using PtrType = base_type<decltype (ptr)>;
        ptr->ISerializable<PtrType>::deserialize (ctx);
      },
      result);
    return result;
  }

  template <typename T, typename VariantT = void>
  void serialize_field (
    const char * key,
    const T     &value,
    Context      ctx,
    bool         optional = false) const
  {
    if constexpr (std::is_same_v<VariantT, void>)
      {
        static_assert (
          is_field_serializable_v<Derived, T>,
          "Field type must be serializable");
      }
    else
      {
        static_assert (
          is_field_serializable_v<Derived, T, VariantT>,
          "Field type must be serializable or convertible to the specified variant");
      }

    auto doc = ctx.mut_doc_;
    auto obj = ctx.mut_obj_;
    /* implement serialization logic for different types */
    if constexpr (std::derived_from<T, ISerializable<T>>)
      {
        yyjson_mut_val * child_obj = yyjson_mut_obj_add_obj (doc, obj, key);
        ctx.mut_obj_ = child_obj;
        value.ISerializable<T>::serialize (ctx);
        if (!yyjson_mut_obj_add_val (doc, obj, key, child_obj))
          {
            throw ZrythmException ("Failed to add field");
          }
      }
    else if constexpr (QHashType<T>)
      {
        yyjson_mut_val * arr = yyjson_mut_obj_add_arr (doc, obj, key);
        for (auto &v : value.values ())
          {
            yyjson_mut_val * child_obj = yyjson_mut_arr_add_obj (doc, arr);
            if constexpr (IsVariant<typename T::value_type>)
              {
                serialize_variant_pointer (v, doc, child_obj, ctx);
              }
            else if constexpr (is_shared_ptr_v<typename T::value_type>)
              {
                using ObjType = typename T::value_type::element_type;
                ctx.mut_obj_ = child_obj;
                v->ISerializable<ObjType>::serialize (ctx);
              }
          }
        yyjson_mut_obj_add_val (doc, obj, key, arr);
      }
    else if constexpr (SmartPtrToContainer<T>)
      {
        if (value)
          {
            // Serialize the contained container
            serialize_field (key, *value, ctx, optional);
          }
        else if (!optional)
          {
            yyjson_mut_obj_add_null (doc, obj, key);
          }
      }
    else if constexpr (OptionalVariantOfSerializablePointers<T>)
      {
        if (value.has_value ())
          {
            yyjson_mut_val * child_obj = yyjson_mut_obj_add_obj (doc, obj, key);
            serialize_variant_pointer (*value, doc, child_obj, ctx);
          }
        else
          {
            yyjson_mut_obj_add_null (doc, obj, key);
          }
      }
    else if constexpr (std::is_same_v<T, QUuid>)
      {
        if (!value.isNull ())
          {
            const auto str = value.toString ().toStdString ();
            yyjson_mut_obj_add_strncpy (
              doc, obj, key, str.c_str (), str.length ());
          }
      }
    else if constexpr (OptionalType<T>)
      {
        if (value.has_value ())
          {
            serialize_field (key, *value, ctx);
          }
        else
          {
            yyjson_mut_obj_add_null (doc, obj, key);
          }
      }
    else if constexpr (StrongTypedef<T>)
      {
        serialize_field (key, type_safe::get (value), ctx, optional);
      }
    else if constexpr (is_convertible_pointer_v<T, VariantT>)
      {
        if (value)
          {
            yyjson_mut_val * child_obj = yyjson_mut_obj_add_obj (doc, obj, key);
            ctx.mut_obj_ = child_obj;
            std::visit (
              [&] (auto &&value_ptr) {
                using CurType = base_type<decltype (value_ptr)>;
                value_ptr->ISerializable<CurType>::serialize (ctx);
              },
              convert_to_variant<VariantT> (value.get ()));
            if (!yyjson_mut_obj_add_val (doc, obj, key, child_obj))
              {
                throw ZrythmException ("Failed to add field");
              }
          }
        else if (optional)
          {
            return;
          }
        else
          {
            yyjson_mut_obj_add_null (doc, obj, key);
          }
      }
    else if constexpr (is_serializable_pointer_v<T>)
      {
        if (value)
          {
            if constexpr (IsRawPointer<T>)
              {
                using ObjType = base_type<T>;
                yyjson_mut_val * child_obj =
                  yyjson_mut_obj_add_obj (doc, obj, key);
                ctx.mut_obj_ = child_obj;
                value->ISerializable<ObjType>::serialize (ctx);
                if (!yyjson_mut_obj_add_val (doc, obj, key, child_obj))
                  {
                    throw ZrythmException ("Failed to add field");
                  }
              }
            else
              {
                using ObjType = typename T::element_type;
                yyjson_mut_val * child_obj =
                  yyjson_mut_obj_add_obj (doc, obj, key);
                ctx.mut_obj_ = child_obj;
                value->ISerializable<ObjType>::serialize (ctx);
                if (!yyjson_mut_obj_add_val (doc, obj, key, child_obj))
                  {
                    throw ZrythmException ("Failed to add field");
                  }
              }
          }
        else if (optional)
          {
            return;
          }
        else
          {
            yyjson_mut_obj_add_null (doc, obj, key);
          }
      }
    else if constexpr (FloatingPointContainer<T>)
      {
        yyjson_mut_val * arr = yyjson_mut_arr (doc);
        for (auto v : value)
          {
            if (std::isnan (v) || std::isinf (v))
              {
                throw ZrythmException (
                  fmt::format ("Invalid float value for '{}':{}", key, v));
              }
            if (!yyjson_mut_arr_add_real (doc, arr, static_cast<double> (v)))
              {
                throw ZrythmException (
                  fmt::format ("Failed to add field {}", key));
              }
          }
        yyjson_mut_obj_add_val (doc, obj, key, arr);
      }
    else if constexpr (std::is_signed_v<T> && std::is_integral_v<T>)
      {
        yyjson_mut_obj_add_int (doc, obj, key, static_cast<int64_t> (value));
      }
    else if constexpr (std::is_unsigned_v<T> && std::is_integral_v<T>)
      {
        yyjson_mut_obj_add_uint (doc, obj, key, static_cast<uint64_t> (value));
      }
    else if constexpr (std::is_same_v<T, float>)
      {
        if (std::isnan (value) || std::isinf (value))
          {
            throw ZrythmException (
              fmt::format ("Invalid float value for '{}':{}", key, value));
          }
        if (
          !yyjson_mut_obj_add_real (doc, obj, key, static_cast<double> (value)))
          {
            throw ZrythmException (fmt::format ("Failed to add field {}", key));
          }
      }
    else if constexpr (std::is_same_v<T, double>)
      {
        if (std::isnan (value) || std::isinf (value))
          {
            throw ZrythmException (
              fmt::format ("Invalid float value for '{}':{}", key, value));
          }
        if (!yyjson_mut_obj_add_real (doc, obj, key, value))
          {
            throw ZrythmException (fmt::format ("Failed to add field {}", key));
          }
      }
    else if constexpr (std::is_same_v<T, bool>)
      {
        yyjson_mut_obj_add_bool (doc, obj, key, value);
      }
    else if constexpr (std::is_same_v<T, std::string>)
      {
        if (!value.empty ())
          {
            yyjson_mut_obj_add_strncpy (
              doc, obj, key, value.data (), value.size ());
          }
      }
    else if constexpr (std::is_same_v<T, QString>)
      {
        if (!value.isEmpty ())
          {
            const auto value_str = value.toStdString ();
            yyjson_mut_obj_add_strncpy (
              doc, obj, key, value_str.data (), value_str.size ());
          }
      }
    else if constexpr (std::is_same_v<T, fs::path>)
      {
        if (!value.empty ())
          {
            const auto value_str = value.string ();
            yyjson_mut_obj_add_strncpy (
              doc, obj, key, value_str.data (), value_str.size ());
          }
      }
    else if constexpr (std::is_same_v<T, std::vector<float>>)
      {
        yyjson_mut_val * arr = yyjson_mut_arr (doc);
        for (float v : value)
          {
            if (std::isnan (v) || std::isinf (v))
              {
                throw ZrythmException (
                  fmt::format ("Invalid float value for '{}':{}", key, value));
              }
            if (!yyjson_mut_arr_add_real (doc, arr, static_cast<double> (v)))
              {
                throw ZrythmException (
                  fmt::format ("Failed to add field {}", key));
              }
          }
        yyjson_mut_obj_add_val (doc, obj, key, arr);
      }
    else if constexpr (std::is_same_v<T, std::vector<double>>)
      {
        yyjson_mut_val * arr = yyjson_mut_arr (doc);
        for (double v : value)
          {
            if (std::isnan (v) || std::isinf (v))
              {
                throw ZrythmException (
                  fmt::format ("Invalid float value for '{}':{}", key, value));
              }
            if (!yyjson_mut_arr_add_real (doc, arr, v))
              {
                throw ZrythmException (
                  fmt::format ("Failed to add field {}", key));
              }
          }
        yyjson_mut_obj_add_val (doc, obj, key, arr);
      }
    else if constexpr (std::is_same_v<T, std::vector<bool>>)
      {
        yyjson_mut_val * arr = yyjson_mut_arr (doc);
        for (bool v : value)
          {
            yyjson_mut_arr_add_bool (doc, arr, v);
          }
        yyjson_mut_obj_add_val (doc, obj, key, arr);
      }
    else if constexpr (std::is_same_v<T, std::vector<std::string>>)
      {
        yyjson_mut_val * arr = yyjson_mut_arr (doc);
        for (const std::string &v : value)
          {
            if (v.empty ())
              {
                yyjson_mut_arr_add_null (doc, arr);
              }
            else
              {
                yyjson_mut_arr_add_strncpy (doc, arr, v.data (), v.size ());
              }
          }
        yyjson_mut_obj_add_val (doc, obj, key, arr);
      }
    else if constexpr (std::is_enum_v<T>)
      {
        try
          {
            if (!yyjson_mut_obj_add_int (
                  doc, obj, key, magic_enum::enum_integer (value)))
              {
                throw ZrythmException (
                  fmt::format ("Failed to add field {}", key));
              }
          }
        catch (const std::exception &e)
          {
            throw ZrythmException (fmt::format ("Invalid enum: {}", e.what ()));
          }
      }
    else if constexpr (SignedIntegralContainer<T>)
      {
        yyjson_mut_val * arr = yyjson_mut_arr (doc);
        for (auto v : value)
          {
            if (!yyjson_mut_arr_add_int (doc, arr, static_cast<int64_t> (v)))
              {
                throw ZrythmException (
                  fmt::format ("Failed to add field {}", key));
              }
          }
        yyjson_mut_obj_add_val (doc, obj, key, arr);
      }
    else if constexpr (UnsignedIntegralContainer<T>)
      {
        yyjson_mut_val * arr = yyjson_mut_arr (doc);
        for (auto v : value)
          {
            yyjson_mut_arr_add_uint (doc, arr, static_cast<uint64_t> (v));
          }
        yyjson_mut_obj_add_val (doc, obj, key, arr);
      }
    else if constexpr (is_container_of_optional_serializable_objects_v<T>)
      {
        yyjson_mut_val * arr = yyjson_mut_arr (doc);
        for (const auto &opt : value)
          {
            if (opt.has_value ())
              {
                yyjson_mut_val * child_obj = yyjson_mut_arr_add_obj (doc, arr);
                ctx.mut_obj_ = child_obj;
                opt.value ().serialize (ctx);
              }
            else
              {
                yyjson_mut_arr_add_null (doc, arr);
              }
          }
        yyjson_mut_obj_add_val (doc, obj, key, arr);
      }
    else if constexpr (is_container_of_convertible_pointers_v<T, VariantT>)
      {
        yyjson_mut_val * arr = yyjson_mut_arr (doc);
        for (const auto &ptr : value)
          {
            if (ptr)
              {
                yyjson_mut_val * child_obj = yyjson_mut_arr_add_obj (doc, arr);
                std::visit (
                  [&] (auto &&casted_ptr) {
                    VariantT var = casted_ptr;
                    serialize_variant_pointer (var, doc, child_obj, ctx);
                  },
                  convert_to_variant<VariantT> (ptr.get ()));
              }
            else
              {
                yyjson_mut_arr_add_null (doc, arr);
              }
          }
        yyjson_mut_obj_add_val (doc, obj, key, arr);
      }
    else if constexpr (VariantOfSerializablePointers<T>)
      {
        yyjson_mut_val * child_obj = yyjson_mut_obj_add_obj (doc, obj, key);
        serialize_variant_pointer (value, doc, child_obj, ctx);
      }
    else if constexpr (VectorOfVariantPointers<T>)
      {
        yyjson_mut_val * arr = yyjson_mut_arr (doc);
        for (const auto &variant_ptr : value)
          {

            std::visit (
              [&] (auto &&ptr) {
                using PtrType = base_type<decltype (ptr)>;
                if (ptr)
                  {
                    yyjson_mut_val * child_obj =
                      yyjson_mut_arr_add_obj (doc, arr);
                    ctx.mut_obj_ = child_obj;
                    ptr->ISerializable<PtrType>::serialize (ctx);
                  }
                else
                  {
                    yyjson_mut_arr_add_null (doc, arr);
                  }
              },
              variant_ptr);
          }
        yyjson_mut_obj_add_val (doc, obj, key, arr);
      }
    else if constexpr (is_container_of_serializable_objects_v<T>)
      {
        yyjson_mut_val * arr = yyjson_mut_arr (doc);
        for (const auto &arr_obj : value)
          {
            yyjson_mut_val * child_obj = yyjson_mut_arr_add_obj (doc, arr);
            ctx.mut_obj_ = child_obj;
            arr_obj.serialize (ctx);
          }
        yyjson_mut_obj_add_val (doc, obj, key, arr);
      }
    else if constexpr (is_container_of_serializable_pointers_v<T>)
      {
        yyjson_mut_val * arr = yyjson_mut_arr (doc);
        for (const auto &ptr : value)
          {
            if (ptr)
              {
                using ObjType = base_type<decltype (ptr)>;
                yyjson_mut_val * child_obj = yyjson_mut_arr_add_obj (doc, arr);
                ctx.mut_obj_ = child_obj;
                ptr->ISerializable<ObjType>::serialize (ctx);
              }
            else
              {
                yyjson_mut_arr_add_null (doc, arr);
              }
          }
        yyjson_mut_obj_add_val (doc, obj, key, arr);
      }
    else if constexpr (is_atomic_serializable_v<T>)
      {
        using UnderlyingType = typename T::value_type;
        UnderlyingType value_copy = value.load ();
        serialize_field (key, value_copy, ctx, optional);
      }
    else
      {
        static_assert (dependent_false_v<T, VariantT>, "Unsupported type");
      }
  }

  template <typename T> std::unique_ptr<T> create_object (const Context &ctx)
  {
    std::unique_ptr<T> obj;
    if constexpr (utils::Initializable<T>)
      {
        if constexpr (ConstructibleWithDependencyHolder<T>)
          {
            obj = T::template create_unique<T> (ctx.dependency_holder_);
          }
        else
          {
            obj = T::template create_unique<T> ();
          }
      }
    else
      {
        if constexpr (ConstructibleWithDependencyHolder<T>)
          {
            obj = std::make_unique<T> (ctx.dependency_holder_);
          }
        else
          {
            obj = std::make_unique<T> ();
          }
      }
    return obj;
  }

  template <typename Variant>
  auto create_object_at_variant_index (size_t index, const Context &ctx)
  {
    auto creator = [&]<size_t... I> (std::index_sequence<I...>) {
      using Result = std::variant<typename std::remove_pointer_t<
        std::variant_alternative_t<I, Variant>> *...>;
      Result result{};

      auto create_type_if_current_index = [&]<size_t N> () {
        using Type =
          std::remove_pointer_t<std::variant_alternative_t<N, Variant>>;
        if (N == index)
          {
            result = Result{ create_object<Type> (ctx).release () };
          }
      };

      (create_type_if_current_index.template operator()<I> (), ...);

      return result;
    };

    return creator (std::make_index_sequence<std::variant_size_v<Variant>>{});
  }

  template <typename T, typename VariantT = void>
  void deserialize_field (
    yyjson_obj_iter &it,
    const char *     key,
    T               &value,
    Context          ctx,
    bool             optional = false)
  {
    yyjson_val * val = yyjson_obj_iter_get (&it, key);
    if (!val)
      {
        if (optional)
          return;

        auto obj_json = json::get_string (ctx.obj_);

        throw ZrythmException (fmt::format (
          "Missing field '{}' in object:\n{}", key, obj_json.c_str ()));
      }

    if constexpr (std::derived_from<T, ISerializable<T>>)
      {
        if (!yyjson_is_obj (val))
          {
            throw ZrythmException ("Type mismatch: " + std::string (key));
          }

        ctx.obj_ = val;
        value.ISerializable<T>::deserialize (ctx);
        return;
      }
    else if constexpr (QHashType<T>)
      {
        if (!yyjson_is_arr (val))
          {
            throw ZrythmException (
              "Expected JSON array for UUID-variant hash table");
          }

        size_t len = yyjson_arr_size (val);
        for (const auto i : std::views::iota (0zu, len))
          {
            yyjson_val * elem = yyjson_arr_get (val, i);
            if constexpr (IsVariant<typename T::value_type>)
              {
                auto child_var = deserialize_variant_pointer<
                  typename T::value_type> (elem, ctx);
                std::visit (
                  [&] (auto &&child) {
                    value.insert (type_safe::get (child->get_uuid ()), child);
                  },
                  child_var);
              }
            else if constexpr (is_shared_ptr_v<typename T::value_type>)
              {
                using ObjType = typename T::value_type::element_type;
                auto child = std::make_shared<ObjType> ();
                ctx.obj_ = elem;
                child->ISerializable<ObjType>::deserialize (ctx);
                value.insert (child->get_uuid (), child);
              }
          }
        return;
      }
    else if constexpr (SmartPtrToContainer<T>)
      {
        using ContainerType = typename T::element_type;

        if (yyjson_is_null (val))
          {
            value.reset ();
            return;
          }

        ContainerType temp;
        deserialize_field (it, key, temp, ctx, optional);
        value = std::make_unique<ContainerType> (std::move (temp));
        return;
      }
    else if constexpr (is_serializable_pointer_v<T>)
      {
        if (yyjson_is_obj (val))
          {
            if constexpr (IsRawPointer<T>)
              {
                using ObjType = base_type<T>;
                value = create_object<ObjType> (ctx).release ();
                ctx.obj_ = val;
                value->ISerializable<ObjType>::deserialize (ctx);
              }
            else
              {
                using ObjType = typename T::element_type;
                if constexpr (is_unique_ptr_v<T>)
                  value = create_object<ObjType> (ctx);
                else if constexpr (is_shared_ptr_v<T>)
                  value = std::make_shared<ObjType> ();
                else
                  static_assert (
                    dependent_false_v<T, VariantT>, "Unsupported pointer type");
                ctx.obj_ = val;
                value->ISerializable<ObjType>::deserialize (ctx);
              }
            return;
          }
        if (yyjson_is_null (val))
          {
            if constexpr (IsRawPointer<T>)
              {
                delete value;
              }
            else
              {
                value.reset ();
              }
            return;
          }
        else
          {
            throw ZrythmException ("Type mismatch: " + std::string (key));
          }
      }
    else if constexpr (OptionalVariantOfSerializablePointers<T>)
      {
        if (yyjson_is_obj (val))
          {
            value =
              deserialize_variant_pointer<typename T::value_type> (val, ctx);
            return;
          }
        else if (yyjson_is_null (val))
          {
            value = std::nullopt;
            return;
          }
      }
    else if constexpr (std::is_same_v<T, QUuid>)
      {
        if (yyjson_is_str (val))
          {
            value = QUuid (QString::fromUtf8 (yyjson_get_str (val)));
            return;
          }
      }
    else if constexpr (OptionalType<T>)
      {
        if (yyjson_is_null (val))
          {
            value = std::nullopt;
          }
        else
          {
            typename T::value_type temp;
            deserialize_field (it, key, temp, ctx);
            value = std::move (temp);
          }
        return;
      }
    else if constexpr (StrongTypedef<T>)
      {
        typename type_safe::underlying_type<T> underlying;
        deserialize_field (it, key, underlying, ctx, optional);
        value = T (underlying);
        return;
      }
    else if constexpr (FloatingPointContainer<T>)
      {
        if (yyjson_is_arr (val))
          {
            size_t len = yyjson_arr_size (val);
            if constexpr (StdArray<T>)
              {
                if (value.size () != len)
                  {
                    throw ZrythmException ("Array size mismatch");
                  }
              }
            else
              {
                value.clear ();
              }

            for (size_t i = 0; i < len; ++i)
              {
                yyjson_val * elem = yyjson_arr_get (val, i);
                if (yyjson_is_real (elem))
                  {
                    using ObjType = typename T::value_type;
                    if constexpr (StdArray<T>)
                      {
                        value[i] = static_cast<ObjType> (yyjson_get_real (elem));
                      }
                    else
                      {
                        value.push_back (
                          static_cast<ObjType> (yyjson_get_real (elem)));
                      }
                  }
              }
            return;
          }
      }
    else if constexpr (
      (std::is_signed_v<T> && std::is_integral_v<T>) || std::is_enum_v<T>)
      {
        if (yyjson_is_int (val))
          {
            value = static_cast<T> (yyjson_get_int (val));
            return;
          }
      }
    else if constexpr (std::is_unsigned_v<T> && std::is_integral_v<T>)
      {
        if (yyjson_is_uint (val))
          {
            value = static_cast<T> (yyjson_get_uint (val));
            return;
          }
      }
    else if constexpr (std::is_same_v<T, float>)
      {
        if (yyjson_is_real (val))
          {
            value = static_cast<float> (yyjson_get_real (val));
            return;
          }
      }
    else if constexpr (std::is_same_v<T, double>)
      {
        if (yyjson_is_real (val))
          {
            value = yyjson_get_real (val);
            return;
          }
      }
    else if constexpr (std::is_same_v<T, bool>)
      {
        if (yyjson_is_bool (val))
          {
            value = yyjson_get_bool (val);
            return;
          }
      }
    else if constexpr (
      std::is_same_v<T, std::string> || std::is_same_v<T, fs::path>
      || std::is_same_v<T, QString>)
      {
        if (yyjson_is_str (val))
          {
            if constexpr (std::is_same_v<T, QString>)
              {
                value = QString::fromUtf8 (yyjson_get_str (val));
              }
            else
              {
                value = yyjson_get_str (val);
              }
            return;
          }
      }

    else if constexpr (std::is_same_v<T, std::vector<float>>)
      {
        if (yyjson_is_arr (val))
          {
            value.clear ();
            size_t len = yyjson_arr_size (val);
            for (size_t i = 0; i < len; ++i)
              {
                yyjson_val * elem = yyjson_arr_get (val, i);
                if (yyjson_is_real (elem))
                  {
                    value.push_back (
                      static_cast<float> (yyjson_get_real (elem)));
                  }
              }
            return;
          }
      }
    else if constexpr (std::is_same_v<T, std::vector<double>>)
      {
        if (yyjson_is_arr (val))
          {
            value.clear ();
            size_t len = yyjson_arr_size (val);
            for (size_t i = 0; i < len; ++i)
              {
                yyjson_val * elem = yyjson_arr_get (val, i);
                if (yyjson_is_real (elem))
                  {
                    value.push_back (yyjson_get_real (elem));
                  }
              }
            return;
          }
      }
    else if constexpr (std::is_same_v<T, std::vector<bool>>)
      {
        if (yyjson_is_arr (val))
          {
            value.clear ();
            size_t len = yyjson_arr_size (val);
            for (size_t i = 0; i < len; ++i)
              {
                yyjson_val * elem = yyjson_arr_get (val, i);
                if (yyjson_is_bool (elem))
                  {
                    value.push_back (yyjson_get_bool (elem));
                  }
              }
            return;
          }
      }
    else if constexpr (std::is_same_v<T, std::vector<std::string>>)
      {
        if (yyjson_is_arr (val))
          {
            value.clear ();
            size_t len = yyjson_arr_size (val);
            for (size_t i = 0; i < len; ++i)
              {
                yyjson_val * elem = yyjson_arr_get (val, i);
                if (yyjson_is_str (elem))
                  {
                    value.emplace_back (yyjson_get_str (elem));
                  }
              }
            return;
          }
      }
    else if constexpr (
      SignedIntegralContainer<T> || UnsignedIntegralContainer<T>)
      {
        if (yyjson_is_arr (val))
          {
            size_t len = yyjson_arr_size (val);

            if constexpr (StdArray<T>)
              {
                if (value.size () != len)
                  {
                    throw ZrythmException ("Array size mismatch");
                  }
              }
            else
              {
                value.clear ();
              }

            for (size_t i = 0; i < len; ++i)
              {
                yyjson_val * elem = yyjson_arr_get (val, i);
                using ObjType = typename T::value_type;
                if constexpr (SignedIntegralContainer<T>)
                  {
                    if (yyjson_is_int (elem))
                      {
                        if constexpr (StdArray<T>)
                          {
                            value[i] =
                              static_cast<ObjType> (yyjson_get_int (elem));
                          }
                        else
                          {
                            value.push_back (
                              static_cast<ObjType> (yyjson_get_int (elem)));
                          }
                      }
                  }
                else
                  {
                    if (yyjson_is_uint (elem))
                      {
                        if constexpr (StdArray<T>)
                          {
                            value[i] =
                              static_cast<ObjType> (yyjson_get_uint (elem));
                          }
                        else
                          {
                            value.push_back (
                              static_cast<ObjType> (yyjson_get_uint (elem)));
                          }
                      }
                  }
              }
            return;
          }
      }
    else if constexpr (VariantOfSerializablePointers<T>)
      {
        if (yyjson_is_obj (val))
          {
            value = deserialize_variant_pointer<T> (val, ctx);
          }
        else
          {
            throw ZrythmException (
              std::format ("Expected object for variant with key {}", key));
          }
      }
    else if constexpr (VectorOfVariantPointers<T>)
      {
        if (yyjson_is_arr (val))
          {
            value.clear ();
            size_t len = yyjson_arr_size (val);
            for (size_t i = 0; i < len; ++i)
              {
                yyjson_val * elem = yyjson_arr_get (val, i);
                if (yyjson_is_obj (elem))
                  {
                    value.push_back (
                      deserialize_variant_pointer<typename T::value_type> (
                        elem, ctx));
                  }
              }
            return;
          }
      }
    else if constexpr (is_container_of_optional_serializable_objects_v<T>)
      {
        if (yyjson_is_arr (val))
          {
            if constexpr (!StdArray<T>)
              {
                value.clear ();
              }
            size_t len = yyjson_arr_size (val);
            for (size_t i = 0; i < len; ++i)
              {
                yyjson_val * elem = yyjson_arr_get (val, i);
                if (yyjson_is_obj (elem))
                  {
                    using OptionalType = typename T::value_type;
                    using ObjType = typename OptionalType::value_type;
                    ObjType obj;
                    ctx.obj_ = elem;
                    obj.deserialize (ctx);
                    if constexpr (StdArray<T>)
                      {
                        value[i] = std::move (obj);
                      }
                    else
                      {
                        value.emplace_back (std::move (obj));
                      }
                  }
                else if (yyjson_is_null (elem))
                  {
                    if constexpr (StdArray<T>)
                      {
                        value[i] = std::nullopt;
                      }
                    else
                      {
                        value.emplace_back (std::nullopt);
                      }
                  }
              }
            return;
          }
      }
    else if constexpr (is_container_of_convertible_pointers_v<T, VariantT>)
      {
        /* TODO*/
        using UnderlyingBaseType =
          base_type<decltype (*std::declval<typename T::value_type> ())>;
        if (yyjson_is_arr (val))
          {
            value.clear ();
            size_t len = yyjson_arr_size (val);
            for (size_t i = 0; i < len; ++i)
              {
                yyjson_val * elem = yyjson_arr_get (val, i);
                if (yyjson_is_obj (elem))
                  {
                    /* this is wrong - we want the derived type but we have to
                     * look for it */
                    auto ptr = std::make_shared<UnderlyingBaseType> ();
                    ctx.obj_ = elem;
                    ptr->ISerializable<UnderlyingBaseType>::deserialize (ctx);
                    value.push_back (std::move (ptr));
                  }
                else if (yyjson_is_null (elem))
                  {
                    value.push_back (nullptr);
                  }
              }
            return;
          }
      }
    else if constexpr (is_container_of_serializable_objects_v<T>)
      {
        if (yyjson_is_arr (val))
          {
            size_t len = yyjson_arr_size (val);
            if constexpr (StdArray<T>)
              {
                if (value.size () != len)
                  {
                    throw ZrythmException ("Array size mismatch");
                  }
              }
            else
              {
                value.clear ();
              }

            for (size_t i = 0; i < len; ++i)
              {
                yyjson_val * elem = yyjson_arr_get (val, i);
                if (yyjson_is_obj (elem))
                  {
                    using ObjType = typename T::value_type;
                    auto obj = create_object<ObjType> (ctx);
                    ctx.obj_ = elem;
                    obj->ISerializable<ObjType>::deserialize (ctx);
                    if constexpr (StdArray<T>)
                      {
                        value[i] = std::move (*obj);
                      }
                    else
                      {
                        value.emplace_back (std::move (*obj));
                      }
                  }
              }
            return;
          }
      }
    else if constexpr (is_container_of_serializable_pointers_v<T>)
      {
        if (yyjson_is_arr (val))
          {
            size_t len = yyjson_arr_size (val);
            if constexpr (StdArray<T>)
              {
                if (value.size () != len)
                  {
                    throw ZrythmException ("Array size mismatch");
                  }
              }
            else
              {
                value.clear ();
              }
            for (size_t i = 0; i < len; ++i)
              {
                yyjson_val * elem = yyjson_arr_get (val, i);
                if (yyjson_is_obj (elem))
                  {
                    using SmartPtrType = typename T::value_type;
                    SmartPtrType ptr;
                    ctx.obj_ = elem;
                    if constexpr (std::is_pointer_v<SmartPtrType>)
                      {
                        using ObjType = base_type<SmartPtrType>;
                        auto unique_ptr = create_object<ObjType> (ctx);
                        ptr = unique_ptr.release ();

                        ptr->ISerializable<ObjType>::deserialize (ctx);
                      }
                    else
                      {
                        using ObjType = typename SmartPtrType::element_type;
                        auto unique_ptr = create_object<ObjType> (ctx);
                        if constexpr (is_unique_ptr_v<SmartPtrType>)
                          ptr = std::move (unique_ptr);
                        else if constexpr (is_shared_ptr_v<SmartPtrType>)
                          ptr = std::shared_ptr<ObjType> (unique_ptr.release ());
                        else
                          {
                            static_assert (
                              dependent_false_v<T, VariantT>,
                              "Unsupported pointer type");
                          }

                        ptr->ISerializable<ObjType>::deserialize (ctx);
                      }
                    if constexpr (StdArray<T>)
                      {
                        if constexpr (std::is_pointer_v<SmartPtrType>)
                          {
                            value[i] = ptr;
                          }
                        else
                          {
                            value[i] = std::move (ptr);
                          }
                      }
                    else
                      {
                        if constexpr (std::is_pointer_v<SmartPtrType>)
                          {
                            value.push_back (ptr);
                          }
                        else
                          {
                            value.emplace_back (std::move (ptr));
                          }
                      }
                  }
                else if (yyjson_is_null (elem))
                  {
                    if constexpr (StdArray<T>)
                      {
                        value[i] = nullptr;
                      }
                    else
                      {
                        value.push_back (nullptr);
                      }
                  }
              }
            return;
          }
      }
    else if constexpr (is_atomic_serializable_v<T>)
      {
        using UnderlyingType = typename T::value_type;
        UnderlyingType temp_value;
        deserialize_field (it, key, temp_value, ctx, optional);
        value.store (temp_value);
        return;
      }

    auto val_to_json = json::get_string (val);
    throw ZrythmException (fmt::format (
      "Could not deserialize '{}::{}' with the following JSON:\n{}",
      typeid (Derived).name (), key, val_to_json.c_str ()));
  }

protected:
  template <typename... Field>
  void serialize_fields (const Context &ctx, Field &&... field) const
  {
    if (ctx.mut_doc_)
      {
        (serialize_field<
           std::remove_reference_t<decltype (field.value)>,
           typename Field::variant_type> (
           field.name, field.value, ctx, field.optional),
         ...);
      }
    else
      {
        yyjson_obj_iter it = yyjson_obj_iter_with (ctx.obj_);
        // FIXME: const cast is a hack
        (const_cast<ISerializable<Derived> *> (
           dynamic_cast<const ISerializable<Derived> *> (this))
           ->deserialize_field<
             std::remove_reference_t<decltype (field.value)>,
             typename Field::variant_type> (
             it, field.name, field.value, ctx, field.optional),
         ...);
      }
  }

  template <typename T, typename VariantType = void>
  static auto make_field (const char * name, T &value, bool optional = false)
  {
    if constexpr (std::is_same_v<VariantType, void>)
      {
        static_assert (
          is_field_serializable_v<Derived, T>,
          "Field type must be serializable");
      }
    else
      {
        static_assert (
          is_field_serializable_v<Derived, T, VariantType>,
          "Field type must be serializable or convertible to the specified variant");
      }

    struct Field
    {
      const char * name;
      T           &value;
      bool         optional;
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif
      using variant_type = VariantType;
#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif
    };
    return Field{ name, value, optional };
  }
};

#define DECLARE_DEFINE_BASE_FIELDS_METHOD() \
public: \
  friend class zrythm::utils::serialization::ISerializableBase; \
  void define_base_fields ( \
    const zrythm::utils::serialization::ISerializableBase::Context &ctx)

#define DECLARE_DEFINE_FIELDS_METHOD() \
public: \
  void define_fields ( \
    const zrythm::utils::serialization::ISerializableBase::Context &ctx)

}; // namespace zrythm::utils::serialization

#endif // __IO_SERIALIZATION_ISERIALIZABLE_H__
