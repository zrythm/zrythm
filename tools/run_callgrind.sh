#!/bin/bash

NO_SCAN_PLUGINS=1 \
    valgrind --tool=callgrind build/src/zrythm
