/** @file
 * @brief A file
 */

/* Preprocessor defines are usually first in the XML file, but if there is a
   user-defined group, that is first. That triggers a corner case in define ID
   extraction -- parsing the var will set current base url to namespaceNS,
   but that shouldn't trigger any error when parsing the define after. */

/** @brief A namespace */
namespace NS {

/** @{ @name Variables */

int var; /**< @brief A variable */

/*@}*/

}

/** @brief A define */
#define A_DEFINE
