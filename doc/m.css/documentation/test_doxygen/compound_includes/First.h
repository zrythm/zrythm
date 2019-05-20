/** @file
 * @brief First file
 */

/* This file needs to be kept the same as
   compound_includes_undocumented_files/First.h except for the above @file
   block. */

/**
@brief This namespace is contained in a single file

So it has only the global include and no per-entry includes, thus also no
detailed sections because there's only brief. (Unless the includes are disabled
globally or the file is not documented.)
*/
namespace Contained {

/** @brief An enum */
enum Enum {};

/** @brief A typedef */
typedef int Int;

/** @brief A variable */
constexpr const int Var = 3;

/** @brief A function */
void foo();

/** @{ @name A group */

/** @brief Flag in a group */
enum Flag {};

/** @brief Alias in a group */
using Main = void;

/** @brief Function in a group */
void bar();

/** @brief Variable in a group */
constexpr void* variable = nullptr;

/*@}*/

}

/**
@brief This namespace is spread over two files

So it has no global include but per-entry includes, properly expanding the
brief-only docs into detailed ones. (Unless the includes are disabled globally
or the files are not documented.)
*/
namespace Spread {

/** @brief An enum */
enum Enum {};

/** @brief A typedef */
typedef int Int;

/** @brief A variable */
constexpr const int Var = 3;

/** @{ @name A group */

/** @brief Alias in a group */
using Main = void;

/** @brief Function in a group */
void bar();

/** @brief Variable in a group */
constexpr void* variable = nullptr;

/*@}*/

}

/**
@brief A class

Global include information for this one. (Unless the includes are disabled
globally or the file is not documented.)
*/
class Class {
    public:
        /** @brief No include information for this one (and thus no details) */
        void foo();
};

/** @defgroup group A group

All entries inside should have include information. (Unless the includes are disabled globally or the file is not documented.)
@{ */

/** @brief An enum */
enum Enum {};

/** @brief A typedef */
typedef int Int;

/** @brief A variable */
constexpr const int Var = 3;

/** @brief A function */
void foo();

/** @brief A define */
#define A_DEFINE

/*@}*/

/**
@brief This namespace should not have a global include

Even though it's in a single file --- the contained namespace might or might
not be in the same file and that could be misleading.
*/
namespace ContainsNamespace {
    /**
     * @brief This namespace should not have a global include either
     *
     * Because it has a class inside.
     */
    namespace ContainsClass {
        /** @brief Union */
        union Union {};

        /** @brief This function should also have a local include */
        void foo();
    }

    /** @brief This function should have a local include */
    void foo();
}

/**
@brief This namespace should have a global include

Even though it has no members that could set the global include for it.
*/
namespace Empty {}
