/** @file
 * @brief File in a subdirectory
 */

namespace Root { namespace Directory { namespace Sub {

/**
@brief A class

@section Class-section A section

Text.

@subsection Class-sub-section Subsection

@subsection Class-sub-section2 Subsection 2

@subsubsection Class-sub-section3 Subsection 3

@section Class-section2 Section 2

*/
class Class {
    public:
        /** @brief A public subclass */
        struct Foo {};

        /** @brief A public typedef */
        typedef void(*Deleter)(int, void*);

        /** @brief A enum value */
        enum E { FooBar = 3 };

        /** @brief A public static var */
        constexpr static int Size = 0;

        /** @brief A public static function */
        static void damage();

        /** @brief A constructor without parameter names */
        constexpr explicit Class(int, void*) noexcept;

        /** @brief Deleted copy */
        Class(Class&) = delete;

        /** @brief Defaulted move */
        Class& operator=(Class&&) = default;

        /** @brief A conversion operator that works only on && */
        explicit operator bool() const &&;

        /** @brief A public variable */
        std::string debug;

        /** @{ @name Group with only undocumented functions should not appear in the docs */

        void undocumented();

        /*@}*/

    protected:
        /** @brief A protected subclass */
        template<class T, class U = void> union Bar {};

        /** @brief A protected alias */
        using Type = double;

        /** @brief Protected enum */
        enum Boolean {
            True, False, FileNotFound
        };

        /** @brief A protected static var */
        static bool False;

        /** @brief A protected static function */
        static void repair();

        /** @brief A protected pure virtual destructor */
        virtual ~Class() = 0;

        /** @brief Protected function */
        constexpr int fetchMeSomeIntegers() const;

        /** @brief A protected variable */
        std::string logger;

        /** @{ */ /* Group w/o a name */

        /** @brief A member that gets ignored because the group has no name */
        int member;

        /*@}*/

        /** @{ @name Group full of non-public stuff which should be marked as such */

        /** @brief Protected flag in a group */
        enum Flag {};

        /** @brief Protected alias in a group */
        using Main = void;

        /** @brief Protected function in a group */
        void foo() const &;

        /** @brief Protected variable in a group */
        void* variable;

    private:
        /** @brief Private virtual function in a group */
        virtual int doStuff() = 0;

        /** @brief Private non-virtual function in a group shouldn't appear in the docs */
        int doStuffIgnored();

        /*@}*/

        /** @brief This shouldn't appear in the docs */
        class Private {};

        /** @brief A documented private virtual function */
        virtual int doSomething() const;

        /* Undocumented private virtual function shouldn't appear in the docs */
        virtual int boom() = 0;

        /** @brief This shouldn't appear in the docs */
        void foobar();
};

/** @relatedalso Class
 * @brief An enum
 */
enum Enum {};

/** @relatedalso Class
 * @brief A typedef
 */
typedef int Int;

/** @relatedalso Class
 * @brief An using declaration
 */
using Float = float;

/** @relatedalso Class
 * @brief A variable
 */
constexpr const int Var = 3;

/** @relatedalso Class
 * @brief A function
 */
void foo();

}}}

/** @relatesalso Root::Directory::Sub::Class
 * @brief A macro
 */
#define A_MACRO(foo) bar
