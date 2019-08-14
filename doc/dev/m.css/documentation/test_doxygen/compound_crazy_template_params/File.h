#include <type_traits>

/** @file
 * @brief A file
 */

/**
@brief Templates can get quite crazy
@tparam T yes they
@param value can
*/
template<class T, std::enable_if<!std::is_function_v<T> && std::is_class_v<T>>* = nullptr> void foo(T* value);

/**
@brief Templates can get quite crazy
@tparam T yes they
@tparam ptr absolutely
@param value can
*/
template<class T, std::enable_if<!std::is_function_v<T> && std::is_class_v<T>>* ptr = nullptr> void bar(T* value);

}
