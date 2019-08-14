/** @brief A non-template class */
struct Foo {
    /** @brief Template variable without template docs */
    template<class T> static T* variable;

    /**
     * @brief Template variable with template docs
     * @tparam T Well, the type
     */
    template<class T> static T& another;
};

/** @brief Template class */
template<class T> struct Bar {
    /** @brief Template variable inside a template class without template docs */
    template<class U> static Foo<U>* instance;

    /**
     * @brief Template variable inside a template class with template docs
     * @tparam U Well, the type
     */
    template<class U> static Foo<U>& another;
};
