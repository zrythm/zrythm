/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2017-2018 Michael Drake <tlsa@netsurf-browser.org>
 */

/**
 * \file
 * \brief Free data structures created by the CYAML load functions.
 *
 * As described in the public API for \ref cyaml_free(), it is preferable for
 * clients to write their own free routines, tailored for their data structure.
 *
 * Recursion and stack usage
 * -------------------------
 *
 * This generic CYAML free routine is implemented using recursion, rather
 * than iteration with a heap-allocated stack.  This is because recursion
 * seems less bad than allocating within the free code, and the stack-cost
 * of these functions isn't huge.  The maximum recursion depth is of course
 * bound by the schema, however schemas for recursively nesting data structures
 * are unbound, e.g. for a data tree structure.
 */

#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "data.h"
#include "util.h"
#include "mem.h"

/**
 * Internal function for freeing a CYAML-parsed data structure.
 *
 * \param[in]  cfg     The client's CYAML library config.
 * \param[in]  schema  The schema describing how to free `data`.
 * \param[in]  data    The data structure to be freed.
 * \param[in]  count   If data is of type \ref CYAML_SEQUENCE, this is the
 *                     number of entries in the sequence.
 */
static void cyaml__free_value(
		const cyaml_config_t *cfg,
		const cyaml_schema_value_t *schema,
		uint8_t * data,
		unsigned count);

/**
 * Internal function for freeing a CYAML-parsed sequence.
 *
 * \param[in]  cfg              The client's CYAML library config.
 * \param[in]  sequence_schema  The schema describing how to free `data`.
 * \param[in]  data             The data structure to be freed.
 * \param[in]  count            The sequence's entry count.
 */
static void cyaml__free_sequence(
		const cyaml_config_t *cfg,
		const cyaml_schema_value_t *sequence_schema,
		uint8_t * const data,
		unsigned count)
{
	const cyaml_schema_value_t *schema = sequence_schema->sequence.entry;
	uint32_t data_size = schema->data_size;

	if (schema->flags & CYAML_FLAG_POINTER) {
		data_size = sizeof(data);
	}

	for (unsigned i = 0; i < count; i++) {
		cyaml__log(cfg, CYAML_LOG_DEBUG, "Freeing sequence entry: %u\n", i);
		cyaml__free_value(cfg, schema, data + data_size * i, 0);
	}
}

/**
 * Internal function for freeing a CYAML-parsed mapping.
 *
 * \param[in]  cfg             The client's CYAML library config.
 * \param[in]  mapping_schema  The schema describing how to free `data`.
 * \param[in]  data            The data structure to be freed.
 */
static void cyaml__free_mapping(
		const cyaml_config_t *cfg,
		const cyaml_schema_value_t *mapping_schema,
		uint8_t * const data)
{
	const cyaml_schema_field_t *schema = mapping_schema->mapping.fields;

	while (schema->key != NULL) {
		unsigned count = 0;
		cyaml__log(cfg, CYAML_LOG_DEBUG, "Freeing key: %s (at offset: %u)\n",
				schema->key, (unsigned)schema->data_offset);
		if (schema->value.type == CYAML_SEQUENCE) {
			cyaml_err_t err;
			count = cyaml_data_read(schema->count_size,
					data + schema->count_offset, &err);
			if (err != CYAML_OK) {
				return;
			}
		}
		cyaml__free_value(cfg, &schema->value,
				data + schema->data_offset, count);
		schema++;
	}
}

/* This function is documented at the forward declaration above. */
static void cyaml__free_value(
		const cyaml_config_t *cfg,
		const cyaml_schema_value_t *schema,
		uint8_t * data,
		unsigned count)
{
	if (schema->flags & CYAML_FLAG_POINTER) {
		data = cyaml_data_read_pointer(data);
		if (data == NULL) {
			return;
		}
	}

	if (schema->type == CYAML_MAPPING) {
		cyaml__free_mapping(cfg, schema, data);
	} else if (schema->type == CYAML_SEQUENCE ||
	           schema->type == CYAML_SEQUENCE_FIXED) {
		if (schema->type == CYAML_SEQUENCE_FIXED) {
			count = schema->sequence.max;
		}
		cyaml__free_sequence(cfg, schema, data, count);
	}

	if (schema->flags & CYAML_FLAG_POINTER) {
		cyaml__log(cfg, CYAML_LOG_DEBUG, "Freeing: %p\n", data);
		cyaml__free(cfg, data);
	}
}

/* Exported function, documented in include/cyaml/cyaml.h */
cyaml_err_t cyaml_free(
		const cyaml_config_t *config,
		const cyaml_schema_value_t *schema,
		cyaml_data_t *data,
		unsigned seq_count)
{
	if (config == NULL) {
		return CYAML_ERR_BAD_PARAM_NULL_CONFIG;
	}
	if (config->mem_fn == NULL) {
		return CYAML_ERR_BAD_CONFIG_NULL_MEMFN;
	}
	if (schema == NULL) {
		return CYAML_ERR_BAD_PARAM_NULL_SCHEMA;
	}
	cyaml__free_value(config, schema, (void *)&data, seq_count);
	return CYAML_OK;
}
