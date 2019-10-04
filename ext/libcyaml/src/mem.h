/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2018 Michael Drake <tlsa@netsurf-browser.org>
 */

/**
 * \file
 * \brief CYAML memory allocation handling.
 */

#ifndef CYAML_MEM_H
#define CYAML_MEM_H

#include "cyaml/cyaml.h"

/**
 * Helper for freeing using the client's choice of allocator routine.
 *
 * \param[in]  config  The CYAML client config.
 * \param[in]  ptr     Pointer to allocation to free.
 */
static inline void cyaml__free(
		const cyaml_config_t *config,
		void *ptr)
{
	config->mem_fn(config->mem_ctx, ptr, 0);
}

/**
 * Helper for new allocations using the client's choice of allocator routine.
 *
 * \note On failure, any existing allocation is still owned by the caller, and
 *       they are responsible for freeing it.
 *
 * \param[in]  config        The CYAML client config.
 * \param[in]  ptr           The existing allocation or NULL.
 * \param[in]  current_size  Size of the current allocation.  (Only needed if
 *                           `clean != false`).
 * \param[in]  new_size      The number of bytes to resize allocation to.
 * \param[in]  clean         Only applies if `new_size > current_size`.
 *                           If `false`, the new memory is uninitialised,
 *                           if `true`, the new memory is initialised to zero.
 * \return Pointer to allocation on success, or `NULL` on failure.
 */
static inline void * cyaml__realloc(
		const cyaml_config_t *config,
		void *ptr,
		size_t current_size,
		size_t new_size,
		bool clean)
{
	uint8_t *temp = config->mem_fn(config->mem_ctx, ptr, new_size);
	if (temp == NULL) {
		return NULL;
	}

	if (clean && (new_size > current_size)) {
		memset(temp + current_size, 0, new_size - current_size);
	}

	return temp;
}

/**
 * Helper for new allocations using the client's choice of allocator routine.
 *
 * \param[in]  config  The CYAML client config.
 * \param[in]  size    The number of bytes to allocate.
 * \param[in]  clean   If `false`, the memory is uninitialised, if `true`,
 *                     the memory is initialised to zero.
 * \return Pointer to allocation on success, or `NULL` on failure.
 */
static inline void * cyaml__alloc(
		const cyaml_config_t *config,
		size_t size,
		bool clean)
{
	return cyaml__realloc(config, NULL, 0, size, clean);
}

/**
 * Helper for string duplication using the client's choice of allocator routine.
 *
 * \param[in]  config  The CYAML client config.
 * \param[in]  str     The string to duplicate.
 * \return Pointer to new string on success, or `NULL` on failure.
 */
static inline char * cyaml__strdup(
		const cyaml_config_t *config,
		const char *str)
{
	size_t len = strlen(str) + 1;
	char *dup = cyaml__alloc(config, len, false);
	if (dup == NULL) {
		return NULL;
	}

	memcpy(dup, str, len);
	return dup;
}

#endif
