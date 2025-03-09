#pragma once

#include <any>
#include <typeindex>
#include <unordered_map>

/**
 * @brief Holds dependencies to be injected into object constructors during
 * deserialization.
 *
 * Objects that require dependencies to be constructed must have a constructor
 * that takes a const reference of this class.
 */
class DeserializationDependencyHolder
{
public:
  template <typename T> void put (T &&dependency)
  {
    deps_[typeid (T)] = std::forward<T> (dependency);
  }

  template <typename T> T &get () const
  {
    auto it = deps_.find (typeid (T));
    if (it == deps_.end ())
      {
        throw std::runtime_error (
          "Missing dependency: " + std::string (typeid (T).name ()));
      }
    return *std::any_cast<T> (&it->second);
  }

private:
  mutable std::unordered_map<std::type_index, std::any> deps_;
};

template <typename T>
concept ConstructibleWithDependencyHolder =
  std::is_constructible_v<T, const DeserializationDependencyHolder &>;
