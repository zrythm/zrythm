;;; SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
;;; SPDX-License-Identifier: LicenseRef-ZrythmLicense

;;; This file generates a list of translators in
;;; the given output type

(add-to-load-path "@SCRIPTS_DIR@")

(define-module (gschema-gen)
  #:use-module (guile-utils)
  #:use-module (ice-9 string-fun)
  #:use-module (ice-9 format)
  #:use-module (ice-9 match)
  #:use-module (ice-9 rdelim)
  #:use-module (ice-9 regex)
  #:use-module (srfi srfi-1)
  #:use-module (sxml simple))

#!
Args:
1: output file
2: "manual" for user manual page or "about" for
   about dialog.
3: input file (translators file)
!#
(define (main . args)

  ;; verify number of args
  (unless (eq? (length args) 4)
    (display "Need 3 arguments")
    (newline)
    (exit -1))

  ;; get args
  (match args
    ((this-program output-file output-type
                   input-file)

     ;; open file
     (with-output-to-file output-file
       (lambda ()
         (with-input-from-file input-file
           (lambda ()
             (let
               ((translators '()))

               ;; collect translators, skipping
               ;; duplicates
               (do ((line (read-line) (read-line)))
                 ((eof-object? line))
                 (let ((matched
                         (string-match "[*] (.+)"
                                       line)))
                   (when matched
                     (let ((translator
                             (match:substring
                               matched 1)))
                       (unless
                         (elem? translator
                                translators)
                         (set! translators
                           (append!
                             translators
                             (list
                               (match:substring
                                 matched 1)))))))))

               ;; display them

               ;; if about dialog
               (when (string=? output-type "about")
                 (display
                   "#define TRANSLATORS_STR \\")
                 (newline)
                 (for-each
                   (lambda (translator)
                     (display
                       (string-append
                         "\"" translator))
                     (if (equal? (last translators)
                                 translator)
                       (display "\"")
                       (display "\\n\" \\"))
                     (newline))
                   translators))))))))))

(apply main (program-arguments))
