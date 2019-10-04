/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2013 Michael Drake <tlsa@netsurf-browser.org>
 */

#ifndef TTEST_H
#define TTEST_H

/**
 * Test cleanup client callback.
 *
 * The caller passes this callback function to ttest_start, and it is called
 * in whichever tlsa-test function finishes the test (ttest_pass, ttest_fail,
 * or ttest_todo).
 *
 * \param[in]  Client data to clean up.
 */
typedef void (*ttest_cleanup_fn)(void *data);

/**
 * Internal tlsa-test report context.
 *
 * Clients should not touch this directly, it is only exposed in this
 * header because tlsa-test is arranged to be header-only, for convenience
 * of utilisation.
 */
typedef struct ttest_report_ctx {
	bool quiet;      /**< Whether to print only the report summary. */
	unsigned tests;  /**< Number of tests started. */
	unsigned todo;   /**< Number of tests marked as unimplemented. */
	unsigned passed; /**< Number of tests passed. */
} ttest_report_ctx_t;

/**
 * Internal tlsa-test test context.
 *
 * Clients should not touch this directly, it is only exposed in this
 * header because tlsa-test is arranged to be header-only, for convenience
 * of utilisation.
 */
typedef struct ttest_ctx {
	ttest_report_ctx_t *report; /**< The tlsa-test report context. */
	const char *name;           /**< The unit test name. */
	ttest_cleanup_fn cleanup;   /**< Client's unit test cleanup function. */
	void *cleanup_data;         /**< Client's unit test cleanup context. */
} ttest_ctx_t;

/**
 * Initialise a tlsa-test report context.
 *
 * \param[in]  quiet  Whether report should be a minimal summary.
 * \return initialised tlsa-test report context.
 */
static inline ttest_report_ctx_t ttest_init(
		bool quiet)
{
	ttest_report_ctx_t rc = {
		.quiet = quiet,
	};

	return rc;
}

/**
 * Start a unit test.
 *
 * The when complete, the test must be competed by calling either ttest_pass,
 * ttest_fail, or ttest_todo.
 *
 * \param[in]  report        The tlsa-test report context.
 * \param[in]  name          Name for this unit test.
 * \param[in]  cleanup       Cleanup function to call on test completion.
 * \param[in]  cleanup_data  Pointer to client cleanup context.
 * \return Returns the unit test context.  The test context must be passed to
 *         the test completion function.
 */
static inline ttest_ctx_t ttest_start(
		ttest_report_ctx_t *report,
		const char *name,
		ttest_cleanup_fn cleanup,
		void *cleanup_data)
{
	ttest_ctx_t tc = {
		.name = name,
		.report = report,
		.cleanup = cleanup,
		.cleanup_data = cleanup_data,
	};

	report->tests++;

	return tc;
}

/**
 * Pass a unit test.
 *
 * This function competes a unit test.  It should be called when a test passes.
 * This function always returns true.
 *
 * \param[in]  tc  Unit test context, returned by ttest_start.
 * \return true.
 */
static inline bool ttest_pass(
		const ttest_ctx_t *tc)
{
	assert(tc != NULL);
	assert(tc->report != NULL);

	tc->report->passed++;

	if (tc->cleanup != NULL) {
		tc->cleanup(tc->cleanup_data);
	}

	if (tc->report->quiet == false) {
		fprintf(stderr, "  PASS: %s\n", tc->name);
	}

	return true;
}

/**
 * Fail a unit test.
 *
 * This function competes a unit test.  It should be called when a test fails.
 * This function always returns false.  Prints the test result.
 *
 * \param[in]  tc      Unit test context, returned by ttest_start.
 * \param[in]  reason  Format string to explain reason for test failure.
 * \param[in]  ...     Additional arguments for formatted rendering.
 * \return false.
 */
static inline bool ttest_fail(
		const ttest_ctx_t *tc,
		const char *reason, ...)
{
	va_list args;

	assert(tc != NULL);
	assert(tc->report != NULL);

	fprintf(stderr, "  FAIL: %s (", tc->name);
	va_start(args, reason);
	vfprintf(stderr, reason, args);
	va_end(args);
	fprintf(stderr, ")\n");

	/* Cleanup after printing result, in case `reason` refers to cleaned up
	 * memory. */
	if (tc->cleanup != NULL) {
		tc->cleanup(tc->cleanup_data);
	}

	return false;
}

/**
 * Make a unit test as unimplemented.
 *
 * This function competes a unit test.  Should be called on unimplemented tests.
 * This function always returns true.
 *
 * \param[in]  tc  Unit test context, returned by ttest_start.
 * \return true.
 */
static inline bool ttest_todo(
		const ttest_ctx_t *tc)
{
	assert(tc != NULL);
	assert(tc->report != NULL);

	tc->report->todo++;

	if (tc->cleanup != NULL) {
		tc->cleanup(tc->cleanup_data);
	}

	if (tc->report->quiet == false) {
		fprintf(stderr, "  TODO: %s\n", tc->name);
	}

	return true;
}

/**
 * Print a visual divider in the test output.
 */
static inline void ttest_divider(void)
{
	fprintf(stderr, "========================================"
			"========================================\n");
}

/**
 * Print a test heading.
 *
 * \param[in]  tr       The tlsa-test report context.
 * \param[in]  heading  The heading to print.
 */
static inline void ttest_heading(
		const ttest_report_ctx_t *tr,
		const char *heading)
{
	if (!tr->quiet) {
		ttest_divider();
		fprintf(stderr, "TEST: %s\n", heading);
		ttest_divider();
	}
}

/**
 * Print the test report summary.
 *
 * \param[in]  tr  The tlsa-test report context.
 */
static inline void ttest_report(
		const ttest_report_ctx_t *tr)
{
	ttest_divider();
	if (tr->todo > 0) {
		fprintf(stderr, "TODO: %u test%s unimplemented.\n",
				tr->todo, (tr->todo > 1) ? "s" : "");
	}
	fprintf(stderr, "%s: %u of %u tests passed.\n",
			(tr->passed == tr->tests - tr->todo) ? "PASS" : "FAIL",
			tr->passed, tr->tests - tr->todo);
	ttest_divider();
}

#endif
