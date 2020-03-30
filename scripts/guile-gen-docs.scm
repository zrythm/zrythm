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
;;; This file generates .rst documentation from a
;;;   given C source file with defined SCM_* macros.
;;; It first generates .texi files and then uses
;;;   pandoc to convert that to .rst

(add-to-load-path "@SCRIPTS_DIR@")

(define-module (guile-gen-texi-docs)
  #:use-module (srfi srfi-1)
  #:use-module (srfi srfi-98)
  #:use-module (ice-9 format)
  #:use-module (ice-9 match)
  #:use-module (ice-9 popen)
  #:use-module (ice-9 rdelim)
  #:use-module (ice-9 regex)
  #:use-module (ice-9 textual-ports)
  #:use-module (guix scripts substitute)
  #:use-module (guix build utils)
  #:use-module (guile-utils))

(define snarf-docs-bin "@GUILE_SNARF_DOCS_BIN@")
(define guild-bin "@GUILD_BIN@")
(define pandoc-bin "@PANDOC_BIN@")
(define texi2html-bin "@TEXI2HTML_BIN@")

(define (base-noext f)
  (basename
    f (cadr (string-split (basename f) #\.))))

(define (*-file private-dir input-file ext)
  (string-append
    (join-path
      (list private-dir
            (base-noext input-file)))
    "." ext))

(define (html-file private-dir input-file)
  (*-file private-dir input-file "html"))

(define (main . args)
  (match args
    ((this-program guile-pkgconf-name
                   input-file output-file
                   private-dir)
     (display output-file)
     (newline)
     (chdir (dirname snarf-docs-bin))
     (unless (file-exists? private-dir)
       (mkdir private-dir))
     ;; convert to html
     (invoke
       texi2html-bin
       (string-append
         "--o="
         (html-file private-dir input-file))
       input-file)
     ;; convert to rst
     (invoke
       pandoc-bin
       (html-file private-dir input-file)
       "-s" "-o" output-file)
     ;; edit
     (substitute*
       output-file
       (("Untitled Document")
        (string-drop-right
          (base-noext input-file) 1)))
     (substitute*
       output-file
       (("^.+$")
         ""))
     (substitute*
       output-file
       (("^----.+$")
         ""))
     (substitute*
       output-file
       (("^| This document.+$")
         ""))
     (substitute*
       output-file
       (("^.+>`__.+$")
         "")))))

(apply main (program-arguments))
