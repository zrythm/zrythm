/** @file
 * @brief A file
 */

/** @brief A namespace */
namespace Namespace {

/**
@brief A function

Detailed function docs.
*/
void foo();

/** @brief Function with just a brief */
void fooBrief();

/**
@brief An enum

Detailed enum docs.
*/
enum Enum {
    Value /**< A value */
};

/** @brief Enum with just a brief */
enum EnumBrief {};

/**
@brief A typedef

Detailed typedef docs.
*/
typedef int Typedef;

/** @brief Typedef with just a brief */
typedef int TypedefBrief;

/**
@brief A variable

Detailed variable docs.
*/
constexpr int Variable = 5;

/** @brief Variable with just a brief */
constexpr int VariableBrief = 5;

/**
@brief A macro

This appears only in the file docs and fully expanded.
*/
#define A_MACRO

}

/* For the undocumented namespace, everything should appear in file docs */
namespace UndocumentedNamespace {

/**
@brief A function

Detailed function docs.
*/
void foo();

/** @brief Function with just a brief */
void fooBrief();

/**
@brief An enum

Detailed enum docs.
*/
enum Enum {
    Value /**< A value */
};

/** @brief Enum with just a brief */
enum EnumBrief {};

/**
@brief A typedef

Detailed typedef docs.
*/
typedef int Typedef;

/** @brief Typedef with just a brief */
typedef int TypedefBrief;

/**
@brief A variable

Detailed variable docs.
*/
constexpr int Variable = 5;

/** @brief Variable with just a brief */
constexpr int VariableBrief = 5;

}
