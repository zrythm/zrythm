// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * API for ISerializable interface.
 */

#ifndef __IO_SERIALIZATION_ISERIALIZABLE_H__
#define __IO_SERIALIZATION_ISERIALIZABLE_H__

#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "utils/exceptions.h"
#include "utils/json.h"
#include "utils/logger.h"
#include "utils/string.h"
#include "utils/types.h"

#include <yyjson.h>

/**
 * @addtogroup io
 *
 * @{
 */

// Helper concept to check if a type is a container
template <typename T> concept ContainerType = requires (T t)
{
  typename T::value_type;
  typename T::iterator;
  typename T::const_iterator;
  {
    t.begin ()
  } -> std::same_as<typename T::iterator>;
  {
    t.end ()
  } -> std::same_as<typename T::iterator>;
  {
    t.size ()
  } -> std::convertible_to<std::size_t>;
};

// Concept for a container with signed integral value types
template <typename T>
concept SignedIntegralContainer =
  ContainerType<T> && std::is_integral_v<typename T::value_type>
  && std::is_signed_v<typename T::value_type>;

template <typename T>
concept UnsignedIntegralContainer =
  ContainerType<T> && std::is_integral_v<typename T::value_type>
  && std::is_unsigned_v<typename T::value_type>;

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
  };
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
 * class CustomObject : public ISerializable<CustomObject> {
 * private:
 *     int value_;
 *     std::string processed_data_;
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
  virtual ~ISerializable () = default;

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
  inline static constexpr bool is_container_v = is_container<T>::value;
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
  template <typename T>
  inline static constexpr bool is_serializable_pointer_v =
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
  inline static constexpr bool is_array_of_serializable_pointers_v =
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
  inline static constexpr bool is_container_of_serializable_objects_v =
    is_container_of_serializable_objects<T>::value;

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
  inline static constexpr bool is_container_of_serializable_pointers_v =
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
  inline static constexpr bool is_convertible_pointer_v =
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
  inline static constexpr bool is_container_of_convertible_pointers_v =
    is_container_of_convertible_pointers<FieldT, VariantT>::value;

  template <typename FieldT, typename VariantT = void>
  static constexpr bool check_serializability ()
  {
    if constexpr (std::is_same_v<VariantT, void>)
      {
        if constexpr (
          std::derived_from<FieldT, ISerializableBase>
          || is_serializable_pointer_v<FieldT> || std::is_integral_v<FieldT>
          || std::is_floating_point_v<FieldT> || std::is_same_v<FieldT, bool>
          || std::is_same_v<FieldT, std::string>
          || std::is_same_v<FieldT, fs::path>
          || std::is_same_v<FieldT, std::vector<bool>>
          || SignedIntegralContainer<FieldT> || UnsignedIntegralContainer<FieldT>
          || std::is_same_v<FieldT, std::vector<int>>
          || std::is_same_v<FieldT, std::vector<unsigned int>>
          || std::is_same_v<FieldT, std::vector<std::string>>
          || is_container_of_serializable_objects_v<FieldT>
          || is_container_of_serializable_pointers_v<FieldT>
          || std::is_enum_v<FieldT> || is_atomic_serializable_v<FieldT>)
          {
            return true;
          }
        return false;
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
  inline static constexpr bool is_field_serializable_v =
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
  inline static constexpr bool is_atomic_serializable_v =
    is_atomic_serializable<T>::value;

  /**
   * @brief Returns the type of the document to be (de)serialized.
   *
   * Only needed to be implemented by root objects.
   */
  virtual std::string get_document_type () const
  {
    static std::string doc_type =
      "Please implement ISerializable::get_document_type()";
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
      static_cast<Derived *> (this)->define_fields (c);
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
  CStringRAII serialize_to_json_string () const
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
    yyjson_mut_obj_add_str (doc, root, "documentType", document_type.c_str ());
    yyjson_mut_obj_add_int (doc, root, "formatMajor", format_major_version);
    yyjson_mut_obj_add_int (doc, root, "formatMinor", format_minor_version);

    Context ctx (
      doc, root, document_type, format_major_version, format_minor_version);
    serialize (ctx);
    yyjson_write_err write_err;
    char *           json = yyjson_mut_write_opts (
      doc, YYJSON_WRITE_PRETTY_TWO_SPACES, nullptr, nullptr, &write_err);
    yyjson_mut_write_file ("/tmp/zrythm.json", doc, 0, nullptr, nullptr);
    z_debug ("done serializing to json");
    yyjson_mut_doc_free (doc);

    if (!json)
      {
        throw ZrythmException (
          fmt::format ("Failed to serialize to JSON:\n{}", write_err.msg));
      }

    return CStringRAII (json);
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
    static_cast<Derived *> (this)->deserialize (ctx);
    z_debug ("done deserializing from json");

    yyjson_doc_free (doc);
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
            using ObjType = typename T::element_type;
            yyjson_mut_val * child_obj = yyjson_mut_obj_add_obj (doc, obj, key);
            ctx.mut_obj_ = child_obj;
            value->ISerializable<ObjType>::serialize (ctx);
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
    else if constexpr (
      std::is_same_v<T, std::string> || std::is_same_v<T, fs::path>)
      {
        if (!value.empty ())
          {
            yyjson_mut_obj_add_str (doc, obj, key, value.c_str ());
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
                yyjson_mut_arr_add_str (doc, arr, v.c_str ());
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
    else if constexpr (is_container_of_convertible_pointers_v<T, VariantT>)
      {
        yyjson_mut_val * arr = yyjson_mut_arr (doc);
        for (const auto &ptr : value)
          {
            if (ptr)
              {
                yyjson_mut_val * child_obj = yyjson_mut_arr_add_obj (doc, arr);
                ctx.mut_obj_ = child_obj;
                std::visit (
                  [&] (auto &&casted_ptr) {
                    using BaseType = base_type<decltype (casted_ptr)>;
                    casted_ptr->ISerializable<BaseType>::serialize (ctx);
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
        static_assert (false, "Unsupported type");
      }
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

        auto obj_json = json_get_string (ctx.obj_);

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
    else if constexpr (is_serializable_pointer_v<T>)
      {
        if (yyjson_is_obj (val))
          {
            using ObjType = typename T::element_type;
            if constexpr (is_unique_ptr_v<T>)
              value = std::make_unique<ObjType> ();
            else if constexpr (is_shared_ptr_v<T>)
              value = std::make_shared<ObjType> ();
            else
              static_assert (false, "Unsupported pointer type");
            ctx.obj_ = val;
            value->ISerializable<ObjType>::deserialize (ctx);
            return;
          }
        else if (yyjson_is_null (val))
          {
            value.reset ();
            return;
          }
        else
          {
            throw ZrythmException ("Type mismatch: " + std::string (key));
          }
      }
    else if constexpr (std::is_signed_v<T> && std::is_integral_v<T>)
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
      std::is_same_v<T, std::string> || std::is_same_v<T, fs::path>)
      {
        if (yyjson_is_str (val))
          {
            value = yyjson_get_str (val);
            return;
          }
      }
    else if constexpr (std::is_enum_v<T>)
      {
        if (yyjson_is_int (val))
          {
            value = static_cast<T> (yyjson_get_int (val));
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
                    ObjType obj;
                    ctx.obj_ = elem;
                    obj.ISerializable<ObjType>::deserialize (ctx);
                    if constexpr (StdArray<T>)
                      {
                        value[i] = std::move (obj);
                      }
                    else
                      {
                        value.emplace_back (std::move (obj));
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
                    using ObjType = typename SmartPtrType::element_type;
                    SmartPtrType ptr;
                    if constexpr (is_unique_ptr_v<SmartPtrType>)
                      ptr = std::make_unique<ObjType> ();
                    else if constexpr (is_shared_ptr_v<SmartPtrType>)
                      ptr = std::make_shared<ObjType> ();
                    else
                      {
                        static_assert (false, "Unsupported pointer type");
                      }
                    ctx.obj_ = elem;
                    ptr->ISerializable<ObjType>::deserialize (ctx);
                    if constexpr (StdArray<T>)
                      {
                        value[i] = std::move (ptr);
                      }
                    else
                      {
                        value.emplace_back (std::move (ptr));
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

    auto val_to_json = json_get_string (val);
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
      using variant_type = VariantType;
#pragma GCC diagnostic pop
    };
    return Field{ name, value, optional };
  }
};

#define DECLARE_DEFINE_BASE_FIELDS_METHOD() \
public: \
  friend class ISerializableBase; \
  void define_base_fields (const ISerializableBase::Context &ctx);

#define DECLARE_DEFINE_FIELDS_METHOD() \
public: \
  void define_fields (const ISerializableBase::Context &ctx);

#if 0
#  define DECLARE_CUSTOM_SERIALIZE_AND_DESERIALIZE_METHODS() \
  public: \
    void serialize (const ISerializableBase::Context &ctx) const; \
    void deserialize (const ISerializableBase::Context &ctx);

#  define DECLARE_CUSTOM_SERIALIZE_AND_DESERIALIZE_BASE_METHODS() \
  public: \
    void serialize_base (const ISerializableBase::Context &ctx) const; \
    void deserialize_base (const ISerializableBase::Context &ctx);
#endif

/**
 * @}
 */

#endif // __IO_SERIALIZATION_ISERIALIZABLE_H__
