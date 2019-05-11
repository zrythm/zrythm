/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2017-2019 Michael Drake <tlsa@netsurf-browser.org>
 */

/**
 * \file
 * \brief Load YAML data into client's data structure.
 *
 * This uses `libyaml` to parse YAML documents, it validates the documents
 * against the client provided schema, and uses the schema to place the data
 * in the client's data structure.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>

#include <yaml.h>

#include "mem.h"
#include "data.h"
#include "util.h"

/** Identifies that no mapping schema entry was found for key. */
#define CYAML_SCHEMA_IDX_NONE 0xffff

/**
 * A CYAML load state machine stack entry.
 */
typedef struct cyaml_state {
	/** Current load state machine state. */
	enum cyaml_state_e state;
	/** Schema for the expected value in this state. */
	const cyaml_schema_value_t *schema;
	/** Anonymous union for schema type specific state. */
	union {
		/** Additional state for \ref CYAML_STATE_IN_STREAM state. */
		struct {
			/** Number of documents read in stream. */
			uint32_t doc_count;
		} stream;
		/**
		 * Additional state for \ref CYAML_STATE_IN_MAP_KEY and
		 * \ref CYAML_STATE_IN_MAP_VALUE states. */
		struct {
			const cyaml_schema_field_t *schema;
			/** Bit field of mapping fields found. */
			cyaml_bitfield_t *fields;
			uint16_t schema_idx;
			uint16_t entries_count;
		} mapping;
		/**  Additional state for \ref CYAML_STATE_IN_SEQUENCE state. */
		struct {
			uint8_t *data;
			uint8_t *count_data;
			uint32_t count;
			uint64_t count_size;
		} sequence;
	};
	uint8_t *data;
} cyaml_state_t;

/**
 * Internal YAML loading context.
 */
typedef struct cyaml_ctx {
	const cyaml_config_t *config; /**< Settings provided by client. */
	cyaml_state_t *state;   /**< Current entry in state stack, or NULL. */
	cyaml_state_t *stack;   /**< State stack */
	uint32_t stack_idx;     /**< Next (empty) state stack slot */
	uint32_t stack_max;     /**< Current stack allocation limit. */
	unsigned seq_count;     /**< Top-level sequence count. */
	yaml_parser_t *parser;  /**< Internal libyaml parser object. */
} cyaml_ctx_t;

/**
 * CYAML events.  These correspond to `libyaml` events.
 */
typedef enum cyaml_event {
	CYAML_EVT_NO_EVENT   = YAML_NO_EVENT,
	CYAML_EVT_STRM_START = YAML_STREAM_START_EVENT,
	CYAML_EVT_STRM_END   = YAML_STREAM_END_EVENT,
	CYAML_EVT_DOC_START  = YAML_DOCUMENT_START_EVENT,
	CYAML_EVT_DOC_END    = YAML_DOCUMENT_END_EVENT,
	CYAML_EVT_ALIAS      = YAML_ALIAS_EVENT,
	CYAML_EVT_SCALAR     = YAML_SCALAR_EVENT,
	CYAML_EVT_SEQ_START  = YAML_SEQUENCE_START_EVENT,
	CYAML_EVT_SEQ_END    = YAML_SEQUENCE_END_EVENT,
	CYAML_EVT_MAP_START  = YAML_MAPPING_START_EVENT,
	CYAML_EVT_MAP_END    = YAML_MAPPING_END_EVENT,
	CYAML_EVT__COUNT,
} cyaml_event_t;

/**
 * Get the CYAML event type from a `libyaml` event.
 *
 * \param[in]  event  The `libyaml` event.
 * \return corresponding CYAML event.
 */
static inline cyaml_event_t cyaml__get_event_type(const yaml_event_t *event)
{
	return (cyaml_event_t)event->type;
}

/**
 * Convert a `libyaml` event to a human readable string.
 *
 * \param[in]  event  The `libyaml` event.
 * \return String representing event.
 */
static const char * cyaml__libyaml_event_type_str(const yaml_event_t *event)
{
	static const char * const strings[] = {
		"NO_EVENT",
		"STREAM_START",
		"STREAM_END",
		"DOC_START",
		"DOC_END",
		"ALIAS",
		"SCALAR",
		"SEQUENCE_START",
		"SEQUENCE_END",
		"MAPPING_START",
		"MAPPING_END",
	};
	return strings[event->type];
}

/**
 * Helper function to read the next YAML input event.
 *
 * This gets the next event from the CYAML load context's `libyaml` parser
 * object.
 *
 * \param[in]  ctx    The CYAML loading context.
 * \param[out] event  On success, returns the new event.
 * \return \ref CYAML_OK on success, or appropriate error otherwise.
 */
static cyaml_err_t cyaml_get_next_event(
		const cyaml_ctx_t *ctx,
		yaml_event_t *event)
{
	if (!yaml_parser_parse(ctx->parser, event)) {
		cyaml__log(ctx->config, CYAML_LOG_ERROR,
				"libyaml: %s\n", ctx->parser->problem);
		return CYAML_ERR_LIBYAML_PARSER;
	}

	if (event->type == YAML_ALIAS_EVENT) {
		/** \todo Add support for alias? */
		yaml_event_delete(event);
		return CYAML_ERR_ALIAS;
	}

	cyaml__log(ctx->config, CYAML_LOG_DEBUG, "Event: %s\n",
			cyaml__libyaml_event_type_str(event));

	return CYAML_OK;
}

/**
 * Get the offset to a mapping field by key in a mapping schema array.
 *
 * \param[in]  ctx  The CYAML loading context.
 * \param[in]  key  Key to search for in mapping schema.
 * \return index the mapping schema's mapping fields array for key, or
 *         \ref CYAML_SCHEMA_IDX_NONE if key is not present in schema.
 */
static inline uint16_t cyaml__get_entry_from_mapping_schema(
		const cyaml_ctx_t *ctx,
		const char *key)
{
	const cyaml_schema_field_t *fields = ctx->state->mapping.schema;
	const cyaml_schema_value_t *schema = ctx->state->schema;
	uint16_t index = 0;

	/* Step through each entry in the schema */
	for (; fields->key != NULL; fields++) {
		if (cyaml__strcmp(ctx->config, schema, fields->key, key) == 0) {
			return index;
		}
		index++;
	}

	return CYAML_SCHEMA_IDX_NONE;
}

/**
 *Helper to get the current mapping field.
 *
 * \note The current load state must be \ref CYAML_STATE_IN_MAP_KEY, or
 *       \ref CYAML_STATE_IN_MAP_VALUE, and there must be a current field
 *       index in the state.
 *
 * \param[in]  ctx  The CYAML loading context.
 * \return Current mapping field's schema entry.
 */
static inline const cyaml_schema_field_t * cyaml_mapping_schema_field(
		const cyaml_ctx_t *ctx)
{
	const cyaml_state_t *state = ctx->state;

	assert(state != NULL);
	assert(state->state == CYAML_STATE_IN_MAP_KEY ||
	       state->state == CYAML_STATE_IN_MAP_VALUE);
	assert(state->mapping.schema_idx != CYAML_SCHEMA_IDX_NONE);

	return state->mapping.schema + state->mapping.schema_idx;
}

/**
 * Ensure that the CYAML load context has space for a new stack entry.
 *
 * \param[in]  ctx     The CYAML loading context.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__stack_ensure(
		cyaml_ctx_t *ctx)
{
	cyaml_state_t *temp;
	uint32_t max = ctx->stack_max + 16;

	if (ctx->stack_idx < ctx->stack_max) {
		return CYAML_OK;
	}

	temp = cyaml__realloc(ctx->config, ctx->stack, 0,
			sizeof(*ctx->stack) * max, false);
	if (temp == NULL) {
		return CYAML_ERR_OOM;
	}

	ctx->stack = temp;
	ctx->stack_max = max;
	ctx->state = ctx->stack + ctx->stack_idx - 1;

	return CYAML_OK;
}

/**
 * Count the entries in a mapping schema array.
 *
 * The mapping schema array must be terminated by an entry with a NULL key.
 *
 * \param[in]  mapping_schema  Array of mapping schema fields.
 * \return Number of entries in mapping_schema array.
 */
static uint16_t cyaml__get_entry_count_from_mapping_schema(
		const cyaml_schema_field_t *mapping_schema)
{
	const cyaml_schema_field_t *entry = mapping_schema;

	while (entry->key != NULL) {
		entry++;
	}

	return entry - mapping_schema;
}

/**
 * Create \ref CYAML_STATE_IN_MAP_KEY state's bitfield array allocation.
 *
 * The bitfield is used to record whether the mapping as all the required
 * fields by mapping schema array index.
 *
 * \param[in]  ctx    The CYAML loading context.
 * \param[in]  state  CYAML load state for a \ref CYAML_STATE_IN_MAP_KEY state.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__mapping_bitfieid_create(
		cyaml_ctx_t *ctx,
		cyaml_state_t *state)
{
	cyaml_bitfield_t *bitfield;
	unsigned count = cyaml__get_entry_count_from_mapping_schema(
			state->mapping.schema);
	size_t size = ((count + CYAML_BITFIELD_BITS - 1) / CYAML_BITFIELD_BITS)
			* sizeof(*bitfield);

	bitfield = cyaml__alloc(ctx->config, size, true);
	if (bitfield == NULL) {
		return CYAML_ERR_OOM;
	}

	state->mapping.fields = bitfield;

	return CYAML_OK;
}

/**
 * Destroy a \ref CYAML_STATE_IN_MAP_KEY state's bitfield array allocation.
 *
 * \param[in]  ctx    The CYAML loading context.
 * \param[in]  state  CYAML load state for a \ref CYAML_STATE_IN_MAP_KEY state.
 */
static void cyaml__mapping_bitfieid_destroy(
		cyaml_ctx_t *ctx,
		cyaml_state_t *state)
{
	cyaml__free(ctx->config, state->mapping.fields);
}

/**
 * Set the bit for the current mapping's current field, to indicate it exists.
 *
 * Current CYAML load state must be \ref CYAML_STATE_IN_MAP_KEY.
 *
 * \param[in]  ctx     The CYAML loading context.
 */
static void cyaml__mapping_bitfieid_set(
		cyaml_ctx_t *ctx)
{
	cyaml_state_t *state = ctx->state;
	unsigned idx = state->mapping.schema_idx;

	state->mapping.fields[idx / CYAML_BITFIELD_BITS] |=
			1 << (idx % CYAML_BITFIELD_BITS);
}

/**
 * Check a mapping had all the required fields.
 *
 * Checks all the bits are set in the bitfield, which correspond to
 * entries in the mapping schema array which are not marked with the
 * \ref CYAML_FLAG_OPTIONAL flag.
 *
 * Current CYAML load state must be \ref CYAML_STATE_IN_MAP_KEY.
 *
 * \param[in]  ctx     The CYAML loading context.
 * \return \ref CYAML_OK if all required fields are present, or
 *         \ref CYAML_ERR_MAPPING_FIELD_MISSING any are missing.
 */
static cyaml_err_t cyaml__mapping_bitfieid_validate(
		cyaml_ctx_t *ctx)
{
	cyaml_state_t *state = ctx->state;
	unsigned count = cyaml__get_entry_count_from_mapping_schema(
			state->mapping.schema);

	for (unsigned i = 0; i < count; i++) {
		if (state->mapping.schema[i].value.flags & CYAML_FLAG_OPTIONAL) {
			continue;
		}
		if (state->mapping.fields[i / CYAML_BITFIELD_BITS] &
				(1 << (i % CYAML_BITFIELD_BITS))) {
			continue;
		}
		cyaml__log(ctx->config, CYAML_LOG_ERROR,
				"Missing required mapping field: %s\n",
				state->mapping.schema[i].key);
		return CYAML_ERR_MAPPING_FIELD_MISSING;
	}

	return CYAML_OK;
}

/**
 * Helper to check if schema is for a \ref CYAML_SEQUENCE type.
 *
 * \param[in]  schema  The schema entry for a type.
 * \return true iff schema is for a \ref CYAML_SEQUENCE type,
 *         false otherwise.
 */
static inline bool cyaml__is_sequence(const cyaml_schema_value_t *schema)
{
	return ((schema->type == CYAML_SEQUENCE) ||
	        (schema->type == CYAML_SEQUENCE_FIXED));
}

/**
 * Push a new entry onto the CYAML load context's stack.
 *
 * \param[in]  ctx     The CYAML loading context.
 * \param[in]  state   The CYAML load state we're pushing a stack entry for.
 * \param[in]  schema  The CYAML schema for the value expected in state.
 * \param[in]  data    Pointer to where value's data should be written.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__stack_push(
		cyaml_ctx_t *ctx,
		enum cyaml_state_e state,
		const cyaml_schema_value_t *schema,
		cyaml_data_t *data)
{
	cyaml_err_t err;
	cyaml_state_t s = {
		.data = data,
		.state = state,
		.schema = schema,
	};

	err = cyaml__stack_ensure(ctx);
	if (err != CYAML_OK) {
		return err;
	}

	switch (state) {
	case CYAML_STATE_IN_MAP_KEY:
		assert(schema->type == CYAML_MAPPING);
		s.mapping.schema = schema->mapping.fields;
		err = cyaml__mapping_bitfieid_create(ctx, &s);
		if (err != CYAML_OK) {
			return err;
		}
		break;
	case CYAML_STATE_IN_SEQUENCE:
		assert(cyaml__is_sequence(schema));
		if (schema->type == CYAML_SEQUENCE_FIXED) {
			if (schema->sequence.min != schema->sequence.max) {
				return CYAML_ERR_SEQUENCE_FIXED_COUNT;
			}
		} else {
			if (ctx->state->state == CYAML_STATE_IN_SEQUENCE) {
				return CYAML_ERR_SEQUENCE_IN_SEQUENCE;

			} else if (ctx->state->state ==
					CYAML_STATE_IN_MAP_KEY) {
				const cyaml_schema_field_t *field =
						cyaml_mapping_schema_field(ctx);
				s.sequence.count_data = ctx->state->data +
						field->count_offset;
				s.sequence.count_size = field->count_size;
			} else {
				assert(ctx->state->state == CYAML_STATE_IN_DOC);
				s.sequence.count_data = (void *)&ctx->seq_count;
				s.sequence.count_size = sizeof(ctx->seq_count);
			}
		}
		break;
	default:
		break;
	}

	cyaml__log(ctx->config, CYAML_LOG_DEBUG,
			"PUSH[%u]: %s\n", ctx->stack_idx,
			cyaml__state_to_str(state));

	ctx->stack[ctx->stack_idx] = s;
	ctx->state = ctx->stack + ctx->stack_idx;
	ctx->stack_idx++;

	return CYAML_OK;
}

/**
 * Pop the current entry on the CYAML load context's stack.
 *
 * This frees any resources owned by the stack entry.
 *
 * \param[in]  ctx     The CYAML loading context.
 */
static void cyaml__stack_pop(
		cyaml_ctx_t *ctx)
{
	uint32_t idx = ctx->stack_idx;

	assert(idx != 0);

	switch (ctx->state->state) {
	case CYAML_STATE_IN_MAP_KEY: /* Fall through. */
	case CYAML_STATE_IN_MAP_VALUE:
		cyaml__mapping_bitfieid_destroy(ctx, ctx->state);
		break;
	default:
		break;
	}

	idx--;

	cyaml__log(ctx->config, CYAML_LOG_DEBUG, "POP[%u]: %s\n", idx,
			cyaml__state_to_str(ctx->state->state));

	ctx->state = (idx == 0) ? NULL : &ctx->stack[idx - 1];
	ctx->stack_idx = idx;
}

/**
 * Helper to make allocations for loaded YAML values.
 *
 * If the current state is sequence, this extends any existing allocation
 * for the sequence.
 *
 * The current CYAML loading context's state is updated with new allocation
 * address, where necessary.
 *
 * \param[in]      ctx            The CYAML loading context.
 * \param[in]      schema         The schema for value to get data pointer for.
 * \param[in]      event          The YAML event value to get data pointer for.
 * \param[in,out]  value_data_io  Current address of value's data.  Updated to
 *                                new address if value is allocation requiring
 *                                an allocation.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__data_handle_pointer(
		cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		const yaml_event_t *event,
		uint8_t **value_data_io)
{
	cyaml_state_t *state = ctx->state;

	if (schema->flags & CYAML_FLAG_POINTER) {
		/* Need to create/extend an allocation. */
		size_t delta = schema->data_size;
		uint8_t *value_data = NULL;
		size_t offset = 0;

		if (schema->type == CYAML_STRING) {
			/* For a string the allocation size is the string
			 * size from the event, plus trailing NULL. */
			delta = strlen((const char *)
					event->data.scalar.value) + 1;
		}

		if (schema->type == CYAML_SEQUENCE) {
			/* Sequence; could be extending allocation. */
			offset = schema->data_size * state->sequence.count;
			value_data = state->sequence.data;
		} else if (schema->type == CYAML_SEQUENCE_FIXED) {
			/* Allocation is only made for full fixed size
			 * of sequence, on creation, and not extended. */
			if (state->sequence.count > 0) {
				*value_data_io = state->sequence.data;
				return CYAML_OK;
			}
			delta = schema->data_size * schema->sequence.max;
		}

		value_data = cyaml__realloc(ctx->config, value_data,
				offset, offset + delta, true);
		if (value_data == NULL) {
			return CYAML_ERR_OOM;
		}

		cyaml__log(ctx->config, CYAML_LOG_DEBUG,
				"Allocation: %p (%zu + %zu bytes)\n",
				value_data, offset, delta);

		if (cyaml__is_sequence(schema)) {
			/* Updated the in sequence state so it knows the new
			 * allocation address. */
			state->sequence.data = value_data;
		}

		/* Write the allocation pointer into the data structure. */
		cyaml_data_write_pointer(value_data, *value_data_io);

		/* Update the caller's pointer so it can write the value to
		 * the right place. */
		*value_data_io = value_data;
	}

	return CYAML_OK;
}

/**
 * Dump a backtrace to the log.
 *
 * \param[in]  ctx     The CYAML loading context.
 */
static void cyaml__backtrace(
		cyaml_ctx_t *ctx)
{
	if (ctx->stack_idx > 1) {
		cyaml__log(ctx->config, CYAML_LOG_ERROR, "Backtrace:\n");
	} else {
		return;
	}

	for (uint32_t idx = ctx->stack_idx - 1; idx != 0; idx--) {
		cyaml_state_t *state = ctx->stack + idx;
		switch (state->state) {
		case CYAML_STATE_IN_MAP_KEY: /* Fall through. */
		case CYAML_STATE_IN_MAP_VALUE:
			if (state->mapping.schema_idx !=
					CYAML_SCHEMA_IDX_NONE) {
				cyaml__log(ctx->config, CYAML_LOG_ERROR,
						"  in mapping field: %s\n",
						state->mapping.schema[
						state->mapping.schema_idx].key);
			} else {
				cyaml__log(ctx->config, CYAML_LOG_ERROR,
						"  in mapping:\n");
			}
			break;
		case CYAML_STATE_IN_SEQUENCE:
			cyaml__log(ctx->config, CYAML_LOG_ERROR,
					"  in sequence entry: %"PRIu32"\n",
					state->sequence.count);
			break;
		default:
			/** \todo \ref CYAML_STATE_IN_DOC handling for multi
			 *        document streams.
			 */
			break;
		}
	}
}

/**
 * Read a value of type \ref CYAML_INT.
 *
 * \param[in]  ctx     The CYAML loading context.
 * \param[in]  schema  The schema for the value to be read.
 * \param[in]  value   String containing scaler value.
 * \param[in]  data    The place to write the value in the output data.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__read_int(
		const cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		const char *value,
		uint8_t *data)
{
	long long temp;
	char *end = NULL;
	int64_t max;
	int64_t min;

	CYAML_UNUSED(ctx);

	if (schema->data_size == 0 || schema->data_size > sizeof(uint64_t)) {
		return CYAML_ERR_INVALID_DATA_SIZE;
	}

	max = ((~(uint64_t)0) >> ((8 - schema->data_size) * 8)) / 2;
	min = (-max) - 1;

	errno = 0;
	temp = strtoll(value, &end, 0);

	if (end == value || errno == ERANGE ||
	    temp < min || temp > max) {
		return CYAML_ERR_INVALID_VALUE;
	}

	return cyaml_data_write(temp, schema->data_size, data);
}

/**
 * Helper to read a number into a uint64_t.
 *
 * \param[in]  value  String containing scaler value.
 * \param[in]  out    The place to write the value in the output data.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static inline cyaml_err_t cyaml__read_uint64_t(
		const char *value,
		uint64_t *out)
{
	long long temp;
	char *end = NULL;

	errno = 0;
	temp = strtoll(value, &end, 0);

	if (end == value || errno == ERANGE || temp < 0) {
		return CYAML_ERR_INVALID_VALUE;
	}

	*out = temp;
	return CYAML_OK;
}

/**
 * Read a value of type \ref CYAML_UINT.
 *
 * \param[in]  ctx     The CYAML loading context.
 * \param[in]  schema  The schema for the value to be read.
 * \param[in]  value   String containing scaler value.
 * \param[in]  data    The place to write the value in the output data.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__read_uint(
		const cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		const char *value,
		uint8_t *data)
{
	cyaml_err_t err;
	uint64_t temp;
	uint64_t max;

	CYAML_UNUSED(ctx);

	if (schema->data_size == 0) {
		return CYAML_ERR_INVALID_DATA_SIZE;
	}

	err = cyaml__read_uint64_t(value, &temp);
	if (err != CYAML_OK) {
		return err;
	}

	max = (~(uint64_t)0) >> ((8 - schema->data_size) * 8);
	if (temp > max) {
		return CYAML_ERR_INVALID_VALUE;
	}

	return cyaml_data_write(temp, schema->data_size, data);
}

/**
 * Read a value of type \ref CYAML_BOOL.
 *
 * \param[in]  ctx     The CYAML loading context.
 * \param[in]  schema  The schema for the value to be read.
 * \param[in]  value   String containing scaler value.
 * \param[in]  data    The place to write the value in the output data.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__read_bool(
		const cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		const char *value,
		uint8_t *data)
{
	bool temp = true;
	static const char * const false_strings[] = {
		"false", "no", "disable", "0",
	};

	CYAML_UNUSED(ctx);

	for (uint32_t i = 0; i < CYAML_ARRAY_LEN(false_strings); i++) {
		if (cyaml_utf8_casecmp(value, false_strings[i]) == 0) {
			temp = false;
			break;
		}
	}

	return cyaml_data_write(temp, schema->data_size, data);
}

/**
 * Read a value of type \ref CYAML_ENUM.
 *
 * \param[in]  ctx     The CYAML loading context.
 * \param[in]  schema  The schema for the value to be read.
 * \param[in]  value   String containing scaler value.
 * \param[in]  data    The place to write the value in the output data.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__read_enum(
		const cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		const char *value,
		uint8_t *data)
{
	const cyaml_strval_t *strings = schema->enumeration.strings;

	for (uint32_t i = 0; i < schema->enumeration.count; i++) {
		if (cyaml__strcmp(ctx->config, schema,
				value, strings[i].str) == 0) {
			return cyaml_data_write(strings[i].val,
					schema->data_size, data);
		}
	}

	if (schema->flags & CYAML_FLAG_STRICT) {
		cyaml__log(ctx->config, CYAML_LOG_ERROR,
				"Invalid enumeration value: %s\n", value);
		return CYAML_ERR_INVALID_VALUE;

	}

	return cyaml__read_int(ctx, schema, value, data);
}

/**
 * Helper to read \ref CYAML_FLOAT of size `sizeof(float)`.
 *
 * \param[in]  ctx     The CYAML loading context.
 * \param[in]  schema  The schema for the value to be read.
 * \param[in]  value   String containing scaler value.
 * \param[in]  data    The place to write the value in the output data.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__read_float_f(
		const cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		const char *value,
		uint8_t *data)
{
	float temp;
	char *end = NULL;

	assert(schema->data_size == sizeof(temp));

	CYAML_UNUSED(ctx);
	CYAML_UNUSED(schema);

	errno = 0;
	temp = strtof(value, &end);

	if (end == value || errno == ERANGE) {
		return CYAML_ERR_INVALID_VALUE;
	}

	memcpy(data, &temp, sizeof(temp));

	return CYAML_OK;
}

/**
 * Helper to read \ref CYAML_FLOAT of size `sizeof(double)`.
 *
 * \param[in]  ctx     The CYAML loading context.
 * \param[in]  schema  The schema for the value to be read.
 * \param[in]  value   String containing scaler value.
 * \param[in]  data    The place to write the value in the output data.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__read_float_d(
		const cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		const char *value,
		uint8_t *data)
{
	double temp;
	char *end = NULL;

	assert(schema->data_size == sizeof(temp));

	CYAML_UNUSED(ctx);
	CYAML_UNUSED(schema);

	errno = 0;
	temp = strtod(value, &end);

	if (end == value || errno == ERANGE) {
		return CYAML_ERR_INVALID_VALUE;
	}

	memcpy(data, &temp, sizeof(temp));

	return CYAML_OK;
}

/**
 * Read a value of type \ref CYAML_FLOAT.
 *
 * The `data_size` of the schema entry must be the size of a known
 * floating point C type.
 *
 * \note The `long double` type was causing problems, so it isn't currently
 *       supported.
 *
 * \param[in]  ctx     The CYAML loading context.
 * \param[in]  schema  The schema for the value to be read.
 * \param[in]  value   String containing scaler value.
 * \param[in]  data    The place to write the value in the output data.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__read_float(
		const cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		const char *value,
		uint8_t *data)
{
	typedef cyaml_err_t (*cyaml_float_fn)(
			const cyaml_ctx_t *ctx,
			const cyaml_schema_value_t *schema,
			const char *value,
			uint8_t *data_target);
	struct float_fns {
		size_t size;
		const cyaml_float_fn func;
	};
	static const struct float_fns fns[] = {
		{
			.size = sizeof(float),
			.func = cyaml__read_float_f,
		},
		{
			.size = sizeof(double),
			.func = cyaml__read_float_d,
		},
	};

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(fns); i++) {
		if (fns[i].size == schema->data_size) {
			assert(fns[i].func != NULL);
			return fns[i].func(ctx, schema, value, data);
		}
	}

	return CYAML_ERR_INVALID_DATA_SIZE;
}

/**
 * Read a value of type \ref CYAML_STRING.
 *
 * \param[in]  ctx     The CYAML loading context.
 * \param[in]  schema  The schema for the value to be read.
 * \param[in]  value   String containing scaler value.
 * \param[in]  data    The place to write the value in the output data.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__read_string(
		const cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		const char *value,
		uint8_t *data)
{
	size_t str_len = strlen(value);

	CYAML_UNUSED(ctx);

	if (schema->string.min > schema->string.max) {
		return CYAML_ERR_BAD_MIN_MAX_SCHEMA;
	} else if (str_len < schema->string.min) {
		return CYAML_ERR_STRING_LENGTH_MIN;
	} else if (str_len > schema->string.max) {
		return CYAML_ERR_STRING_LENGTH_MAX;
	}

	memcpy(data, value, str_len + 1);

	return CYAML_OK;
}

/**
 * Read a scalar value.
 *
 * \param[in]  ctx     The CYAML loading context.
 * \param[in]  schema  The schema for the value to be read.
 * \param[in]  data    The place to write the value in the output data.
 * \param[in]  event   The `libyaml` event providing the scalar value data.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__read_scalar_value(
		const cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		cyaml_data_t *data,
		yaml_event_t *event)
{
	const char *value = (const char *)event->data.scalar.value;
	typedef cyaml_err_t (*cyaml_read_scalar_fn)(
			const cyaml_ctx_t *ctx,
			const cyaml_schema_value_t *schema,
			const char *value,
			uint8_t *data_target);
	static const cyaml_read_scalar_fn fn[CYAML__TYPE_COUNT] = {
		[CYAML_INT]    = cyaml__read_int,
		[CYAML_UINT]   = cyaml__read_uint,
		[CYAML_BOOL]   = cyaml__read_bool,
		[CYAML_ENUM]   = cyaml__read_enum,
		[CYAML_FLOAT]  = cyaml__read_float,
		[CYAML_STRING] = cyaml__read_string,
	};

	cyaml__log(ctx->config, CYAML_LOG_INFO, "  <%s>\n", value);

	assert(fn[schema->type] != NULL);

	return fn[schema->type](ctx, schema, value, data);
}

/**
 * Set a flag in a \ref CYAML_FLAGS value.
 *
 * \param[in]      ctx        The CYAML loading context.
 * \param[in]      schema     The schema for the value to be read.
 * \param[in]      value      String containing scaler value.
 * \param[in,out]  flags_out  Current flags, updated on success.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__set_flag(
		const cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		const char *value,
		uint64_t *flags_out)
{
	const cyaml_strval_t *strings = schema->enumeration.strings;

	for (uint32_t i = 0; i < schema->enumeration.count; i++) {
		if (cyaml__strcmp(ctx->config, schema,
				value, strings[i].str) == 0) {
			*flags_out |= strings[i].val;
			return CYAML_OK;
		}
	}

	if (!(schema->flags & CYAML_FLAG_STRICT)) {
		long long temp;
		char *end = NULL;
		uint64_t max = (~(uint64_t)0) >> ((8 - schema->data_size) * 8);

		errno = 0;
		temp = strtoll(value, &end, 0);

		if (!(end == value || errno == ERANGE ||
		      temp < 0 || (uint64_t)temp > max)) {
			*flags_out |= temp;
			return CYAML_OK;
		}
	}

	cyaml__log(ctx->config, CYAML_LOG_ERROR, "Unknown flag: %s\n", value);

	return CYAML_ERR_INVALID_VALUE;
}

/**
 * Read a value of type \ref CYAML_FLAGS.
 *
 * Since \ref CYAML_FLAGS is a composite value (a sequence of scalars), rather
 * than a simple scaler, this consumes events from the YAML input stream.
 *
 * \param[in]  ctx     The CYAML loading context.
 * \param[in]  schema  The schema for the value to be read.
 * \param[in]  data    The place to write the value in the output data.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__read_flags_value(
		cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		cyaml_data_t *data)
{
	bool exit = false;
	uint64_t value = 0;
	yaml_event_t event;
	cyaml_err_t err = CYAML_OK;

	while (!exit) {
		cyaml_event_t cyaml_event;
		err = cyaml_get_next_event(ctx, &event);
		if (err != CYAML_OK) {
			return err;
		}
		cyaml_event = cyaml__get_event_type(&event);

		switch (cyaml_event) {
		case CYAML_EVT_SCALAR:
			err = cyaml__set_flag(ctx, schema,
					(const char *)event.data.scalar.value,
					&value);
			if (err != CYAML_OK) {
				yaml_event_delete(&event);
				return err;
			}
			break;
		case CYAML_EVT_SEQ_END:
			exit = true;
			break;
		default:
			yaml_event_delete(&event);
			return CYAML_ERR_UNEXPECTED_EVENT;
		}

		yaml_event_delete(&event);
	}

	err = cyaml_data_write(value, schema->data_size, data);
	if (err != CYAML_OK) {
		return err;
	}

	cyaml__log(ctx->config, CYAML_LOG_INFO,
			"  <Flags: 0x%"PRIx64">\n", value);

	return err;
}

/**
 * Set some bits in a \ref CYAML_BITFIELD value.
 *
 * If the fiven bit value name is one expected by the schema, then this
 * function consumes an event from the YAML input stream.
 *
 * \param[in]      ctx        The CYAML loading context.
 * \param[in]      schema     The schema for the value to be read.
 * \param[in]      name       String containing scaler bit value name.
 * \param[in,out]  bits_out   Current bits, updated on success.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__set_bitval(
		const cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		const char *name,
		uint64_t *bits_out)
{
	const cyaml_bitdef_t *bitdef = schema->bitfield.bitdefs;
	yaml_event_t event;
	cyaml_err_t err;
	uint64_t value;
	uint64_t mask;
	uint32_t i;

	for (i = 0; i < schema->bitfield.count; i++) {
		if (bitdef[i].bits + bitdef[i].offset > schema->data_size * 8) {
			return CYAML_ERR_BAD_BITVAL_IN_SCHEMA;
		}
		if (cyaml__strcmp(ctx->config, schema,
				name, bitdef[i].name) == 0) {
			break;
		}
	}

	if (i == schema->bitfield.count) {
		cyaml__log(ctx->config, CYAML_LOG_ERROR,
				"Unknown bit value: %s\n", name);
		return CYAML_ERR_INVALID_VALUE;
	}

	err = cyaml_get_next_event(ctx, &event);
	if (err != CYAML_OK) {
		return err;
	}

	switch (cyaml__get_event_type(&event)) {
	case CYAML_EVT_SCALAR:
		err = cyaml__read_uint64_t(
				(const char *)event.data.scalar.value, &value);
		yaml_event_delete(&event);
		if (err != CYAML_OK) {
			return err;
		}
		break;
	default:
		yaml_event_delete(&event);
		return CYAML_ERR_UNEXPECTED_EVENT;
	}

	mask = (~(uint64_t)0) >> ((8 * sizeof(uint64_t)) - bitdef[i].bits);
	if (value > mask) {
		cyaml__log(ctx->config, CYAML_LOG_ERROR,
				"Value too big for bits: %s\n", name);
		return CYAML_ERR_INVALID_VALUE;
	}

	*bits_out |= value << bitdef[i].offset;
	return CYAML_OK;
}

/**
 * Read a value of type \ref CYAML_BITFIELD.
 *
 * Since \ref CYAML_FLAGS is a composite value (a mapping), rather
 * than a simple scaler, this consumes events from the YAML input stream.
 *
 * \param[in]  ctx     The CYAML loading context.
 * \param[in]  schema  The schema for the value to be read.
 * \param[in]  data    The place to write the value in the output data.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__read_bitfield_value(
		cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		cyaml_data_t *data)
{
	bool exit = false;
	uint64_t value = 0;
	yaml_event_t event;
	cyaml_err_t err = CYAML_OK;

	while (!exit) {
		cyaml_event_t cyaml_event;
		err = cyaml_get_next_event(ctx, &event);
		if (err != CYAML_OK) {
			return err;
		}
		cyaml_event = cyaml__get_event_type(&event);
		switch (cyaml_event) {
		case CYAML_EVT_SCALAR:
			err = cyaml__set_bitval(ctx, schema,
					(const char *)event.data.scalar.value,
					&value);
			if (err != CYAML_OK) {
				yaml_event_delete(&event);
				return err;
			}
			break;
		case CYAML_EVT_MAP_END:
			exit = true;
			break;
		default:
			yaml_event_delete(&event);
			return CYAML_ERR_UNEXPECTED_EVENT;
		}

		yaml_event_delete(&event);
	}

	err = cyaml_data_write(value, schema->data_size, data);
	if (err != CYAML_OK) {
		return err;
	}

	cyaml__log(ctx->config, CYAML_LOG_INFO,
			"  <Bits: 0x%"PRIx64">\n", value);

	return err;
}

/**
 * Entirely consume an ignored value.
 *
 * This ignores all the descendants of the value, e.g. if the `ignored` key's
 * value is of type \ref CYAML_IGNORE, all of the following is ignored:
 *
 * ```
 * ignored:
 *     - foo: 7
 *       bar: 9
 *     - foo: 1
 *       bar: 2
 * ```
 *
 * \param[in]  ctx          The CYAML loading context.
 * \param[in]  cyaml_event  The event for the value to ignore.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__consume_ignored_value(
		cyaml_ctx_t *ctx,
		cyaml_event_t cyaml_event)
{
	if (cyaml_event != CYAML_EVT_SCALAR) {
		unsigned level = 1;

		assert(cyaml_event == CYAML_EVT_SEQ_START ||
		       cyaml_event == CYAML_EVT_MAP_START);

		while (level > 0) {
			cyaml_err_t err;
			yaml_event_t event;

			err = cyaml_get_next_event(ctx, &event);
			if (err != CYAML_OK) {
				return err;
			}
			switch (cyaml__get_event_type(&event)) {
			case CYAML_EVT_SEQ_START: /* Fall through */
			case CYAML_EVT_MAP_START:
				level++;
				break;

			case CYAML_EVT_SEQ_END: /* Fall through */
			case CYAML_EVT_MAP_END:
				level--;
				break;

			default:
				break;
			}
			yaml_event_delete(&event);
		}
	}

	return CYAML_OK;
}

/**
 * Handle a YAML event corresponding to a YAML data value.
 *
 * \param[in]  ctx     The CYAML loading context.
 * \param[in]  schema  CYAML schema for the expected value.
 * \param[in]  data    Pointer to where value's data should be written.
 * \param[in]  event   The YAML event to handle.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__read_value(
		cyaml_ctx_t *ctx,
		const cyaml_schema_value_t *schema,
		uint8_t *data,
		yaml_event_t *event)
{
	cyaml_event_t cyaml_event = cyaml__get_event_type(event);
	cyaml_err_t err = CYAML_OK;

	cyaml__log(ctx->config, CYAML_LOG_DEBUG,
			"Reading value of type '%s'%s\n",
			cyaml__type_to_str(schema->type),
			schema->flags & CYAML_FLAG_POINTER ? " (pointer)" : "");

	if (!cyaml__is_sequence(schema)) {
		/* Since sequences extend their allocation for each entry,
		 * they're handled in the sequence-specific code.
		 */
		err = cyaml__data_handle_pointer(ctx, schema, event, &data);
		if (err != CYAML_OK) {
			return err;
		}
	}

	switch (schema->type) {
	case CYAML_INT:   /* Fall through. */
	case CYAML_UINT:  /* Fall through. */
	case CYAML_BOOL:  /* Fall through. */
	case CYAML_ENUM:  /* Fall through. */
	case CYAML_FLOAT: /* Fall through. */
	case CYAML_STRING:
		if (cyaml_event != CYAML_EVT_SCALAR) {
			return CYAML_ERR_INVALID_VALUE;
		}
		err = cyaml__read_scalar_value(ctx, schema, data, event);
		break;
	case CYAML_FLAGS:
		if (cyaml_event != CYAML_EVT_SEQ_START) {
			return CYAML_ERR_INVALID_VALUE;
		}
		err = cyaml__read_flags_value(ctx, schema, data);
		break;
	case CYAML_MAPPING:
		if (cyaml_event != CYAML_EVT_MAP_START) {
			return CYAML_ERR_INVALID_VALUE;
		}
		err = cyaml__stack_push(ctx, CYAML_STATE_IN_MAP_KEY,
				schema, data);
		break;
	case CYAML_BITFIELD:
		if (cyaml_event != CYAML_EVT_MAP_START) {
			return CYAML_ERR_INVALID_VALUE;
		}
		err = cyaml__read_bitfield_value(ctx, schema, data);
		break;
	case CYAML_SEQUENCE: /* Fall through. */
	case CYAML_SEQUENCE_FIXED:
		if (cyaml_event != CYAML_EVT_SEQ_START) {
			cyaml__log(ctx->config, CYAML_LOG_ERROR,
					"Expecting sequence, got: %s\n",
					cyaml__libyaml_event_type_str(event));
			return CYAML_ERR_INVALID_VALUE;
		}
		err = cyaml__stack_push(ctx, CYAML_STATE_IN_SEQUENCE,
				schema, data);
		break;
	case CYAML_IGNORE:
		err = cyaml__consume_ignored_value(ctx, cyaml_event);
		break;
	default:
		err = CYAML_ERR_BAD_TYPE_IN_SCHEMA;
		break;
	}

	return err;
}

/**
 * YAML loading handler for start of stream in the \ref CYAML_STATE_START state.
 *
 * \param[in]  ctx    The CYAML loading context.
 * \param[in]  event  The YAML event to handle.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__stream_start(
		cyaml_ctx_t *ctx,
		yaml_event_t *event)
{
	CYAML_UNUSED(event);
	return cyaml__stack_push(ctx, CYAML_STATE_IN_STREAM,
			ctx->state->schema, ctx->state->data);
}

/**
 * YAML loading handler for documents in the \ref CYAML_STATE_IN_STREAM state.
 *
 * \param[in]  ctx    The CYAML loading context.
 * \param[in]  event  The YAML event to handle.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__doc_start(
		cyaml_ctx_t *ctx,
		yaml_event_t *event)
{
	CYAML_UNUSED(event);
	if (ctx->state->stream.doc_count == 1) {
		cyaml__log(ctx->config, CYAML_LOG_WARNING,
				"Ignoring documents after first in stream\n");
		cyaml__stack_pop(ctx);
		return CYAML_OK;
	}
	ctx->state->stream.doc_count++;
	return cyaml__stack_push(ctx, CYAML_STATE_IN_DOC,
			ctx->state->schema, ctx->state->data);
}

/**
 * YAML loading handler for finalising the \ref CYAML_STATE_IN_STREAM state.
 *
 * \param[in]  ctx    The CYAML loading context.
 * \param[in]  event  The YAML event to handle.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__stream_end(
		cyaml_ctx_t *ctx,
		yaml_event_t *event)
{
	CYAML_UNUSED(event);
	cyaml__stack_pop(ctx);
	return CYAML_OK;
}

/**
 * YAML loading handler for the root value in the \ref CYAML_STATE_IN_DOC state.
 *
 * \param[in]  ctx    The CYAML loading context.
 * \param[in]  event  The YAML event to handle.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__doc_root_value(
		cyaml_ctx_t *ctx,
		yaml_event_t *event)
{
	return cyaml__read_value(ctx, ctx->state->schema,
			ctx->state->data, event);
}

/**
 * YAML loading handler for finalising the \ref CYAML_STATE_IN_DOC state.
 *
 * \param[in]  ctx    The CYAML loading context.
 * \param[in]  event  The YAML event to handle.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__doc_end(
		cyaml_ctx_t *ctx,
		yaml_event_t *event)
{
	CYAML_UNUSED(event);
	cyaml__stack_pop(ctx);
	return CYAML_OK;
}

/**
 * YAML loading handler for new mapping fields in the
 * \ref CYAML_STATE_IN_MAP_KEY state.
 *
 * \param[in]  ctx    The CYAML loading context.
 * \param[in]  event  The YAML event to handle.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__map_key(
		cyaml_ctx_t *ctx,
		yaml_event_t *event)
{
	const char *key;
	cyaml_err_t err = CYAML_OK;

	key = (const char *)event->data.scalar.value;
	ctx->state->mapping.schema_idx =
			cyaml__get_entry_from_mapping_schema(ctx, key);
	cyaml__log(ctx->config, CYAML_LOG_INFO, "[%s]\n", key);

	if (ctx->state->mapping.schema_idx == CYAML_SCHEMA_IDX_NONE) {
		yaml_event_t ignore_event;
		cyaml_event_t cyaml_event;
		if (!(ctx->config->flags &
				CYAML_CFG_IGNORE_UNKNOWN_KEYS)) {
			cyaml__log(ctx->config, CYAML_LOG_DEBUG,
					"Unexpected key: %s\n", key);
			return CYAML_ERR_INVALID_KEY;
		}
		cyaml__log(ctx->config, CYAML_LOG_DEBUG,
				"Ignoring key: %s\n", key);
		err = cyaml_get_next_event(ctx, &ignore_event);
		if (err != CYAML_OK) {
			return err;
		}
		cyaml_event = cyaml__get_event_type(&ignore_event);
		yaml_event_delete(&ignore_event);
		return cyaml__consume_ignored_value(ctx, cyaml_event);
	}
	cyaml__mapping_bitfieid_set(ctx);

	/* Toggle mapping sub-state to value */
	ctx->state->state = CYAML_STATE_IN_MAP_VALUE;

	return err;
}

/**
 * YAML loading handler for finalising the \ref CYAML_STATE_IN_MAP_KEY state.
 *
 * \param[in]  ctx    The CYAML loading context.
 * \param[in]  event  The YAML event to handle.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__map_end(
		cyaml_ctx_t *ctx,
		yaml_event_t *event)
{
	cyaml_err_t err;

	CYAML_UNUSED(event);

	err = cyaml__mapping_bitfieid_validate(ctx);
	if (err != CYAML_OK) {
		return err;
	}

	cyaml__stack_pop(ctx);
	return CYAML_OK;
}

/**
 * YAML loading handler for the \ref CYAML_STATE_IN_MAP_VALUE state.
 *
 * \param[in]  ctx    The CYAML loading context.
 * \param[in]  event  The YAML event to handle.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__map_value(
		cyaml_ctx_t *ctx,
		yaml_event_t *event)
{
	cyaml_state_t *state = ctx->state;
	const cyaml_schema_field_t *entry = cyaml_mapping_schema_field(ctx);
	cyaml_data_t *data = state->data + entry->data_offset;

	/* Toggle mapping sub-state back to key.  Do this before
	 * reading value, because reading value might increase the
	 * CYAML context stack allocation, causing the state entry
	 * to move. */
	state->state = CYAML_STATE_IN_MAP_KEY;

	return cyaml__read_value(ctx, &entry->value, data, event);
}

/**
 * YAML loading handler for new sequence entries in the
 * \ref CYAML_STATE_IN_SEQUENCE state.
 *
 * \param[in]  ctx    The CYAML loading context.
 * \param[in]  event  The YAML event to handle.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__seq_entry(
		cyaml_ctx_t *ctx,
		yaml_event_t *event)
{
	cyaml_err_t err;
	cyaml_state_t *state = ctx->state;
	uint8_t *value_data = state->data;
	const cyaml_schema_value_t *schema = state->schema;

	if (state->sequence.count + 1 > state->schema->sequence.max) {
		cyaml__log(ctx->config, CYAML_LOG_ERROR,
				"Excessive entries (%"PRIu32" max) "
				"in sequence.\n",
				state->schema->sequence.max);
		return CYAML_ERR_SEQUENCE_ENTRIES_MAX;
	}

	err = cyaml__data_handle_pointer(ctx, schema, event, &value_data);
	if (err != CYAML_OK) {
		return err;
	}

	cyaml__log(ctx->config, CYAML_LOG_DEBUG,
			"Sequence entry: %u (%"PRIu32" bytes)\n",
			state->sequence.count, schema->data_size);
	value_data += schema->data_size * state->sequence.count;
	state->sequence.count++;

	if (schema->type != CYAML_SEQUENCE_FIXED) {
		err = cyaml_data_write(state->sequence.count,
				state->sequence.count_size,
				state->sequence.count_data);
		if (err != CYAML_OK) {
			cyaml__log(ctx->config, CYAML_LOG_ERROR,
					"Failed writing sequence count\n");
			if (schema->flags & CYAML_FLAG_POINTER) {
				cyaml__log(ctx->config, CYAML_LOG_DEBUG,
						"Freeing %p\n",
						state->sequence.data);
				cyaml__free(ctx->config, state->sequence.data);
			}
			return err;
		}
	}

	/* Read the actual value */
	err = cyaml__read_value(ctx, schema->sequence.entry,
			value_data, event);
	if (err != CYAML_OK) {
		return err;
	}

	return CYAML_OK;
}

/**
 * YAML loading handler for finalising the \ref CYAML_STATE_IN_SEQUENCE state.
 *
 * \param[in]  ctx    The CYAML loading context.
 * \param[in]  event  The YAML event to handle.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__seq_end(
		cyaml_ctx_t *ctx,
		yaml_event_t *event)
{
	cyaml_state_t *state = ctx->state;

	CYAML_UNUSED(event);

	if (state->sequence.count < state->schema->sequence.min) {
		cyaml__log(ctx->config, CYAML_LOG_ERROR, "Insufficient entries "
				"(%"PRIu32" of %"PRIu32" min) in sequence.\n",
				state->sequence.count,
				state->schema->sequence.min);
		return CYAML_ERR_SEQUENCE_ENTRIES_MIN;
	}
	cyaml__log(ctx->config, CYAML_LOG_DEBUG, "Sequence count: %u\n",
			state->sequence.count);

	cyaml__stack_pop(ctx);
	return CYAML_OK;
}

/**
 * Check that common load parameters from client are valid.
 *
 * \param[in] config         The client's CYAML library config.
 * \param[in] schema         The schema describing the content of data.
 * \param[in] data_tgt       Points to client's address to write data to.
 * \param[in] seq_count_tgt  Points to client's address to write sequence count.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static inline cyaml_err_t cyaml__validate_load_params(
		const cyaml_config_t *config,
		const cyaml_schema_value_t *schema,
		cyaml_data_t * const *data_tgt,
		const unsigned *seq_count_tgt)
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
	if ((schema->type == CYAML_SEQUENCE) != (seq_count_tgt != NULL)) {
		return CYAML_ERR_BAD_PARAM_SEQ_COUNT;
	}
	if (!(schema->flags & CYAML_FLAG_POINTER)) {
		return CYAML_ERR_TOP_LEVEL_NON_PTR;
	}
	if (data_tgt == NULL) {
		return CYAML_ERR_BAD_PARAM_NULL_DATA;
	}
	return CYAML_OK;
}

/**
 * YAML loading helper dispatch function.
 *
 * Dispatches events to the appropriate event handler function for the
 * current combination of load state machine state (from the load context)
 * and event type.
 *
 * \param[in]  ctx    The CYAML loading context.
 * \param[in]  event  The YAML event to handle.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static inline cyaml_err_t cyaml__load_event(
		cyaml_ctx_t *ctx,
		yaml_event_t *event)
{
	cyaml_state_t *state = ctx->state;
	typedef cyaml_err_t (* const cyaml_read_fn)(
			cyaml_ctx_t *ctx,
			yaml_event_t *event);
	static const cyaml_read_fn fns[CYAML_STATE__COUNT][CYAML_EVT__COUNT] = {
		[CYAML_STATE_START] = {
			[CYAML_EVT_STRM_START] = cyaml__stream_start,
		},
		[CYAML_STATE_IN_STREAM] = {
			[CYAML_EVT_DOC_START]  = cyaml__doc_start,
			[CYAML_EVT_STRM_END]   = cyaml__stream_end,
		},
		[CYAML_STATE_IN_DOC] = {
			[CYAML_EVT_SCALAR]     = cyaml__doc_root_value,
			[CYAML_EVT_SEQ_START]  = cyaml__doc_root_value,
			[CYAML_EVT_MAP_START]  = cyaml__doc_root_value,
			[CYAML_EVT_DOC_END]    = cyaml__doc_end,
		},
		[CYAML_STATE_IN_MAP_KEY] = {
			[CYAML_EVT_SCALAR]     = cyaml__map_key,
			[CYAML_EVT_MAP_END]    = cyaml__map_end,
		},
		[CYAML_STATE_IN_MAP_VALUE] = {
			[CYAML_EVT_SCALAR]     = cyaml__map_value,
			[CYAML_EVT_SEQ_START]  = cyaml__map_value,
			[CYAML_EVT_MAP_START]  = cyaml__map_value,
		},
		[CYAML_STATE_IN_SEQUENCE] = {
			[CYAML_EVT_SCALAR]     = cyaml__seq_entry,
			[CYAML_EVT_SEQ_START]  = cyaml__seq_entry,
			[CYAML_EVT_MAP_START]  = cyaml__seq_entry,
			[CYAML_EVT_SEQ_END]    = cyaml__seq_end,
		},
	};
	const cyaml_read_fn fn = fns[state->state][event->type];
	cyaml_err_t err = CYAML_ERR_INTERNAL_ERROR;

	if (fn != NULL) {
		cyaml__log(ctx->config, CYAML_LOG_DEBUG, "Handle state %s\n",
				cyaml__state_to_str(state->state));
		err = fn(ctx, event);
	}

	return err;
}

/**
 * The main YAML loading function.
 *
 * The public interfaces are wrappers around this.
 *
 * \param[in]  config         Client's CYAML configuration structure.
 * \param[in]  schema         CYAML schema for the YAML to be loaded.
 * \param[out] data_out       Returns the caller-owned loaded data on success.
 *                            Untouched on failure.
 * \param[out] seq_count_out  On success, returns the sequence entry count.
 *                            Untouched on failure.
 *                            Must be non-NULL if top-level schema type is
 *                            \ref CYAML_SEQUENCE, otherwise, must be NULL.
 * \param[in]  parser         An initialised `libyaml` parser object
 *                            with its input set.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
static cyaml_err_t cyaml__load(
		const cyaml_config_t *config,
		const cyaml_schema_value_t *schema,
		cyaml_data_t **data_out,
		unsigned *seq_count_out,
		yaml_parser_t *parser)
{
	cyaml_data_t *data = NULL;
	cyaml_ctx_t ctx = {
		.config = config,
		.parser = parser,
	};
	cyaml_err_t err = CYAML_OK;

	err = cyaml__validate_load_params(config, schema,
			data_out, seq_count_out);
	if (err != CYAML_OK) {
		return err;
	}

	err = cyaml__stack_push(&ctx, CYAML_STATE_START, schema, &data);
	if (err != CYAML_OK) {
		goto out;
	}

	do {
		yaml_event_t event;

		err = cyaml_get_next_event(&ctx, &event);
		if (err != CYAML_OK) {
			goto out;
		}

		err = cyaml__load_event(&ctx, &event);
		yaml_event_delete(&event);
		if (err != CYAML_OK) {
			goto out;
		}
	} while (ctx.state->state > CYAML_STATE_START);

	cyaml__stack_pop(&ctx);

	assert(ctx.stack_idx == 0);

	*data_out = data;
	if (seq_count_out != NULL) {
		*seq_count_out = ctx.seq_count;
	}
out:
	if (err != CYAML_OK) {
		cyaml_free(config, schema, data, ctx.seq_count);
		cyaml__backtrace(&ctx);
	}
	while (ctx.stack_idx > 0) {
		cyaml__stack_pop(&ctx);
	}
	cyaml__free(config, ctx.stack);
	return err;
}

/* Exported function, documented in include/cyaml/cyaml.h */
cyaml_err_t cyaml_load_file(
		const char *path,
		const cyaml_config_t *config,
		const cyaml_schema_value_t *schema,
		cyaml_data_t **data_out,
		unsigned *seq_count_out)
{
	FILE *file;
	cyaml_err_t err;
	yaml_parser_t parser;

	/* Initialize parser */
	if (!yaml_parser_initialize(&parser)) {
		return CYAML_ERR_LIBYAML_PARSER_INIT;
	}

	/* Open input file. */
	file = fopen(path, "r");
	if (file == NULL) {
		yaml_parser_delete(&parser);
		return CYAML_ERR_FILE_OPEN;
	}

	/* Set input file */
	yaml_parser_set_input_file(&parser, file);

	/* Parse the input */
	err = cyaml__load(config, schema, data_out, seq_count_out, &parser);
	if (err != CYAML_OK) {
		yaml_parser_delete(&parser);
		fclose(file);
		return err;
	}

	/* Cleanup */
	yaml_parser_delete(&parser);
	fclose(file);

	return CYAML_OK;
}

/* Exported function, documented in include/cyaml/cyaml.h */
cyaml_err_t cyaml_load_data(
		const uint8_t *input,
		size_t input_len,
		const cyaml_config_t *config,
		const cyaml_schema_value_t *schema,
		cyaml_data_t **data_out,
		unsigned *seq_count_out)
{
	cyaml_err_t err;
	yaml_parser_t parser;

	/* Initialize parser */
	if (!yaml_parser_initialize(&parser)) {
		return CYAML_ERR_LIBYAML_PARSER_INIT;
	}

	/* Set input data */
	yaml_parser_set_input_string(&parser, input, input_len);

	/* Parse the input */
	err = cyaml__load(config, schema, data_out, seq_count_out, &parser);
	if (err != CYAML_OK) {
		yaml_parser_delete(&parser);
		return err;
	}

	/* Cleanup */
	yaml_parser_delete(&parser);

	return CYAML_OK;
}
