/** @file
 * @brief A file.
 */

/** @brief Strongly-typed enum */
enum class Boolean: unsigned char {
    True = 7,           /**< True. */
    False,              /**< False? */
    FileNotFound = -1   /**< Haha. */
};

/** @brief Typed enum */
enum E: int {
    Value = 42 /**< Value */
};

/** @brief Strong implicitly typed enum */
enum class F {};
