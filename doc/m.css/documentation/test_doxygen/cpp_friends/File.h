/** @file
 * @brief A file
 */

/**
@brief A friend class

Not displayed among @ref Class friends becase Doxygen doesn't provide any
useful info for it.
*/
class FriendClassWarning {};

/**
@brief A friend class

Not displayed among @ref Class friends becase Doxygen doesn't provide any
useful info for it.
*/
class GroupedFriendClassWarning {};

/** @brief A class */
class Class {
    public:
        /* Ignored */
        friend class FriendClass;
        friend struct FriendStruct;
        friend union FriendUnion;

        /** @brief Ignored friend class with a warning because it has docs */
        friend class FriendClassWarning;

        /** @brief A friend function */
        friend void friendFunction(int a, void* b);

        /** @{ @name Group with friend functions */

        /** @brief Ignored friend class with a warning because it has docs */
        friend class GroupedFriendClassWarning;

        /** @brief A friend grouped function */
        friend void friendGroupedFunction();

        /*@}*/
};

/** @brief Class with template parameters */
template<class T, class U = void, class = int> class Template {
    protected: /* Shouldn't matter */
        /**
         * @brief Friend function
         * @tparam V This is a V
         *
         * This is broken in doxygen itself as it doesn't include any scope
         * from the class.
         */
        template<class V> friend void foobar();
};
