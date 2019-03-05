/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2017 Michael Drake <tlsa@netsurf-browser.org>
 */

/**
 * \file
 * \brief CYAML common utility functions.
 */

#ifndef CYAML_UTIL_H
#define CYAML_UTIL_H

#include "cyaml/cyaml.h"
#include "utf8.h"

/** Compile time assertion macro. */
#define cyaml_static_assert(e) \
{ \
	enum { \
		cyaml_static_assert_check = 1 / (!!(e)) \
	}; \
}

/** Macro to squash unused variable compiler warnings. */
#define CYAML_UNUSED(_x) ((void)(_x))

/** CYAML bitfield type. */
typedef uint32_t cyaml_bitfield_t;

/** Number of bits in \ref cyaml_bitfield_t. */
#define CYAML_BITFIELD_BITS (sizeof(cyaml_bitfield_t) * CHAR_BIT)

/** CYAML state machine states. */
enum cyaml_state_e {
	CYAML_STATE_START,        /**< Initial state. */
	CYAML_STATE_IN_STREAM,    /**< In a stream. */
	CYAML_STATE_IN_DOC,       /**< In a document. */
	CYAML_STATE_IN_MAP_KEY,   /**< In a mapping. */
	CYAML_STATE_IN_MAP_VALUE, /**< In a mapping. */
	CYAML_STATE_IN_SEQUENCE,  /**< In a sequence. */
	CYAML_STATE__COUNT,       /**< Count of states, **not a valid
	                               state itself**. */
};

/**
 * Convert a CYAML state into a human readable string.
 *
 * \param[in]  state  The state to convert.
 * \return String representing state.
 */
static inline const char * cyaml__state_to_str(enum cyaml_state_e state)
{
	static const char * const strings[CYAML_STATE__COUNT] = {
		[CYAML_STATE_START]        = "start",
		[CYAML_STATE_IN_STREAM]    = "in stream",
		[CYAML_STATE_IN_DOC]       = "in doc",
		[CYAML_STATE_IN_MAP_KEY]   = "in mapping (key)",
		[CYAML_STATE_IN_MAP_VALUE] = "in mapping (value)",
		[CYAML_STATE_IN_SEQUENCE]  = "in sequence",
	};
	if ((unsigned)state >= CYAML_STATE__COUNT) {
		return "<invalid>";
	}
	return strings[state];
}

/**
 * Convert a CYAML type into a human readable string.
 *
 * \param[in]  type  The state to convert.
 * \return String representing state.
 */
static inline const char * cyaml__type_to_str(cyaml_type_e type)
{
	static const char * const strings[CYAML__TYPE_COUNT] = {
		[CYAML_INT]            = "INT",
		[CYAML_UINT]           = "UINT",
		[CYAML_BOOL]           = "BOOL",
		[CYAML_ENUM]           = "ENUM",
		[CYAML_FLAGS]          = "FLAGS",
		[CYAML_FLOAT]          = "FLOAT",
		[CYAML_STRING]         = "STRING",
		[CYAML_MAPPING]        = "MAPPING",
		[CYAML_SEQUENCE]       = "SEQUENCE",
		[CYAML_SEQUENCE_FIXED] = "SEQUENCE_FIXED",
		[CYAML_IGNORE]         = "IGNORE",
	};
	if ((unsigned)type >= CYAML__TYPE_COUNT) {
		return "<invalid>";
	}
	return strings[type];
}

/**
 * Log to client's logging function, if provided.
 *
 * \param[in] cfg    CYAML client config structure.
 * \param[in] level  Log level of message to log.
 * \param[in] fmt    Format string for message to log.
 * \param[in] ...    Additional arguments used by fmt.
 */
static inline void cyaml__log(
		const cyaml_config_t *cfg,
		cyaml_log_t level,
		char *fmt, ...)
{
	if (level >= cfg->log_level && cfg->log_fn != NULL) {
		va_list args;
		va_start(args, fmt);
		cfg->log_fn(level, fmt, args);
		va_end(args);
	}
}

/**
 * Check if comparason should be case sensitive.
 *
 * As described in the API, schema flags take priority over config flags.
 *
 * \param[in]  config  Client's CYAML configuration structure.
 * \param[in]  schema  The CYAML schema for the value to be compared.
 * \return Whether to use case-sensitive comparason.
 */
static inline bool cyaml__is_case_sensitive(
		const cyaml_config_t *config,
		const cyaml_schema_value_t *schema)
{
	if (schema->flags & CYAML_FLAG_CASE_INSENSITIVE) {
		return false;

	} else if (schema->flags & CYAML_FLAG_CASE_SENSITIVE) {
		return true;

	} else if (config->flags & CYAML_CFG_CASE_INSENSITIVE) {
		return false;

	}

	return true;
}

/**
 * Compare two strings.
 *
 * Depending on the client's configuration, and the value's schema,
 * this will do either a case-sensitive or case-insensitive comparason.
 *
 * \param[in]  config  Client's CYAML configuration structure.
 * \param[in]  schema  The CYAML schema for the value to be compared.
 * \param[in]  str1    First string to be compared.
 * \param[in]  str2    Second string to be compared.
 * \return 0 if and only if strings are equal.
 */
static inline int cyaml__strcmp(
		const cyaml_config_t *config,
		const cyaml_schema_value_t *schema,
		const void * const str1,
		const void * const str2)
{
	if (cyaml__is_case_sensitive(config, schema)) {
		return strcmp(str1, str2);
	}

	return cyaml_utf8_casecmp(str1, str2);
}

#endif
