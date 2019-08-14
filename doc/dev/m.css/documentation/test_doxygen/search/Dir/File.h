/** @dir /Dir
 * @brief A directory
 *
 * @m_keywords{glDirectory() glDir() GL_DIR}
 */

/** @file
 * @brief A file
 *
 * @m_keywords{glFile()}
 */

/**
 * @brief A namespace
 *
 * @m_keywords{glNamespace()}
 */
namespace Namespace {

/**
@brief A class

@section section2 A section, stuff inside should be propagated outside

-   Keywords inside a list
-   @note inside a note should be propagated outside: @m_keywords{glClass()}
 */
class Class {
    public:
        /**
         * @brief Function without arguments
         *
         * @m_keywords{glFoo()}
         */
        void foo();

        void foo() const; /**< @overload */

        void foo() && = delete; /**< @overload */

        /** @brief Function with arguments */
        void foo(const Enum& first, Typedef second);
};

/**
 * @brief A variable
 *
 * @m_keyword{min(),GLSL: min(),2}
 */
constexpr int Variable = 5;

/**
 * @brief A typedef
 *
 * @m_keywords{GL_TYPEDEF}
 */
typedef int Typedef;

/**
@brief An enum

@m_keywords{GL_ENUM}
@see Enum values as keywords should be propagated outside of notes as well:
    @m_enum_values_as_keywords
 */
enum class Enum {
    /** @brief Only a brief and no value */
    OnlyABrief,

    /**
     * Enum value
     *
     * @m_keywords{GL_ENUM_VALUE_EXT}
     */
    Value = GL_ENUM_VALUE
};

/** @defgroup group A group
 *
 * @m_keywords{GL_GROUP}
 * @{
 */

/**
 * @brief An union
 *
 * @m_keywords{glUnion()}
 */
union Union {};

/**
 * @brief A struct
 *
 * @m_keywords{glStruct()}
 */
struct Struct {};

/*@}*/

}

/**
 * @brief A macro
 *
 * @m_keyword{glMacro(),,}
 */
#define MACRO

/** @brief Macro function */
#define MACRO_FUNCTION()

/** @brief Macro function with params */
#define MACRO_FUNCTION_WITH_PARAMS(params)

namespace UndocumentedNamespace {}

class UndocumentedClass {};

void undocumentedFunction();

constexpr int UndocumentedVariable = 42;

typedef int UndocumentedTypedef;

enum class UndocumentedEnum {
    UndocumentedValue
};

#define UNDOCUMENTED_MACRO
