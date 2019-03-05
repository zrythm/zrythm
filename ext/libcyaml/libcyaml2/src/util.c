/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2017 Michael Drake <tlsa@netsurf-browser.org>
 */

/**
 * \file
 * \brief Utility functions.
 */

#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "util.h"

/** Flag that indicates a release in \ref cyaml_version. */
#define CYAML_RELEASE_FLAG (1u << 31)

/** Stringification helper macro. */
#define CYAML_STR_HELPER(_x) #_x

/** Stringification macro. */
#define CYAML_STR(_x) CYAML_STR_HELPER(_x)

/* Version depends on whether we're a development build. */
#if VERSION_DEVEL
	/** Version string is composed from components in Makefile. */
	#define CYAML_VERSION_STR \
			CYAML_STR(VERSION_MAJOR) "." \
			CYAML_STR(VERSION_MINOR) "." \
			CYAML_STR(VERSION_PATCH) "-DEVEL"

	/* Exported constant, documented in include/cyaml/cyaml.h */
	const uint32_t cyaml_version =
			((VERSION_MAJOR << 16) |
			 (VERSION_MINOR <<  8) |
			 (VERSION_PATCH <<  0));
#else
	/** Version string is composed from components in Makefile. */
	#define CYAML_VERSION_STR \
			CYAML_STR(VERSION_MAJOR) "." \
			CYAML_STR(VERSION_MINOR) "." \
			CYAML_STR(VERSION_PATCH)

	/* Exported constant, documented in include/cyaml/cyaml.h */
	const uint32_t cyaml_version =
			((VERSION_MAJOR << 16) |
			 (VERSION_MINOR <<  8) |
			 (VERSION_PATCH <<  0) |
			 CYAML_RELEASE_FLAG);
#endif

/* Exported constant, documented in include/cyaml/cyaml.h */
const char *cyaml_version_str = CYAML_VERSION_STR;

/* Exported function, documented in include/cyaml/cyaml.h */
void cyaml_log(
		cyaml_log_t level,
		const char *fmt,
		va_list args)
{
	static const char * const strings[] = {
		[CYAML_LOG_DEBUG]   = "DEBUG",
		[CYAML_LOG_INFO]    = "INFO",
		[CYAML_LOG_NOTICE]  = "NOTICE",
		[CYAML_LOG_WARNING] = "WARNING",
		[CYAML_LOG_ERROR]   = "ERROR",
	};
	fprintf(stderr, "libcyaml: %s: ", strings[level]);
	vfprintf(stderr, fmt, args);
}

/* Exported function, documented in include/cyaml/cyaml.h */
const char * cyaml_strerror(
		cyaml_err_t err)
{
	static const char * const strings[CYAML_ERR__COUNT] = {
		[CYAML_OK]                        = "Success",
		[CYAML_ERR_OOM]                   = "Memory allocation failed",
		[CYAML_ERR_ALIAS]                 = "YAML alias unsupported",
		[CYAML_ERR_FILE_OPEN]             = "Could not open file",
		[CYAML_ERR_INVALID_KEY]           = "Invalid key",
		[CYAML_ERR_INVALID_VALUE]         = "Invalid value",
		[CYAML_ERR_INTERNAL_ERROR]        = "Internal error",
		[CYAML_ERR_UNEXPECTED_EVENT]      = "Unexpected event",
		[CYAML_ERR_STRING_LENGTH_MIN]     = "String length too short",
		[CYAML_ERR_STRING_LENGTH_MAX]     = "String length too long",
		[CYAML_ERR_INVALID_DATA_SIZE]     = "Data size must be 0 < X <= 8 bytes",
		[CYAML_ERR_TOP_LEVEL_NON_PTR]     = "Top-level schema value must be pointer",
		[CYAML_ERR_BAD_TYPE_IN_SCHEMA]    = "Schema contains invalid type",
		[CYAML_ERR_BAD_MIN_MAX_SCHEMA]    = "Bad schema: min exceeds max",
		[CYAML_ERR_BAD_PARAM_SEQ_COUNT]   = "Bad parameter: seq_count",
		[CYAML_ERR_BAD_PARAM_NULL_DATA]   = "Bad parameter: NULL data",
		[CYAML_ERR_SEQUENCE_ENTRIES_MIN]  = "Sequence with too few entries",
		[CYAML_ERR_SEQUENCE_ENTRIES_MAX]  = "Sequence with too many entries",
		[CYAML_ERR_SEQUENCE_FIXED_COUNT]  = "Sequence fixed has unequal min max",
		[CYAML_ERR_SEQUENCE_IN_SEQUENCE]  = "Non-fixed sequence in sequence",
		[CYAML_ERR_MAPPING_FIELD_MISSING] = "Missing required mapping field",
		[CYAML_ERR_BAD_CONFIG_NULL_MEMFN] = "Bad config: NULL mem function",
		[CYAML_ERR_BAD_PARAM_NULL_CONFIG] = "Bad parameter: NULL config",
		[CYAML_ERR_BAD_PARAM_NULL_SCHEMA] = "Bad parameter: NULL schema",
		[CYAML_ERR_LIBYAML_EMITTER_INIT]  = "libyaml emitter init failed",
		[CYAML_ERR_LIBYAML_PARSER_INIT]   = "libyaml parser init failed",
		[CYAML_ERR_LIBYAML_EVENT_INIT]    = "libyaml event init failed",
		[CYAML_ERR_LIBYAML_EMITTER]       = "libyaml emitter error",
		[CYAML_ERR_LIBYAML_PARSER]        = "libyaml parser error",
	};
	if ((unsigned)err >= CYAML_ERR__COUNT) {
		return "Invalid error code";
	}
	return strings[err];
}
