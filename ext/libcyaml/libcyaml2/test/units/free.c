/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2017-2018 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

#include <cyaml/cyaml.h>

#include "ttest.h"

/** Macro to squash unused variable compiler warnings. */
#define UNUSED(_x) ((void)(_x))

/**
 * Test cyaml_free with NULL data.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_free_null_data(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_err_t err;
	struct target_struct {
		int test_value_int;
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_INT("test_int", CYAML_FLAG_DEFAULT,
				struct target_struct, test_value_int),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	ttest_ctx_t tc = ttest_start(report, __func__, NULL, NULL);

	err = cyaml_free(config, &top_schema, NULL, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, "Free failed: %s", cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test cyaml_free with NULL config.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_free_null_config(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_err_t err;
	ttest_ctx_t tc = ttest_start(report, __func__, NULL, NULL);

	UNUSED(config);

	err = cyaml_free(NULL, NULL, NULL, 0);
	if (err != CYAML_ERR_BAD_PARAM_NULL_CONFIG) {
		return ttest_fail(&tc, "Free failed: %s", cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test cyaml_free with NULL memory allocation function.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_free_null_mem_fn(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_err_t err;
	cyaml_config_t cfg = *config;
	ttest_ctx_t tc = ttest_start(report, __func__, NULL, NULL);

	cfg.mem_fn = NULL;

	err = cyaml_free(&cfg, NULL, NULL, 0);
	if (err != CYAML_ERR_BAD_CONFIG_NULL_MEMFN) {
		return ttest_fail(&tc, "Free failed: %s", cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test cyaml_free with NULL schema.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_free_null_schema(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	cyaml_err_t err;
	ttest_ctx_t tc = ttest_start(report, __func__, NULL, NULL);

	err = cyaml_free(config, NULL, NULL, 0);
	if (err != CYAML_ERR_BAD_PARAM_NULL_SCHEMA) {
		return ttest_fail(&tc, "Free failed: %s", cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Run the CYAML freeing unit tests.
 *
 * \param[in]  rc         The ttest report context.
 * \param[in]  log_level  CYAML log level.
 * \param[in]  log_fn     CYAML logging function, or NULL.
 * \return true iff all unit tests pass, otherwise false.
 */
bool free_tests(
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

	ttest_heading(rc, "Free tests");

	pass &= test_free_null_data(rc, &config);
	pass &= test_free_null_mem_fn(rc, &config);
	pass &= test_free_null_config(rc, &config);
	pass &= test_free_null_schema(rc, &config);

	return pass;
}
