/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2018 Michael Drake <tlsa@netsurf-browser.org>
 */

/**
 * \file
 * \brief CYAML memory allocation handling.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

/** Macro to squash unused variable compiler warnings. */
#define CYAML_UNUSED(_x) ((void)(_x))

/* Exported function, documented in include/cyaml/cyaml.h */
void * cyaml_mem(
		void *ctx,
		void *ptr,
		size_t size)
{
	CYAML_UNUSED(ctx);

	if (size == 0) {
		free(ptr);
		return NULL;
	}

	return realloc(ptr, size);
}
