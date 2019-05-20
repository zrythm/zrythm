#ifndef DOXYGEN_GENERATING_OUTPUT
#define MAGNUM_EXPORT __attribute__ ((visibility ("default")))
#else
#define MAGNUM_EXPORT
#endif

/** @brief Root namespace */
namespace Magnum {

/** @brief Always returns `true` */
constexpr bool MAGNUM_EXPORT hasCoolThings() { return true; }

}
