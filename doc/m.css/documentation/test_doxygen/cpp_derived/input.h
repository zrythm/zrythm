namespace Namespace {

/** @brief Private base class, should not list any derived */
class PrivateBase {};

}

namespace Another {

/** @brief Protected base, should list a fully-qualified derived, but w/o any label */
class ProtectedBase {};

}

namespace Namespace {

class UndocumentedBase {};

/** @brief Virtual base, should list a derived (omitting the common namespace), but w/o any label */
class VirtualBase {};

/**
@brief A class

Should list one protected base (fully qualified) and one virtual base (omitting
the common namespace), one derived class (fully qualified).
*/
class A: PrivateBase, protected Another::ProtectedBase, public UndocumentedBase, public virtual VirtualBase {};

}

/** @brief Another namespace */
namespace Another {

/** @brief A derived class */
class Derived: public Namespace::A {};

/** @brief A final derived class */
struct Final final: Namespace::A {};

}

namespace Namespace {

struct UndocumentedDerived: A {};

}

/** @brief A base class outside of a namespace */
class BaseOutsideANamespace {};

/** @brief A derived class outside of a namespace */
class DerivedOutsideANamespace: public BaseOutsideANamespace {};
