/** @file
 * @brief File in directory
 */

namespace Root { namespace Directory {

/** @brief A class */
class Class {};

/** @brief A structure */
struct Struct {};

/** @brief An union */
union Union {};

/** @brief An enum */
enum Enum {};

/** @brief A typedef */
typedef int Int;

/** @brief An using declaration */
using Float = float;

/** @brief A variable */
constexpr const int Var = 3;

/** @brief A function */
void foo();

/** @{ @name A group */

/** @brief Flag in a group */
enum Flag {};

/** @brief Alias in a group */
using Main = void;

/** @brief Function in a group */
void bar();

/** @brief Variable in a group */
constexpr void* variable = nullptr;

/** @brief A define in a group */
#define A_DEFINE_IN_A_GROUP

/*@}*/

}}

/** @brief A define */
#define A_DEFINE
