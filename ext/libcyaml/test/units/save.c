/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2018-2019 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <cyaml/cyaml.h>

#include "../../src/data.h"
#include "ttest.h"

/** Macro to squash unused variable compiler warnings. */
#define UNUSED(_x) ((void)(_x))

/** Helper macro to count bytes of YAML input data. */
#define YAML_LEN(_y) (sizeof(_y) - 1)

/**
 * Unit test context data.
 */
typedef struct test_data {
	char **buffer;
	const struct cyaml_config *config;
} test_data_t;

/**
 * Common clean up function to free data allocated by tests.
 *
 * \param[in]  data  The unit test context data.
 */
static void cyaml_cleanup(void *data)
{
	struct test_data *td = data;

	if (td->config->mem_fn != NULL && td->buffer != NULL) {
		td->config->mem_fn(td->config->mem_ctx, *(td->buffer), 0);
	}
}

/**
 * Test saving an unsigned integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_uint(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"test_uint: 555\n"
		"...\n";
	static const struct target_struct {
		unsigned test_uint;
	} data = {
		.test_uint = 555,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("test_uint", CYAML_FLAG_DEFAULT,
				struct target_struct, test_uint),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a float.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_float(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"test_float: 3.14\n"
		"...\n";
	static const struct target_struct {
		float test_float;
	} data = {
		.test_float = 3.14,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("test_float", CYAML_FLAG_DEFAULT,
				struct target_struct, test_float),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a double.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_double(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"test_float: 3.14\n"
		"...\n";
	static const struct target_struct {
		double test_float;
	} data = {
		.test_float = 3.14,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("test_float", CYAML_FLAG_DEFAULT,
				struct target_struct, test_float),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a string.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"test_string: This is a test, of sorts.\n"
		"...\n";
	static const struct target_struct {
		char test_string[32];
	} data = {
		.test_string = "This is a test, of sorts.",
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING("test_string", CYAML_FLAG_DEFAULT,
				struct target_struct, test_string, 0),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a positive signed integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_int_pos(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"test_int: 90\n"
		"...\n";
	static const struct target_struct {
		int test_int;
	} data = {
		.test_int = 90,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("test_int", CYAML_FLAG_DEFAULT,
				struct target_struct, test_int),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a negative signed integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_int_neg(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"test_int: -14\n"
		"...\n";
	static const struct target_struct {
		int test_int;
	} data = {
		.test_int = -14,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("test_int", CYAML_FLAG_DEFAULT,
				struct target_struct, test_int),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a negative signed 64-bit integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_int_64(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"test_int: -9223372036854775800\n"
		"...\n";
	static const struct target_struct {
		int64_t test_int;
	} data = {
		.test_int = -9223372036854775800,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("test_int", CYAML_FLAG_DEFAULT,
				struct target_struct, test_int),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a boolean.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_bool_true(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"test_bool: true\n"
		"...\n";
	static const struct target_struct {
		bool test_bool;
	} data = {
		.test_bool = true,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BOOL("test_bool", CYAML_FLAG_DEFAULT,
				struct target_struct, test_bool),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a boolean.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_bool_false(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"test_bool: false\n"
		"...\n";
	static const struct target_struct {
		bool test_bool;
	} data = {
		.test_bool = false,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BOOL("test_bool", CYAML_FLAG_DEFAULT,
				struct target_struct, test_bool),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a string pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_string_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"test_string: This is a test, of sorts.\n"
		"...\n";
	static const struct target_struct {
		const char *test_string;
	} data = {
		.test_string = "This is a test, of sorts.",
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("test_string", CYAML_FLAG_POINTER,
				struct target_struct, test_string,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a strict enum.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_enum_strict(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_e {
		FIRST, SECOND, THIRD, FOURTH, COUNT
	};
	static const cyaml_strval_t strings[COUNT] = {
		{ "first",  0 },
		{ "second", 1 },
		{ "third",  2 },
		{ "fourth", 3 },
	};
	static const unsigned char ref[] =
		"---\n"
		"test_enum: third\n"
		"...\n";
	static const struct target_struct {
		enum test_e test_enum;
	} data = {
		.test_enum = THIRD,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_ENUM("test_enum", CYAML_FLAG_STRICT,
				struct target_struct, test_enum, strings, 4),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a numerical enum.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_enum_number(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_e {
		FIRST, SECOND, THIRD, FOURTH
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  0 },
		{ "second", 1 },
		{ "third",  2 },
		{ "fourth", 3 },
	};
	static const unsigned char ref[] =
		"---\n"
		"test_enum: 99\n"
		"...\n";
	static const struct target_struct {
		enum test_e test_enum;
	} data = {
		.test_enum = 99,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_ENUM("test_enum", CYAML_FLAG_DEFAULT,
				struct target_struct, test_enum, strings, 4),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sparse, unordered enum.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_enum_sparse(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_e {
		FIRST  = -33,
		SECOND = 1,
		THIRD  = 256,
		FOURTH = 3,
		FIFTH  = 999,
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  FIRST },
		{ "second", SECOND },
		{ "third",  THIRD },
		{ "fourth", FOURTH },
		{ "fifth",  FIFTH },
	};
	static const unsigned char ref[] =
		"---\n"
		"test_enum: third\n"
		"...\n";
	static const struct target_struct {
		enum test_e test_enum;
	} data = {
		.test_enum = THIRD,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_ENUM("test_enum", CYAML_FLAG_STRICT,
				struct target_struct, test_enum,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a mapping.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_mapping(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	};
	static const unsigned char ref[] =
		"---\n"
		"mapping:\n"
		"  a: 123\n"
		"  b: 9999\n"
		"...\n";
	static const struct target_struct {
		struct value_s test_mapping;
	} data = {
		.test_mapping = {
			.a = 123,
			.b = 9999
		},
	};
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT, struct value_s, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT, struct value_s, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_MAPPING("mapping", CYAML_FLAG_DEFAULT,
				struct target_struct, test_mapping,
				test_mapping_schema),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a mapping pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_mapping_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	} value = {
		.a = 123,
		.b = 9999
	};
	static const unsigned char ref[] =
		"---\n"
		"mapping:\n"
		"  a: 123\n"
		"  b: 9999\n"
		"...\n";
	const struct target_struct {
		struct value_s *test_mapping;
	} data = {
		.test_mapping = &value,
	};
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT, struct value_s, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT, struct value_s, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_MAPPING_PTR("mapping", CYAML_FLAG_POINTER,
				struct target_struct, test_mapping,
				test_mapping_schema),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a strict flags value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_flags_strict(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_f {
		NONE   = 0,
		FIRST  = (1 << 0),
		SECOND = (1 << 1),
		THIRD  = (1 << 2),
		FOURTH = (1 << 3),
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
	};
	static const unsigned char ref[] =
		"---\n"
		"test_flags:\n"
		"- first\n"
		"- fourth\n"
		"...\n";
	static const struct target_struct {
		enum test_f test_flags;
	} data = {
		.test_flags = FIRST | FOURTH,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("test_flags", CYAML_FLAG_STRICT,
				struct target_struct, test_flags, strings, 4),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a numerical flags value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_flags_number(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_f {
		NONE   = 0,
		FIRST  = (1 << 0),
		SECOND = (1 << 1),
		THIRD  = (1 << 2),
		FOURTH = (1 << 3),
	};
	static const cyaml_strval_t strings[] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
	};
	static const unsigned char ref[] =
		"---\n"
		"test_flags:\n"
		"- first\n"
		"- fourth\n"
		"- 1024\n"
		"...\n";
	static const struct target_struct {
		enum test_f test_flags;
	} data = {
		.test_flags = FIRST | FOURTH | 1024,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("test_flags", CYAML_FLAG_DEFAULT,
				struct target_struct, test_flags, strings, 4),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sparse flags value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_flags_sparse(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_f {
		NONE   = 0,
		FIRST  = (1 <<  3),
		SECOND = (1 <<  9),
		THIRD  = (1 << 10),
		FOURTH = (1 << 21),
	};
	static const cyaml_strval_t strings[] = {
		{ "none",   NONE   },
		{ "first",  FIRST  },
		{ "second", SECOND },
		{ "third",  THIRD  },
		{ "fourth", FOURTH },
	};
	static const unsigned char ref[] =
		"---\n"
		"test_flags:\n"
		"- first\n"
		"- fourth\n"
		"...\n";
	static const struct target_struct {
		enum test_f test_flags;
	} data = {
		.test_flags = FIRST | FOURTH,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("test_flags", CYAML_FLAG_DEFAULT,
				struct target_struct, test_flags,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a bitfield value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_bitfield(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
		{ .name = "b", .offset =  3, .bits =  7 },
		{ .name = "c", .offset = 10, .bits = 32 },
		{ .name = "d", .offset = 42, .bits =  8 },
		{ .name = "e", .offset = 50, .bits = 14 },
	};
	static const unsigned char ref[] =
		"---\n"
		"test_bitfield:\n"
		"  a: 0x7\n"
		"  b: 0x7f\n"
		"  c: 0xffffffff\n"
		"  d: 0xff\n"
		"  e: 0x3fff\n"
		"...\n";
	static const struct target_struct {
		uint64_t test_bitfield;
	} data = {
		.test_bitfield = 0xFFFFFFFFFFFFFFFFu,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BITFIELD("test_bitfield", CYAML_FLAG_DEFAULT,
				struct target_struct, test_bitfield,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sparse bitfield value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_bitfield_sparse(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
		{ .name = "b", .offset =  3, .bits =  7 },
		{ .name = "c", .offset = 10, .bits = 32 },
		{ .name = "d", .offset = 42, .bits =  8 },
		{ .name = "e", .offset = 50, .bits = 14 },
	};
	static const unsigned char ref[] =
		"---\n"
		"test_bitfield:\n"
		"  a: 0x7\n"
		"  b: 0x7f\n"
		"  e: 0x3fff\n"
		"...\n";
	static const struct target_struct {
		uint64_t test_bitfield;
	} data = {
		.test_bitfield = (   0x7llu <<  0) |
		                 (  0x7Fllu <<  3) |
		                 (0x3FFFllu << 50),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BITFIELD("test_bitfield", CYAML_FLAG_DEFAULT,
				struct target_struct, test_bitfield,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of integers.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- 1\n"
		"- 1\n"
		"- 2\n"
		"- 3\n"
		"- 5\n"
		"- 8\n"
		"...\n";
	static const struct target_struct {
		int seq[6];
		uint32_t seq_count;
	} data = {
		.seq = { 1, 1, 2, 3, 5, 8 },
		.seq_count = CYAML_ARRAY_LEN(data.seq),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data.seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of unsigned integers.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_uint(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- 1\n"
		"- 1\n"
		"- 2\n"
		"- 3\n"
		"- 5\n"
		"- 8\n"
		"...\n";
	static const struct target_struct {
		unsigned seq[6];
		uint32_t seq_count;
	} data = {
		.seq = { 1, 1, 2, 3, 5, 8 },
		.seq_count = CYAML_ARRAY_LEN(data.seq),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, *(data.seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of enums.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_enum(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD,
		TEST_ENUM__COUNT,
	};
	static const cyaml_strval_t strings[TEST_ENUM__COUNT] = {
		[TEST_ENUM_FIRST]  = { "first",  0 },
		[TEST_ENUM_SECOND] = { "second", 1 },
		[TEST_ENUM_THIRD]  = { "third",  2 },
	};
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- first\n"
		"- second\n"
		"- third\n"
		"...\n";
	static const struct target_struct {
		enum test_enum seq[3];
		uint32_t seq_count;
	} data = {
		.seq = { TEST_ENUM_FIRST, TEST_ENUM_SECOND, TEST_ENUM_THIRD },
		.seq_count = CYAML_ARRAY_LEN(data.seq),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_ENUM(CYAML_FLAG_DEFAULT, *(data.seq),
				strings, TEST_ENUM__COUNT),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of boolean values.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_bool(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- true\n"
		"- false\n"
		"- true\n"
		"- false\n"
		"- true\n"
		"- false\n"
		"- true\n"
		"- false\n"
		"...\n";
	static const struct target_struct {
		bool seq[8];
		uint32_t seq_count;
	} data = {
		.seq = { true, false, true, false, true, false, true, false },
		.seq_count = CYAML_ARRAY_LEN(data.seq),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_BOOL(CYAML_FLAG_DEFAULT, *(data.seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of flags.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_flags(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_flags {
		TEST_FLAGS_NONE   = 0,
		TEST_FLAGS_FIRST  = (1 << 0),
		TEST_FLAGS_SECOND = (1 << 1),
		TEST_FLAGS_THIRD  = (1 << 2),
		TEST_FLAGS_FOURTH = (1 << 3),
		TEST_FLAGS_FIFTH  = (1 << 4),
		TEST_FLAGS_SIXTH  = (1 << 5),
	};
	static const cyaml_strval_t strings[] = {
		{ "none",   TEST_FLAGS_NONE },
		{ "first",  TEST_FLAGS_FIRST },
		{ "second", TEST_FLAGS_SECOND },
		{ "third",  TEST_FLAGS_THIRD },
		{ "fourth", TEST_FLAGS_FOURTH },
		{ "fifth",  TEST_FLAGS_FIFTH },
		{ "sixth",  TEST_FLAGS_SIXTH },
	};
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- - second\n"
		"  - fifth\n"
		"  - 1024\n"
		"- - first\n"
		"- - fourth\n"
		"  - sixth\n"
		"...\n";
	static const struct target_struct {
		enum test_flags seq[3];
		uint32_t seq_count;
	} data = {
		.seq = {
			TEST_FLAGS_SECOND | TEST_FLAGS_FIFTH | 1024,
			TEST_FLAGS_FIRST,
			TEST_FLAGS_FOURTH | TEST_FLAGS_SIXTH },
		.seq_count = CYAML_ARRAY_LEN(data.seq),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_FLAGS(CYAML_FLAG_DEFAULT, *(data.seq),
				strings, CYAML_ARRAY_LEN(strings)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of strings.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- This\n"
		"- is\n"
		"- merely\n"
		"- a\n"
		"- test\n"
		"...\n";
	static const struct target_struct {
		char seq[5][7];
		uint32_t seq_count;
	} data = {
		.seq = {
			"This",
			"is",
			"merely",
			"a",
			"test", },
		.seq_count = CYAML_ARRAY_LEN(data.seq),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_DEFAULT, *(data.seq), 0, 6),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of strings.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_string_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- This\n"
		"- is\n"
		"- merely\n"
		"- a\n"
		"- test\n"
		"...\n";
	static const struct target_struct {
		char *seq[5];
		uint32_t seq_count;
	} data = {
		.seq = {
			"This",
			"is",
			"merely",
			"a",
			"test", },
		.seq_count = CYAML_ARRAY_LEN(data.seq),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, *(data.seq),
				0, CYAML_UNLIMITED),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of mappings.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_mapping(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	};
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- a: 123\n"
		"  b: 9999\n"
		"- a: 4000\n"
		"  b: 62000\n"
		"- a: 1\n"
		"  b: 765\n"
		"...\n";
	static const struct target_struct {
		struct value_s seq[3];
		uint32_t seq_count;
	} data = {
		.seq = {
			[0] = { .a = 123,  .b = 9999  },
			[1] = { .a = 4000, .b = 62000 },
			[2] = { .a = 1,    .b = 765   }, },
		.seq_count = CYAML_ARRAY_LEN(data.seq),
	};
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT, struct value_s, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT, struct value_s, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,
				struct value_s, test_mapping_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of mappings pointers.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_mapping_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const struct value_s {
		short a;
		long b;
	} v[3] = {
		{ .a =  123, .b =  9999, },
		{ .a = 4000, .b = 62000, },
		{ .a =    1, .b =   765, },
	};
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- a: 123\n"
		"  b: 9999\n"
		"- a: 4000\n"
		"  b: 62000\n"
		"- a: 1\n"
		"  b: 765\n"
		"...\n";
	static const struct target_struct {
		const struct value_s *seq[3];
		uint32_t seq_count;
	} data = {
		.seq = {
			[0] = &v[0],
			[1] = &v[1],
			[2] = &v[2], },
		.seq_count = CYAML_ARRAY_LEN(data.seq),
	};
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT, struct value_s, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT, struct value_s, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct value_s, test_mapping_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of fixed-length sequences.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_sequence_fixed_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- - 1\n"
		"  - 2\n"
		"  - 3\n"
		"- - 4\n"
		"  - 5\n"
		"  - 6\n"
		"- - 7\n"
		"  - 8\n"
		"  - 9\n"
		"- - 10\n"
		"  - 11\n"
		"  - 12\n"
		"...\n";
	static const struct target_struct {
		int seq[4][3];
		uint32_t seq_count;
	} data = {
		.seq = {
			{  1,  2,  3 },
			{  4,  5,  6 },
			{  7,  8,  9 },
			{ 10, 11, 12 },
		},
		.seq_count = CYAML_ARRAY_LEN(data.seq),
	};
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, **(data.seq)),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_DEFAULT, **(data.seq),
				&entry_schema_int, CYAML_ARRAY_LEN(*data.seq))
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END,
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of fixed-length sequences pointers.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_sequence_fixed_ptr_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const int v[4][3] = {
		{  1,  2,  3 },
		{  4,  5,  6 },
		{  7,  8,  9 },
		{ 10, 11, 12 },
	};
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- - 1\n"
		"  - 2\n"
		"  - 3\n"
		"- - 4\n"
		"  - 5\n"
		"  - 6\n"
		"- - 7\n"
		"  - 8\n"
		"  - 9\n"
		"- - 10\n"
		"  - 11\n"
		"  - 12\n"
		"...\n";
	static const struct target_struct {
		const int *seq[4];
		uint32_t seq_count;
	} data = {
		.seq = {
			v[0],
			v[1],
			v[2],
			v[3],
		},
		.seq_count = CYAML_ARRAY_LEN(data.seq),
	};
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, **(data.seq)),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_POINTER, **(data.seq),
				&entry_schema_int, CYAML_ARRAY_LEN(*v))
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END,
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a flattened sequence of fixed-length sequences.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_sequence_fixed_flat_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- - 1\n"
		"  - 2\n"
		"  - 3\n"
		"- - 4\n"
		"  - 5\n"
		"  - 6\n"
		"- - 7\n"
		"  - 8\n"
		"  - 9\n"
		"- - 10\n"
		"  - 11\n"
		"  - 12\n"
		"...\n";
	static const struct target_struct {
		int seq[12];
		uint32_t seq_count;
	} data = {
		.seq = {
			 1,  2,  3,
			 4,  5,  6,
			 7,  8,  9,
			10, 11, 12,
		},
		/* Note: count is count of entries of the outer sequence
		 * entries, so, 4, not 12. */
		.seq_count = CYAML_ARRAY_LEN(data.seq) / 3,
	};
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_DEFAULT, int,
				&entry_schema_int, 3),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "sequence",
			.value = {
				.type = CYAML_SEQUENCE,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = sizeof(int[3]),
				.sequence = {
					.entry = &entry_schema,
					.min = 0,
					.max = CYAML_UNLIMITED,
				}
			},
			.data_offset = offsetof(struct target_struct, seq),
			.count_size = sizeof(data.seq_count),
			.count_offset = offsetof(struct target_struct, seq_count),
		},
		CYAML_FIELD_END,
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of integers.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_ptr_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- 1\n"
		"- 1\n"
		"- 2\n"
		"- 3\n"
		"- 5\n"
		"- 8\n"
		"...\n";
	static const int seq[6] = { 1, 1, 2, 3, 5, 8 };
	static const struct target_struct {
		const int *seq;
		uint32_t seq_count;
	} data = {
		.seq = seq,
		.seq_count = CYAML_ARRAY_LEN(seq),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data.seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of unsigned integers.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_ptr_uint(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- 1\n"
		"- 1\n"
		"- 2\n"
		"- 3\n"
		"- 5\n"
		"- 8\n"
		"...\n";
	static const unsigned seq[6] = { 1, 1, 2, 3, 5, 8 };
	static const struct target_struct {
		const unsigned *seq;
		uint32_t seq_count;
	} data = {
		.seq = seq,
		.seq_count = CYAML_ARRAY_LEN(seq),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, *(data.seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of enums.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_ptr_enum(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD,
		TEST_ENUM__COUNT,
	};
	static const cyaml_strval_t strings[TEST_ENUM__COUNT] = {
		[TEST_ENUM_FIRST]  = { "first",  0 },
		[TEST_ENUM_SECOND] = { "second", 1 },
		[TEST_ENUM_THIRD]  = { "third",  2 },
	};
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- first\n"
		"- second\n"
		"- third\n"
		"...\n";
	static const enum test_enum seq[3] = {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD
	};
	static const struct target_struct {
		const enum test_enum *seq;
		uint32_t seq_count;
	} data = {
		.seq = seq,
		.seq_count = CYAML_ARRAY_LEN(seq),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_ENUM(CYAML_FLAG_DEFAULT, *(data.seq),
				strings, TEST_ENUM__COUNT),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of boolean values.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_ptr_bool(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- true\n"
		"- false\n"
		"- true\n"
		"- false\n"
		"- true\n"
		"- false\n"
		"- true\n"
		"- false\n"
		"...\n";
	static const bool seq[8] = {
		true, false, true, false,
		true, false, true, false
	};
	static const struct target_struct {
		const bool *seq;
		uint32_t seq_count;
	} data = {
		.seq = seq,
		.seq_count = CYAML_ARRAY_LEN(seq),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_BOOL(CYAML_FLAG_DEFAULT, *(data.seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of flags.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_ptr_flags(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_flags {
		TEST_FLAGS_NONE   = 0,
		TEST_FLAGS_FIRST  = (1 << 0),
		TEST_FLAGS_SECOND = (1 << 1),
		TEST_FLAGS_THIRD  = (1 << 2),
		TEST_FLAGS_FOURTH = (1 << 3),
		TEST_FLAGS_FIFTH  = (1 << 4),
		TEST_FLAGS_SIXTH  = (1 << 5),
	};
	static const cyaml_strval_t strings[] = {
		{ "none",   TEST_FLAGS_NONE },
		{ "first",  TEST_FLAGS_FIRST },
		{ "second", TEST_FLAGS_SECOND },
		{ "third",  TEST_FLAGS_THIRD },
		{ "fourth", TEST_FLAGS_FOURTH },
		{ "fifth",  TEST_FLAGS_FIFTH },
		{ "sixth",  TEST_FLAGS_SIXTH },
	};
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- - second\n"
		"  - fifth\n"
		"  - 1024\n"
		"- - first\n"
		"- - fourth\n"
		"  - sixth\n"
		"...\n";
	static const enum test_flags seq[3] = {
		TEST_FLAGS_SECOND | TEST_FLAGS_FIFTH | 1024,
		TEST_FLAGS_FIRST,
		TEST_FLAGS_FOURTH | TEST_FLAGS_SIXTH,
	};
	static const struct target_struct {
		const enum test_flags *seq;
		uint32_t seq_count;
	} data = {
		.seq = seq,
		.seq_count = CYAML_ARRAY_LEN(seq),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_FLAGS(CYAML_FLAG_DEFAULT, *(data.seq),
				strings, CYAML_ARRAY_LEN(strings)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of strings.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_ptr_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- This\n"
		"- is\n"
		"- merely\n"
		"- a\n"
		"- test\n"
		"...\n";
	static const char seq[][7] = {
		"This",
		"is",
		"merely",
		"a",
		"test",
	};
	static const struct target_struct {
		const char (*seq)[7];
		uint32_t seq_count;
	} data = {
		.seq = seq,
		.seq_count = CYAML_ARRAY_LEN(seq),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_DEFAULT, *(data.seq), 0, 6),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of strings.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_ptr_string_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- This\n"
		"- is\n"
		"- merely\n"
		"- a\n"
		"- test\n"
		"...\n";
	static const char *seq[] = {
		"This",
		"is",
		"merely",
		"a",
		"test",
	};
	static const struct target_struct {
		const char **seq;
		uint32_t seq_count;
	} data = {
		.seq = seq,
		.seq_count = CYAML_ARRAY_LEN(seq),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, *(data.seq),
				0, CYAML_UNLIMITED),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of mappings.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_ptr_mapping(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	};
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- a: 123\n"
		"  b: 9999\n"
		"- a: 4000\n"
		"  b: 62000\n"
		"- a: 1\n"
		"  b: 765\n"
		"...\n";
	static const struct value_s seq[3] = {
		[0] = { .a = 123,  .b = 9999  },
		[1] = { .a = 4000, .b = 62000 },
		[2] = { .a = 1,    .b = 765   },
	};
	static const struct target_struct {
		const struct value_s *seq;
		uint32_t seq_count;
	} data = {
		.seq = seq,
		.seq_count = CYAML_ARRAY_LEN(seq),
	};
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT, struct value_s, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT, struct value_s, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,
				struct value_s, test_mapping_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of mappings pointers.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_ptr_mapping_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const struct value_s {
		short a;
		long b;
	} v[3] = {
		{ .a =  123, .b =  9999, },
		{ .a = 4000, .b = 62000, },
		{ .a =    1, .b =   765, },
	};
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- a: 123\n"
		"  b: 9999\n"
		"- a: 4000\n"
		"  b: 62000\n"
		"- a: 1\n"
		"  b: 765\n"
		"...\n";
	static const struct value_s *seq[3] = {
		[0] = &v[0],
		[1] = &v[1],
		[2] = &v[2],
	};
	static const struct target_struct {
		const struct value_s **seq;
		uint32_t seq_count;
	} data = {
		.seq = seq,
		.seq_count = CYAML_ARRAY_LEN(seq),
	};
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT, struct value_s, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT, struct value_s, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct value_s, test_mapping_schema),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of fixed-length sequences.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_ptr_sequence_fixed_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- - 1\n"
		"  - 2\n"
		"  - 3\n"
		"- - 4\n"
		"  - 5\n"
		"  - 6\n"
		"- - 7\n"
		"  - 8\n"
		"  - 9\n"
		"- - 10\n"
		"  - 11\n"
		"  - 12\n"
		"...\n";
	static const int seq[4][3] = {
		{  1,  2,  3 },
		{  4,  5,  6 },
		{  7,  8,  9 },
		{ 10, 11, 12 },
	};
	static const struct target_struct {
		const int (*seq)[3];
		uint32_t seq_count;
	} data = {
		.seq = seq,
		.seq_count = CYAML_ARRAY_LEN(seq),
	};
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, **(data.seq)),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_DEFAULT, **(data.seq),
				&entry_schema_int, CYAML_ARRAY_LEN(*data.seq))
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END,
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence of fixed-length sequences pointers.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_ptr_sequence_fixed_ptr_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const int v[4][3] = {
		{  1,  2,  3 },
		{  4,  5,  6 },
		{  7,  8,  9 },
		{ 10, 11, 12 },
	};
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- - 1\n"
		"  - 2\n"
		"  - 3\n"
		"- - 4\n"
		"  - 5\n"
		"  - 6\n"
		"- - 7\n"
		"  - 8\n"
		"  - 9\n"
		"- - 10\n"
		"  - 11\n"
		"  - 12\n"
		"...\n";
	static const int *seq[] = {
		v[0],
		v[1],
		v[2],
		v[3],
	};
	static const struct target_struct {
		const int **seq;
		uint32_t seq_count;
	} data = {
		.seq = seq,
		.seq_count = CYAML_ARRAY_LEN(seq),
	};
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, **(data.seq)),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_POINTER, **(data.seq),
				&entry_schema_int, CYAML_ARRAY_LEN(*v))
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(ref)),
		CYAML_FIELD_END,
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a flattened sequence of fixed-length sequences.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_sequence_ptr_sequence_fixed_flat_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"sequence:\n"
		"- - 1\n"
		"  - 2\n"
		"  - 3\n"
		"- - 4\n"
		"  - 5\n"
		"  - 6\n"
		"- - 7\n"
		"  - 8\n"
		"  - 9\n"
		"- - 10\n"
		"  - 11\n"
		"  - 12\n"
		"...\n";
	static const int seq[12] = {
		 1,  2,  3,
		 4,  5,  6,
		 7,  8,  9,
		10, 11, 12,
	};
	static const struct target_struct {
		const int *seq;
		uint32_t seq_count;
	} data = {
		.seq = seq,
		/* Note: count is count of entries of the outer sequence
		 * entries, so, 4, not 12. */
		.seq_count = CYAML_ARRAY_LEN(seq) / 3,
	};
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_DEFAULT, int,
				&entry_schema_int, 3),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "sequence",
			.value = {
				.type = CYAML_SEQUENCE,
				.flags = CYAML_FLAG_POINTER,
				.data_size = sizeof(int[3]),
				.sequence = {
					.entry = &entry_schema,
					.min = 0,
					.max = CYAML_UNLIMITED,
				}
			},
			.data_offset = offsetof(struct target_struct, seq),
			.count_size = sizeof(data.seq_count),
			.count_offset = offsetof(struct target_struct, seq_count),
		},
		CYAML_FIELD_END,
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving optional mapping field.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_optional_uint(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"test_uint: 555\n"
		"...\n";
	static const struct target_struct {
		unsigned test_uint;
	} data = {
		.test_uint = 555,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("test_uint", CYAML_FLAG_OPTIONAL,
				struct target_struct, test_uint),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving optional mapping field with pointer value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_optional_uint_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"test_uint: 555\n"
		"...\n";
	static unsigned test_uint = 555;
	static const struct target_struct {
		unsigned *test_uint;
	} data = {
		.test_uint = &test_uint,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT_PTR("test_uint",
				CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER,
				struct target_struct, test_uint),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving optional mapping field with NULL pointer value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_optional_uint_ptr_null(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"--- {}\n"
		"...\n";
	static const struct target_struct {
		unsigned *test_uint;
	} data = {
		.test_uint = NULL,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT_PTR("test_uint",
				CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER,
				struct target_struct, test_uint),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving an unsigned integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_entry_ignored(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"test_uint: 555\n"
		"...\n";
	static const struct target_struct {
		unsigned test_uint;
	} data = {
		.test_uint = 555,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("test_uint", CYAML_FLAG_DEFAULT,
				struct target_struct, test_uint),
		CYAML_FIELD_IGNORE("foo", CYAML_FLAG_DEFAULT),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving with schema with sequence_fixed top level type.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_schema_top_level_sequence_fixed(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"- 7\n"
		"- 6\n"
		"- 5\n"
		"...\n";
	int data[3] = { 7, 6, 5 };
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(CYAML_FLAG_POINTER, int,
				&entry_schema, 3)
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema, data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence with flow style configured.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_sequence_config_flow_style(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"--- [7, 6, 5]\n"
		"...\n";
	int data[3] = { 7, 6, 5 };
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, int,
				&entry_schema, 0, 3),
	};
	char *buffer = NULL;
	size_t len = 0;
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.buffer = &buffer,
		.config = &cfg,
	};
	cyaml_err_t err;

	cfg.flags |= CYAML_CFG_STYLE_FLOW;
	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, &cfg, &top_schema, data, 3);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a sequence with block style configured.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_sequence_config_block_style(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"- 7\n"
		"- 6\n"
		"- 5\n"
		"...\n";
	int data[3] = { 7, 6, 5 };
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, int,
				&entry_schema, 0, 3),
	};
	char *buffer = NULL;
	size_t len = 0;
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.buffer = &buffer,
		.config = &cfg,
	};
	cyaml_err_t err;

	cfg.flags |= CYAML_CFG_STYLE_BLOCK;
	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, &cfg, &top_schema, data, 3);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a mapping with flow style value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_value_flow_style(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const char ref[] =
		"--- {a: 555, b: 99, c: 7}\n"
		"...\n";
	static const struct target_struct {
		unsigned a;
		unsigned b;
		unsigned c;
	} data = {
		.a = 555,
		.b = 99,
		.c = 7,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_UINT("b", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_UINT("c", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER | CYAML_FLAG_FLOW,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.buffer = &buffer,
		.config = &cfg,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving a mapping with block style value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_mapping_value_block_style(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"---\n"
		"a: 555\n"
		"b: 99\n"
		"c: 7\n"
		"...\n";
	static const struct target_struct {
		unsigned a;
		unsigned b;
		unsigned c;
	} data = {
		.a = 555,
		.b = 99,
		.c = 7,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_UINT("b", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_UINT("c", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER | CYAML_FLAG_BLOCK,
				struct target_struct, mapping_schema),
	};
	char *buffer = NULL;
	size_t len = 0;
	test_data_t td = {
		.buffer = &buffer,
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_data(&buffer, &len, config, &top_schema,
				&data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Test saving without document delimiters.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_save_no_document_delimiters(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char ref[] =
		"test_uint: 555\n";
	static const struct target_struct {
		unsigned test_uint;
	} data = {
		.test_uint = 555,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("test_uint", CYAML_FLAG_DEFAULT,
				struct target_struct, test_uint),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	cyaml_config_t cfg = *config;
	char *buffer;
	size_t len;
	test_data_t td = {
		.buffer = &buffer,
		.config = &cfg,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	cfg.flags &= ~CYAML_CFG_DOCUMENT_DELIM;
	err = cyaml_save_data(&buffer, &len, &cfg, &top_schema, &data, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (len != YAML_LEN(ref) || memcmp(ref, buffer, len) != 0) {
		return ttest_fail(&tc, "Bad data:\n"
				"EXPECTED (%zu):\n\n%.*s\n\n"
				"GOT (%zu):\n\n%.*s\n",
				YAML_LEN(ref), YAML_LEN(ref), ref,
				len, len, buffer);
	}

	return ttest_pass(&tc);
}

/**
 * Run the YAML saving unit tests.
 *
 * \param[in]  rc         The ttest report context.
 * \param[in]  log_level  CYAML log level.
 * \param[in]  log_fn     CYAML logging function, or NULL.
 * \return true iff all unit tests pass, otherwise false.
 */
bool save_tests(
		ttest_report_ctx_t *rc,
		cyaml_log_t log_level,
		cyaml_log_fn_t log_fn)
{
	bool pass = true;
	cyaml_config_t config = {
		.log_fn = log_fn,
		.mem_fn = cyaml_mem,
		.log_level = log_level,
		.flags = CYAML_CFG_DOCUMENT_DELIM,
	};

	ttest_heading(rc, "Save single entry mapping tests: simple types");

	pass &= test_save_mapping_entry_uint(rc, &config);
	pass &= test_save_mapping_entry_float(rc, &config);
	pass &= test_save_mapping_entry_double(rc, &config);
	pass &= test_save_mapping_entry_string(rc, &config);
	pass &= test_save_mapping_entry_int_64(rc, &config);
	pass &= test_save_mapping_entry_int_pos(rc, &config);
	pass &= test_save_mapping_entry_int_neg(rc, &config);
	pass &= test_save_mapping_entry_bool_true(rc, &config);
	pass &= test_save_mapping_entry_bool_false(rc, &config);
	pass &= test_save_mapping_entry_string_ptr(rc, &config);
	pass &= test_save_mapping_entry_enum_strict(rc, &config);
	pass &= test_save_mapping_entry_enum_number(rc, &config);
	pass &= test_save_mapping_entry_enum_sparse(rc, &config);

	ttest_heading(rc, "Save single entry mapping tests: complex types");

	pass &= test_save_mapping_entry_mapping(rc, &config);
	pass &= test_save_mapping_entry_bitfield(rc, &config);
	pass &= test_save_mapping_entry_mapping_ptr(rc, &config);
	pass &= test_save_mapping_entry_flags_strict(rc, &config);
	pass &= test_save_mapping_entry_flags_number(rc, &config);
	pass &= test_save_mapping_entry_flags_sparse(rc, &config);
	pass &= test_save_mapping_entry_bitfield_sparse(rc, &config);

	ttest_heading(rc, "Save single entry mapping tests: sequences");

	pass &= test_save_mapping_entry_sequence_int(rc, &config);
	pass &= test_save_mapping_entry_sequence_uint(rc, &config);
	pass &= test_save_mapping_entry_sequence_enum(rc, &config);
	pass &= test_save_mapping_entry_sequence_bool(rc, &config);
	pass &= test_save_mapping_entry_sequence_flags(rc, &config);
	pass &= test_save_mapping_entry_sequence_string(rc, &config);
	pass &= test_save_mapping_entry_sequence_mapping(rc, &config);
	pass &= test_save_mapping_entry_sequence_string_ptr(rc, &config);
	pass &= test_save_mapping_entry_sequence_mapping_ptr(rc, &config);
	pass &= test_save_mapping_entry_sequence_sequence_fixed_int(rc, &config);
	pass &= test_save_mapping_entry_sequence_sequence_fixed_ptr_int(rc, &config);
	pass &= test_save_mapping_entry_sequence_sequence_fixed_flat_int(rc, &config);

	ttest_heading(rc, "Save single entry mapping tests: ptr sequences");

	pass &= test_save_mapping_entry_sequence_ptr_int(rc, &config);
	pass &= test_save_mapping_entry_sequence_ptr_enum(rc, &config);
	pass &= test_save_mapping_entry_sequence_ptr_uint(rc, &config);
	pass &= test_save_mapping_entry_sequence_ptr_bool(rc, &config);
	pass &= test_save_mapping_entry_sequence_ptr_flags(rc, &config);
	pass &= test_save_mapping_entry_sequence_ptr_string(rc, &config);
	pass &= test_save_mapping_entry_sequence_ptr_mapping(rc, &config);
	pass &= test_save_mapping_entry_sequence_ptr_string_ptr(rc, &config);
	pass &= test_save_mapping_entry_sequence_ptr_mapping_ptr(rc, &config);
	pass &= test_save_mapping_entry_sequence_ptr_sequence_fixed_int(rc, &config);
	pass &= test_save_mapping_entry_sequence_ptr_sequence_fixed_ptr_int(rc, &config);
	pass &= test_save_mapping_entry_sequence_ptr_sequence_fixed_flat_int(rc, &config);

	ttest_heading(rc, "Save tests: optional mapping fields");

	pass &= test_save_mapping_entry_optional_uint(rc, &config);
	pass &= test_save_mapping_entry_optional_uint_ptr(rc, &config);
	pass &= test_save_mapping_entry_optional_uint_ptr_null(rc, &config);

	ttest_heading(rc, "Save tests: various");

	pass &= test_save_mapping_entry_ignored(rc, &config);
	pass &= test_save_no_document_delimiters(rc, &config);
	pass &= test_save_mapping_value_flow_style(rc, &config);
	pass &= test_save_mapping_value_block_style(rc, &config);
	pass &= test_save_sequence_config_flow_style(rc, &config);
	pass &= test_save_sequence_config_block_style(rc, &config);
	pass &= test_save_schema_top_level_sequence_fixed(rc, &config);

	return pass;
}
