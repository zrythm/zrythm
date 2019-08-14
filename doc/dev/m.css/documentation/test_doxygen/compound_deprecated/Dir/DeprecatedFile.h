/** @dir Dir
 * @brief A directory
 */

/** @dir DeprecatedSubdir
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
@brief A base class
@deprecated This class is deprecated.
*/
struct BaseDeprecatedClass {};

/**
@brief A class
@deprecated This class is deprecated.
*/
struct DeprecatedClass: BaseDeprecatedClass {};

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
enum DeprecatedEnum {
    /** Enum value */
    Value = 15
};

/** @brief An enum */
enum Enum {
    /**
     * Enum value
     * @deprecated This enum is deprecated.
     */
    DeprecatedValue = 15
};

/** @defgroup group A group
 * @{
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

/** @defgroup deprecated-group A group
 * @ingroup group
 * @deprecated This group is deprecated.
 */

}

/**
@brief A macro
@deprecated This macro is deprecated
*/
#define DEPRECATED_MACRO(a, b, c)
