#!/bin/bash

set +e

meson test \
        -C _build

exit_code=$?

python3 .gitlab-ci/meson-junit-report.py \
        --project-name=zrythm \
        --job-id "${CI_JOB_NAME}" \
        --output "_build/${CI_JOB_NAME}-report.xml" \
        _build/meson-logs/testlog.json

exit $exit_code
