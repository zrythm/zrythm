/** @file
 * @brief A file.
 */

/** @brief Class with template parameters */
template<class T, class U = void, class = int> struct Template {
    /**
     * @brief Template alias
     * @tparam V Well, this is V as well
     *
     * Should include both template lists, with _3 for unnamed parameter.
     */
    template<class V, bool = false> using Foo = Buuu<V, false>;
};

/**
@brief A templated type with just template details
@tparam T Template param
*/
template<class T, typename = void> using Foo = Bar<T, int>;
