/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2018 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <cyaml/cyaml.h>

#include "../../src/mem.h"
#include "../../src/util.h"

#include "ttest.h"

/**
 * Test cyaml memory functions.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_util_memory_funcs(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	ttest_ctx_t tc = ttest_start(report, __func__, NULL, NULL);
	unsigned char *mem, *tmp;

	/* Create test allocation */
	mem = cyaml__alloc(config, 0xff, true);
	if (mem == NULL) {
		return ttest_fail(&tc, "Memory allocation failed.");
	}

	/* Check allocation was zeroed. */
	for (unsigned i = 0; i < 0x7f; i++) {
		if (mem[i] != 0) {
			return ttest_fail(&tc, "Allocation not cleaned.");
		}
	}

	/* Set our own known values */
	for (unsigned i = 0; i < 0xff; i++) {
		mem[i] = 0xff;
	}

	/* Shrink allocation */
	tmp = cyaml__realloc(config, mem, 0xff, 0x7f, true);
	if (tmp == NULL) {
		return ttest_fail(&tc, "Allocation shrink failed.");
	}
	mem = tmp;

	/* Check for our own values. */
	for (unsigned i = 0; i < 0x7f; i++) {
		if (mem[i] != 0xff) {
			return ttest_fail(&tc, "Data trampled by realloc.");
		}
	}

	/* Free test allocation. */
	cyaml__free(config, mem);

	return ttest_pass(&tc);
}

/**
 * Test cyaml memory functions.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_util_strdup(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	ttest_ctx_t tc = ttest_start(report, __func__, NULL, NULL);
	const char *orig = "Hello!";
	char *copy;

	/* Create test allocation */
	copy = cyaml__strdup(config, orig);
	if (copy == NULL) {
		return ttest_fail(&tc, "Memory allocation failed.");
	}

	/* Confirm string duplication. */
	if (strcmp(orig, copy) != 0) {
		return ttest_fail(&tc, "String not copied correctly.");
	}

	/* Free test allocation. */
	cyaml__free(config, copy);

	return ttest_pass(&tc);
}

/**
 * Test invalid state machine state.
 *
 * \param[in]  report  The test report context.
 * \return true if test passes, false otherwise.
 */
static bool test_util_state_invalid(
		ttest_report_ctx_t *report)
{
	ttest_ctx_t tc = ttest_start(report, __func__, NULL, NULL);

	if (strcmp(cyaml__state_to_str(CYAML_STATE__COUNT),
			"<invalid>") != 0) {
		return ttest_fail(&tc, "Wrong string for invalid state.");
	}

	if (strcmp(cyaml__state_to_str(CYAML_STATE__COUNT + 1),
			"<invalid>") != 0) {
		return ttest_fail(&tc, "Wrong string for invalid state.");
	}

	return ttest_pass(&tc);
}

/**
 * Test CYAML_OK error code.
 *
 * \param[in]  report  The test report context.
 * \return true if test passes, false otherwise.
 */
static bool test_util_err_success_zero(
		ttest_report_ctx_t *report)
{
	ttest_ctx_t tc = ttest_start(report, __func__, NULL, NULL);

	if (CYAML_OK != 0) {
		return ttest_fail(&tc, "CYAML_OK value not zero");
	}

	return ttest_pass(&tc);
}

/**
 * Test valid cyaml_strerror strings.
 *
 * \param[in]  report  The test report context.
 * \return true if test passes, false otherwise.
 */
static bool test_util_err_strings_valid(
		ttest_report_ctx_t *report)
{
	ttest_ctx_t tc = ttest_start(report, __func__, NULL, NULL);

	if (cyaml_strerror(CYAML_OK) == NULL) {
		return ttest_fail(&tc, "CYAML_OK string is NULL");
	}

	if (strcmp(cyaml_strerror(CYAML_OK), "Success") != 0) {
		return ttest_fail(&tc, "CYAML_OK string not 'Success'");
	}

	for (unsigned i = 1; i <= CYAML_ERR__COUNT; i++) {
		if (cyaml_strerror(i) == NULL) {
			return ttest_fail(&tc, "Error code %u string is NULL",
					i);
		}

		for (unsigned j = 0; j < i; j++) {
			if (strcmp(cyaml_strerror(j), cyaml_strerror(i)) == 0) {
				return ttest_fail(&tc, "Two error codes "
						"have same string (%u and %u)",
						i, j);
			}
		}
	}

	return ttest_pass(&tc);
}

/**
 * Test invalid cyaml_strerror strings.
 *
 * \param[in]  report  The test report context.
 * \return true if test passes, false otherwise.
 */
static bool test_util_err_strings_invalid(
		ttest_report_ctx_t *report)
{
	ttest_ctx_t tc = ttest_start(report, __func__, NULL, NULL);

	if (strcmp(cyaml_strerror(CYAML_ERR__COUNT),
			"Invalid error code") != 0) {
		return ttest_fail(&tc, "CYAML_ERR__COUNT string not "
				"'Invalid error code'");
	}

	if (strcmp(cyaml_strerror(CYAML_ERR__COUNT + 1),
			"Invalid error code") != 0) {
		return ttest_fail(&tc, "CYAML_ERR__COUNT + 1 string not "
				"'Invalid error code'");
	}

	if (strcmp(cyaml_strerror(-1), "Invalid error code") != 0) {
		return ttest_fail(&tc, "-1 string not 'Invalid error code'");
	}

	return ttest_pass(&tc);
}

/**
 * Run the CYAML util unit tests.
 *
 * \param[in]  rc         The ttest report context.
 * \param[in]  log_level  CYAML log level.
 * \param[in]  log_fn     CYAML logging function, or NULL.
 * \return true iff all unit tests pass, otherwise false.
 */
bool util_tests(
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

	ttest_heading(rc, "Memory utility functions");

	pass &= test_util_strdup(rc, &config);
	pass &= test_util_memory_funcs(rc, &config);

	ttest_heading(rc, "Error code tests");

	pass &= test_util_state_invalid(rc);
	pass &= test_util_err_success_zero(rc);
	pass &= test_util_err_strings_valid(rc);
	pass &= test_util_err_strings_invalid(rc);

	return pass;
}
