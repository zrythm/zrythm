/** @file
 * @brief A file
 */

/**
@brief Class with template parameters
@tparam T Template parameter T
@tparam U Template parameter U

Should have it displayed on top.
*/
template<class T, class U = void, class = int> struct Template {
    /**
     * @brief Another
     *
     * Should have just one template with _3 for unnamed parameter.
     */
    void bar();

    protected:
        /**
        * @brief Variable
        *
        * Should have just one template with _3 for unnamed parameter. Should
        * also have the `protected` label in the details.
        */
        int a;

        /**
        * @brief Enum
        *
        * Should have just one template with _3 for unnamed parameter. Should
        * also have the `protected` label in the details.
        */
        enum Bar {};

        /**
         * @brief Typedef
         *
         * Should have just one template with _3 for unnamed parameter. Should
         * also have the `protected` label in the details.
         */
        typedef A B;

        /**
         * @brief Template alias
         *
         * Should have just one template with _3 for unnamed parameter. Should
         * also have the `protected` label in the details. Template parameters
         * are tested in `cpp_template_aliases` test as they need newer
         * Doxygen.
         */
        using Foo = Buuu<U, false>;

        /**
        * @brief Function
        * @tparam V Well, this is V
        *
        * Should not repeat default parameter from class, should include both U
        * and V in the docs. Should also have the `protected` label in the
        * details.
        */
        template<class V, int = 3> void foo();
};

/**
@brief Specialized class template

Should have `template<>` displayed on top.
*/
template<> struct Template<void> {
    /**
     * @brief Function
     *
     * Should still have both templates repeated.
     */
    template<int b> void baz();
};

/**
@brief Class with wrong template parameter description
@param T Documented wrongly as parameter
@tparam WTF And this one does not exist
*/
template<class T> struct TemplateWarning {};

/**
@brief Namespace docs

And we have some detailed docs as well.
*/
namespace Namee {}

/** @brief A namespace */
namespace Foo {

/**
@brief Function with *everything*
@tparam T A template, innit
@param a        That's a for you
@param b        Well, a string
@param things   And an array!
@param stuff    Another array
@return It returns!
@retval 0       Zero?
@retval 42      The Answer.

Ooooh, more text!
*/
template<class T> int foo(int a, std::string b, char(&things)[5], bool, char(&)[42], int stuff[], double[1337]);

/**
@brief Input and output
@param[in]      in      Input
@param[out]     out     Output
@param[in,out]  shit    Well, that's messy
*/
constexpr void bar(int in, int& out, void* shit) noexcept;

/**
@brief Function
@return With just return value docs should still have detailed section
*/
int justReturn();

/**
@brief Function
@retval 42 With just return value docs should still have detailed section
*/
int justReturnValues();

/**
@brief Function
@exception std::bad_exception With just exception docs should still have detailed section
*/
int justExceptions();

/**
@brief A function with scattered docs

@param a    First parameter docs

@tparam B   Second template parameter docs

This is a function that has the docs all scattered around. They should get
merged and reordered.

@tparam A   First template parameter docs

@param b    Second parameter docs

That goes also for the return values.

@retval 0       Zero

Yes?

 - We also need to
 - extract them out of a list
@retval 1337    1337 h4xx0r?!
@retval 42      The answer. To everything

@exception std::bad_function_call if you call the function bad
@exception std::future_error if you are from the future
*/
template<class A, class B> int bar(int a, int b);

/**
@brief Function with one description for all params
@param x, y, z Coordinates in 3D space
*/
void thisIsAShittyWayToPassAVectorButWhatever(float x, float y, float z);

}

/** @brief A namespace */
namespace Eno {

/** @brief Boolean */
enum Boolean {
    True = 7,           /**< True. */
    False,              /**< False? */
    FileNotFound = -1   /**< Haha. */
};

enum {
    /** Value of an anonymous enum */
    Value = 34
};

}

/** @brief A namespace */
namespace Type {

/**
@brief Another typedef

Details.
*/
typedef Me Ugly;

/**
@brief Function pointer typedef

Huh.
*/
typedef void(*Func)(int, std::string&);

}

/** @brief A namespace */
namespace Var {

/**
@brief A value

Details.
*/
constexpr const int a = 25;

}

/** @brief A namespace */
namespace Warning {

/**
@brief Wrong
@param wrong This parameter is not here
@return Returns nothing.
@return Returns nothing, but second time. This is ignored.

Function details.

@return Returns nothing, third time, in a different paragraph. Ignored as well.
*/
void bar();

}

/**
@brief A define

Details.
*/
#define A_DEFINE

/**
@brief A macro
@return Hahah. Nothing.
*/
#define A_MACRO()

/**
@brief Macro with parameters
@param foo Foo to bar
@param bar Bar to foo

Details? No.
*/
#define MACRO(foo, bar)
