#include <type_traits>

/** @brief Pathological cases of noexcept */
struct Foo {
    /**
     * @brief Combined default and noexcept
     *
     * Details.
     */
    Foo(Foo&&) noexcept = default;

    /**
     * @brief Conditional noexcept
     *
     * Details.
     */
    Foo(const Foo&) noexcept(std::is_nothrow_copy_constructible<Foo>::value);

    /**
     * @brief Combined conditional noexcept and delete
     *
     * Details.
     */
    Foo& operator=(const Foo&) noexcept(std::is_nothrow_copy_constructible<Foo>::value) = delete;

    /**
     * @brief Const, conditional noexcept and a pure virtual
     *
     * Details.
     */
    virtual void foo() const noexcept(false) = 0;

    /**
     * @brief Random type and constexpr together
     *
     * This is okay.
     */
    constexpr Foo& bar() noexcept;

    /**
     * @brief decltype(auto) and constexpr together
     *
     * For some reason, due to decltype(auto), Doxygen swaps the order, causing
     * the constexpr to be hard to detect. Don't even ask how it handles
     * trailing return types. It's just HORRIBLE.
     */
    constexpr decltype(auto) baz() noexcept;
};

/** @brief Base class */
class Base {
    public:
        /** @brief Do a thing with crazy attribs */
        virtual void doThing() const noexcept(false);

    private:
        /** @brief Do another thing, privately */
        virtual void doAnotherThing();

        /**
         * @brief Do yet another thing, privately
         *
         * The override will get hidden because `INHERIT_DOCS` is disabled.
         */
        virtual void doYetAnotherThing();

        /* Not documented, should be hidden */
        virtual void doUndocumentedThing();
};

/** @brief A derived class */
class Derived: public Base {
    protected:
        /** @brief Do a thing, overriden, now protected */
        void doThing() const noexcept(false) override;

    private:
        /**
         * @brief Do another thing, privately, with different docs
         *
         * Documented, so not hidden.
         */
        void doAnotherThing() override;

        /* Hidden because there's no doc block */
        void doYetAnotherThing() override;

        /* Hidden as well because even the parent was undocumented */
        void doUndocumentedThing() override;
};

/** @brief Various uses of final keywords */
struct Final final: protected Base {
    /**
     * @brief Final w/o override (will cause a compiler warning)
     *
     * Details.
     */
    void doThing() const noexcept(false) final;

    /**
     * @brief Final override
     *
     * Details.
     */
    void doAnotherThing() final override;

    /**
     * @brief Override final
     *
     * Details.
     */
    void doYetAnotherThing() override final;
};
