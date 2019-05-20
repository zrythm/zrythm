/** @file
 * @brief Second file
 */

/* This file needs to be kept the same as
   compound_includes_undocumented_files/Second.h except for the above @file
   block. */

namespace Spread {

/** @brief A function */
void foo();

/** @{ @name A group */

/** @brief Flag in a group */
enum Flag {};

/*@}*/

/** @related Class
 * @brief A related enum in a different file. Shouldn't be shown in namespace docs but it is :(
 */
enum RelatedEnum {};

/** @related Class
 * @brief A related typedef in a different file
 */
typedef int RelatedInt;

/** @related Class
 * @brief A related variable in a different file
 */
constexpr const int RelatedVar = 3;

/** @related Class
 * @brief A related function in a different file
 */
void relatedFunc();

/** @related Class
 * @brief A related define in a different file
 */
#define RELATED_DEFINE

}
