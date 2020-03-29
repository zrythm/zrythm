#!@GUILE@ -s
!#
;;; Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
;;;
;;; This file is part of Zrythm
;;;
;;; Zrythm is free software: you can redistribute it and/or modify
;;; it under the terms of the GNU Affero General Public License as published by
;;; the Free Software Foundation, either version 3 of the License, or
;;; (at your option) any later version.
;;;
;;; Zrythm is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU Affero General Public License for more details.
;;;
;;; You should have received a copy of the GNU Affero General Public License
;;; along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
;;;
;;; This file generates .texi documentation from a
;;;   given C source file with defined SCM_* macros.

(add-to-load-path "@SCRIPTS_DIR@")

(define-module (guile-gen-texi-docs)
  #:use-module (srfi srfi-1)
  #:use-module (srfi srfi-98)
  #:use-module (ice-9 match)
  #:use-module (ice-9 popen)
  #:use-module (ice-9 rdelim)
  #:use-module (ice-9 textual-ports)
  #:use-module (guile-utils))

(define (snarf-docs-bin)
  "@GUILE_SNARF_DOCS_BIN@")

(define (base-noext f)
  (basename f ".c"))

(define (doc-file private-dir input-file)
  (string-append
    (join-path
      (list private-dir
            (base-noext input-file)))
    ".doc"))

(define (main . args)
  (match args
    ((this-program guile-pkgconf-name
                   input-file output-file
                   private-dir)
     (chdir (dirname (snarf-docs-bin)))
     (unless (file-exists? private-dir)
       (mkdir private-dir))
     (invoke-with-fail
       (list
         "@GUILE_SNARF_DOCS_BIN@" "-o"
         (doc-file private-dir input-file)
         input-file "--"
         (get-cflags-from-pkgconf-name
           guile-pkgconf-name)))
     (with-output-to-file output-file
       (lambda ()
         (let
           ((doc-file-contents
              (file-to-string
                (doc-file private-dir input-file)))
            (port
              (open-output-pipe
                "@GUILD_BIN@ snarf-check-and-output-texi")))
           (display doc-file-contents port)))))))

(apply main (program-arguments))

