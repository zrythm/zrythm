/** @file
 * @brief First file
 */

/**
@brief This namespace is spread over two files

So it has per-entry includes next to template info, properly expanding the
brief-only docs into detailed ones.
*/
namespace Spread {

/** @brief A template alias */
template<class T> using Alias = T;

/** @brief A templated function */
template<class T> void bar();

}

/**
@brief A templated struct

Has global include info next to the template info.
*/
template<class T> struct Struct {};
