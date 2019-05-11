/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2017-2019 Michael Drake <tlsa@netsurf-browser.org>
 */

/**
 * \file
 * \brief CYAML library public header.
 *
 * CYAML is a C library for parsing and serialising structured YAML documents.
 */

#ifndef CYAML_H
#define CYAML_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

/**
 * CYAML library version string.
 */
extern const char *cyaml_version_str;

/**
 * CYAML library version number suitable for comparisons.
 *
 * Version number binary composition is `0bRUUUUUUUJJJJJJJJNNNNNNNNPPPPPPPP`.
 *
 * | Character | Meaning                                |
 * | --------- | -------------------------------------- |
 * | `R`       | Release flag.  If set, it's a release. |
 * | `U`       | Unused / reserved.                     |
 * | `J`       | Major component of version.            |
 * | `N`       | Minor component of version.            |
 * | `P`       | Patch component of version.            |
 */
extern const uint32_t cyaml_version;

/**
 * CYAML value types.
 *
 * These are the fundamental data types that apply to "values" in CYAML.
 *
 * CYAML "values" are represented in by \ref cyaml_schema_value.
 */
typedef enum cyaml_type {
	CYAML_INT,      /**< Value is a signed integer. */
	CYAML_UINT,     /**< Value is an unsigned signed integer. */
	CYAML_BOOL,     /**< Value is a boolean. */
	/**
	 * Value is an enum.  Values of this type require a string / value
	 * mapping array in the schema entry, to define the list of valid
	 * enum values.
	 */
	CYAML_ENUM,
	/**
	 * Value is a flags bit field.  Values of this type require a string /
	 * value list in the schema entry, to define the list of valid flag
	 * values.  Each bit is a boolean flag.  To store values of various
	 * bit sizes, use a \ref CYAML_BITFIELD instead.
	 *
	 * In the YAML, a \ref CYAML_FLAGS value must be presented as a
	 * sequence of strings.
	 */
	CYAML_FLAGS,
	CYAML_FLOAT,    /**< Value is floating point. */
	CYAML_STRING,   /**< Value is a string. */
	/**
	 * Value is a mapping.  Values of this type require mapping schema
	 * array in the schema entry.
	 */
	CYAML_MAPPING,
	/**
	 * Value is a bit field.  Values of this type require an array of value
	 * definititions in the schema entry.  If the bitfield is used to store
	 * only single-bit flags, it may be better to use \ref CYAML_FLAGS
	 * instead.
	 *
	 * In the YAML, a \ref CYAML_FLAGS value must be presented as a
	 * mapping of bitfield entry names to their numerical values.
	 */
	CYAML_BITFIELD,
	/**
	 * Value is a sequence.  Values of this type must be the direct
	 * children of a mapping.  They require:
	 *
	 * - A schema describing the type of the sequence entries.
	 * - Offset to the array entry count member in the mapping structure.
	 * - Size in bytes of the count member in the mapping structure.
	 * - The minimum and maximum number of sequence count entries.
	 *   Set `max` to \ref CYAML_UNLIMITED to make array count
	 *   unconstrained.
	 */
	CYAML_SEQUENCE,
	/**
	 * Value is a **fixed length** sequence.  It is similar to \ref
	 * CYAML_SEQUENCE, however:
	 *
	 * - Values of this type do not need to be direct children of a mapping.
	 * - The minimum and maximum entry count must be the same.  If not
	 *   \ref CYAML_ERR_SEQUENCE_FIXED_COUNT will be returned.
	 * - Thee offset and size of the count structure member is unused.
	 *   Because the count is a schema-defined constant, it does not need
	 *   to be recorded.
	 */
	CYAML_SEQUENCE_FIXED,
	/**
	 * Value of this type is completely ignored.  This is most useful for
	 * ignoring particular keys in a mapping, when CYAML client sets
	 * configuration of \ref CYAML_CFG_IGNORE_UNKNOWN_KEYS.
	 */
	CYAML_IGNORE,
	/**
	 * Count of the valid CYAML types.  This value is **not a valid type**
	 * itself.
	 */
	CYAML__TYPE_COUNT,
} cyaml_type_e;

/**
 * CYAML value flags.
 *
 * These may be bitwise-ORed together.
 */
typedef enum cyaml_flag {
	CYAML_FLAG_DEFAULT  = 0,        /**< Default value flags (none set). */
	CYAML_FLAG_OPTIONAL = (1 << 0), /**< Mapping field is optional. */
	CYAML_FLAG_POINTER  = (1 << 1), /**< Value is a pointer to its type. */
	/**
	 * Make value handling strict.
	 *
	 * For \ref CYAML_ENUM and \ref CYAML_FLAGS types, in strict mode
	 * the YAML must contain a matching string.  Without strict, numerical
	 * values are also permitted.
	 *
	 * * For \ref CYAML_ENUM, the value becomes the value of the enum.
	 *   The numerical value is treated as signed.
	 * * For \ref CYAML_FLAGS, the values are bitwise ORed together.
	 *   The numerical values are treated as unsigned,
	 */
	CYAML_FLAG_STRICT   = (1 << 2),
	/**
	 * When saving, emit mapping / sequence value in block style.
	 *
	 * This can be used to override, for this value, any default style set
	 * in the \ref cyaml_cfg_flags CYAML behavioural configuration flags.
	 *
	 * \note This is ignored unless the value's type is \ref CYAML_MAPPING,
	 *       \ref CYAML_SEQUENCE, or \ref CYAML_SEQUENCE_FIXED.
	 *
	 * \note If both \ref CYAML_FLAG_BLOCK and \ref CYAML_FLAG_FLOW are set,
	 *       then block style takes precedence.
	 *
	 * \note If neither block nor flow style set either here, or in the
	 *       \ref cyaml_cfg_flags CYAML behavioural configuration flags,
	 *       then libyaml's default behaviour is used.
	 */
	CYAML_FLAG_BLOCK    = (1 << 3),
	/**
	 * When saving, emit mapping / sequence value in flow style.
	 *
	 * This can be used to override, for this value, any default style set
	 * in the \ref cyaml_cfg_flags CYAML behavioural configuration flags.
	 *
	 * \note This is ignored unless the value's type is \ref CYAML_MAPPING,
	 *       \ref CYAML_SEQUENCE, or \ref CYAML_SEQUENCE_FIXED.
	 *
	 * \note If both \ref CYAML_FLAG_BLOCK and \ref CYAML_FLAG_FLOW are set,
	 *       then block style takes precedence.
	 *
	 * \note If neither block nor flow style set either here, or in the
	 *       \ref cyaml_cfg_flags CYAML behavioural configuration flags,
	 *       then libyaml's default behaviour is used.
	 */
	CYAML_FLAG_FLOW     = (1 << 4),
	/**
	 * When comparing strings for this value, compare with case sensitivity.
	 *
	 * By default, strings are compared with case sensitivity.
	 * If \ref CYAML_CFG_CASE_INSENSITIVE is set, this can override
	 * the configured behaviour for this specific value.
	 *
	 * \note If both \ref CYAML_FLAG_CASE_SENSITIVE and
	 *       \ref CYAML_FLAG_CASE_INSENSITIVE are set,
	 *       then case insensitive takes precedence.
	 *
	 * \note This applies to values of types \ref CYAML_MAPPING,
	 *       \ref CYAML_ENUM, and \ref CYAML_FLAGS.  For mappings,
	 *       it applies to matching of the mappings' keys.  For
	 *       enums and flags it applies to the comparison of
	 *       \ref cyaml_strval strings.
	 */
	CYAML_FLAG_CASE_SENSITIVE   = (1 << 5),
	/**
	 * When comparing strings for this value, compare with case sensitivity.
	 *
	 * By default, strings are compared with case sensitivity.
	 * If \ref CYAML_CFG_CASE_INSENSITIVE is set, this can override
	 * the configured behaviour for this specific value.
	 *
	 * \note If both \ref CYAML_FLAG_CASE_SENSITIVE and
	 *       \ref CYAML_FLAG_CASE_INSENSITIVE are set,
	 *       then case insensitive takes precedence.
	 *
	 * \note This applies to values of types \ref CYAML_MAPPING,
	 *       \ref CYAML_ENUM, and \ref CYAML_FLAGS.  For mappings,
	 *       it applies to matching of the mappings' keys.  For
	 *       enums and flags it applies to the comparison of
	 *       \ref cyaml_strval strings.
	 */
	CYAML_FLAG_CASE_INSENSITIVE = (1 << 6),
} cyaml_flag_e;

/**
 * Mapping between a string and a signed value.
 *
 * Used for \ref CYAML_ENUM and \ref CYAML_FLAGS types.
 */
typedef struct cyaml_strval {
	const char *str; /**< String representing enum or flag value. */
	int64_t val;     /**< Value of given string. */
} cyaml_strval_t;

/**
 * Bitfield value info.
 *
 * Used for \ref CYAML_BITFIELD type.
 */
typedef struct cyaml_bitdef {
	const char *name; /**< String representing the value's name. */
	uint8_t offset;   /**< Bit offset to value in bitfield. */
	uint8_t bits;     /**< Maximum bits available for value. */
} cyaml_bitdef_t;

/**
 * Schema definition for a value.
 *
 * \note There are convenience macros for each of the types to assist in
 *       building a CYAML schema data structure for your YAML documents.
 *
 * This is the fundamental building block of CYAML schemas.  The load, save and
 * free functions take parameters of this type to explain what the top-level
 * type of the YAML document should be.
 *
 * Values of type \ref CYAML_SEQUENCE and \ref CYAML_SEQUENCE_FIXED
 * contain a reference to another \ref cyaml_schema_value representing
 * the type of the entries of the sequence.  For example, if you want
 * a sequence of integers, you'd have a \ref cyaml_schema_value for the
 * for the sequence value type, and another for the integer value type.
 *
 * Values of type \ref CYAML_MAPPING contain an array of \ref cyaml_schema_field
 * entries, defining the YAML keys allowed by the mapping.  Each field contains
 * a \ref cyaml_schema_value representing the schema for the value.
 */
typedef struct cyaml_schema_value {
	/**
	 * The type of the value defined by this schema entry.
	 */
	enum cyaml_type type;
	/** Flags indicating value's characteristics. */
	enum cyaml_flag flags;
	/**
	 * Size of the value's client data type in bytes.
	 *
	 * For example, `short` `int`, `long`, `int8_t`, etc are all signed
	 * integer types, so they would have the type \ref CYAML_INT,
	 * however, they have different sizes.
	 */
	uint32_t data_size;
	/** Anonymous union containing type-specific attributes. */
	union {
		/** \ref CYAML_STRING type-specific schema data. */
		struct {
			/**
			 * Minimum string length (bytes).
			 *
			 * \note Excludes trailing NUL.
			 */
			uint32_t min;
			/**
			 * Maximum string length (bytes).
			 *
			 * \note Excludes trailing NULL, so for character array
			 *       strings (rather than pointer strings), this
			 *       must be no more than `data_size - 1`.
			 */
			uint32_t max;
		} string;
		/** \ref CYAML_MAPPING type-specific schema data. */
		struct {
			/**
			 * Array of cyaml mapping field schema definitions.
			 *
			 * The array must be terminated by an entry with a
			 * NULL key.  See \ref cyaml_schema_field_t
			 * and \ref CYAML_FIELD_END for more info.
			 */
			const struct cyaml_schema_field *fields;
		} mapping;
		/** \ref CYAML_BITFIELD type-specific schema data. */
		struct {
			/** Array of bit defintiions for the bitfield. */
			const struct cyaml_bitdef *bitdefs;
			/** Entry count for bitdefs array. */
			uint32_t count;
		} bitfield;
		/**
		 * \ref CYAML_SEQUENCE and \ref CYAML_SEQUENCE_FIXED
		 * type-specific schema data.
		 */
		struct {
			/**
			 * Schema definition for the type of the entries in the
			 * sequence.
			 *
			 * All of a sequence's entries must be of the same
			 * type, and a sequence can not have an entry type of
			 * type \ref CYAML_SEQUENCE (although \ref
			 * CYAML_SEQUENCE_FIXED is allowed).  That is, you
			 * can't have a sequence of variable-length sequences.
			 */
			const struct cyaml_schema_value *entry;
			/**
			 * Minimum number of sequence entries.
			 *
			 * \note min and max must be the same for \ref
			 *       CYAML_SEQUENCE_FIXED.
			 */
			uint32_t min;
			/**
			 * Maximum number of sequence entries.
			 *
			 * \note min and max must be the same for \ref
			 *       CYAML_SEQUENCE_FIXED.
			 */
			uint32_t max;
		} sequence;
		/**
		 * \ref CYAML_ENUM and \ref CYAML_FLAGS type-specific schema
		 * data.
		 */
		struct {
			/** Array of string / value mappings defining enum. */
			const cyaml_strval_t *strings;
			/** Entry count for strings array. */
			uint32_t count;
		} enumeration;
	};
} cyaml_schema_value_t;

/**
 * Schema definition entry for mapping fields.
 *
 * YAML mappings are key:value pairs.  CYAML only supports scalar mapping keys,
 * i.e. the keys must be strings.  Each mapping field schema contains a
 * \ref cyaml_schema_value to define field's value.
 *
 * The schema for mappings is composed of an array of entries of this
 * data structure.  It specifies the name of the key, and the type of
 * the value.  It also specifies the offset into the data at which value
 * data should be placed.  The array is terminated by an entry with a NULL key.
 */
typedef struct cyaml_schema_field {
	/**
	 * String for YAML mapping key that his schema entry describes,
	 * or NULL to indicated the end of an array of \ref cyaml_schema_field
	 * entries.
	 */
	const char *key;
	/**
	 * Offset in data structure at which the value for this key should
	 * be placed / read from.
	 */
	uint32_t data_offset;
	/**
	 * \ref CYAML_SEQUENCE only: Offset to sequence
	 * entry count member in mapping's data structure.
	 */
	uint32_t count_offset;
	/**
	 * \ref CYAML_SEQUENCE only: Size in bytes of sequence
	 * entry count member in mapping's data structure.
	 */
	uint8_t count_size;
	/**
	 * Defines the schema for the mapping field's value.
	 */
	struct cyaml_schema_value value;
} cyaml_schema_field_t;

/**
 * CYAML behavioural configuration flags for clients
 *
 * These may be bitwise-ORed together.
 */
typedef enum cyaml_cfg_flags {
	/**
	 * This indicates CYAML's default behaviour.
	 */
	CYAML_CFG_DEFAULT             = 0,
	/**
	 * When set, unknown mapping keys are ignored when loading YAML.
	 * Without this flag set, CYAML's default behaviour is to return
	 * with the error \ref CYAML_ERR_INVALID_KEY.
	 */
	CYAML_CFG_IGNORE_UNKNOWN_KEYS = (1 << 0),
	/**
	 * When saving, emit mapping / sequence values in block style.
	 *
	 * This setting can be overridden for specific values using schema
	 * value flags (\ref cyaml_flag).
	 *
	 * \note This only applies to values of type \ref CYAML_MAPPING,
	 *       \ref CYAML_SEQUENCE, or \ref CYAML_SEQUENCE_FIXED.
	 *
	 * \note If both \ref CYAML_CFG_STYLE_BLOCK and
	 *       \ref CYAML_CFG_STYLE_FLOW are set, then block style takes
	 *       precedence.
	 */
	CYAML_CFG_STYLE_BLOCK         = (1 << 1),
	/**
	 * When saving, emit mapping / sequence values in flow style.
	 *
	 * This setting can be overridden for specific values using schema
	 * value flags (\ref cyaml_flag).
	 *
	 * \note This only applies to values of type \ref CYAML_MAPPING,
	 *       \ref CYAML_SEQUENCE, or \ref CYAML_SEQUENCE_FIXED.
	 *
	 * \note If both \ref CYAML_CFG_STYLE_BLOCK and
	 *       \ref CYAML_CFG_STYLE_FLOW are set, then block style takes
	 *       precedence.
	 */
	CYAML_CFG_STYLE_FLOW          = (1 << 2),
	/**
	 * When saving, emit "---" at document start and "..." at document end.
	 *
	 * If this flag isn't set, these document delimiting marks will not
	 * be emitted.
	 */
	CYAML_CFG_DOCUMENT_DELIM      = (1 << 3),
	/**
	 * When comparing strings, compare without case sensitivity.
	 *
	 * By default, strings are compared with case sensitivity.
	 */
	CYAML_CFG_CASE_INSENSITIVE    = (1 << 4),
} cyaml_cfg_flags_t;

/**
 * CYAML function return codes indicating success or reason for failure.
 *
 * Use \ref cyaml_strerror() to convert and error code to a human-readable
 * string.
 */
typedef enum cyaml_err {
	CYAML_OK,                        /**< Success. */
	CYAML_ERR_OOM,                   /**< Memory allocation failed. */
	CYAML_ERR_ALIAS,                 /**< YAML alias is unsupported. */
	CYAML_ERR_FILE_OPEN,             /**< Failed to open file. */
	CYAML_ERR_INVALID_KEY,           /**< Mapping key rejected by schema. */
	CYAML_ERR_INVALID_VALUE,         /**< Value rejected by schema. */
	CYAML_ERR_INTERNAL_ERROR,        /**< Internal error in LibCYAML. */
	CYAML_ERR_UNEXPECTED_EVENT,      /**< YAML event rejected by schema. */
	CYAML_ERR_STRING_LENGTH_MIN,     /**< String length too short. */
	CYAML_ERR_STRING_LENGTH_MAX,     /**< String length too long. */
	CYAML_ERR_INVALID_DATA_SIZE,     /**< Value's data size unsupported. */
	CYAML_ERR_TOP_LEVEL_NON_PTR,     /**< Top level type must be pointer. */
	CYAML_ERR_BAD_TYPE_IN_SCHEMA,    /**< Schema contains invalid type. */
	CYAML_ERR_BAD_MIN_MAX_SCHEMA,    /**< Schema minimum exceeds maximum. */
	CYAML_ERR_BAD_PARAM_SEQ_COUNT,   /**< Bad seq_count param for schema. */
	CYAML_ERR_BAD_PARAM_NULL_DATA,   /**< Client gave NULL data argument. */
	CYAML_ERR_BAD_BITVAL_IN_SCHEMA,  /**< Bit value beyond bitfield size. */
	CYAML_ERR_SEQUENCE_ENTRIES_MIN,  /**< Too few sequence entries. */
	CYAML_ERR_SEQUENCE_ENTRIES_MAX,  /**< Too many sequence entries. */
	CYAML_ERR_SEQUENCE_FIXED_COUNT,  /**< Mismatch between min and max. */
	CYAML_ERR_SEQUENCE_IN_SEQUENCE,  /**< Non-fixed sequence in sequence. */
	CYAML_ERR_MAPPING_FIELD_MISSING, /**< Required mapping field missing. */
	CYAML_ERR_BAD_CONFIG_NULL_MEMFN, /**< Client gave NULL mem function. */
	CYAML_ERR_BAD_PARAM_NULL_CONFIG, /**< Client gave NULL config arg. */
	CYAML_ERR_BAD_PARAM_NULL_SCHEMA, /**< Client gave NULL schema arg. */
	CYAML_ERR_LIBYAML_EMITTER_INIT,  /**< Failed to initialise libyaml. */
	CYAML_ERR_LIBYAML_PARSER_INIT,   /**< Failed to initialise libyaml. */
	CYAML_ERR_LIBYAML_EVENT_INIT,    /**< Failed to initialise libyaml. */
	CYAML_ERR_LIBYAML_EMITTER,       /**< Error inside libyaml emitter. */
	CYAML_ERR_LIBYAML_PARSER,        /**< Error inside libyaml parser. */
	CYAML_ERR__COUNT,                /**< Count of CYAML return codes.
	                                  *   This is **not a valid return
	                                  *   code** itself.
	                                  */
} cyaml_err_t;

/**
 * Value schema helper macro for values with \ref CYAML_INT type.
 *
 * \param[in]  _flags         Any behavioural flags relevant to this value.
 * \param[in]  _type          The C type for this value.
 */
#define CYAML_VALUE_INT( \
		_flags, _type) \
	.type = CYAML_INT, \
	.flags = (_flags), \
	.data_size = sizeof(_type)

/**
 * Mapping schema helper macro for keys with \ref CYAML_INT type.
 *
 * Use this for integers contained in structs.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 */
#define CYAML_FIELD_INT( \
		_key, _flags, _structure, _member) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_INT(((_flags) & (~CYAML_FLAG_POINTER)), \
				(((_structure *)NULL)->_member)), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Mapping schema helper macro for keys with \ref CYAML_INT type.
 *
 * Use this for pointers to integers.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 */
#define CYAML_FIELD_INT_PTR( \
		_key, _flags, _structure, _member) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_INT(((_flags) | CYAML_FLAG_POINTER), \
				(*(((_structure *)NULL)->_member))), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Value schema helper macro for values with \ref CYAML_UINT type.
 *
 * \param[in]  _flags         Any behavioural flags relevant to this value.
 * \param[in]  _type          The C type for this value.
 */
#define CYAML_VALUE_UINT( \
		_flags, _type) \
	.type = CYAML_UINT, \
	.flags = (_flags), \
	.data_size = sizeof(_type)

/**
 * Mapping schema helper macro for keys with \ref CYAML_UINT type.
 *
 * Use this for unsigned integers contained in structs.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 */
#define CYAML_FIELD_UINT( \
		_key, _flags, _structure, _member) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_UINT(((_flags) & (~CYAML_FLAG_POINTER)), \
				(((_structure *)NULL)->_member)), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Mapping schema helper macro for keys with \ref CYAML_UINT type.
 *
 * Use this for pointers to unsigned integers.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 */
#define CYAML_FIELD_UINT_PTR( \
		_key, _flags, _structure, _member) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_UINT(((_flags) | CYAML_FLAG_POINTER), \
				(*(((_structure *)NULL)->_member))), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Value schema helper macro for values with \ref CYAML_BOOL type.
 *
 * \param[in]  _flags         Any behavioural flags relevant to this value.
 * \param[in]  _type          The C type for this value.
 */
#define CYAML_VALUE_BOOL( \
		_flags, _type) \
	.type = CYAML_BOOL, \
	.flags = (_flags), \
	.data_size = sizeof(_type)

/**
 * Mapping schema helper macro for keys with \ref CYAML_BOOL type.
 *
 * Use this for boolean types contained in structs.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 */
#define CYAML_FIELD_BOOL( \
		_key, _flags, _structure, _member) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_BOOL(((_flags) & (~CYAML_FLAG_POINTER)), \
				(((_structure *)NULL)->_member)), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Mapping schema helper macro for keys with \ref CYAML_BOOL type.
 *
 * Use this for pointers to boolean types.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 */
#define CYAML_FIELD_BOOL_PTR( \
		_key, _flags, _structure, _member) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_BOOL(((_flags) | CYAML_FLAG_POINTER), \
				(*(((_structure *)NULL)->_member))), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Value schema helper macro for values with \ref CYAML_ENUM type.
 *
 * \param[in]  _flags         Any behavioural flags relevant to this value.
 * \param[in]  _type          The C type for this value.
 * \param[in]  _strings       Array of string data for enumeration values.
 * \param[in]  _strings_count Number of entries in _strings.
 */
#define CYAML_VALUE_ENUM( \
		_flags, _type, _strings, _strings_count) \
	.type = CYAML_ENUM, \
	.flags = (_flags), \
	.data_size = sizeof(_type), \
	.enumeration = { \
		.strings = _strings, \
		.count = _strings_count, \
	}

/**
 * Mapping schema helper macro for keys with \ref CYAML_ENUM type.
 *
 * Use this for enumerated types contained in structs.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 * \param[in]  _strings       Array of string data for enumeration values.
 * \param[in]  _strings_count Number of entries in _strings.
 */
#define CYAML_FIELD_ENUM( \
		_key, _flags, _structure, _member, _strings, _strings_count) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_ENUM(((_flags) & (~CYAML_FLAG_POINTER)), \
				(((_structure *)NULL)->_member), \
				_strings, _strings_count), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Mapping schema helper macro for keys with \ref CYAML_ENUM type.
 *
 * Use this for pointers to enumerated types.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 * \param[in]  _strings       Array of string data for enumeration values.
 * \param[in]  _strings_count Number of entries in _strings.
 */
#define CYAML_FIELD_ENUM_PTR( \
		_key, _flags, _structure, _member, _strings, _strings_count) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_ENUM(((_flags) | CYAML_FLAG_POINTER), \
				(*(((_structure *)NULL)->_member)), \
				_strings, _strings_count), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Value schema helper macro for values with \ref CYAML_FLAGS type.
 *
 * \param[in]  _flags         Any behavioural flags relevant to this value.
 * \param[in]  _type          The C type for this value.
 * \param[in]  _strings       Array of string data for flag values.
 * \param[in]  _strings_count Number of entries in _strings.
 */
#define CYAML_VALUE_FLAGS( \
		_flags, _type, _strings, _strings_count) \
	.type = CYAML_FLAGS, \
	.flags = (_flags), \
	.data_size = sizeof(_type), \
	.enumeration = { \
		.strings = _strings, \
		.count = _strings_count, \
	}

/**
 * Mapping schema helper macro for keys with \ref CYAML_FLAGS type.
 *
 * Use this for flag types contained in structs.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 * \param[in]  _strings       Array of string data for flag values.
 * \param[in]  _strings_count Number of entries in _strings.
 */
#define CYAML_FIELD_FLAGS( \
		_key, _flags, _structure, _member, _strings, _strings_count) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_FLAGS(((_flags) & (~CYAML_FLAG_POINTER)), \
				(((_structure *)NULL)->_member), \
				_strings, _strings_count), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Mapping schema helper macro for keys with \ref CYAML_FLAGS type.
 *
 * Use this for pointers to flag types.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 * \param[in]  _strings       Array of string data for flag values.
 * \param[in]  _strings_count Number of entries in _strings.
 */
#define CYAML_FIELD_FLAGS_PTR( \
		_key, _flags, _structure, _member, _strings, _strings_count) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_FLAGS(((_flags) | CYAML_FLAG_POINTER), \
				(*(((_structure *)NULL)->_member)), \
				_strings, _strings_count), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Value schema helper macro for values with \ref CYAML_BITFIELD type.
 *
 * \param[in]  _flags         Any behavioural flags relevant to this value.
 * \param[in]  _type          The C type for this value.
 * \param[in]  _bitvals       Array of bitfield value data for the bitfield.
 * \param[in]  _bitvals_count Number of entries in _bitvals.
 */
#define CYAML_VALUE_BITFIELD( \
		_flags, _type, _bitvals, _bitvals_count) \
	.type = CYAML_BITFIELD, \
	.flags = (_flags), \
	.data_size = sizeof(_type), \
	.bitfield = { \
		.bitdefs = _bitvals, \
		.count = _bitvals_count, \
	}

/**
 * Mapping schema helper macro for keys with \ref CYAML_BITFIELD type.
 *
 * Use this for bitfield types contained in structs.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 * \param[in]  _bitvals       Array of bitfield value data for the bitfield.
 * \param[in]  _bitvals_count Number of entries in _bitvals.
 */
#define CYAML_FIELD_BITFIELD( \
		_key, _flags, _structure, _member, _bitvals, _bitvals_count) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_BITFIELD(((_flags) & (~CYAML_FLAG_POINTER)), \
				(((_structure *)NULL)->_member), \
				_bitvals, _bitvals_count), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Mapping schema helper macro for keys with \ref CYAML_BITFIELD type.
 *
 * Use this for pointers to bitfied types.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 * \param[in]  _bitvals       Array of bitfield value data for the bitfield.
 * \param[in]  _bitvals_count Number of entries in _bitvals.
 */
#define CYAML_FIELD_BITFIELD_PTR( \
		_key, _flags, _structure, _member, _bitvals, _bitvals_count) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_BITFIELD(((_flags) | CYAML_FLAG_POINTER), \
				(*(((_structure *)NULL)->_member)), \
				_bitvals, _bitvals_count), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Value schema helper macro for values with \ref CYAML_FLOAT type.
 *
 * \param[in]  _flags         Any behavioural flags relevant to this value.
 * \param[in]  _type          The C type for this value.
 */
#define CYAML_VALUE_FLOAT( \
		_flags, _type) \
	.type = CYAML_FLOAT, \
	.flags = (_flags), \
	.data_size = sizeof(_type)

/**
 * Mapping schema helper macro for keys with \ref CYAML_FLOAT type.
 *
 * Use this for floating point types contained in structs.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 */
#define CYAML_FIELD_FLOAT( \
		_key, _flags, _structure, _member) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_FLOAT(((_flags) & (~CYAML_FLAG_POINTER)), \
				(((_structure *)NULL)->_member)), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Mapping schema helper macro for keys with \ref CYAML_FLOAT type.
 *
 * Use this for pointers to floating point types.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 */
#define CYAML_FIELD_FLOAT_PTR( \
		_key, _flags, _structure, _member) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_FLOAT(((_flags) | CYAML_FLAG_POINTER), \
				(*(((_structure *)NULL)->_member))), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Value schema helper macro for values with \ref CYAML_STRING type.
 *
 * \note If the string is an array (`char str[N];`) then the \ref
 *       CYAML_FLAG_POINTER flag must **not** be set, and the max
 *       length must be no more than `N-1`.
 *
 *       If the string is a pointer (`char *str;`), then the \ref
 *       CYAML_FLAG_POINTER flag **must be set**.
 *
 * \param[in]  _flags         Any behavioural flags relevant to this value.
 * \param[in]  _type          The C type for this value.
 * \param[in]  _min           Minimum string length in bytes.
 *                            Excludes trailing '\0'.
 * \param[in]  _max           The C type for this value.
 *                            Excludes trailing '\0'.
 */
#define CYAML_VALUE_STRING( \
		_flags, _type, _min, _max) \
	.type = CYAML_STRING, \
	.flags = (_flags), \
	.data_size = sizeof(_type), \
	.string = { \
		.min = _min, \
		.max = _max, \
	}

/**
 * Mapping schema helper macro for keys with \ref CYAML_STRING type.
 *
 * Use this for fields with C array type, e.g. `char str[N];`.  This fills the
 * maximum string length (`N-1`) out automatically.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 * \param[in]  _min        Minimum string length in bytes.  Excludes '\0'.
 */
#define CYAML_FIELD_STRING( \
		_key, _flags, _structure, _member, _min) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_STRING(((_flags) & (~CYAML_FLAG_POINTER)), \
				(((_structure *)NULL)->_member), _min, \
				sizeof(((_structure *)NULL)->_member) - 1), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Mapping schema helper macro for keys with \ref CYAML_STRING type.
 *
 * Use this for fields with C pointer type, e.g. `char *str;`.  This creates
 * a separate allocation for the string data, and fills in the pointer.
 *
 * Use `0` for _min and \ref CYAML_UNLIMITED for _max for unconstrained string
 * lengths.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 * \param[in]  _min        Minimum string length in bytes.  Excludes '\0'.
 * \param[in]  _max        Maximum string length in bytes.  Excludes '\0'.
 */
#define CYAML_FIELD_STRING_PTR( \
		_key, _flags, _structure, _member, _min, _max) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_STRING(((_flags) | CYAML_FLAG_POINTER), \
				(((_structure *)NULL)->_member), \
				_min, _max), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Value schema helper macro for values with \ref CYAML_MAPPING type.
 *
 * \param[in]  _flags         Any behavioural flags relevant to this value.
 * \param[in]  _type          The C type of structure corresponding to mapping.
 * \param[in]  _fields        Pointer to mapping fields schema array.
 */
#define CYAML_VALUE_MAPPING( \
		_flags, _type, _fields) \
	.type = CYAML_MAPPING, \
	.flags = (_flags), \
	.data_size = sizeof(_type), \
	.mapping = { \
		.fields = _fields, \
	}

/**
 * Mapping schema helper macro for keys with \ref CYAML_MAPPING type.
 *
 * Use this for structures contained within other structures.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the containing mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 * \param[in]  _fields     Pointer to mapping fields schema array.
 */
#define CYAML_FIELD_MAPPING( \
		_key, _flags, _structure, _member, _fields) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_MAPPING(((_flags) & (~CYAML_FLAG_POINTER)), \
				(((_structure *)NULL)->_member), _fields), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Mapping schema helper macro for keys with \ref CYAML_MAPPING type.
 *
 * Use this for pointers to structures.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the containing mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 * \param[in]  _fields     Pointer to mapping fields schema array.
 */
#define CYAML_FIELD_MAPPING_PTR( \
		_key, _flags, _structure, _member, _fields) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_MAPPING(((_flags) | CYAML_FLAG_POINTER), \
				(*(((_structure *)NULL)->_member)), _fields), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Value schema helper macro for values with \ref CYAML_SEQUENCE type.
 *
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _type       The C type of sequence **entries**.
 * \param[in]  _entry      Pointer to schema for the **entries** in sequence.
 * \param[in]  _min        Minimum number of sequence entries required.
 * \param[in]  _max        Maximum number of sequence entries required.
 */
#define CYAML_VALUE_SEQUENCE( \
		_flags, _type, _entry, _min, _max) \
	.type = CYAML_SEQUENCE, \
	.flags = (_flags), \
	.data_size = sizeof(_type), \
	.sequence = { \
		.entry = _entry, \
		.min = _min, \
		.max = _max, \
	}

/**
 * Mapping schema helper macro for keys with \ref CYAML_SEQUENCE type.
 *
 * To use this, there must be a member in {_structure} called "{_member}_count",
 * for storing the number of entries in the sequence.
 *
 * For example, for the following structure:
 *
 * ```
 * struct my_structure {
 *         unsigned *my_sequence;
 *         unsigned  my_sequence_count;
 * };
 * ```
 *
 * Pass the following as parameters:
 *
 * | Parameter  | Value                 |
 * | ---------- | --------------------- |
 * | _structure | `struct my_structure` |
 * | _member    | `my_sequence`         |
 *
 * If you want to call the structure member for storing the sequence entry
 * count something else, then use \ref CYAML_FIELD_SEQUENCE_COUNT instead.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 * \param[in]  _entry      Pointer to schema for the **entries** in sequence.
 * \param[in]  _min        Minimum number of sequence entries required.
 * \param[in]  _max        Maximum number of sequence entries required.
 */
#define CYAML_FIELD_SEQUENCE( \
		_key, _flags, _structure, _member, _entry, _min, _max) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_SEQUENCE((_flags), \
				(*(((_structure *)NULL)->_member)), \
				_entry, _min, _max), \
	}, \
	.data_offset = offsetof(_structure, _member), \
	.count_size = sizeof(((_structure *)NULL)->_member ## _count), \
	.count_offset = offsetof(_structure, _member ## _count), \
}

/**
 * Mapping schema helper macro for keys with \ref CYAML_SEQUENCE type.
 *
 * Compared to .\ref CYAML_FIELD_SEQUENCE, this macro takes an extra `_count`
 * parameter, allowing the structure member name for the sequence entry count
 * to be provided explicitly.
 *
 * For example, for the following structure:
 *
 * ```
 * struct my_structure {
 *         unsigned *things;
 *         unsigned  n_things;
 * };
 * ```
 *
 * Pass the following as parameters:
 *
 * | Parameter  | Value                 |
 * | ---------- | --------------------- |
 * | _structure | `struct my_structure` |
 * | _member    | `things`              |
 * | _count     | `n_things`            |
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 * \param[in]  _count      The member in _structure for this sequence's
 *                         entry count.
 * \param[in]  _entry      Pointer to schema for the **entries** in sequence.
 * \param[in]  _min        Minimum number of sequence entries required.
 * \param[in]  _max        Maximum number of sequence entries required.
 */
#define CYAML_FIELD_SEQUENCE_COUNT( \
		_key, _flags, _structure, _member, _count, _entry, _min, _max) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_SEQUENCE((_flags), \
				(*(((_structure *)NULL)->_member)), \
				_entry, _min, _max), \
	}, \
	.data_offset = offsetof(_structure, _member), \
	.count_size = sizeof(((_structure *)NULL)->_count), \
	.count_offset = offsetof(_structure, _count), \
}

/**
 * Value schema helper macro for values with \ref CYAML_SEQUENCE_FIXED type.
 *
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _type       The C type of sequence **entries**.
 * \param[in]  _entry      Pointer to schema for the **entries** in sequence.
 * \param[in]  _count      Number of sequence entries required.
 */
#define CYAML_VALUE_SEQUENCE_FIXED( \
		_flags, _type, _entry, _count) \
	.type = CYAML_SEQUENCE_FIXED, \
	.flags = (_flags), \
	.data_size = sizeof(_type), \
	.sequence = { \
		.entry = _entry, \
		.min = _count, \
		.max = _count, \
	}

/**
 * Mapping schema helper macro for keys with \ref CYAML_SEQUENCE_FIXED type.
 *
 * \param[in]  _key        String defining the YAML mapping key for this value.
 * \param[in]  _flags      Any behavioural flags relevant to this value.
 * \param[in]  _structure  The structure corresponding to the mapping.
 * \param[in]  _member     The member in _structure for this mapping value.
 * \param[in]  _entry      Pointer to schema for the **entries** in sequence.
 * \param[in]  _count      Number of sequence entries required.
 */
#define CYAML_FIELD_SEQUENCE_FIXED( \
		_key, _flags, _structure, _member, _entry, _count) \
{ \
	.key = _key, \
	.value = { \
		CYAML_VALUE_SEQUENCE_FIXED((_flags), \
				(*(((_structure *)NULL)->_member)), \
				_entry, _count), \
	}, \
	.data_offset = offsetof(_structure, _member) \
}

/**
 * Mapping schema helper macro for keys with \ref CYAML_IGNORE type.
 *
 * \param[in]  _key    String defining the YAML mapping key to ignore.
 * \param[in]  _flags  Any behavioural flags relevant to this key.
 */
#define CYAML_FIELD_IGNORE( \
		_key, _flags) \
{ \
	.key = _key, \
	.value = { \
		.type = CYAML_IGNORE, \
		.flags = (_flags), \
	}, \
}

/**
 * Mapping schema helper macro for terminating an array of mapping fields.
 *
 * CYAML mapping schemas are formed from an array of \ref cyaml_schema_field
 * entries, and an entry with a NULL key indicates the end of the array.
 */
#define CYAML_FIELD_END { .key = NULL }

/**
 * Identifies that a \ref CYAML_SEQUENCE has unconstrained maximum entry
 * count.
 */
#define CYAML_UNLIMITED 0xffffffff

/**
 * Helper macro for counting array elements.
 *
 * \note Don't use this macro on pointers.
 *
 * \param[in] _a  A C array.
 * \return Array element count.
 */
#define CYAML_ARRAY_LEN(_a) ((sizeof(_a)) / (sizeof(_a[0])))

/**
 * Data loaded or saved by CYAML has this type.  CYAML schemas are used
 * to describe the data contained.
 */
typedef void cyaml_data_t;

/** CYAML logging levels. */
typedef enum cyaml_log_e {
	CYAML_LOG_DEBUG,   /**< Debug level logging. */
	CYAML_LOG_INFO,    /**< Info level logging. */
	CYAML_LOG_NOTICE,  /**< Notice level logging. */
	CYAML_LOG_WARNING, /**< Warning level logging. */
	CYAML_LOG_ERROR,   /**< Error level logging. */
} cyaml_log_t;

/**
 * CYAML logging function prototype.
 *
 * Clients may implement this to manage logging from CYAML themselves.
 * Otherwise, consider using the standard logging function, \ref cyaml_log.
 *
 * \param[in] level  Log level of message to log.
 * \param[in] fmt    Format string for message to log.
 * \param[in] args   Additional arguments used by fmt.
 */
typedef void (*cyaml_log_fn_t)(
		cyaml_log_t level,
		const char *fmt,
		va_list args);

/**
 * CYAML memory allocation / freeing function.
 *
 * Clients may implement this to handle memory allocation / freeing.
 *
 * \param[in] ctx    Client's private allocation context.
 * \param[in] ptr    Existing allocation to resize, or NULL.
 * \param[in] size   The new size for the allocation.  \note setting 0 must
 *                   be treated as free().
 * \return If `size == 0`, returns NULL.  If `size > 0`, returns NULL on failure,
 *         and any existing allocation is left untouched, or return non-NULL as
 *         the new allocation on success, and the original pointer becomes
 *         invalid.
 */
typedef void * (*cyaml_mem_fn_t)(
		void *ctx,
		void *ptr,
		size_t size);

/**
 * Client CYAML configuration data.
 *
 * \todo Should provide facility for client to provide its own custom
 *       allocation functions.
 */
typedef struct cyaml_config {
	/**
	 * Client function to use for logging.
	 *
	 * Clients can implement their own logging function and set it here.
	 * Otherwise, set `log_fn` to \ref cyaml_log if CYAML's default
	 * logging to `stderr` is suitable (see its documentation for more
	 * details), or set to `NULL` to suppress all logging.
	 *
	 * \note Useful backtraces are issued through the `log_fn` at
	 *       \ref CYAML_LOG_ERROR level.  If your application needs
	 *       to load user YAML data, these backtraces can help users
	 *       figure out what's wrong with their YAML, causing it to
	 *       be rejected by your schema.
	 */
	cyaml_log_fn_t log_fn;
	/**
	 * Client function to use for memory allocation handling.
	 *
	 * Clients can implement their own, or pass \ref cyaml_mem to use
	 * CYAML's default allocator.
	 *
	 * \note Depending on platform, when using CYAML's default allocator,
	 *       clients may need to take care to ensure any allocated memory
	 *       is freed using \ref cyaml_mem too.
	 */
	cyaml_mem_fn_t mem_fn;
	/**
	 * Client memory function context pointer.
	 *
	 * Clients using their own custom allocation function can pass their
	 * context here, which will be passed through to their mem_fn.
	 *
	 * The default allocation function, \ref cyaml_mem doesn't require an
	 * allocation context, so pass NULL for the mem_ctx if using that.
	 */
	void *mem_ctx;
	/**
	 * Minimum logging priority level to be issued.
	 *
	 * Specifying e.g. \ref CYAML_LOG_WARNING will cause only warnings and
	 * errors to emerge.
	 */
	cyaml_log_t log_level;
	/** CYAML behaviour flags. */
	cyaml_cfg_flags_t flags;
} cyaml_config_t;

/**
 * Standard CYAML logging function.
 *
 * This logs to `stderr`.  It clients want to log elsewhere they must
 * implement their own logging function, and pass it to CYAML in the
 * \ref cyaml_config_t structure.
 *
 * \note This default logging function composes single log messages from
 *       multiple separate fprintfs to `stderr`.  If the client application
 *       writes to `stderr` from multiple threads, individual \ref cyaml_log
 *       messages may get broken up by the client applications logging.  To
 *       avoid this, clients should implement their own \ref cyaml_log_fn_t and
 *       pass it in via \ref cyaml_config_t.
 *
 * \param[in] level  Log level of message to log.
 * \param[in] fmt    Format string for message to log.
 * \param[in] args   Additional arguments used by fmt.
 */
extern void cyaml_log(
		cyaml_log_t level,
		const char *fmt,
		va_list args);

/**
 * CYAML default memory allocation / freeing function.
 *
 * This is used when clients don't supply their own.  It is exposed to
 * enable clients to use the same allocator as libcyaml used internally
 * to allocate/free memory when they have not provided their own allocation
 * function.
 *
 * \param[in] ctx    Allocation context, unused.
 * \param[in] ptr    Existing allocation to resize, or NULL.
 * \param[in] size   The new size for the allocation.  \note When `size == 0`
 *                   this frees `ptr`.
 * \return If `size == 0`, returns NULL.  If `size > 0`, returns NULL on failure,
 *         and any existing allocation is left untouched, or return non-NULL as
 *         the new allocation on success, and the original pointer becomes
 *         invalid.
 */
extern void * cyaml_mem(
		void *ctx,
		void *ptr,
		size_t size);

/**
 * Load a YAML document from a file at the given path.
 *
 * \note In the event of the top-level mapping having only optional fields,
 *       and the YAML not setting any of them, this function can return \ref
 *       CYAML_OK, and `NULL` in the `data_out` parameter.
 *
 * \param[in]  path           Path to YAML file to load.
 * \param[in]  config         Client's CYAML configuration structure.
 * \param[in]  schema         CYAML schema for the YAML to be loaded.
 * \param[out] data_out       Returns the caller-owned loaded data on success.
 *                            Untouched on failure.
 * \param[out] seq_count_out  On success, returns the sequence entry count.
 *                            Untouched on failure.
 *                            Must be non-NULL if top-level schema type is
 *                            \ref CYAML_SEQUENCE, otherwise, must be NULL.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
extern cyaml_err_t cyaml_load_file(
		const char *path,
		const cyaml_config_t *config,
		const cyaml_schema_value_t *schema,
		cyaml_data_t **data_out,
		unsigned *seq_count_out);

/**
 * Load a YAML document from a data buffer.
 *
 * \note In the event of the top-level mapping having only optional fields,
 *       and the YAML not setting any of them, this function can return \ref
 *       CYAML_OK, and `NULL` in the `data_out` parameter.
 *
 * \param[in]  input          Buffer to load YAML data from.
 * \param[in]  input_len      Length of input in bytes.
 * \param[in]  config         Client's CYAML configuration structure.
 * \param[in]  schema         CYAML schema for the YAML to be loaded.
 * \param[out] data_out       Returns the caller-owned loaded data on success.
 *                            Untouched on failure.
 * \param[out] seq_count_out  On success, returns the sequence entry count.
 *                            Untouched on failure.
 *                            Must be non-NULL if top-level schema type is
 *                            \ref CYAML_SEQUENCE, otherwise, must be NULL.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
extern cyaml_err_t cyaml_load_data(
		const uint8_t *input,
		size_t input_len,
		const cyaml_config_t *config,
		const cyaml_schema_value_t *schema,
		cyaml_data_t **data_out,
		unsigned *seq_count_out);

/**
 * Save a YAML document to a file at the given path.
 *
 * \param[in] path       Path to YAML file to write.
 * \param[in] config     Client's CYAML configuration structure.
 * \param[in] schema     CYAML schema for the YAML to be saved.
 * \param[in] data       The caller-owned data to be saved.
 * \param[in] seq_count  If top level type is sequence, this should be the
 *                       entry count, otherwise it is ignored.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
extern cyaml_err_t cyaml_save_file(
		const char *path,
		const cyaml_config_t *config,
		const cyaml_schema_value_t *schema,
		const cyaml_data_t *data,
		unsigned seq_count);

/**
 * Save a YAML document into a string in memory.
 *
 * This allocates a buffer containing the serialised YAML data.
 *
 * To free the returned YAML string, clients should use the \ref cyaml_mem_fn_t
 * function set in the \ref cyaml_config_t passed to this function.
 * For example:
 *
 * ```
 * char *yaml;
 * size_t len;
 * err = cyaml_save_file(&yaml, &len, &config, &client_schema, client_data, 0);
 * if (err == CYAML_OK) {
 *         // Use `yaml`:
 *         printf("%*s\n", len, yaml);
 *         // Free `yaml`:
 *         config.mem_fn(config.mem_ctx, yaml, 0);
 * }
 * ```
 *
 * \note The returned YAML string does not have a trailing '\0'.
 *
 * \param[out] output     Returns the caller-owned serialised YAML data on
 *                        success, untouched on failure.  Clients should use
 *                        the \ref cyaml_mem_fn_t function set in the \ref
 *                        cyaml_config_t to free the data.
 * \param[out] len        Returns the length of the data in output on success,
 *                        untouched on failure.
 * \param[out] len        Path to YAML file to write.
 * \param[in]  config     Client's CYAML configuration structure.
 * \param[in]  schema     CYAML schema for the YAML to be saved.
 * \param[in]  data       The caller-owned data to be saved.
 * \param[in]  seq_count  If top level type is sequence, this should be the
 *                        entry count, otherwise it is ignored.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
extern cyaml_err_t cyaml_save_data(
		char **output,
		size_t *len,
		const cyaml_config_t *config,
		const cyaml_schema_value_t *schema,
		const cyaml_data_t *data,
		unsigned seq_count);

/**
 * Free data returned by a CYAML load function.
 *
 * This is a convenience function, which is here purely to minimise the
 * amount of code required in clients.  Clients would be better off writing
 * their own free function for the specific data once loaded.
 *
 * \note This is a recursive operation, freeing all nested data.
 *
 * \param[in] config     The client's CYAML library config.
 * \param[in] schema     The schema describing the content of data.  Must match
 *                       the schema given to the CYAML load function used to
 *                       load the data.
 * \param[in] data       The data structure to free.
 * \param[in] seq_count  If top level type is sequence, this should be the
 *                       entry count, otherwise it is ignored.
 * \return \ref CYAML_OK on success, or appropriate error code otherwise.
 */
extern cyaml_err_t cyaml_free(
		const cyaml_config_t *config,
		const cyaml_schema_value_t *schema,
		cyaml_data_t *data,
		unsigned seq_count);

/**
 * Convert a cyaml error code to a human-readable string.
 *
 * \param[in] err  Error code code to convert.
 * \return String representing err.  The string is '\0' terminated, and owned
 *         by libcyaml.
 */
extern const char * cyaml_strerror(
		cyaml_err_t err);

#ifdef __cplusplus
}
#endif

#endif
