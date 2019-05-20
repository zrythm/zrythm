/** @defgroup group A group

Detailed description.
@{ */

/** @brief A foo */
void foo();

/*@}*/

/** @defgroup group2 Another group
@brief Brief description
@{ */

/** @brief A bar */
void bar();

/*@}*/

/**
@defgroup subgroup A subgroup
@brief Subgroup brief description
@ingroup group

@section subgroup-description Description

Subgroup description. There are **no members**, so there should be also no
Reference section in the TOC.
*/
