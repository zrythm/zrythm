/** @brief Base class */
class Base {
    virtual void doThing()noexcept;
    virtual void doAnotherThing();
    virtual void doYetAnotherThing();
};

/** @brief Keywords without spaces */
struct Foo: Base {
    /** @brief Final w/o override (will cause a compiler warning), w/o a space */
    void doThing() &&noexcept final;

    /** @brief Do more things, without a space */
    void doMoreStuff() &&noexcept(false);

    /** @brief Final override, without a space */
    void doAnotherThing() &&final override;

    /** @brief Override final, without a space */
    void doYetAnotherThing() &&override final;
};
