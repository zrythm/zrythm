/** @dir DeprecatedDir
 * @brief A directory
 * @deprecated This dir is deprecated.
 */

/** @file
 * @brief A file
 * @deprecated This file is deprecated.
 */

/**
@brief A namespace
@deprecated This namespace is deprecated.
*/
namespace DeprecatedNamespace {

/**
@brief A class
@deprecated This class is deprecated.
*/
struct DeprecatedClass {};

/**
@brief A function
@deprecated This function is deprecated.
*/
void deprecatedFoo(int a, bool b, double c);

/**
@brief A variable
@deprecated This variable is deprecated.
*/
constexpr int DeprecatedVariable = 5;

/**
@brief A typedef
@deprecated This typedef is deprecated.
*/
typedef int DeprecatedTypedef;

/**
@brief An enum
@deprecated This enum is deprecated.
*/
enum class DeprecatedEnum {
    /** Enum value */
    Value = 15
};

/** @brief An enum */
enum class Enum {
    /**
     * Enum value
     * @deprecated This enum is deprecated.
     */
    DeprecatedValue = 15
};

/** @defgroup deprecated-group A group
 * @deprecated This group is deprecated.
 */

/**
@brief An union
@deprecated This union is deprecated.
*/
union DeprecatedUnion {};

/**
@brief A struct
@deprecated This struct is deprecated.
*/
struct DeprecatedStruct {};

/*@}*/

}

/**
@brief A macro
@deprecated This macro is deprecated
*/
#define DEPRECATED_MACRO(a, b, c)
