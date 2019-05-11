/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2017-2019 Michael Drake <tlsa@netsurf-browser.org>
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
	cyaml_data_t **data;
	unsigned *seq_count;
	const struct cyaml_config *config;
	const struct cyaml_schema_value *schema;
} test_data_t;

/**
 * Common clean up function to free data loaded by tests.
 *
 * \param[in]  data  The unit test context data.
 */
static void cyaml_cleanup(void *data)
{
	struct test_data *td = data;
	unsigned seq_count = 0;

	if (td->seq_count != NULL) {
		seq_count = *(td->seq_count);
	}

	if (td->data != NULL) {
		cyaml_free(td->config, td->schema, *(td->data), seq_count);
	}
}

/**
 * Test loading a positive signed integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_int_pos(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int value = 90;
	static const unsigned char yaml[] =
		"test_int: 90\n";
	struct target_struct {
		int test_value_int;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("test_int", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_int),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_int != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a negative signed integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_int_neg(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int value = -77;
	static const unsigned char yaml[] =
		"test_int: -77\n";
	struct target_struct {
		int test_value_int;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("test_int", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_int),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_int != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to a positive signed integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_int_pos_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int value = 90;
	static const unsigned char yaml[] =
		"test_int: 90\n";
	struct target_struct {
		int *test_value_int;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT_PTR("test_int", CYAML_FLAG_POINTER,
				struct target_struct, test_value_int),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_int != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to a  negative signed integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_int_neg_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int value = -77;
	static const unsigned char yaml[] =
		"test_int: -77\n";
	struct target_struct {
		int *test_value_int;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT_PTR("test_int", CYAML_FLAG_POINTER,
				struct target_struct, test_value_int),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_int != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an unsigned integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_uint(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned value = 9999;
	static const unsigned char yaml[] =
		"test_uint: 9999\n";
	struct target_struct {
		unsigned test_value_uint;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT("test_uint", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_uint),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_uint != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to an unsigned integer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_uint_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned value = 9999;
	static const unsigned char yaml[] =
		"test_uint: 9999\n";
	struct target_struct {
		unsigned *test_value_uint;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_UINT_PTR("test_uint", CYAML_FLAG_POINTER,
				struct target_struct, test_value_uint),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_uint != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a floating point value as a float.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_float(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	float value = 3.14159;
	static const unsigned char yaml[] =
		"test_fp: 3.14159\n";
	struct target_struct {
		float test_value_fp;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("test_fp", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_fp),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_fp != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %f, got: %f",
				value, data_tgt->test_value_fp);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a floating point value as a pointer to float.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_float_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	float value = 3.14159;
	static const unsigned char yaml[] =
		"test_fp: 3.14159\n";
	struct target_struct {
		float *test_value_fp;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT_PTR("test_fp", CYAML_FLAG_POINTER,
				struct target_struct, test_value_fp),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_fp != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %lf, got: %lf",
				value, data_tgt->test_value_fp);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a floating point value as a double.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_double(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	double value = 3.14159;
	static const unsigned char yaml[] =
		"test_fp: 3.14159\n";
	struct target_struct {
		double test_value_fp;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT("test_fp", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_fp),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_fp != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %lf, got: %lf",
				value, data_tgt->test_value_fp);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a floating point value as a pointer to double.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_double_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	double value = 3.14159;
	static const unsigned char yaml[] =
		"test_fp: 3.14159\n";
	struct target_struct {
		double *test_value_fp;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLOAT_PTR("test_fp", CYAML_FLAG_POINTER,
				struct target_struct, test_value_fp),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_fp != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: %lf, got: %lf",
				value, data_tgt->test_value_fp);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a boolean value (true).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_bool_true(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	bool value = true;
	static const unsigned char yaml[] =
		"test_bool: true\n";
	struct target_struct {
		unsigned test_value_bool;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BOOL("test_bool", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bool),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_bool != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a boolean value (false).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_bool_false(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	bool value = false;
	static const unsigned char yaml[] =
		"test_bool: false\n";
	struct target_struct {
		unsigned test_value_bool;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BOOL("test_bool", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bool),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_bool != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to a boolean value (true).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_bool_true_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	bool value = true;
	static const unsigned char yaml[] =
		"test_bool: true\n";
	struct target_struct {
		unsigned *test_value_bool;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BOOL_PTR("test_bool", CYAML_FLAG_POINTER,
				struct target_struct, test_value_bool),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_bool != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to a boolean value (false).
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_bool_false_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	bool value = false;
	static const unsigned char yaml[] =
		"test_bool: false\n";
	struct target_struct {
		unsigned *test_value_bool;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BOOL_PTR("test_bool", CYAML_FLAG_POINTER,
				struct target_struct, test_value_bool),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_bool != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an enumeration.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_enum(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD,
		TEST_ENUM__COUNT,
	} value = TEST_ENUM_SECOND;
	static const cyaml_strval_t strings[TEST_ENUM__COUNT] = {
		[TEST_ENUM_FIRST]  = { "first",  0 },
		[TEST_ENUM_SECOND] = { "second", 1 },
		[TEST_ENUM_THIRD]  = { "third",  2 },
	};
	static const unsigned char yaml[] =
		"test_enum: second\n";
	struct target_struct {
		enum test_enum test_value_enum;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_ENUM("test_enum", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_enum,
				strings, TEST_ENUM__COUNT),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_enum != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to an enumeration.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_enum_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD,
		TEST_ENUM__COUNT,
	} value = TEST_ENUM_SECOND;
	static const cyaml_strval_t strings[TEST_ENUM__COUNT] = {
		[TEST_ENUM_FIRST]  = { "first",  0 },
		[TEST_ENUM_SECOND] = { "second", 1 },
		[TEST_ENUM_THIRD]  = { "third",  2 },
	};
	static const unsigned char yaml[] =
		"test_enum: second\n";
	struct target_struct {
		enum test_enum *test_value_enum;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_ENUM_PTR("test_enum", CYAML_FLAG_POINTER,
				struct target_struct, test_value_enum,
				strings, TEST_ENUM__COUNT),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_enum != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sparse enumeration.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_enum_sparse(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST  = 3,
		TEST_ENUM_SECOND = 77,
		TEST_ENUM_THIRD  = 183,
		TEST_ENUM_FOURTH = 9900,
	} value = TEST_ENUM_SECOND;
	static const cyaml_strval_t strings[] = {
		{ "first",  TEST_ENUM_FIRST },
		{ "second", TEST_ENUM_SECOND },
		{ "third",  TEST_ENUM_THIRD },
		{ "fourth", TEST_ENUM_FOURTH },
	};
	static const unsigned char yaml[] =
		"test_enum: second\n";
	struct target_struct {
		enum test_enum test_value_enum;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_ENUM("test_enum", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_enum,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_enum != value) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a string to a character array.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *value = "Hello World!";
	static const unsigned char yaml[] =
		"test_string: Hello World!\n";
	struct target_struct {
		char test_value_string[50];
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING("test_string", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_string, 0),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (strcmp(data_tgt->test_value_string, value) != 0) {
		fprintf(stderr, "expected: %s\n", value);
		for (unsigned i = 0; i < strlen(value) + 1; i++) {
			fprintf(stderr, "%2.2x ", value[i]);
		}
		fprintf(stderr, "\n");
		fprintf(stderr, "     got: %s\n", data_tgt->test_value_string);
		for (unsigned i = 0; i < sizeof(data_tgt->test_value_string); i++) {
			fprintf(stderr, "%2.2x ", value[i]);
		}
		fprintf(stderr, "\n");
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a string to a allocated char pointer.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_string_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *value = "Hello World!";
	static const unsigned char yaml[] =
		"test_string: Hello World!\n";
	struct target_struct {
		char * test_value_string;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("test_string", CYAML_FLAG_POINTER,
				struct target_struct, test_value_string,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (strcmp(data_tgt->test_value_string, value) != 0) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an ignored value with descendants.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_ignore_deep(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"ignore:\n"
		"    foo: bar\n"
		"    bar:\n"
		"      - 1\n"
		"      - 2\n";
	struct target_struct {
		bool foo;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_IGNORE("ignore", CYAML_FLAG_DEFAULT),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->foo != false) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an ignored value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_ignore_scalar(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"ignore: foo\n";
	struct target_struct {
		bool foo;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_IGNORE("ignore", CYAML_FLAG_DEFAULT),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->foo != false) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a flag word.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_flags(
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
	} value = TEST_FLAGS_SECOND | TEST_FLAGS_FIFTH | 1024;
	static const cyaml_strval_t strings[] = {
		{ "none",   TEST_FLAGS_NONE },
		{ "first",  TEST_FLAGS_FIRST },
		{ "second", TEST_FLAGS_SECOND },
		{ "third",  TEST_FLAGS_THIRD },
		{ "fourth", TEST_FLAGS_FOURTH },
		{ "fifth",  TEST_FLAGS_FIFTH },
		{ "sixth",  TEST_FLAGS_SIXTH },
	};
	static const unsigned char yaml[] =
		"test_flags:\n"
		"    - second\n"
		"    - fifth\n"
		"    - 1024\n";
	struct target_struct {
		enum test_flags test_value_flags;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("test_flags", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_flags,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_flags != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: 0x%x, got: 0x%x\n",
				value, data_tgt->test_value_flags);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to a flag word.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_flags_ptr(
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
	} value = TEST_FLAGS_SECOND | TEST_FLAGS_FIFTH | 1024;
	static const cyaml_strval_t strings[] = {
		{ "none",   TEST_FLAGS_NONE },
		{ "first",  TEST_FLAGS_FIRST },
		{ "second", TEST_FLAGS_SECOND },
		{ "third",  TEST_FLAGS_THIRD },
		{ "fourth", TEST_FLAGS_FOURTH },
		{ "fifth",  TEST_FLAGS_FIFTH },
		{ "sixth",  TEST_FLAGS_SIXTH },
	};
	static const unsigned char yaml[] =
		"test_flags:\n"
		"    - second\n"
		"    - fifth\n"
		"    - 1024\n";
	struct target_struct {
		enum test_flags *test_value_flags;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS_PTR("test_flags", CYAML_FLAG_POINTER,
				struct target_struct, test_value_flags,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_flags != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: 0x%x, got: 0x%x\n",
				value, data_tgt->test_value_flags);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an empty flag word.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_flags_empty(
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
	} value = TEST_FLAGS_NONE;
	static const cyaml_strval_t strings[] = {
		{ "none",   TEST_FLAGS_NONE },
		{ "first",  TEST_FLAGS_FIRST },
		{ "second", TEST_FLAGS_SECOND },
		{ "third",  TEST_FLAGS_THIRD },
		{ "fourth", TEST_FLAGS_FOURTH },
		{ "fifth",  TEST_FLAGS_FIFTH },
		{ "sixth",  TEST_FLAGS_SIXTH },
	};
	static const unsigned char yaml[] =
		"test_flags: []\n";
	struct target_struct {
		enum test_flags test_value_flags;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("test_flags", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_flags,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_flags != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: 0x%x, got: 0x%x\n",
				value, data_tgt->test_value_flags);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sparse flag word.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_flags_sparse(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_flags {
		TEST_FLAGS_NONE   = 0,
		TEST_FLAGS_FIRST  = (1 <<  0),
		TEST_FLAGS_SECOND = (1 <<  4),
		TEST_FLAGS_THIRD  = (1 <<  7),
		TEST_FLAGS_FOURTH = (1 << 11),
		TEST_FLAGS_FIFTH  = (1 << 14),
		TEST_FLAGS_SIXTH  = (1 << 20),
	} value = TEST_FLAGS_SECOND | TEST_FLAGS_FIFTH;
	static const cyaml_strval_t strings[] = {
		{ "none",   TEST_FLAGS_NONE },
		{ "first",  TEST_FLAGS_FIRST },
		{ "second", TEST_FLAGS_SECOND },
		{ "third",  TEST_FLAGS_THIRD },
		{ "fourth", TEST_FLAGS_FOURTH },
		{ "fifth",  TEST_FLAGS_FIFTH },
		{ "sixth",  TEST_FLAGS_SIXTH },
	};
	static const unsigned char yaml[] =
		"test_flags:\n"
		"    - second\n"
		"    - fifth\n";
	struct target_struct {
		enum test_flags test_value_flags;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_FLAGS("test_flags", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_flags,
				strings, CYAML_ARRAY_LEN(strings)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_flags != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: 0x%x, got: 0x%x\n",
				value, data_tgt->test_value_flags);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a bitfield.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_bitfield(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	uint64_t value = 0xFFFFFFFFFFFFFFFFu;
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
		{ .name = "b", .offset =  3, .bits =  7 },
		{ .name = "c", .offset = 10, .bits = 32 },
		{ .name = "d", .offset = 42, .bits =  8 },
		{ .name = "e", .offset = 50, .bits = 14 },
	};
	static const unsigned char yaml[] =
		"test_bitfield:\n"
		"    a: 0x7\n"
		"    b: 0x7f\n"
		"    c: 0xffffffff\n"
		"    d: 0xff\n"
		"    e: 0x3fff\n";
	struct target_struct {
		uint64_t test_value_bitfield;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BITFIELD("test_bitfield", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_bitfield,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->test_value_bitfield != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: 0x%x, got: 0x%x\n",
				value, data_tgt->test_value_bitfield);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a pointer to a bitfield.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_bitfield_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	uint64_t value = 0xFFFFFFFFFFFFFFFFu;
	static const cyaml_bitdef_t bitvals[] = {
		{ .name = "a", .offset =  0, .bits =  3 },
		{ .name = "b", .offset =  3, .bits =  7 },
		{ .name = "c", .offset = 10, .bits = 32 },
		{ .name = "d", .offset = 42, .bits =  8 },
		{ .name = "e", .offset = 50, .bits = 14 },
	};
	static const unsigned char yaml[] =
		"test_bitfield:\n"
		"    a: 0x7\n"
		"    b: 0x7f\n"
		"    c: 0xffffffff\n"
		"    d: 0xff\n"
		"    e: 0x3fff\n";
	struct target_struct {
		uint64_t *test_value_bitfield;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_BITFIELD_PTR("test_bitfield", CYAML_FLAG_POINTER,
				struct target_struct, test_value_bitfield,
				bitvals, CYAML_ARRAY_LEN(bitvals)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt->test_value_bitfield != value) {
		return ttest_fail(&tc, "Incorrect value: "
				"expected: 0x%x, got: 0x%x\n",
				value, data_tgt->test_value_bitfield);
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping, to an internal structure.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_mapping(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	} value;
	static const unsigned char yaml[] =
		"mapping:\n"
		"    a: 123\n"
		"    b: 9999\n";
	struct target_struct {
		struct value_s test_value_mapping;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT, struct value_s, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT, struct value_s, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_MAPPING("mapping", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_mapping,
				test_mapping_schema),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	memset(&value, 0, sizeof(value));
	value.a = 123;
	value.b = 9999;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (memcmp(&data_tgt->test_value_mapping, &value, sizeof(value)) != 0) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping, to an allocated structure.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_mapping_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	} value;
	static const unsigned char yaml[] =
		"mapping:\n"
		"    a: 123\n"
		"    b: 9999\n";
	struct target_struct {
		struct value_s *test_value_mapping;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field test_mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT, struct value_s, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT, struct value_s, b),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_MAPPING_PTR("mapping", CYAML_FLAG_POINTER,
				struct target_struct, test_value_mapping,
				test_mapping_schema),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	memset(&value, 0, sizeof(value));
	value.a = 123;
	value.b = 9999;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (memcmp(data_tgt->test_value_mapping, &value, sizeof(value)) != 0) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers into an int[].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of enums into an enum test_enum[].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_enum(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD,
		TEST_ENUM__COUNT,
	} ref[] = { TEST_ENUM_FIRST, TEST_ENUM_SECOND, TEST_ENUM_THIRD };
	static const cyaml_strval_t strings[TEST_ENUM__COUNT] = {
		[TEST_ENUM_FIRST]  = { "first",  0 },
		[TEST_ENUM_SECOND] = { "second", 1 },
		[TEST_ENUM_THIRD]  = { "third",  2 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - first\n"
		"    - second\n"
		"    - third\n";
	struct target_struct {
		enum test_enum seq[CYAML_ARRAY_LEN(ref)];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_ENUM(CYAML_FLAG_DEFAULT,
				*(data_tgt->seq), strings, TEST_ENUM__COUNT),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of unsigned integers into an unsigned[].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_uint(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned ref[] = { 99999, 99998, 99997, 99996, 99995, 99994 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 99999\n"
		"    - 99998\n"
		"    - 99997\n"
		"    - 99996\n"
		"    - 99995\n"
		"    - 99994\n";
	struct target_struct {
		unsigned seq[CYAML_ARRAY_LEN(ref)];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of boolean values into an bool[].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_bool(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	bool ref[] = { true, false, true, false, true, false, true, false };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - true\n"
		"    - false\n"
		"    - yes\n"
		"    - no\n"
		"    - enable\n"
		"    - disable\n"
		"    - 1\n"
		"    - 0\n";
	struct target_struct {
		bool seq[CYAML_ARRAY_LEN(ref)];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_BOOL(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of flag sequences into an array of flag words.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_flags(
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
	} ref[3] = {
		TEST_FLAGS_SECOND | TEST_FLAGS_FIFTH | 1024,
		TEST_FLAGS_FIRST,
		TEST_FLAGS_FOURTH | TEST_FLAGS_SIXTH
	};
	#define TEST_FLAGS__COUNT 6
	static const cyaml_strval_t strings[TEST_FLAGS__COUNT] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
		{ "fifth",  (1 << 4) },
		{ "sixth",  (1 << 5) },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - - second\n"
		"      - fifth\n"
		"      - 1024\n"
		"    - - first\n"
		"    - - fourth\n"
		"      - sixth\n";
	struct target_struct {
		enum test_flags seq[3];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_FLAGS(CYAML_FLAG_DEFAULT, *(data_tgt->seq),
				strings, TEST_FLAGS__COUNT),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_DEFAULT,
				struct target_struct, seq, &entry_schema,
				0, CYAML_ARRAY_LEN(data_tgt->seq)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of strings into an array of char[7].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *ref[] = {
		"This",
		"is",
		"merely",
		"a",
		"test",
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - This\n"
		"    - is\n"
		"    - merely\n"
		"    - a\n"
		"    - test\n";
	struct target_struct {
		char seq[5][7];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_DEFAULT, *(data_tgt->seq), 0, 6),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (strcmp(data_tgt->seq[i], ref[i]) != 0) {
			return ttest_fail(&tc, "Incorrect value (i=%u)", i);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of strings into an array of allocated strings.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_string_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *ref[] = {
		"This",
		"is",
		"merely",
		"a",
		"test",
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - This\n"
		"    - is\n"
		"    - merely\n"
		"    - a\n"
		"    - test\n";
	struct target_struct {
		char *seq[5];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, *(data_tgt->seq),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (strcmp(data_tgt->seq[i], ref[i]) != 0) {
			return ttest_fail(&tc, "Incorrect value (i=%u)", i);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of mappings into an array of structures.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_mapping(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	} ref[3];
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - a: 123\n"
		"      b: 9999\n"
		"    - a: 4000\n"
		"      b: 62000\n"
		"    - a: 1\n"
		"      b: 765\n";
	struct target_struct {
		struct value_s seq[3];
		uint32_t seq_count;
	} *data_tgt = NULL;
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
				0, CYAML_ARRAY_LEN(data_tgt->seq)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	memset(ref, 0, sizeof(ref));
	ref[0].a = 123;
	ref[0].b = 9999;
	ref[1].a = 4000;
	ref[1].b = 62000;
	ref[2].a = 1;
	ref[2].b = 765;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	if (memcmp(data_tgt->seq, ref, sizeof(ref)) != 0) {
		return ttest_fail(&tc, "Incorrect value");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of mappings into an array of pointers to structs.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_mapping_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	} ref[3] = {
		{ .a =  123, .b =  9999, },
		{ .a = 4000, .b = 62000, },
		{ .a =    1, .b =   765, },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - a: 123\n"
		"      b: 9999\n"
		"    - a: 4000\n"
		"      b: 62000\n"
		"    - a: 1\n"
		"      b: 765\n";
	struct target_struct {
		struct value_s *seq[3];
		uint32_t seq_count;
	} *data_tgt = NULL;
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
				0, CYAML_ARRAY_LEN(data_tgt->seq)),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (ref[i].a != data_tgt->seq[i]->a) {
			return ttest_fail(&tc, "Incorrect value");
		}
		if (ref[i].b != data_tgt->seq[i]->b) {
			return ttest_fail(&tc, "Incorrect value");
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of sequences of int into int[4][3].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_sequence_fixed_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[4][3] = {
		{  1,  2,  3 },
		{  4,  5,  6 },
		{  7,  8,  9 },
		{ 10, 11, 12 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - [  1,  2,  3 ]\n"
		"    - [  4,  5,  6 ]\n"
		"    - [  7,  8,  9 ]\n"
		"    - [ 10, 11, 12 ]\n";
	struct target_struct {
		int seq[4][3];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, **(data_tgt->seq)),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_DEFAULT, **(data_tgt->seq),
				&entry_schema_int, CYAML_ARRAY_LEN(*ref)),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count: "
				"expected %u, got %u",
				CYAML_ARRAY_LEN(ref), data_tgt->seq_count);
	}

	for (unsigned j = 0; j < CYAML_ARRAY_LEN(ref); j++) {
		for (unsigned i = 0; i < CYAML_ARRAY_LEN(*ref); i++) {
			if (data_tgt->seq[j][i] != ref[j][i]) {
				return ttest_fail(&tc,
						"Incorrect value "
						"(i=%u, j=%u): "
						"got: %i, expected: %i", i, j,
						data_tgt->seq[j][i], ref[j][i]);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of sequences of int into int*[4].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_sequence_fixed_ptr_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[4][3] = {
		{  1,  2,  3 },
		{  4,  5,  6 },
		{  7,  8,  9 },
		{ 10, 11, 12 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - [  1,  2,  3 ]\n"
		"    - [  4,  5,  6 ]\n"
		"    - [  7,  8,  9 ]\n"
		"    - [ 10, 11, 12 ]\n";
	struct target_struct {
		int *seq[4];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, **(data_tgt->seq)),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_POINTER, **(data_tgt->seq),
				&entry_schema_int, CYAML_ARRAY_LEN(*ref)),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count: "
				"expected %u, got %u",
				CYAML_ARRAY_LEN(ref), data_tgt->seq_count);
	}

	for (unsigned j = 0; j < CYAML_ARRAY_LEN(ref); j++) {
		for (unsigned i = 0; i < CYAML_ARRAY_LEN(*ref); i++) {
			if (data_tgt->seq[j][i] != ref[j][i]) {
				return ttest_fail(&tc,
						"Incorrect value "
						"(i=%u, j=%u): "
						"got: %i, expected: %i", i, j,
						data_tgt->seq[j][i], ref[j][i]);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of sequences of int into one-dimensional int[].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_sequence_fixed_flat_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[4][3] = {
		{  1,  2,  3 },
		{  4,  5,  6 },
		{  7,  8,  9 },
		{ 10, 11, 12 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - [  1,  2,  3 ]\n"
		"    - [  4,  5,  6 ]\n"
		"    - [  7,  8,  9 ]\n"
		"    - [ 10, 11, 12 ]\n";
	struct target_struct {
		int seq[12];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_DEFAULT, int,
				&entry_schema_int, CYAML_ARRAY_LEN(*ref)),
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
			.count_size = sizeof(data_tgt->seq_count),
			.count_offset = offsetof(struct target_struct, seq_count),
		},
		CYAML_FIELD_END,
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	/* Note: count is count of entries of the outer sequence entries,
	 * so, 4, not 12. */
	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count: "
				"expected %u, got %u",
				CYAML_ARRAY_LEN(ref), data_tgt->seq_count);
	}

	for (unsigned j = 0; j < CYAML_ARRAY_LEN(ref); j++) {
		for (unsigned i = 0; i < CYAML_ARRAY_LEN(*ref); i++) {
			if (data_tgt->seq[j * CYAML_ARRAY_LEN(*ref) + i] != ref[j][i]) {
				return ttest_fail(&tc,
						"Incorrect value "
						"(i=%u, j=%u): "
						"got: %i, expected: %i", i, j,
						data_tgt->seq[j * CYAML_ARRAY_LEN(*ref) + i],
						ref[j][i]);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers to allocated int* array.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int *seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of enums to allocated enum test_enum* array.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_enum(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD,
		TEST_ENUM__COUNT,
	} ref[] = { TEST_ENUM_FIRST, TEST_ENUM_SECOND, TEST_ENUM_THIRD };
	static const cyaml_strval_t strings[TEST_ENUM__COUNT] = {
		[TEST_ENUM_FIRST]  = { "first",  0 },
		[TEST_ENUM_SECOND] = { "second", 1 },
		[TEST_ENUM_THIRD]  = { "third",  2 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - first\n"
		"    - second\n"
		"    - third\n";
	struct target_struct {
		enum test_enum *seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_ENUM(CYAML_FLAG_DEFAULT,
				*(data_tgt->seq), strings, TEST_ENUM__COUNT),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of unsigned integers to allocated unsigned* array.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_uint(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned ref[] = { 99999, 99998, 99997, 99996, 99995, 99994 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 99999\n"
		"    - 99998\n"
		"    - 99997\n"
		"    - 99996\n"
		"    - 99995\n"
		"    - 99994\n";
	struct target_struct {
		unsigned *seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of boolean values to allocated bool* array.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_bool(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	bool ref[] = { true, false, true, false, true, false, true, false };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - true\n"
		"    - false\n"
		"    - yes\n"
		"    - no\n"
		"    - enable\n"
		"    - disable\n"
		"    - 1\n"
		"    - 0\n";
	struct target_struct {
		bool *seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_BOOL(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of flag sequences to allocated flag words array.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_flags(
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
	} ref[3] = {
		TEST_FLAGS_SECOND | TEST_FLAGS_FIFTH | 1024,
		TEST_FLAGS_FIRST,
		TEST_FLAGS_FOURTH | TEST_FLAGS_SIXTH
	};
	#define TEST_FLAGS__COUNT 6
	static const cyaml_strval_t strings[TEST_FLAGS__COUNT] = {
		{ "first",  (1 << 0) },
		{ "second", (1 << 1) },
		{ "third",  (1 << 2) },
		{ "fourth", (1 << 3) },
		{ "fifth",  (1 << 4) },
		{ "sixth",  (1 << 5) },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - - second\n"
		"      - fifth\n"
		"      - 1024\n"
		"    - - first\n"
		"    - - fourth\n"
		"      - sixth\n";
	struct target_struct {
		enum test_flags *seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_FLAGS(CYAML_FLAG_DEFAULT, *(data_tgt->seq),
				strings, TEST_FLAGS__COUNT),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of strings to allocated array of char[7].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_string(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *ref[] = {
		"This",
		"is",
		"merely",
		"a",
		"test",
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - This\n"
		"    - is\n"
		"    - merely\n"
		"    - a\n"
		"    - test\n";
	struct target_struct {
		char (*seq)[7];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_DEFAULT, *(data_tgt->seq), 0, 6),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (strcmp(data_tgt->seq[i], ref[i]) != 0) {
			return ttest_fail(&tc, "Incorrect value (i=%u)", i);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of strings to allocated array of
 * allocated strings.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_string_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	const char *ref[] = {
		"This",
		"is",
		"merely",
		"a",
		"test",
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - This\n"
		"    - is\n"
		"    - merely\n"
		"    - a\n"
		"    - test\n";
	struct target_struct {
		char **seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, *(data_tgt->seq),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (strcmp(data_tgt->seq[i], ref[i]) != 0) {
			return ttest_fail(&tc, "Incorrect value (i=%u)", i);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of mappings to allocated array mapping structs.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_mapping(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	} ref[3] = {
		{ .a =  123, .b =  9999, },
		{ .a = 4000, .b = 62000, },
		{ .a =    1, .b =   765, },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - a: 123\n"
		"      b: 9999\n"
		"    - a: 4000\n"
		"      b: 62000\n"
		"    - a: 1\n"
		"      b: 765\n";
	struct target_struct {
		struct value_s *seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (ref[i].a != data_tgt->seq[i].a) {
			return ttest_fail(&tc, "Incorrect value");
		}
		if (ref[i].b != data_tgt->seq[i].b) {
			return ttest_fail(&tc, "Incorrect value");
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of mappings to allocated array of pointers to
 * mapping structs.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_mapping_ptr(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct value_s {
		short a;
		long b;
	} ref[3] = {
		{ .a =  123, .b =  9999, },
		{ .a = 4000, .b = 62000, },
		{ .a =    1, .b =   765, },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - a: 123\n"
		"      b: 9999\n"
		"    - a: 4000\n"
		"      b: 62000\n"
		"    - a: 1\n"
		"      b: 765\n";
	struct target_struct {
		struct value_s **seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (ref[i].a != data_tgt->seq[i]->a) {
			return ttest_fail(&tc, "Incorrect value");
		}
		if (ref[i].b != data_tgt->seq[i]->b) {
			return ttest_fail(&tc, "Incorrect value");
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of sequences of integers to allocated array
 * of int[3].
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_sequence_fixed_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[4][3] = {
		{  1,  2,  3 },
		{  4,  5,  6 },
		{  7,  8,  9 },
		{ 10, 11, 12 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - [  1,  2,  3 ]\n"
		"    - [  4,  5,  6 ]\n"
		"    - [  7,  8,  9 ]\n"
		"    - [ 10, 11, 12 ]\n";
	struct target_struct {
		int (*seq)[3];
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_DEFAULT, int,
				&entry_schema_int, CYAML_ARRAY_LEN(*ref)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_END,
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count: "
				"expected %u, got %u",
				CYAML_ARRAY_LEN(ref), data_tgt->seq_count);
	}

	for (unsigned j = 0; j < CYAML_ARRAY_LEN(ref); j++) {
		for (unsigned i = 0; i < CYAML_ARRAY_LEN(*ref); i++) {
			if (data_tgt->seq[j][i] != ref[j][i]) {
				return ttest_fail(&tc,
						"Incorrect value "
						"(i=%u, j=%u): "
						"got: %i, expected: %i", i, j,
						data_tgt->seq[j][i], ref[j][i]);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of sequences of integers to allocated array
 * of allocated arrays of integers.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_sequence_fixed_ptr_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[4][3] = {
		{  1,  2,  3 },
		{  4,  5,  6 },
		{  7,  8,  9 },
		{ 10, 11, 12 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - [  1,  2,  3 ]\n"
		"    - [  4,  5,  6 ]\n"
		"    - [  7,  8,  9 ]\n"
		"    - [ 10, 11, 12 ]\n";
	struct target_struct {
		int **seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_POINTER, int,
				&entry_schema_int, CYAML_ARRAY_LEN(*ref)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("sequence", CYAML_FLAG_POINTER,
				struct target_struct, seq, &entry_schema,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_END,
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count: "
				"expected %u, got %u",
				CYAML_ARRAY_LEN(ref), data_tgt->seq_count);
	}

	for (unsigned j = 0; j < CYAML_ARRAY_LEN(ref); j++) {
		for (unsigned i = 0; i < CYAML_ARRAY_LEN(*ref); i++) {
			if (data_tgt->seq[j][i] != ref[j][i]) {
				return ttest_fail(&tc,
						"Incorrect value "
						"(i=%u, j=%u): "
						"got: %i, expected: %i", i, j,
						data_tgt->seq[j][i], ref[j][i]);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of sequences of integers a one-dimensional allocated
 * array of integers.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_ptr_sequence_fixed_flat_int(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[4][3] = {
		{  1,  2,  3 },
		{  4,  5,  6 },
		{  7,  8,  9 },
		{ 10, 11, 12 },
	};
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - [  1,  2,  3 ]\n"
		"    - [  4,  5,  6 ]\n"
		"    - [  7,  8,  9 ]\n"
		"    - [ 10, 11, 12 ]\n";
	struct target_struct {
		int *seq;
		uint32_t seq_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema_int = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(
				CYAML_FLAG_DEFAULT, int,
				&entry_schema_int, CYAML_ARRAY_LEN(*ref)),
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
			.count_size = sizeof(data_tgt->seq_count),
			.count_offset = offsetof(struct target_struct, seq_count),
		},
		CYAML_FIELD_END,
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	/* Note: count is count of entries of the outer sequence entries,
	 * so, 4, not 12. */
	if (CYAML_ARRAY_LEN(ref) != data_tgt->seq_count) {
		return ttest_fail(&tc, "Incorrect sequence count: "
				"expected %u, got %u",
				CYAML_ARRAY_LEN(ref), data_tgt->seq_count);
	}

	for (unsigned j = 0; j < CYAML_ARRAY_LEN(ref); j++) {
		for (unsigned i = 0; i < CYAML_ARRAY_LEN(*ref); i++) {
			if (data_tgt->seq[j * CYAML_ARRAY_LEN(*ref) + i] != ref[j][i]) {
				return ttest_fail(&tc,
						"Incorrect value "
						"(i=%u, j=%u): "
						"got: %i, expected: %i", i, j,
						data_tgt->seq[j * CYAML_ARRAY_LEN(*ref) + i],
						ref[j][i]);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 1 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[1];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 2 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[2];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 3 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_3(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[3];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 4 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_4(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[4];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 5 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_5(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[5];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 6 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_6(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[6];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 7 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_7(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[7];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence of integers with 8 byte sequence count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_entry_sequence_count_8(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	int ref[] = { 1, 1, 2, 3, 5, 8 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 1\n"
		"    - 1\n"
		"    - 2\n"
		"    - 3\n"
		"    - 5\n"
		"    - 8\n";
	struct target_struct {
		int seq[CYAML_ARRAY_LEN(ref)];
		uint8_t seq_count[8];
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, *(data_tgt->seq)),
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
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != cyaml_data_read(sizeof(data_tgt->seq_count),
			data_tgt->seq_count, &err)) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->seq[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->seq[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with scalar top level type.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_schema_top_level_scalar(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"7\n";
	int *value = NULL;
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_POINTER, int)
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &value,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &value, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (value == NULL) {
		return ttest_fail(&tc, "Data NULL on success.");
	}

	if (*value != 7) {
		return ttest_fail(&tc, "Bad value.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with sequence_fixed top level type.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_schema_top_level_sequence(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"- 7\n"
		"- 6\n"
		"- 5\n";
	int *value = NULL;
	unsigned count = 0;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE(CYAML_FLAG_POINTER, int,
				&entry_schema, 0, CYAML_UNLIMITED)
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &value,
		.seq_count = &count,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &value, &count);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (count != 3) {
		return ttest_fail(&tc, "Unexpected sequence count.");
	}

	if (value == NULL) {
		return ttest_fail(&tc, "Data NULL on success.");
	}

	if ((value[0] != 7) && (value[1] != 6) && (value[2] != 5)) {
		return ttest_fail(&tc, "Bad value.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading with schema with sequence_fixed top level type.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_schema_top_level_sequence_fixed(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const unsigned char yaml[] =
		"- 7\n"
		"- 6\n"
		"- 5\n";
	int *value = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int)
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_SEQUENCE_FIXED(CYAML_FLAG_POINTER, int,
				&entry_schema, 3)
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &value,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &value, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (value == NULL) {
		return ttest_fail(&tc, "Data NULL on success.");
	}

	if ((value[0] != 7) && (value[1] != 6) && (value[2] != 5)) {
		return ttest_fail(&tc, "Bad value.");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a stream with more than one document.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_multiple_documents_ignored(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		signed char a;
	} data = {
		.a = 9,
	};
	static const unsigned char yaml[] =
		"a: 9\n"
		"---\n"
		"b: foo\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->a != data.a) {
		return ttest_fail(&tc, "Incorrect value for entry a");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping multiple fields.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_with_multiple_fields(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		signed char a;
		short b;
		int c;
		long d;
		long long e;
	} data = {
		.a = 9,
		.b = 90,
		.c = 900,
		.d = 9000,
		.e = 90000,
	};
	static const unsigned char yaml[] =
		"a: 9\n"
		"b: 90\n"
		"c: 900\n"
		"d: 9000\n"
		"e: 90000\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("a", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_INT("c", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_INT("d", CYAML_FLAG_DEFAULT,
				struct target_struct, d),
		CYAML_FIELD_INT("e", CYAML_FLAG_DEFAULT,
				struct target_struct, e),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->a != data.a) {
		return ttest_fail(&tc, "Incorrect value for entry a");
	}
	if (data_tgt->b != data.b) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	if (data_tgt->d != data.d) {
		return ttest_fail(&tc, "Incorrect value for entry d");
	}
	if (data_tgt->e != data.e) {
		return ttest_fail(&tc, "Incorrect value for entry e");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with optional fields.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_with_optional_fields(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	long values[] = { 4, 3, 2, 1 };
	struct target_struct {
		char *a;
		char b[10];
		int c;
		long d[4];
		long *e;
		unsigned e_count;
		char *f;
		char *g;
		char h[10];
		int i;
		long j[4];
		long *k;
		unsigned k_count;
	} data = {
		.a = "Hello",
		.b = "World!",
		.c = 0,
		.d = { 0, 0, 0, 0 },
		.e = values,
		.f = "Required!",
		.g = NULL,
		.h = "\0",
		.i = 9876,
		.j = { 1, 2, 3, 4 },
		.k = NULL,
	};
	static const unsigned char yaml[] =
		"a: Hello\n"
		"b: World!\n"
		"e: [ 4, 3, 2, 1 ]\n"
		"f: Required!\n"
		"i: 9876\n"
		"j: [ 1, 2, 3, 4 ]\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_value sequence_entry = {
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, sizeof(long)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("a",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, a, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING("b", CYAML_FLAG_OPTIONAL,
				struct target_struct, b, 0),
		CYAML_FIELD_INT("c", CYAML_FLAG_OPTIONAL,
				struct target_struct, c),
		CYAML_FIELD_SEQUENCE_FIXED("d", CYAML_FLAG_OPTIONAL,
				struct target_struct, d, &sequence_entry, 4),
		CYAML_FIELD_SEQUENCE("e",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, e, &sequence_entry,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("f",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, f, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING_PTR("g",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, g, 0, CYAML_UNLIMITED),
		CYAML_FIELD_STRING("h", CYAML_FLAG_OPTIONAL,
				struct target_struct, h, 0),
		CYAML_FIELD_INT("i", CYAML_FLAG_OPTIONAL,
				struct target_struct, i),
		CYAML_FIELD_SEQUENCE_FIXED("j", CYAML_FLAG_OPTIONAL,
				struct target_struct, j, &sequence_entry, 4),
		CYAML_FIELD_SEQUENCE("k",
				CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
				struct target_struct, k, &sequence_entry,
				0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (strcmp(data_tgt->a, data.a) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry a: "
				"Expected: %s, got: %s",
				data.a, data_tgt->a);
	}
	if (strcmp(data_tgt->b, data.b) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	for (unsigned i = 0; i < 4; i++) {
		if (data_tgt->d[i] != data.d[i]) {
			return ttest_fail(&tc, "Incorrect value for entry d");
		}
	}
	for (unsigned i = 0; i < 4; i++) {
		if (data_tgt->e[i] != data.e[i]) {
			return ttest_fail(&tc, "Incorrect value for entry e "
					"Index: %u: Expected: %ld, got: %ld",
					i, data.e[i], data_tgt->e[i]);
		}
	}
	if (strcmp(data_tgt->f, data.f) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry f: "
				"Expected: %s, got: %s",
				data.f, data_tgt->f);
	}
	if (data_tgt->g != data.g) {
		return ttest_fail(&tc, "Incorrect value for entry g: "
				"Expected: %s, got: %s",
				data.g, data_tgt->g);
	}
	if (strcmp(data_tgt->h, data.h) != 0) {
		return ttest_fail(&tc, "Incorrect value for entry h");
	}
	if (data_tgt->i != data.i) {
		return ttest_fail(&tc, "Incorrect value for entry i");
	}
	for (unsigned i = 0; i < 4; i++) {
		if (data_tgt->j[i] != data.j[i]) {
			return ttest_fail(&tc, "Incorrect value for entry j");
		}
	}
	if (data_tgt->k != data.k) {
		return ttest_fail(&tc, "Incorrect value for entry k");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with only optional fields.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_only_optional_fields(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		int c;
		int i;
	};
	static const unsigned char yaml[] =
		"\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("c", CYAML_FLAG_OPTIONAL,
				struct target_struct, c),
		CYAML_FIELD_INT("i", CYAML_FLAG_OPTIONAL,
				struct target_struct, i),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt != NULL) {
		return ttest_fail(&tc, "Shouldn't have allocated anything");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with unknown keys ignored by config.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_ignored_unknown_keys(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		short b;
		int c;
		long d;
		long long e;
	} data = {
		.b = 90,
		.c = 900,
		.d = 9000,
		.e = 90000,
	};
	static const unsigned char yaml[] =
		"a: 9\n"
		"b: 90\n"
		"c: 900\n"
		"d: 9000\n"
		"e: 90000\n"
		"f: 900000\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("b", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_INT("c", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_INT("d", CYAML_FLAG_DEFAULT,
				struct target_struct, d),
		CYAML_FIELD_INT("e", CYAML_FLAG_DEFAULT,
				struct target_struct, e),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	cfg.flags |= CYAML_CFG_IGNORE_UNKNOWN_KEYS;
	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->b != data.b) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	if (data_tgt->d != data.d) {
		return ttest_fail(&tc, "Incorrect value for entry d");
	}
	if (data_tgt->e != data.e) {
		return ttest_fail(&tc, "Incorrect value for entry e");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence with max size 4, and only 2 entries in YAML.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_sequence_without_max_entries(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		const char *seq[4];
		unsigned seq_count;
	} data = {
		.seq = { "1", "2", NULL, NULL },
		.seq_count = 2,
	};
	static const unsigned char yaml[] =
		"seq: [ 1, 2 ]\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_value sequence_entry = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, char *,
				0, CYAML_UNLIMITED)
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("seq", CYAML_FLAG_OPTIONAL,
				struct target_struct, seq, &sequence_entry,
				0, 4),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->seq_count != 2) {
		return ttest_fail(&tc, "Incorrect sequence entry count");
	}
	for (unsigned i = 0; i < 4; i++) {
		if ((data_tgt->seq[i] == NULL) != (data.seq[i] == NULL)) {
			return ttest_fail(&tc, "Incorrect value for entry %u",
					i);
		}
		if (data_tgt->seq[i] != NULL) {
			if (strcmp(data_tgt->seq[i], data.seq[i]) != 0) {
				return ttest_fail(&tc,
						"Incorrect value for entry %u",
						i);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading without a logging function.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_no_log(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_config_t cfg = *config;
	struct target_struct {
		const char *seq[4];
		unsigned seq_count;
	} data = {
		.seq = { "1", "2", NULL, NULL },
		.seq_count = 2,
	};
	static const unsigned char yaml[] =
		"seq: [ 1, 2 ]\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_value sequence_entry = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, char *,
				0, CYAML_UNLIMITED)
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("seq", CYAML_FLAG_OPTIONAL,
				struct target_struct, seq, &sequence_entry,
				0, 4),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	cfg.log_fn = NULL;
	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->seq_count != 2) {
		return ttest_fail(&tc, "Incorrect sequence entry count");
	}
	for (unsigned i = 0; i < 4; i++) {
		if ((data_tgt->seq[i] == NULL) != (data.seq[i] == NULL)) {
			return ttest_fail(&tc, "Incorrect value for entry %u",
					i);
		}
		if (data_tgt->seq[i] != NULL) {
			if (strcmp(data_tgt->seq[i], data.seq[i]) != 0) {
				return ttest_fail(&tc,
						"Incorrect value for entry %u",
						i);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a sequence with arbitrary C struct member name for entry count.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_schema_sequence_entry_count_member(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	unsigned ref[] = { 99999, 99998, 99997, 99996, 99995, 99994 };
	static const unsigned char yaml[] =
		"sequence:\n"
		"    - 99999\n"
		"    - 99998\n"
		"    - 99997\n"
		"    - 99996\n"
		"    - 99995\n"
		"    - 99994\n";
	struct target_struct {
		unsigned *entries;
		uint32_t n_entries;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value entry_schema = {
		CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, *(data_tgt->entries)),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE_COUNT("sequence", CYAML_FLAG_POINTER,
				struct target_struct, entries, n_entries,
				&entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (CYAML_ARRAY_LEN(ref) != data_tgt->n_entries) {
		return ttest_fail(&tc, "Incorrect sequence count");
	}

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(ref); i++) {
		if (data_tgt->entries[i] != ref[i]) {
			return ttest_fail(&tc, "Incorrect value (i=%u): "
					"got: %i, expected: %i", i,
					data_tgt->entries[i], ref[i]);
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test loading an enum with case insensitive string matching configured.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_enum_insensitive(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_ENUM_FIRST,
		TEST_ENUM_SECOND,
		TEST_ENUM_THIRD,
		TEST_ENUM__COUNT,
	} ref = TEST_ENUM_SECOND;
	static const cyaml_strval_t strings[TEST_ENUM__COUNT] = {
		{ "first",  TEST_ENUM_FIRST  },
		{ "second", TEST_ENUM_SECOND },
		{ "third",  TEST_ENUM_THIRD  },
	};
	static const unsigned char yaml[] =
		"SECOND\n";
	enum test_enum *data_tgt = NULL;
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_ENUM(CYAML_FLAG_POINTER,
				*data_tgt, strings, TEST_ENUM__COUNT),
	};
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	cfg.flags |= CYAML_CFG_CASE_INSENSITIVE;
	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt != ref) {
		return ttest_fail(&tc, "Incorrect value for enum");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a flags value with case insensitive string matching configured.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_flags_insensitive(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	enum test_enum {
		TEST_FLAG_FIRST  = (1 << 1),
		TEST_FLAG_SECOND = (1 << 3),
		TEST_FLAG_THIRD  = (1 << 5),
	} ref = TEST_FLAG_FIRST | TEST_FLAG_THIRD;
	static const cyaml_strval_t strings[] = {
		{ "first",  TEST_FLAG_FIRST  },
		{ "second", TEST_FLAG_SECOND },
		{ "third",  TEST_FLAG_THIRD  },
	};
	static const unsigned char yaml[] =
		"- First\n"
		"- Third\n";
	enum test_enum *data_tgt = NULL;
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_FLAGS(CYAML_FLAG_POINTER,
				*data_tgt, strings, CYAML_ARRAY_LEN(strings)),
	};
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	cfg.flags |= CYAML_CFG_CASE_INSENSITIVE;
	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (*data_tgt != ref) {
		return ttest_fail(&tc, "Incorrect value for enum");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with case insensitive string matching configured.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_fields_cfg_insensitive_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		signed char a;
		short b;
		int c;
		long d;
		long long e;
	} data = {
		.a = 9,
		.b = 90,
		.c = 900,
		.d = 9000,
		.e = 90000,
	};
	static const unsigned char yaml[] =
		"Lollipop: 9\n"
		"Squiggle: 90\n"
		"Unicorns: 900\n"
		"Cheerful: 9000\n"
		"LibCYAML: 90000\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("lollipop", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_INT("squiggle", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_INT("unicorns", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_INT("cheerful", CYAML_FLAG_DEFAULT,
				struct target_struct, d),
		CYAML_FIELD_INT("libcyaml", CYAML_FLAG_DEFAULT,
				struct target_struct, e),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	cfg.flags |= CYAML_CFG_CASE_INSENSITIVE;
	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->a != data.a) {
		return ttest_fail(&tc, "Incorrect value for entry a");
	}
	if (data_tgt->b != data.b) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	if (data_tgt->d != data.d) {
		return ttest_fail(&tc, "Incorrect value for entry d");
	}
	if (data_tgt->e != data.e) {
		return ttest_fail(&tc, "Incorrect value for entry e");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with case insensitive string matching configured.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_fields_cfg_insensitive_2(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		signed char a;
		short b;
		int c;
		long d;
		long long e;
	} data = {
		.a = 9,
		.b = 90,
		.c = 900,
		.d = 9000,
		.e = 90000,
	};
	static const unsigned char yaml[] =
		"Plinth: 9\n"
		"..Cusp?: 90\n"
		"..Cusp!: 900\n"
		"Bleat: 9000\n"
		"Foo~-|Bar: 90000\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("plinth", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_INT("..cusp?", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_INT("..cusp!", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_INT("bleat", CYAML_FLAG_DEFAULT,
				struct target_struct, d),
		CYAML_FIELD_INT("foO~-|baR", CYAML_FLAG_DEFAULT,
				struct target_struct, e),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	cfg.flags |= CYAML_CFG_CASE_INSENSITIVE;
	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->a != data.a) {
		return ttest_fail(&tc, "Incorrect value for entry a");
	}
	if (data_tgt->b != data.b) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	if (data_tgt->d != data.d) {
		return ttest_fail(&tc, "Incorrect value for entry d");
	}
	if (data_tgt->e != data.e) {
		return ttest_fail(&tc, "Incorrect value for entry e");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with case insensitive string matching configured.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_fields_cfg_insensitive_3(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		signed char a;
		short b;
		int c;
		long d;
		long long e;
	} data = {
		.a = 9,
		.b = 90,
		.c = 900,
		.d = 9000,
		.e = 90000,
	};
	static const unsigned char yaml[] =
		"Pling: 9\n"
		"Plin: 90\n"
		"Pli: 900\n"
		"Pl: 9000\n"
		"P: 90000\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("plin", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_INT("pli", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_INT("pl", CYAML_FLAG_DEFAULT,
				struct target_struct, d),
		CYAML_FIELD_INT("p", CYAML_FLAG_DEFAULT,
				struct target_struct, e),
		CYAML_FIELD_INT("pling", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	cfg.flags |= CYAML_CFG_CASE_INSENSITIVE;
	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->a != data.a) {
		return ttest_fail(&tc, "Incorrect value for entry a");
	}
	if (data_tgt->b != data.b) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	if (data_tgt->d != data.d) {
		return ttest_fail(&tc, "Incorrect value for entry d");
	}
	if (data_tgt->e != data.e) {
		return ttest_fail(&tc, "Incorrect value for entry e");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with case sensitive string matching for value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_fields_value_sensitive_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		signed char a;
		short b;
		int c;
		long d;
		long long e;
	} data = {
		.a = 9,
		.b = 90,
		.c = 900,
		.d = 9000,
		.e = 90000,
	};
	static const unsigned char yaml[] =
		"pling: 9\n"
		"PLing: 90\n"
		"PLINg: 900\n"
		"pliNG: 9000\n"
		"PLING: 90000\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("pling", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_INT("PLing", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_INT("PLINg", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_INT("pliNG", CYAML_FLAG_DEFAULT,
				struct target_struct, d),
		CYAML_FIELD_INT("PLING", CYAML_FLAG_DEFAULT,
				struct target_struct, e),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(
				CYAML_FLAG_POINTER | CYAML_FLAG_CASE_SENSITIVE,
				struct target_struct, mapping_schema),
	};
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	cfg.flags |= CYAML_CFG_CASE_INSENSITIVE;
	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->a != data.a) {
		return ttest_fail(&tc, "Incorrect value for entry a");
	}
	if (data_tgt->b != data.b) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	if (data_tgt->d != data.d) {
		return ttest_fail(&tc, "Incorrect value for entry d");
	}
	if (data_tgt->e != data.e) {
		return ttest_fail(&tc, "Incorrect value for entry e");
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a mapping with case insensitive string matching for value.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_load_mapping_fields_value_insensitive_1(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		signed char a;
		short b;
		int c;
		long d;
		long long e;
	} data = {
		.a = 9,
		.b = 90,
		.c = 900,
		.d = 9000,
		.e = 90000,
	};
	static const unsigned char yaml[] =
		"Pling: 9\n"
		"Plin: 90\n"
		"Pli: 900\n"
		"Pl: 9000\n"
		"P: 90000\n";
	struct target_struct *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("plin", CYAML_FLAG_DEFAULT,
				struct target_struct, b),
		CYAML_FIELD_INT("pli", CYAML_FLAG_DEFAULT,
				struct target_struct, c),
		CYAML_FIELD_INT("pl", CYAML_FLAG_DEFAULT,
				struct target_struct, d),
		CYAML_FIELD_INT("p", CYAML_FLAG_DEFAULT,
				struct target_struct, e),
		CYAML_FIELD_INT("pling", CYAML_FLAG_DEFAULT,
				struct target_struct, a),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(
				CYAML_FLAG_POINTER |
				CYAML_FLAG_CASE_INSENSITIVE,
				struct target_struct, mapping_schema),
	};
	cyaml_config_t cfg = *config;
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = &cfg,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	cfg.flags |= CYAML_CFG_CASE_INSENSITIVE;
	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_data(yaml, YAML_LEN(yaml), &cfg, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	if (data_tgt->a != data.a) {
		return ttest_fail(&tc, "Incorrect value for entry a");
	}
	if (data_tgt->b != data.b) {
		return ttest_fail(&tc, "Incorrect value for entry b");
	}
	if (data_tgt->c != data.c) {
		return ttest_fail(&tc, "Incorrect value for entry c");
	}
	if (data_tgt->d != data.d) {
		return ttest_fail(&tc, "Incorrect value for entry d");
	}
	if (data_tgt->e != data.e) {
		return ttest_fail(&tc, "Incorrect value for entry e");
	}

	return ttest_pass(&tc);
}

/**
 * Run the YAML loading unit tests.
 *
 * \param[in]  rc         The ttest report context.
 * \param[in]  log_level  CYAML log level.
 * \param[in]  log_fn     CYAML logging function, or NULL.
 * \return true iff all unit tests pass, otherwise false.
 */
bool load_tests(
		ttest_report_ctx_t *rc,
		cyaml_log_t log_level,
		cyaml_log_fn_t log_fn)
{
	bool pass = true;
	cyaml_config_t config = {
		.log_fn = log_fn,
		.mem_fn = cyaml_mem,
		.log_level = log_level,
		.flags = CYAML_CFG_DEFAULT,
	};

	ttest_heading(rc, "Load single entry mapping tests: simple types");

	pass &= test_load_mapping_entry_enum(rc, &config);
	pass &= test_load_mapping_entry_uint(rc, &config);
	pass &= test_load_mapping_entry_float(rc, &config);
	pass &= test_load_mapping_entry_double(rc, &config);
	pass &= test_load_mapping_entry_string(rc, &config);
	pass &= test_load_mapping_entry_int_pos(rc, &config);
	pass &= test_load_mapping_entry_int_neg(rc, &config);
	pass &= test_load_mapping_entry_enum_ptr(rc, &config);
	pass &= test_load_mapping_entry_uint_ptr(rc, &config);
	pass &= test_load_mapping_entry_float_ptr(rc, &config);
	pass &= test_load_mapping_entry_bool_true(rc, &config);
	pass &= test_load_mapping_entry_bool_false(rc, &config);
	pass &= test_load_mapping_entry_double_ptr(rc, &config);
	pass &= test_load_mapping_entry_string_ptr(rc, &config);
	pass &= test_load_mapping_entry_int_pos_ptr(rc, &config);
	pass &= test_load_mapping_entry_int_neg_ptr(rc, &config);
	pass &= test_load_mapping_entry_enum_sparse(rc, &config);
	pass &= test_load_mapping_entry_ignore_deep(rc, &config);
	pass &= test_load_mapping_entry_ignore_scalar(rc, &config);
	pass &= test_load_mapping_entry_bool_true_ptr(rc, &config);
	pass &= test_load_mapping_entry_bool_false_ptr(rc, &config);

	ttest_heading(rc, "Load single entry mapping tests: complex types");

	pass &= test_load_mapping_entry_flags(rc, &config);
	pass &= test_load_mapping_entry_mapping(rc, &config);
	pass &= test_load_mapping_entry_bitfield(rc, &config);
	pass &= test_load_mapping_entry_flags_ptr(rc, &config);
	pass &= test_load_mapping_entry_mapping_ptr(rc, &config);
	pass &= test_load_mapping_entry_flags_empty(rc, &config);
	pass &= test_load_mapping_entry_bitfield_ptr(rc, &config);
	pass &= test_load_mapping_entry_flags_sparse(rc, &config);

	ttest_heading(rc, "Load single entry mapping tests: sequences");

	pass &= test_load_mapping_entry_sequence_int(rc, &config);
	pass &= test_load_mapping_entry_sequence_enum(rc, &config);
	pass &= test_load_mapping_entry_sequence_uint(rc, &config);
	pass &= test_load_mapping_entry_sequence_bool(rc, &config);
	pass &= test_load_mapping_entry_sequence_flags(rc, &config);
	pass &= test_load_mapping_entry_sequence_string(rc, &config);
	pass &= test_load_mapping_entry_sequence_mapping(rc, &config);
	pass &= test_load_mapping_entry_sequence_string_ptr(rc, &config);
	pass &= test_load_mapping_entry_sequence_mapping_ptr(rc, &config);
	pass &= test_load_mapping_entry_sequence_sequence_fixed_int(rc, &config);
	pass &= test_load_mapping_entry_sequence_sequence_fixed_ptr_int(rc, &config);
	pass &= test_load_mapping_entry_sequence_sequence_fixed_flat_int(rc, &config);

	ttest_heading(rc, "Load single entry mapping tests: ptr sequences");

	pass &= test_load_mapping_entry_sequence_ptr_int(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_enum(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_uint(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_bool(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_flags(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_string(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_mapping(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_string_ptr(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_mapping_ptr(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_sequence_fixed_int(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_sequence_fixed_ptr_int(rc, &config);
	pass &= test_load_mapping_entry_sequence_ptr_sequence_fixed_flat_int(rc, &config);

	ttest_heading(rc, "Load tests: sequence count sizes");

	pass &= test_load_mapping_entry_sequence_count_1(rc, &config);
	pass &= test_load_mapping_entry_sequence_count_2(rc, &config);
	pass &= test_load_mapping_entry_sequence_count_3(rc, &config);
	pass &= test_load_mapping_entry_sequence_count_4(rc, &config);
	pass &= test_load_mapping_entry_sequence_count_5(rc, &config);
	pass &= test_load_mapping_entry_sequence_count_6(rc, &config);
	pass &= test_load_mapping_entry_sequence_count_7(rc, &config);
	pass &= test_load_mapping_entry_sequence_count_8(rc, &config);

	ttest_heading(rc, "Load tests: various");

	pass &= test_load_no_log(rc, &config);
	pass &= test_load_schema_top_level_scalar(rc, &config);
	pass &= test_load_schema_top_level_sequence(rc, &config);
	pass &= test_load_multiple_documents_ignored(rc, &config);
	pass &= test_load_mapping_with_multiple_fields(rc, &config);
	pass &= test_load_mapping_with_optional_fields(rc, &config);
	pass &= test_load_mapping_only_optional_fields(rc, &config);
	pass &= test_load_mapping_ignored_unknown_keys(rc, &config);
	pass &= test_load_sequence_without_max_entries(rc, &config);
	pass &= test_load_schema_top_level_sequence_fixed(rc, &config);
	pass &= test_load_schema_sequence_entry_count_member(rc, &config);

	ttest_heading(rc, "Load tests: case sensitivity");

	pass &= test_load_enum_insensitive(rc, &config);
	pass &= test_load_flags_insensitive(rc, &config);
	pass &= test_load_mapping_fields_cfg_insensitive_1(rc, &config);
	pass &= test_load_mapping_fields_cfg_insensitive_2(rc, &config);
	pass &= test_load_mapping_fields_cfg_insensitive_3(rc, &config);
	pass &= test_load_mapping_fields_value_sensitive_1(rc, &config);
	pass &= test_load_mapping_fields_value_insensitive_1(rc, &config);

	return pass;
}
