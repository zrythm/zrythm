/** @file
 * @brief The file
 */

/** @brief A base class in the root namespace */
struct Base {};

/** @brief A namespace */
namespace Namespace {

/** @brief A namespaced class with both base and derived in the root NS */
struct BothBaseAndDerivedInRootNamespace: Base {}

}

/** @brief A derived class in the root namespace */
struct Derived: Namespace::BothBaseAndDerivedInRootNamespace {};
