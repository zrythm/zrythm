#!@GUILE@ -s
!#
;;; SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
;;; SPDX-License-Identifier: LicenseRef-ZrythmLicense

(add-to-load-path "@SCRIPTS_DIR@")

(define-module (guile-snarf-wrap)
  #:use-module (srfi srfi-1)
  #:use-module (srfi srfi-98)
  #:use-module (ice-9 match)
  #:use-module (guile-utils))

(define (main . args)
  ;; get args
  (match args
    ((this-program guile-pkgconf-name
                   output-file input-file)
     (unless
       (zero?
         (system*
           "@GUILE_SNARF@"
           "-o" output-file input-file
           "-I@MESON_SOURCE_ROOT@/inc"
           (get-cflags-from-pkgconf-name
             guile-pkgconf-name)))
       (exit -1)))))

(apply main (program-arguments))
