namespace {

/** @brief Struct in an anonymous namespace */
struct PrivateStruct { int a; /**< A var */ };

}

/** @brief A class */
class A {
    private:
        /** @brief Private class */
        class PrivateClass { int a; /**< A var */ };
};

/** @brief A class with just brief docs */
class Brief {};
